/* global require */
'use strict';

var url = require('url');
var log = require('./logger').logger.getLogger('LegacyClient');

//FIXME: to keep compatible to previous MCU, should be removed later.
var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720}
};

var resolutionValue2Name = {
    'r352x288': 'cif',
    'r640x480': 'vga',
    'r800x600': 'svga',
    'r1024x768': 'xga',
    'r640x360': 'r640x360',
    'r1280x720': 'hd720p',
    'r320x240': 'sif',
    'r480x320': 'hvga',
    'r480x360': 'r480x360',
    'r176x144': 'qcif',
    'r192x144': 'r192x144',
    'r1920x1080': 'hd1080p',
    'r3840x2160': 'uhd_4k',
    'r360x360': 'r360x360',
    'r480x480': 'r480x480',
    'r720x720': 'r720x720'
};

function widthHeight2Resolution(width, height) {
  var k = 'r' + width + 'x' + height;
  return resolutionValue2Name[k] ? resolutionValue2Name[k] : k;
}

function resolution2WidthHeight(resolution) {
  var r = resolution.indexOf('r'),
    x = resolution.indexOf('x'),
    w = (r === 0 && x > 1 ? Number(resolution.substring(1, x)) : 640),
    h = (r === 0 && x > 1 ? Number(resolution.substring(x, resolution.length)) : 480);
  return resolutionName2Value[resolution] ? resolutionName2Value[resolution] : {width: w, height: h};
}

var ql2brl = {
  'bestquality': 'x1.4',
  'betterquality': 'x1.2',
  'standard': undefined,
  'betterspeed': 'x0.8',
  'bestspeed': 'x0.6'
};

function qualityLevel2BitrateLevel (ql) {
  ql = ql.toLowerCase();
  return ql2brl[ql] ? ql2brl[ql] : undefined;
}

var idPattern = /^[0-9a-zA-Z\-]+$/;
function isValidIdString(str) {
  return (typeof str === 'string') && idPattern.test(str);
}

var formatDate = function(date, format) {
  var dateTime = {
    'M+': date.getMonth() + 1,
    'd+': date.getDate(),
    'h+': date.getHours(),
    'm+': date.getMinutes(),
    's+': date.getSeconds(),
    'q+': Math.floor((date.getMonth() + 3) / 3),
    'S+': date.getMilliseconds()
  };

  if (/(y+)/i.test(format)) {
    format = format.replace(RegExp.$1, (date.getFullYear() + '').substr(4 - RegExp.$1.length));
  }

  for (var k in dateTime) {
    if (new RegExp('(' + k + ')').test(format)) {
      format = format.replace(RegExp.$1, RegExp.$1.length == 1 ?
        dateTime[k] : ('00' + dateTime[k]).substr(('' + dateTime[k]).length));
    }
  }

  return format;
};

function safeCall () {
  var callback = arguments[0];
  if (typeof callback === 'function') {
    var args = Array.prototype.slice.call(arguments, 1);
    callback.apply(null, args);
  }
}

const getErrorMessage = function (err) {
  if (typeof err === 'string') {
    return err;
  } else if (err && err.message) {
    return err.message;
  } else {
    log.debug('Unknown error:', err);
    return 'Unknown';
  }
};

var LegacyClient = function(clientId, sigConnection, portal) {
  var that = {
    id: clientId,
    connection: sigConnection
  };

  let published = {/*StreamId: {type: "webrtc" | "streamingIn", mix: boolean(IfMix)}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)
  let ref2subId = {/*PeerId | StreamingOutURL | RecorderId: SubscriptionId}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)
  let subId2ref = {/*SubscriptionId: PeerId | StreamingOutURL | RecorderId}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)

  const getViewLabelFromStreamId = (streamId) => {
    var bar_pos = streamId.indexOf("-");
    return ((bar_pos >= 0) && (bar_pos < (streamId.length - 1))) ? streamId.substring(bar_pos + 1) : streamId;
  };

  const sendMsg = (evt, data) => {
    that.connection.sendMessage(evt, data);
  };

  const listenAt = (socket) => {
    socket.on('publish', function(options, url, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var stream_id = Math.round(Math.random() * 1000000000000000000) + '';
      var pub_info = {};

      if (options.state === 'erizo') {
        pub_info.type = 'webrtc';
        pub_info.media = {
          audio: (options.audio === false ? false : {source: 'mic'}),
          video: (options.video === false ? false : {source: (options.video && options.video.device === 'screen') ? 'screen-cast' : 'camera'})
        }
      } else if (options.state === 'url') {
        pub_info.type = 'streaming';
        pub_info.connection = {
          url: url,
          transportProtocol: options.transport || 'tcp',
          bufferSize: options.bufferSize || 2048
        };
        pub_info.media = {
          audio: (options.audio === undefined ? 'auto' : !!options.audio),
          video: (options.video === undefined ? 'auto' : !!options.video)
        };
      } else {
        return safeCall(callback, 'error', 'stream type(options.state) error.');
      }

      options.attributes && (pub_info.attributes = options.attributes);

      return portal.publish(clientId, stream_id, pub_info)
      .then(function(result) {
        if (options.state === 'erizo') {
          safeCall(callback, 'initializing', stream_id);
          published[stream_id] = {type: 'webrtc', mix: !(options.unmix || (options.video && options.video.device === 'screen'))};
        } else {
          safeCall(callback, 'success', stream_id);
          published[stream_id] = {type: 'streamingIn', mix: !options.unmix};
        }
      })
      .catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.publish failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('unpublish', function(streamId, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      return portal.unpublish(clientId, streamId)
      .then(function() {
        safeCall(callback, 'success');
        published[streamId] && (delete published[streamId]);
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unpublish failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('mix', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var streamId = options.streamId;

      return Promise.all(options.mixStreams.map((mixStreamId) => {
          var view_label = (getViewLabelFromStreamId(mixStreamId) || 'common');
          return portal.streamControl(clientId,
                                      streamId,
                                      {
                                        operation: 'mix',
                                        data: view_label
                                      });
        })).then((result) => {
          safeCall(callback, 'success');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.mix failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unmix', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var streamId = options.streamId;

      return Promise.all(options.mixStreams.map((mixStreamId) => {
          var view_label = (getViewLabelFromStreamId(mixStreamId) || 'common');
          return portal.streamControl(clientId,
                                      streamId,
                                      {
                                        operation: 'unmix',
                                        data: view_label
                                      });
        })).then((result) => {
          safeCall(callback, 'success');
        }).catch((err) => {
          const err_message = getErrorMessage(err);
          log.info('portal.unmix failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('setVideoBitrate', function(options, callback) {
      safeCall(callback, 'error', 'No longer supported signaling');
    });

    socket.on('subscribe', function(options, unused, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!isValidIdString(options.streamId)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      var peer_id = options.streamId;
      if (ref2subId[peer_id]) {
        return safeCall(callback, 'error', 'Subscription is ongoing');
      }

      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      ref2subId[peer_id] = subscription_id;
      subId2ref[subscription_id] = peer_id;

      var sub_desc = {type: 'webrtc', media: {}};
      (options.audio || options.audio === undefined) && (sub_desc.media.audio = {from: options.streamId});
      (options.video || options.video === undefined) && (sub_desc.media.video = {from: options.streamId});
      (options.video && options.video.resolution && (typeof options.video.resolution.width === 'number') && (typeof options.video.resolution.height === 'number')) &&
      (sub_desc.media.video.parameters || (sub_desc.media.video.parameters = {})) && (sub_desc.media.video.parameters.resolution = options.video.resolution);
      options.video && (typeof options.video.resolution === 'string') && (sub_desc.media.video.resolution = resolution2WidthHeight(options.video.resolution));
      options.video && options.video.quality_level && (sub_desc.media.video.parameters || (sub_desc.media.video.parameters = {})) && (sub_desc.media.video.parameters.bitrate = qualityLevel2BitrateLevel(options.video.quality_level));

      if (!sub_desc.media.audio && !sub_desc.media.video) {
          return safeCall(callback, 'error', 'bad options');
      }

      return portal.subscribe(clientId, subscription_id, sub_desc
        ).then(function(result) {
          safeCall(callback, 'initializing', peer_id);
          log.debug('portal.subscribe succeeded');
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.subscribe failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unsubscribe', function(streamId, callback) {
      log.debug('on:unsubscribe, streamId:', streamId);
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var peer_id = streamId;
      if (!ref2subId[peer_id]) {
        return safeCall(callback, 'error', 'Subscription does NOT exist');
      }
      var subscription_id = ref2subId[peer_id];

      delete ref2subId[peer_id];
      delete subId2ref[subscription_id];

      return portal.unsubscribe(clientId,  subscription_id)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('signaling_message', function(message, to_to_deprecated, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      var session_id = (ref2subId[message.streamId] ? ref2subId[message.streamId] : message.streamId);
      if (session_id) {
        portal.onSessionSignaling(clientId, session_id, message.msg);
      }
      safeCall(callback, 'ok');
    });

    socket.on('addExternalOutput', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      log.debug('Add serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:' && parsed_url.protocol !== 'http:') || !parsed_url.slashes || !parsed_url.host) {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      if (!(options.streamId === undefined || isValidIdString(options.streamId))) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      var streaming_url = parsed_url.format();
      if (ref2subId[streaming_url]) {
        return safeCall(callback, 'error', 'Streaming-out is ongoing');
      }

      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      ref2subId[streaming_url] = subscription_id;
      subId2ref[subscription_id] = streaming_url;

      var target_stream_id = (options.streamId || that.commonViewStream);
      var sub_desc = {
        type: 'streaming',
        media: {
          audio: (options.audio === false ? false: {from: target_stream_id, format: {codec: options.audio && options.audio.codecs ? options.audio.codecs[0] : 'aac'}}),
          video: (options.video === false ? false: {from: target_stream_id, format: {codec: options.video && options.video.codecs ? options.video.codecs[0] : 'h264'}})
        },
        connection: {
          url: parsed_url.format()
        }
      };
      if (sub_desc.media.audio && sub_desc.media.audio.format && sub_desc.media.audio.format.codec === 'aac' || sub_desc.media.audio.format.codec === 'opus') {
        sub_desc.media.audio.format.sampleRate = 48000;
        sub_desc.media.audio.format.channelNum = 2;
      }
      ((sub_desc.media.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (sub_desc.media.video.parameters || (sub_desc.media.video.parameters = {})) && (sub_desc.media.video.parameters.resolution = options.resolution);
      (sub_desc.media.video && (typeof options.resolution === 'string')) && (sub_desc.media.video.parameters || (sub_desc.media.video.parameters = {})) && (sub_desc.media.video.parameters.resolution = resolution2WidthHeight(options.resolution));

      return portal.subscribe(clientId, subscription_id, sub_desc
        ).then((result) => {  
          log.debug('portal.subscribe succeeded');
          safeCall(callback, 'success', {url: sub_desc.connection.url});
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.subscribe failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('updateExternalOutput', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      log.debug('Update serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:' && parsed_url.protocol !== 'http:') || !parsed_url.slashes || !parsed_url.host) {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      if (!(options.streamId === undefined || isValidIdString(options.streamId))) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      var streaming_url = options.url;
      var subscription_id = ref2subId[streaming_url];
      if (!subscription_id) {
        return safeCall(callback, 'error', 'Streaming-out does NOT exist');
      }
      var update = {};

      if (options.streamId) {
        update.audio = {from: options.streamId};
        update.video = {from: options.streamId};
      }

      if (options.resolution) {
        update.video = (update.video || {});
        update.video.parameters = {};
        (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number') && (update.video.parameters.resolution = options.resolution);
        (typeof options.resolution === 'string') && (update.video.parameters.resolution = resolution2WidthHeight(options.resolution));
      }

      return portal.subscriptionControl(clientId, subscription_id, {operation: 'update', data: update}
        ).then(function(subscriptionId) {
          safeCall(callback, 'success', {url: options.url});
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.subscriptionControl failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('removeExternalOutput', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      log.debug('Remove serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var streaming_url = options.url;
      if (!ref2subId[streaming_url]) {
        return safeCall(callback, 'error', 'Streaming-out does NOT exist');
      }
      var subscription_id = ref2subId[streaming_url];

      delete ref2subId[streaming_url];
      delete subId2ref[subscription_id];

      return portal.unsubscribe(clientId, subscription_id)
      .then(function() {
        safeCall(callback, 'success', {url: options.url});
      }, function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('startRecorder', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!(options.recorderId === undefined || isValidIdString(options.recorderId))) {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      if (!(options.audioStreamId === undefined || isValidIdString(options.audioStreamId))) {
        return safeCall(callback, 'error', 'Invalid audio stream id');
      }

      if (!(options.videoStreamId === undefined || isValidIdString(options.videoStreamId))) {
        return safeCall(callback, 'error', 'Invalid video stream id');
      }

      var unspecifiedStreamIds = (options.audioStreamId === undefined && options.videoStreamId === undefined);

      if ((options.audioStreamId || unspecifiedStreamIds) && (options.audioCodec !== undefined) && (['pcmu', 'opus', 'aac'].indexOf(options.audioCodec) < 0)) {
        return safeCall(callback, 'error', 'Invalid audio codec');
      }

      if ((options.videoStreamId || unspecifiedStreamIds) && (options.videoCodec !== undefined) && (['vp8', 'h264'].indexOf(options.videoCodec) < 0)) {
        return safeCall(callback, 'error', 'Invalid video codec');
      }

      //Behavior Change: The 'startRecorder' request with options.recorderId being specified will be considered
      //as a contineous recording request in v3.5 and later releases.
      if (options.recorderId) {
        if ((options.audioStreamId === undefined) && (options.videoStreamId === undefined)) {
          return safeCall(callback, 'error', 'Neither audio.from nor video.from was specified');
        }

        if (!ref2subId[options.recorderId]) {
          return safeCall(callback, 'error', 'Recording does NOT exist');
        }

        var subscription_id = options.recorderId;
        var update = {
          operation: 'update',
          data: {}
        };

        options.audioStreamId && (update.data.audio = {from: options.audioStreamId});
        options.videoStreamId && (update.data.video = {from: options.videoStreamId});
        return portal.subscriptionControl(clientId, subscription_id, update)
          .then((result) => {
            safeCall(callback, 'success', {recorderId: options.recorderId});
          }).catch(function(err) {
            const err_message = getErrorMessage(err);
            log.info('portal.subscribe failed:', err_message);
            safeCall(callback, 'error', err_message);
          });
      }

      //Behavior Change: The options.path and options.interval parameters will be ignored in v3.5 and later releases. 
      var recorder_id = Math.round(Math.random() * 1000000000000000000) + '';
      var subscription_id = recorder_id;
      ref2subId[recorder_id] = subscription_id;
      subId2ref[subscription_id] = recorder_id;

      var sub_desc = {
        type: 'recording',
        media: {
          audio: false,
          video: false
        },
        connection: {
          container: (options.audioCodec === 'aac' ? 'mp4' : 'mkv')
        }
      };

      (options.audioStreamId || unspecifiedStreamIds) && (sub_desc.media.audio = {from: options.audioStreamId || that.commonViewStream});
      sub_desc.media.audio && (sub_desc.media.audio.format = ((a) => {
        if (a) {
          if (a === 'opus' || a === 'aac') {
            return {codec: a, sampleRate: 48000, channelNum: 2};
          } else {
            return {codec: a}
          }
        } else {
          return {codec: 'opus', sampleRate: 48000, channelNum: 2};
        }
      })(options.audioCodec));

      (options.videoStreamId || unspecifiedStreamIds) && (sub_desc.media.video = {from: options.videoStreamId || that.commonViewStream});
      sub_desc.media.video && (sub_desc.media.video.format = {codec: (options.videoCodec || 'vp8')});

      return portal.subscribe(clientId, subscription_id, sub_desc)
        .then(function(result) {
          log.debug('portal.subscribe succeeded, result:', result);

          var recording_file = subscription_id + '.' + sub_desc.connection.container;
          safeCall(callback, 'success', {recorderId: recorder_id, path: recording_file, host: 'unknown'});
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.subscribe failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('stopRecorder', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (options.recorderId === undefined || !isValidIdString(options.recorderId)) {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      var recorder_id = options.recorderId;
      if (!ref2subId[recorder_id]) {
        return safeCall(callback, 'error', 'Recording does NOT exist');
      }
      var subscription_id = ref2subId[recorder_id];

      delete ref2subId[recorder_id];
      delete subId2ref[subscription_id];

      return portal.unsubscribe(clientId, subscription_id)
      .then(function() {
        safeCall(callback, 'success', {recorderId: options.recorderId, host: 'unknown'});
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('getRegion', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!isValidIdString(options.id)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      var view_label = (options.mixStreamId ? getViewLabelFromStreamId(options.mixStreamId) : 'common');
      return portal.streamControl(clientId, options.id, {operation: 'get-region', data: view_label})
      .then(function(result) {
        safeCall(callback, 'success', {region: result.region});
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.streamControl failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('setRegion', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!isValidIdString(options.id)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      if (!isValidIdString(options.region)) {
        return safeCall(callback, 'error', 'Invalid region id');
      }

      var view_label = (options.mixStreamId ? getViewLabelFromStreamId(options.mixStreamId) : 'common');
      return portal.streamControl(clientId, options.id, {operation: 'set-region', data: {view: view_label, region: options.region}})
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.streamControl failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('mute', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!options.streamId) {
        return safeCall(callback, 'error', 'no stream ID');
      }

      if (['video', 'audio', 'av'].indexOf(options.track) < 0) {
        return safeCall(callback, 'error', `invalid track ${options.track}`);
      }

      return portal.streamControl(clientId, options.streamId, {operation: 'pause', data: options.track})
      .then(function() {
          safeCall(callback, 'success');
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.streamControl failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unmute', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!options.streamId) {
        return safeCall(callback, 'error', 'no stream ID');
      }

      if (['video', 'audio', 'av'].indexOf(options.track) < 0) {
        return safeCall(callback, 'error', `invalid track ${options.track}`);
      }

      return portal.streamControl(clientId, options.streamId, {operation: 'play', data: options.track})
      .then(function() {
          safeCall(callback, 'success');
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.streamControl failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('setPermission', function(options, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if (!options.targetId) {
        return safeCall(callback, 'error', 'no targetId specified');
      }

      if (!options.action) {
        return safeCall(callback, 'error', 'no action specified');
      }

      return portal.setPermission(clientId, options.targetId, [{operation: options.action, value: options.update}])
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.setPermission failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    })

    socket.on('customMessage', function(msg, callback) {
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      switch (msg.type) {
        case 'data':
          if (typeof msg.receiver !== 'string' || msg.receiver === '') {
            return safeCall(callback, 'error', 'Invalid receiver');
          }

          return portal.text(clientId, msg.receiver, msg.data)
            .then(function() {
              safeCall(callback, 'success');
            }).catch(function(err) {
              const err_message = getErrorMessage(err);
              log.info('portal.text failed:', err_message);
              safeCall(callback, 'error', err_message);
            });
        case 'control':
          if (typeof msg.payload !== 'object') {
            return safeCall(callback, 'error', 'Invalid payload');
          }

          if (!isValidIdString(msg.payload.streamId)) {
            return safeCall(callback, 'error', 'Invalid connection id');
          }

          if (typeof msg.payload.action !== 'string' || !(/^((audio)|(video))-((in)|(out))-((on)|(off))$/.test(msg.payload.action))) {
            return safeCall(callback, 'error', 'Invalid action');
          }

          var cmdOpts = msg.payload.action.split('-'),
              track = cmdOpts[0],
              direction = (cmdOpts[1] === 'out' ? 'in' : 'out'),
              operation = (cmdOpts[2] === 'on' ? 'play' : 'pause');

          if (cmdOpts[1] === 'out') {
            return portal.streamControl(clientId, msg.payload.streamId, {operation: operation, data: track})
              .then(() => {
                safeCall(callback, 'success');
              }).catch((err) => {
                const err_message = getErrorMessage(err);
                log.info('portal.streamControl failed:', err_message);
                safeCall(callback, 'error', err_message);
              });
          } else {
            var peer_id = msg.payload.streamId;
            var subscription_id = ref2subId[peer_id];
            if (subscription_id === undefined) {
              return safeCall(callback, 'error', 'Subscription does NOT exist');
            }
            return portal.subscriptionControl(clientId, subscription_id, {operation: operation, data: track})
              .then(function() {
                safeCall(callback, 'success');
              }).catch(function(err) {
                const err_message = getErrorMessage(err);
                log.info('portal.subscriptionControl failed:', err_message);
                safeCall(callback, 'error', err_message);
              });
          }
        default:
          return safeCall(callback, 'error', 'Invalid message type');
      }
    });
  };

  const notifyParticipantActivity = (participantActivity) => {
    if (participantActivity.action === 'join') {
      sendMsg('user_join', {user: {id: participantActivity.data.id, name: participantActivity.data.user, role: participantActivity.data.role}});
    } else if (participantActivity.action === 'leave') {
      sendMsg('user_leave', {user: {id: participantActivity.data}});
    } else {
      log.info('Unknown participant activity message:', participantActivity);
    }
  };

  const notifyStreamInfo = (streamInfo) => {
    log.debug('notifyStreamInfo, streamInfo:', streamInfo);
    if (streamInfo.status === 'add') {
      sendMsg('add_stream', {id: streamInfo.data.id, audio: !!streamInfo.data.media.audio, video: !!streamInfo.data.media.video ? {device: streamInfo.data.type === 'mixed'? 'mcu' : streamInfo.data.media.video.source} : false, from: streamInfo.data.type === 'mixed' ? '' : streamInfo.data.info.owner, attributes: streamInfo.data.info.attributes});
    } else if (streamInfo.status === 'update') {
      var st_update = {id: streamInfo.id};
      if ((streamInfo.data.field === 'audio.status') || (streamInfo.data.field === 'video.status')) {//Forward stream update
        st_update.event = 'StateChange';
        st_update.state = streamInfo.data.value;
      } else if (streamInfo.data.field === 'video.layout') {//Mixed stream update
        st_update.event = 'VideoLayoutChanged';
        st_update.data = streamInfo.data.value.map(convertStreamRegion);
      }
      sendMsg('update_stream', st_update);
    } else if (streamInfo.status === 'remove') {
      sendMsg('remove_stream', {id: streamInfo.id});
    } else {
      log.info('Unknown stream info:', streamInfo);
    }
  };

  const notifySessionProgress = (sessionProgress) => {
    log.debug('notifySessionProgress, sessionProgress:', sessionProgress);
    var id = sessionProgress.id;
    if (sessionProgress.status === 'soac') {
      if (published[id]) {
        sendMsg('signaling_message_erizo', {streamId: id, mess: sessionProgress.data});
      } else {
        var peer_id = subId2ref[id];
        if (peer_id) {
          sendMsg('signaling_message_erizo', {peerId: peer_id, mess: sessionProgress.data});
        }
      }
    } else if (sessionProgress.status === 'ready') {
      if (published[id]) {
        if (published[id].type === 'webrtc') {
          sendMsg('signaling_message_erizo', {streamId: id, mess: {type: 'ready'}});
        }

        if (published[id].mix) {
          portal.streamControl(clientId, id, {operation: 'mix', data: 'common'})
            .catch((err) => {
              log.info('Mix stream failed, reason:', getErrorMessage(err));
            });
        }
      } else {
        var peer_id = subId2ref[id];
        if (peer_id) {
          sendMsg('signaling_message_erizo', {peerId: peer_id, mess: {type: 'ready'}});
        }
      }
    } else if (sessionProgress.status === 'error') {
      var ref = subId2ref[id];
      if (ref) {
        if (ref === id) {
          var recorder_id = id;
          log.debug('recorder error, recorder_id:', ref);
          portal.unsubscribe(clientId, id);
          sendMsg('remove_recorder', {id: recorder_id});
        } else if (ref.indexOf('rtsp') !== -1 || ref.indexOf('rtmp') !== -1 || ref.indexOf('http') !== -1) {
          sendMsg('connection_failed', {url: ref});
        } else {
          sendMsg('connection_failed', {streamId: ref});
        }
        delete ref2subId[ref];
        delete subId2ref[id];
      } else {
        sendMsg('connection_failed', {streamId: id});
      }
    } else {
      log.info('Unknown session progress message:', sessionProgress);
    }
  };

  //FIXME: Client side need to handle the incompetibility, since the definition of region has been extended.
  const convertStreamRegion = (stream2region) => {
    var calRational = (r) => (r.numerator / r.denominator);
    return {
      streamId: stream2region.stream,
      id: stream2region.region.id,
      left: calRational(stream2region.region.area.left),
      top: calRational(stream2region.region.area.top),
      relativeSize: calRational(stream2region.region.area.width)
    };
  };

  const convertJoinResponse = (response) => {
    return {
      clientId: that.id,
      id: response.room.id,
      streams: response.room.streams.map((st) => {
        var stream = {
          id: st.id,
          audio: !!st.media.audio,
          video: st.media.video ? {} : false,
          socket: ''
        };

        if (st.info.attributes) {
          stream.attributes = st.info.attributes;
        }

        if (st.type === 'mixed') {
          stream.view = st.info.label;
          stream.video.layout = st.info.layout.map(convertStreamRegion);
          if (st.info.label === 'common') {
            that.commonViewStream = st.id;
          }
          stream.from = '';
        } else {
          stream.from = st.info.owner;
        }

        if (st.media.video) {
          if (st.type === 'mixed') {
            stream.video.device = 'mcu';
            stream.video.resolutions = [st.media.video.parameters.resolution];

            st.media.video.optional && st.media.video.optional.parameters && st.media.video.optional.parameters.resolution && st.media.video.optional.parameters.resolution.forEach(function(reso) {
              stream.video.resolutions.push(reso);
            });
          } else if (st.media.video.source === 'screen-cast'){
            stream.video.device = 'screen';
          } else if (st.media.video.source === 'camera') {
            stream.video.device = 'camera';
          }
        }
        return stream;
      }),
      users: response.room.participants.map((ptt) => {
        return {
          id: ptt.id,
          name: ptt.user,
          role: ptt.role
        };
      })
    };
  };

  that.notify = function(event, data) {
    log.debug('notify, event:', event, 'data:', data);
    if (event === 'participant') {
      notifyParticipantActivity(data);
    } else if (event === 'stream') {
      notifyStreamInfo(data);
    } else if (event === 'progress') {
      notifySessionProgress(data);
    } else {
      sendMsg(event, data);
    }
  };

  that.join = (token) => {
    return portal.join(clientId, token)
      .then(function(result){
        that.inRoom = result.data.room.id;
        that.tokenCode = result.tokenCode;
        return convertJoinResponse(result.data);
      });
  };

  that.leave = () => {
    if(that.inRoom) {
      return portal.leave(that.id).catch(() => {
        that.inRoom = undefined;
        that.tokenCode = undefined;
      });
    }
  };

  that.resetConnection = (conn) => {
    that.connection.close(false);
    that.connection = conn;
    listenAt(that.connection.socket);
    return Promise.resolve('ok');
  };

  that.drop = () => {
    that.connection.sendMessage('drop');
    that.leave();
    that.connection.close(true);
  };

  listenAt(that.connection.socket);
  return that;
};

module.exports = LegacyClient;

