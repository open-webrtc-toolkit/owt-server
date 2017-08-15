/* global require */
'use strict';

var path = require('path');
var url = require('url');
var log = require('./logger').logger.getLogger('SocketIOServer');
var crypto = require('crypto');
var vsprintf = require("sprintf-js").vsprintf;

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
  'best_quality': '1.4x',
  'better_quality': '1.2x',
  'standard': '1.0x',
  'better_speed': '0.8x',
  'best_speed': '0.6x'
};

function qualityLevel2BitrateLevel (ql) {
  return ql2brl[ql] ? ql2brl[ql] : '1.0x';
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

var Client = function(participant_id, socket, portal, observer, reconnection_spec, on_disconnect) {
  var that = {};
  // If reconnection is enabled, server will keep session for |reconnectionTimeout| seconds after client is disconnected. Client should send "logout" before leaving.
  let reconnection_enabled = false;
  let reconnection_ticket_id;
  let pending_messages = [];  // Messages need to be sent when reconnection is success.
  let disconnect_timeout;  // Timeout function for disconnection. It will be executed if disconnect timeout reached, will be cleared if other client valid reconnection ticket success.
  let disconnected = false;
  let old_clients = [];  // Old clients before reconnections.
  let published = {/*StreamId: {type: "webrtc" | "streamingIn", mix: boolean(IfMix)}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)
  let ref2subId = {/*PeerId | StreamingOutURL | RecorderId: SubscriptionId}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)
  let subId2ref = {/*SubscriptionId: PeerId | StreamingOutURL | RecorderId}*/}; //FIXME: for compatibility purpose for old clients(such as v3.4.x)

  // client_info has client version and platform.
  const checkClientAbility = function(ua){
    if(!ua||!ua.sdk||!ua.sdk.version||!ua.sdk.type||!ua.runtime||!ua.runtime.version||!ua.runtime.name||!ua.os||!ua.os.version||!ua.os.name){
      return false;
    }
    // client_info is introduced in 3.3. It's the same version as reconnection. So it's safe to ignore SDK version.
    // JavaScript SDK 3.3 does not support reconnection.
    if(ua.sdk.type === 'Objective-C' || ua.sdk.type === 'C++' || ua.sdk.type === 'Android'){
      reconnection_enabled = true;
    }
    return true;
  };

  const calculateSignature = function(reconnection_ticket) {
    const to_sign = vsprintf('%s,%s,%s', [
      reconnection_ticket.participantId, reconnection_ticket.notBefore,
      reconnection_ticket.notAfter
    ]);
    const signed = crypto.createHmac('sha256', reconnection_spec.reconnectionKey)
                     .update(to_sign)
                     .digest('hex');
    return (new Buffer(signed)).toString('base64');
  };

  const generateReconnectionTicket = function() {
    const now = Date.now();
    const ticket = {
      participantId: participant_id,
      ticketId: Math.random().toString(36).substring(2),
      notBefore: now,
      // Unit for reconnectionTicketLifetime is second.
      notAfter: now + reconnection_spec.reconnectionTicketLifetime * 1000
    };
    ticket.signature = calculateSignature(ticket);
    reconnection_ticket_id = ticket.ticketId;
    return (new Buffer(JSON.stringify(ticket))).toString('base64');
  };

  const validateReconnectionTicket = function(ticket) {
    if(!reconnection_enabled) {
      return Promise.reject('Reconnection is not allowed.');
    }
    if(ticket.participantId!==participant_id){
      return Promise.reject('Participant ID is not matched.');
    }
    let signature = calculateSignature(ticket);
    if(signature!=ticket.signature){
      return Promise.reject('Invalid reconnection ticket signature');
    }
    const now = Date.now();
    if(now<ticket.notBefore||now>ticket.notAfter){
      return Promise.reject('Ticket is expired.');
    }
    if(disconnect_timeout){
      clearTimeout(disconnect_timeout);
      disconnect_timeout=undefined;
    }
    disconnected = true;
    socket.disconnect(true);
    return Promise.resolve();
  };

  const send_msg = function (event, data) {
    //log.debug('send_msg, event:', event, 'data:', data);
    try {
      socket.emit(event, data);
    } catch (e) {
      log.error('socket.emit error:', e.message);
    }
  };

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

  const getViewLabelFromStreamId = (streamId) => {
    var bar_pos = streamId.indexOf("-");
    return ((bar_pos >= 0) && (bar_pos < (streamId.length - 1))) ? streamId.substring(bar_pos + 1) : streamId;
  };

  that.listen = function() {
    // Join portal. It returns room info.
    const joinPortal = function(participant_id, token){
      return portal.join(participant_id, token).then(function(result){
        if(disconnected){
          portal.leave(participant_id).catch(function(err) {
            log.warn('Portal leave:', err);
          });
          return Promise.reject('Leaved conference before join success.');
        }
        that.inRoom = result.room.id;
        that.tokenCode = result.tokenCode;
        observer.onJoin(result.tokenCode);
        return {clientId: participant_id,
                id: result.room.id,
                streams: result.room.streams.map((st) => {
                  var stream = {
                    id: st.id,
                    audio: !!st.media.audio,
                    video: st.media.video ? {} : false,
                    socket: ''
                  };

                  if (st.type === 'mixed') {
                    stream.view = st.info.label;
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
                users: result.room.participants.map((ptt) => {
                  return {
                    id: ptt.id,
                    name: ptt.user,
                    role: ptt.role
                  };
                })};
      });
    };

    const joinPortalFailed = function(err, callback){
      const err_message = getErrorMessage(err);
      safeCall(callback, 'error', err_message);
      log.info('Participant', participant_id, 'join portal failed, reason:', err_message);
      socket.disconnect();
    };

    socket.on('login', function(login_info, callback) {
      new Promise(function(resolve){
        resolve(JSON.parse((new Buffer(login_info.token, 'base64')).toString()));
      }).then(function(token){
        return new Promise(function(resolve, reject){
          if(checkClientAbility(login_info.userAgent)){
            resolve(token);
          } else {
            reject('User agent info is incorrect.');
          }
        });
      }).then(function(token){
        return joinPortal(participant_id, token);
      }).then(function(room_info) {
        if(reconnection_enabled){
          room_info.reconnectionTicket=generateReconnectionTicket();
        }
        safeCall(callback, 'success', room_info);
      }).catch(function(err) {
        joinPortalFailed(err, callback);
      });
    });

    // Login after reconnection.
    socket.on('relogin', function(reconnection_ticket, callback) {
      new Promise(function(resolve){
        resolve(JSON.parse((new Buffer(reconnection_ticket, 'base64')).toString()));
      }).then(function(ticket){
        return reconnection_spec.reconnectionCallback(participant_id, ticket);
      }).then(function(result){
        // Authentication passed. Restore session.
        pending_messages=pending_messages.concat(result.pendingStreams);
        old_clients=result.oldClients;
        that.inRoom=result.roomId;
        participant_id=result.participantId;
        published=result.published;
        ref2subId=result.ref2subId;
        subId2ref=result.subId2ref;
        for(let client of old_clients){
          client.replaceSocket(socket);
        }
      }).then(function(){
        reconnection_enabled=true;
        const ticket = generateReconnectionTicket();
        safeCall(callback, 'success', ticket);
      }).catch(function(err){
        const err_message = getErrorMessage(err);
        log.info('relogin failed:', err_message);
        safeCall(callback, 'error', err_message);
      }).then(function(){
        for(let message of pending_messages){
          if (message && message.event) {
            that.notify(message.event, message.data);
          }
        }
        pending_messages=[];
      });
    });

    socket.on('refreshReconnectionTicket', function(callback){
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      if(!reconnection_enabled)
        safeCall(callback,'error','Reconnection is not enabled.');
      const ticket = generateReconnectionTicket();
      safeCall(callback, 'success', ticket);
    });

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

      return portal.publish(participant_id, stream_id, pub_info)
      .then(function(result) {
        log.debug('portal.publish -', result);
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

      return portal.unpublish(participant_id, streamId)
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
          return portal.streamControl(participant_id,
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
          return portal.streamControl(participant_id,
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
      (sub_desc.media.video.spec || (sub_desc.media.video.spec = {})) && (sub_desc.media.video.spec.resolution = options.video.resolution);
      options.video && (typeof options.video.resolution === 'string') && (sub_desc.media.video.resolution = resolution2WidthHeight(options.video.resolution));
      options.video && options.video.quality_level && (sub_desc.media.video.spec || (sub_desc.media.video.spec = {})) && (sub_desc.media.video.spec.bitrateLevel = qualityLevel2BitrateLevel(options.video.quality_level));

      if (!sub_desc.media.audio && !sub_desc.media.video) {
          return safeCall(callback, 'error', 'bad options');
      }

      return portal.subscribe(participant_id, subscription_id, sub_desc
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

      return portal.unsubscribe(participant_id,  subscription_id)
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

      var session_id = (published[message.streamId] ? message.streamId : ref2subId[message.streamId]);
      if (session_id) {
        portal.onSessionSignaling(participant_id, session_id, message.msg);
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
          audio: (options.audio === false ? false: {from: target_stream_id, spec: {codec: options.audio && options.audio.codecs ? options.audio.codecs[0] : 'aac'}}),
          video: (options.video === false ? false: {from: target_stream_id, spec: {codec: options.video && options.video.codecs ? options.video.codecs[0] : 'h264'}})
        },
        connection: {
          url: parsed_url.format()
        }
      };
      ((sub_desc.media.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (sub_desc.media.video.spec.resolution = options.resolution);
      (sub_desc.media.video && (typeof options.resolution === 'string')) && (sub_desc.media.video.spec.resolution = resolution2WidthHeight(options.resolution));

      return portal.subscribe(participant_id, subscription_id, sub_desc
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
        update.video.spec = {};
        (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number') && (update.video.spec.resolution = options.resolution);
        (typeof options.resolution === 'string') && (update.video.spec.resolution = resolution2WidthHeight(options.resolution));
      }

      return portal.subscriptionControl(participant_id, subscription_id, {operation: 'update', data: update}
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

      return portal.unsubscribe(participant_id, subscription_id)
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
        return portal.subscriptionControl(participant_id, subscription_id, update)
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
      sub_desc.media.audio && (sub_desc.media.audio.codec = (function(c) {return (c === 'opus' ? 'opus_48000_2' : (c === 'aac' ? 'aac_48000_2' : c));})(options.audioCodec || 'opus'));

      (options.videoStreamId || unspecifiedStreamIds) && (sub_desc.media.video = {from: options.videoStreamId || that.commonViewStream});
      sub_desc.media.video && (sub_desc.media.video.codec = (options.videoCodec || 'vp8'));

      return portal.subscribe(participant_id, subscription_id, sub_desc)
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

      return portal.unsubscribe(participant_id, subscription_id)
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
      return portal.streamControl(participant_id, options.id, {operation: 'get-region', data: view_label})
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
      return portal.streamControl(participant_id, options.id, {operation: 'set-region', data: {view: view_label, region: options.region}})
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

      return portal.streamControl(participant_id, options.streamId, {operation: 'pause', data: options.track})
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

      return portal.streamControl(participant_id, options.streamId, {operation: 'play', data: options.track})
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

      return portal.setPermission(participant_id, options.targetId, [{operation: options.action, value: options.update}])
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

          return portal.text(participant_id, msg.receiver, msg.data)
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
            return portal.streamControl(participant_id, msg.payload.streamId, {operation: operation, data: track})
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
            return portal.subscriptionControl(participant_id, subscription_id, {operation: operation, data: track})
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

    const leavePortal = function(){
      if(that.inRoom){
        return portal.leave(participant_id).then(function(){
          that.tokenCode && observer.onLeave(that.tokenCode);
          that.inRoom = undefined;
          that.tokenCode = undefined;
        });
      } else {
        return Promise.reject('Not in a conference.');
      }
    };

    socket.on('logout', function(callback){
      if(!that.inRoom){
        return safeCall(callback, 'error', 'Illegal request');
      }

      log.debug(vsprintf('Reconnection for %s is disabled because of client logout.', [participant_id]));
      reconnection_enabled=false;
      leavePortal().then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        // Expect client does not response to this error and disconnect soon.
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('disconnect', function(reason) {
      log.debug(participant_id+' disconnected, reason: '+reason);
      if(disconnected){
        return;
      }else if(reconnection_enabled){
        disconnect_timeout = setTimeout(function(){
          log.info(participant_id+' failed to reconnect. Leaving portal.');
          leavePortal().catch(function(err) {
            log.warn('LeavePortal:', err);
          });
          disconnected = true;
          on_disconnect();
        }, reconnection_spec.reconnectionTimeout*1000);
      } else {
        log.debug(participant_id+' is going to leave portal');
        leavePortal().catch(function(err) {
          //log.warn('LeavePortal:', err);
        });
        disconnected = true;
        on_disconnect();
      }
    });
  };

  const notifyParticipantActivity = (participantActivity) => {
    if (participantActivity.action === 'join') {
      send_msg('user_join', {user: {id: participantActivity.data.id, name: participantActivity.data.user.name, role: participantActivity.data.role}});
    } else if (participantActivity.action === 'leave') {
      send_msg('user_leave', {user: {id: participantActivity.data}});
    } else {
      log.info('Unknown participant activity message:', participantActivity);
    }
  };

  const notifyStreamInfo = (streamInfo) => {
    log.debug('notifyStreamInfo, streamInfo:', streamInfo);
    if (streamInfo.status === 'add') {
      send_msg('add_stream', {id: streamInfo.data.id, audio: !!streamInfo.data.media.audio, video: !!streamInfo.data.media.video ? {device: streamInfo.data.type === 'mixed'? 'mcu' : streamInfo.data.media.video.source} : false, from: streamInfo.data.type === 'mixed' ? '' : streamInfo.data.info.owner, attributes: streamInfo.data.info.attributes});
    } else if (streamInfo.status === 'update') {
      var st_update = {id: streamInfo.id};
      if ((streamInfo.data.field === 'audio.status') || (streamInfo.data.field === 'video.status')) {//Forward stream update
        st_update.event = 'StateChange';
        st_update.state = streamInfo.data.value;
      } else if (streamInfo.data.field === 'video.layout') {//Mixed stream update
        st_update.event = 'VideoLayoutChanged';
        st_update.data = streamInfo.data.value.map((stream2region) => {//FIXME: Client side need to handle the incompetibility, since the definition of region has been extended.
          return {
            streamId: stream2region.stream,
            id: stream2region.region.id,
            left: stream2region.region.area.left,
            top: stream2region.region.area.top,
            relativeSize: stream2region.region.area.width
          };
        });
      }
      send_msg('update_stream', st_update);
    } else if (streamInfo.status === 'remove') {
      send_msg('remove_stream', {id: streamInfo.id});
    } else {
      log.info('Unknown stream info:', streamInfo);
    }
  };

  const notifySessionProgress = (sessionProgress) => {
    var id = sessionProgress.id;
    if (sessionProgress.status === 'soac') {
      if (published[id]) {
        send_msg('signaling_message_erizo', {streamId: id, mess: sessionProgress.data});
      } else {
        var peer_id = subId2ref[id];
        if (peer_id) {
          send_msg('signaling_message_erizo', {peerId: peer_id, mess: sessionProgress.data});
        }
      }
    } else if (sessionProgress.status === 'ready') {
      if (published[id]) {
        if (published[id].type === 'webrtc') {
          send_msg('signaling_message_erizo', {streamId: id, mess: {type: 'ready'}});
        }

        if (published[id].mix) {
          portal.streamControl(participant_id, id, {operation: 'mix', data: 'common'})
            .catch((err) => {
              log.info('Mix stream failed, reason:', getErrorMessage(err));
            });
        }
      } else {
        var peer_id = subId2ref[id];
        if (peer_id) {
          send_msg('signaling_message_erizo', {peerId: peer_id, mess: {type: 'ready'}});
        }
      }
    } else if (sessionProgress.status === 'error') {
      var ref = subId2ref[id];
      if (ref) {
        if (ref === id) {
          var recorder_id = id;
          log.debug('recorder error, recorder_id:', ref);
          portal.unsubscribe(participant_id, id);
          send_msg('remove_recorder', {id: recorder_id});
        } else if (ref.indexOf('rtsp') !== -1 || ref.indexOf('rtmp') !== -1 || ref.indexOf('http') !== -1) {
          send_msg('connection_failed', {url: ref});
        } else {
          send_msg('connection_failed', {streamId: ref});
        }
      } else {
        send_msg('connection_failed', {streamId: id});
      }
    } else {
      log.info('Unknown session progress message:', sessionProgress);
    }
  };

  that.notify = function(event, data) {
    log.debug('notify, event:', event, 'data:', data);
    var protocol_version = '0.0.1';//FIXME: The actual protocol version should be considered here.
    if (protocol_version.startsWith('1.')) {
      send_msg(event, data);
    } else {
      if (event === 'participant') {
        notifyParticipantActivity(data);
      } else if (event === 'stream') {
        notifyStreamInfo(data);
      } else if (event === 'progress') {
        notifySessionProgress(data);
      } else {
        send_msg(event, data);
      }
    }
  };

  that.drop = function() {
    send_msg('drop');  // Explicitly let client know it is dropped.
    reconnection_enabled = false;
    socket.disconnect();
  };

  that.validateReconnectionTicket = function(ticket){
    if(ticket.participantId!==participant_id){
      return Promise.reject('Participant ID is not matched.');
    }
    return validateReconnectionTicket(ticket).then(function(){
      old_clients.push(that);
      return {
        pendingMessages:pending_messages,
        oldClients:old_clients,
        published: published,
        ref2subId: ref2subId,
        subId2ref: subId2ref
      };
    });
  };

  // Replace old socket object with the new one after reconnection. If the old
  // socket is not being replaced, callbacks will use a dead socket which leads
  // to message lost. Socket should be replaced after every successful
  // reconnection. That is, socket object is always the latest one for current
  // participant.
  that.replaceSocket = function(newSocket){
    socket = newSocket;
  }

  return that;
};


var SocketIOServer = function(spec, portal, observer) {
  var that = {};
  var io;
  var clients = {};
  // A Socket.IO server has a unique reconnection key. Client cannot reconnect to another Socket.IO server in the cluster.
  var reconnection_key = require('crypto').randomBytes(64).toString('hex');
  var sioOptions = {};
  if (spec.pingInterval) {
    sioOptions.pingInterval = spec.pingInterval * 1000;
  }
  if (spec.pingTimeout) {
    sioOptions.pingTimeout = spec.pingTimeout * 1000;
  }

  var startInsecure = function(port) {
    var server = require('http').createServer().listen(port);
    io = require('socket.io').listen(server, sioOptions);
    run();
    return Promise.resolve('ok');
  };

  var startSecured = function(port, keystorePath) {
    return new Promise(function(resolve, reject) {
      var cipher = require('./cipher');
      var keystore = path.resolve(path.dirname(keystorePath), '.woogeen.keystore');
      cipher.unlock(cipher.k, keystore, function(err, passphrase) {
        if (!err) {
          var server = require('https').createServer({pfx: require('fs').readFileSync(keystorePath), passphrase: passphrase}).listen(port);
          io = require('socket.io').listen(server, sioOptions);
          run();
          resolve('ok');
        } else {
          reject(err);
        }
      });
    });
  };

  var run = function() {
    io.sockets.on('connection', function(socket) {
      var participant_id = socket.id;
      /*
        Reconnection logic:
        1. New client asks for reconnection ticket verification.
        2. |reconnection_callback| is executed, it finds old client from |clients| and sends ticket to the old client.
        3. Old client verify this ticket. (Old client knows ticket ID, reconnection enabled or not). If verification success, it returns all client information for session recovery.
        4. Server replaces old client with the new client.
        5. New client recovery old session with information provided by the old client.
        6. New client emits pending messages.
      */
      const reconnection_callback = function(participant_id, ticket) {
        // Check ticket and replace old client with the new one. |participant_id| is current client's participant ID.
        const client = clients[ticket.participantId];
        if(!client){
          return Promise.reject('Invalid reconnection ticket.')
        }
        return client.validateReconnectionTicket(ticket).then(function(old_client_info){
          clients[ticket.participantId]=clients[participant_id];
          delete clients[participant_id];
          return Promise.resolve({
            pendingStreams:old_client_info.pendingMessages,
            roomId:client.inRoom,
            participantId:ticket.participantId,
            oldClients:old_client_info.oldClients,
            published: old_client_info.published,
            ref2subId: old_client_info.ref2subId,
            subId2ref: old_client_info.subId2ref
          });
        });
      };
      const reconnection_spec = {
        reconnectionTicketLifetime: spec.reconnectionTicketLifetime,
        reconnectionTimeout: spec.reconnectionTimeout,
        reconnectionKey: reconnection_key,
        reconnectionCallback: reconnection_callback
      };
      clients[participant_id] = Client(participant_id, socket, portal, observer, reconnection_spec, function() {
        delete clients[participant_id];
      });
      clients[participant_id].listen();
    });
  };

  that.start = function() {
    if (!spec.ssl) {
      return startInsecure(spec.port);
    } else {
      return startSecured(spec.port, spec.keystorePath);
    }
  };

  that.stop = function() {
    clients = {};
    io && io.close();
    io = undefined;
  };

  that.notify = function(participantId, event, data) {
    log.debug('notify participant:', participantId, 'event:', event, 'data:', data);
    if (clients[participantId]) {
      clients[participantId].notify(event, data);
      return Promise.resolve('ok');
    } else {
      return Promise.reject('participant does not exist');
    }
  };

  that.drop = function(participantId) {
    if (participantId === 'all') {
      for(var pid in clients) {
        clients[pid].drop();
      }
      return Promise.resolve('ok');
    } else if (clients[participantId]) {
      clients[participantId].drop();
      return Promise.resolve('ok');
    } else {
      return Promise.reject('user not in room');
    }
  };

  return that;
};



module.exports = SocketIOServer;

