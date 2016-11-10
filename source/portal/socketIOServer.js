/* global require */
'use strict';

var path = require('path');
var url = require('url');
var log = require('./logger').logger.getLogger('SocketIOServer');

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

var idPattern = /^[0-9a-zA-Z]+$/;
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

var Client = function(participant_id, socket, portal, on_disconnect) {
  var that = {};

  that.listen = function() {
    socket.on('token', function(token, callback) {
      portal.join(participant_id, token)
      .then(function(result) {
        that.inRoom = result.session_id;
        safeCall(callback, 'success', {clientId: participant_id,
                                       id: result.session_id,
                                       streams: result.streams.map(function(st) {
                                         st.video && (st.video.resolutions instanceof Array) && (st.video.resolutions = st.video.resolutions.map(resolution2WidthHeight));
                                         return st;
                                       }),
                                       users: result.participants});
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('token login failed:', err_message);
        safeCall(callback, 'error', err_message);
        socket.disconnect();
      });
    });

    socket.on('publish', function(options, url, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      var stream_id = Math.random() * 1000000000000000000 + '',
        connection_type,
        stream_description = {};

      if (options.state === 'erizo') {
        connection_type = 'webrtc';
      } else if (options.state === 'url') {
        connection_type = 'avstream';
        stream_description.url = url;
        stream_description.transport = options.transport;
        stream_description.bufferSize = options.bufferSize;
      } else {
        return safeCall(callback, 'error', 'stream type(options.state) error.');
      }

      stream_description.audio = (options.audio === undefined ? true : !!options.audio);
      stream_description.video = (options.video === false ? false : (typeof options.video === 'object' ? options.video : {}));
      ((stream_description.video && options.video && options.video.resolution && (typeof options.video.resolution.width === 'number') && (typeof options.video.resolution.height === 'number')) && (stream_description.video.resolution = widthHeight2Resolution(options.video.resolution.width, options.video.resolution.height))) ||
      (stream_description.video && (typeof stream_description.video.resolution !== 'string' || stream_description.video.resolution === '') && (stream_description.video.resolution = 'unknown'));
      stream_description.video && (typeof stream_description.video.device !== 'string' || stream_description.video.device === '') && (stream_description.video.device = 'unknown');
      var unmix = (options.unmix === true || (stream_description.video && (stream_description.video.device === 'screen'))) ? true : false;

      return portal.publish(participant_id, stream_id, connection_type, stream_description, function(status) {
        if (status.type === 'failed') {
          socket.emit('connection_failed', {});
          safeCall(callback, 'error', status.reason);
        } else {
          if (connection_type === 'webrtc') {
            if (status.type === 'initializing') {
              safeCall(callback, 'initializing', stream_id);
            } else {
              socket.emit('signaling_message_erizo', {streamId: stream_id, mess: status});
            }
          } else {
            if (status.type === 'ready') {
              safeCall(callback, 'success', stream_id);
            } else if (status.type !== 'initializing') {
              safeCall(callback, status);
            }
          }
        }
      }, unmix).then(function(connectionLocality) {
        log.debug('portal.publish succeeded, connection locality:', connectionLocality);
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.publish failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('unpublish', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unpublish(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unpublish failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('addToMixer', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.mix(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.mix failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('removeFromMixer', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unmix(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unmix failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('setVideoBitrate', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.setVideoBitrate(participant_id, options.id, options.bitrate)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.setVideoBitrate failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('subscribe', function(options, unused, callback) {
      if (!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      var subscription_description = {};
      (options.audio || options.audio === undefined) && (subscription_description.audio = {fromStream: options.streamId});
      (options.video || options.video === undefined) && (subscription_description.video = {fromStream: options.streamId});
      (options.video && options.video.resolution && (typeof options.video.resolution.width === 'number') && (typeof options.video.resolution.height === 'number')) &&
      (subscription_description.video.resolution = widthHeight2Resolution(options.video.resolution.width, options.video.resolution.height));

      //FIXME - a: use the target stream id as the subscription_id to keep compatible with client SDK, should be fixed and use random strings independently later.
      var subscription_id = participant_id + '-sub-' + ((subscription_description.audio && subscription_description.audio.fromStream) ||
                                                        (subscription_description.video && subscription_description.video.fromStream));

      return portal.subscribe(participant_id, subscription_id, 'webrtc', subscription_description, function(status) {
        if (status.type === 'failed') {
          safeCall(callback, 'error', status.reason);
        } else if (status.type === 'initializing') {
          safeCall(callback, 'initializing', options.streamId/*FIXME -a */);
        } else {
          socket.emit('signaling_message_erizo', {peerId: options.streamId/*FIXME -a */, mess: status});
        }
      }).then(function(connectionLocality) {
        log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.subscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('unsubscribe', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      var subscription_id = participant_id + '-sub-' + streamId; //FIXME - a: keep compatible with client SDK, should be refined later.
      return portal.unsubscribe(participant_id, subscription_id)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('signaling_message', function(message, to_to_deprecated, callback) {
      portal.onConnectionSignalling(participant_id, message.streamId, message.msg);
      safeCall(callback, 'ok');
    });

    socket.on('addExternalOutput', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      log.debug('Add serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:' && parsed_url.protocol !== 'http:') || !parsed_url.slashes || !parsed_url.host) {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      if (options.streamId === undefined) {
        options.streamId = that.inRoom;
      }

      var subscription_description = {};
      (options.audio || options.audio === undefined) && (subscription_description.audio = {fromStream: options.streamId, codecs: (options.audio && options.audio.codecs) || ['aac']});
      (options.video || options.video === undefined) && (subscription_description.video = {fromStream: options.streamId, codecs: (options.video && options.video.codecs) || ['h264']});
      ((subscription_description.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (subscription_description.video.resolution = widthHeight2Resolution(options.resolution.width, options.resolution.height));
      subscription_description.url = parsed_url.format();

      var subscription_id = subscription_description.url;
      return portal.subscribe(participant_id, subscription_id, 'avstream', subscription_description, function(status) {
        if (status.type === 'failed') {
          log.info('addExternalOutput onConnection error:', status.reason);
          safeCall(callback, 'error', status.reason);
        } else if (status.type === 'ready') {
          safeCall(callback, 'success', {url: subscription_description.url});
        }
      }).then(function(connectionLocality) {
        log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.subscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('updateExternalOutput', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      log.debug('Update serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:' && parsed_url.protocol !== 'http:') || !parsed_url.slashes || !parsed_url.host) {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      if (options.streamId === undefined) {
        options.streamId = that.inRoom;
      }

      var subscription_description = {};
      (options.audio || options.audio === undefined) && (subscription_description.audio = {fromStream: options.streamId, codecs: (options.audio && options.audio.codecs) || ['aac']});
      (options.video || options.video === undefined) && (subscription_description.video = {fromStream: options.streamId, codecs: (options.video && options.video.codecs) || ['h264']});
      ((subscription_description.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (subscription_description.video.resolution = widthHeight2Resolution(options.resolution.width, options.resolution.height));
      subscription_description.url = parsed_url.format();

      return portal.unsubscribe(participant_id, options.url)
      .then(function() {
        return portal.subscribe(participant_id, options.url, 'avstream', subscription_description, function(status) {
          if (status.type === 'failed') {
            log.info('updateExternalOutput onConnection error:', status.reason);
            safeCall(callback, 'error', status.reason);
          } else if (status.type === 'ready') {
            safeCall(callback, 'success', {url: subscription_description.url});
          }
        });
      }).then(function(subscriptionId) {
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unsubscribe/subscribe exception:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('removeExternalOutput', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      log.debug('Remove serverUrl:', options.url, 'options:', options);

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      return portal.unsubscribe(participant_id, options.url)
      .then(function() {
        safeCall(callback, 'success', {url: options.url});
      }, function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('startRecorder', function(options, callback) {
      if (!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
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

      if ((options.audioStreamId || unspecifiedStreamIds) && (options.audioCodec !== undefined) && (['pcmu', 'opus'].indexOf(options.audioCodec) < 0)) {
        return safeCall(callback, 'error', 'Invalid audio codec');
      }

      if ((options.videoStreamId || unspecifiedStreamIds) && (options.videoCodec !== undefined) && (['vp8', 'h264'].indexOf(options.videoCodec) < 0)) {
        return safeCall(callback, 'error', 'Invalid video codec');
      }

      var subscription_description = {audio: false, video: false};
      (options.audioStreamId || unspecifiedStreamIds) && (subscription_description.audio = {fromStream: options.audioStreamId || that.inRoom});
      (subscription_description.audio && (typeof options.audioCodec === 'string')) && (subscription_description.audio.codecs = [options.audioCodec]);
      subscription_description.audio && (subscription_description.audio.codecs = (subscription_description.audio.codecs || ['opus']).map(function(c) {return (c === 'opus' ? 'opus_48000_2' : c);}));
      (options.videoStreamId || unspecifiedStreamIds) && (subscription_description.video = {fromStream: options.videoStreamId || that.inRoom});
      (subscription_description.video && (typeof options.videoCodec === 'string')) && (subscription_description.video.codecs = [options.videoCodec]);
      subscription_description.video && (subscription_description.video.codecs = subscription_description.video.codecs || ['vp8']);
      options.path && (subscription_description.path = options.path);
      subscription_description.interval = (options.interval && options.interval > 0) ? options.interval : -1;

      var subscription_id = options.recorderId || formatDate(new Date, 'yyyyMMddhhmmssSS');
      var recording_file, recorder_added = false;
      return portal.subscribe(participant_id, subscription_id, 'recording', subscription_description, function(status) {
        if (status.type === 'failed') {
          if (recorder_added) {
            that.notify('remove_recorder', {id: subscription_id});
          } else {
            safeCall(callback, 'error', status.reason);
          }
        } else if (status.type === 'ready') {
          recorder_added = true;
          safeCall(callback, 'success', {recorderId: subscription_id, path: recording_file, host: 'unknown'});
        }
      }).then(function(connectionLocality) {
        log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
        recording_file = path.join(options.path || '', 'room_' + that.inRoom + '-' + subscription_id + '.mkv' );
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.subscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('stopRecorder', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!(options.recorderId === undefined || isValidIdString(options.recorderId))) {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      return portal.unsubscribe(participant_id, options.recorderId)
      .then(function() {
        safeCall(callback, 'success', {recorderId: options.recorderId, host: 'unknown'});
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.unsubscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('getRegion', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!isValidIdString(options.id)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      return portal.getRegion(participant_id, options.id)
      .then(function(regionId) {
        safeCall(callback, 'success', {region: regionId});
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.getRegion failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('setRegion', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!isValidIdString(options.id)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      if (!isValidIdString(options.region)) {
        return safeCall(callback, 'error', 'Invalid region id');
      }

      return portal.setRegion(participant_id, options.id, options.region)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.setRegion failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('setMute', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!options.streamId) {
        return safeCall(callback, 'error', 'no stream ID');
      }

      return portal.setMute(participant_id, options.streamId, options.muted)
      .then(function() {
          safeCall(callback, 'success');
        }).catch(function(err) {
          var err_message = (typeof err === 'string' ? err: err.message);
          log.info('portal.setMute failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('setPermission', function(options, callback) {
      if (!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!options.targetId) {
        return safeCall(callback, 'error', 'no targetId specified');
      }

      if (!options.act) {
        return safeCall(callback, 'error', 'no action specified');
      }

      return portal.setPermission(participant_id, options.targetId, options.act, options.updatedValue, false)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.info('portal.setPermission failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    })

    socket.on('customMessage', function(msg, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
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
              var err_message = (typeof err === 'string' ? err: err.message);
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
              on_off = cmdOpts[2];

          return portal.mediaOnOff(participant_id, msg.payload.streamId, track, direction, on_off)
            .then(function() {
              safeCall(callback, 'success');
            }).catch(function(err) {
              var err_message = (typeof err === 'string' ? err: err.message);
              log.info('portal.mediaOnOff failed:', err_message);
              safeCall(callback, 'error', err_message);
            });
        default:
          return safeCall(callback, 'error', 'Invalid message type');
      }
    });

    socket.on('disconnect', function() {
      if (that.inRoom) {
        portal.leave(participant_id).catch(function(err) {
          var err_message = (typeof err === 'string' ? err: err.message);
          log.info('portal.leave failed:', err_message);
        });
      }
      on_disconnect();
    });
  };

  that.notify = function(event, data) {
    log.debug('socket.emit, event:', event, 'data:', data);
    socket.emit(event, data);
  };

  that.drop = function() {
    socket.disconnect();
  };

  return that;
};


var SocketIOServer = function(spec, portal) {
  var that = {};
  var io;
  var clients = {};

  var startInsecure = function(port) {
    var server = require('http').createServer().listen(port);
    io = require('socket.io').listen(server);
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
          io = require('socket.io').listen(server);
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
      clients[participant_id] = Client(participant_id, socket, portal, function() {
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

  that.drop = function(participantId, fromRoom) {
    if (clients[participantId] && (fromRoom === undefined || clients[participantId].inRoom === fromRoom)) {
      clients[participantId].drop();
      return Promise.resolve('ok');
    } else {
      return Promise.reject('user not in room');
    }
  };

  that.updateMuteState = function(participantId, streamId, isMuted) {
    var state = isMuted? 'off' : 'on';

    var videoPromise = portal.mediaOnOff(participantId, streamId, 'video', 'in', state);
    var audioPromise = portal.mediaOnOff(participantId, streamId, 'audio', 'in', state);
    return Promise.all([videoPromise, audioPromise]);
  };

  that.updatePermission = function(targetId, act, updatedValue) {
    log.debug('permission update:', targetId, act, updatedValue);

    return portal.setPermission('session', targetId, act, updatedValue, true);
  };

  return that;
};



module.exports = SocketIOServer;

