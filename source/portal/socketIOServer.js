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

function safeCall () {
  var callback = arguments[0];
  if (typeof callback === 'function') {
    var args = Array.prototype.slice.call(arguments, 1);
    callback.apply(null, args);
  }
}

var Client = function(participantId, socket, portal, on_disconnect) {
  var that = {};
  var participant_id = participantId;

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
        log.info('token login failed:', err.message);
        safeCall(callback, 'error', err.message);
        socket.disconnect();
      });
    });

    socket.on('publish', function(options, url, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      var connection_type, stream_id;
      var stream_description = {};

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
      stream_description.video && (typeof stream_description.video.resolution !== 'string') && (stream_description.video.resolution = 'unknown');
      stream_description.video && (typeof stream_description.video.device !== 'string') && (stream_description.video.device = 'unknown');
      var unmix = (options.unmix === true || (stream_description.video && (stream_description.video.device === 'screen'))) ? true : false;

      return portal.publish(participant_id, connection_type, stream_description, function(status) {
        if (status.type === 'failed') {
          socket.emit('connection_failed', {});
          safeCall(callback, 'error', status.reason);
        } else {
          if (connection_type === 'webrtc') {
            socket.emit('signaling_message_erizo', {streamId: stream_id, mess: status});
          } else {
            safeCall(callback, status);
          }
        }
      }, unmix).then(function(streamId) {
        stream_id = streamId;
        if (connection_type === 'webrtc') {
          safeCall(callback, 'initializing', streamId);
        } else {
          safeCall(callback, 'success', streamId);
        }
      }).catch(function(err) {
        log.info('portal.publish exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('unpublish', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unpublish(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }, function(err) {
        log.info('portal.unpublish failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.unpublish exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('addToMixer', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.mix(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }, function(err) {
        log.info('portal.mix failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.mix exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('removeFromMixer', function(streamId, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unmix(participant_id, streamId)
      .then(function() {
        safeCall(callback, 'success');
      }, function(err) {
        log.info('portal.unmix failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.unmix exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('setVideoBitrate', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.setVideoBitrate(participant_id, options.id, options.bitrate)
      .then(function() {
        safeCall(callback, 'success');
      }, function(err) {
        log.info('portal.setVideoBitrate failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.setVideoBitrate exception:', err.message);
        safeCall(callback, 'error', err.message);
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

      var subscription_id;
      return portal.subscribe(participant_id, 'webrtc', subscription_description, function(status) {
        if (status.type === 'failed') {
          safeCall(callback, 'error', status.reason);
        } else {
          socket.emit('signaling_message_erizo', {peerId: options.streamId/*FIXME -a */, mess: status});
        }
      }).then(function(subscriptionId) {
        subscription_id = subscriptionId;
        safeCall(callback, 'initializing', subscriptionId);
      }).catch(function(err) {
        log.info('portal.subscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
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
      }, function(err) {
        log.info('portal.unsubscribe failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.unsubscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('signaling_message', function(message, to_to_deprecated, callback) {
      portal.onConnectionSignalling(participant_id, message.streamId, message.msg);
      safeCall(callback, 'ok');
    });

    socket.on('startRtspOut', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (typeof options.url !== 'string' || options.url === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:') || !parsed_url.slashes || !parsed_url.host) {
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
      subscription_description.url = options.url;

      var subscription_id, stream_url;
      return portal.subscribe(participant_id, 'avstream', subscription_description, function(status) {
        if (status.type === 'failed') {
          safeCall(callback, 'error', status.reason);
        } else if (status.type === 'ready') {
          safeCall(callback, 'success', {id: subscription_id, url: stream_url});
        }
      }).then(function(subscriptionId) {
        subscription_id = subscriptionId;
        var url_lead = parsed_url;
        url_lead.pathname = path.join(url_lead.pathname || '/', 'room_' + that.inRoom + '-' + subscription_id + '.sdp');
        stream_url = url_lead.format();
      }).catch(function(err) {
        log.info('portal.subscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('stopRtspOut', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (typeof options.id !== 'string' || options.id === '') {
        return safeCall(callback, 'error', 'Invalid RTSP/RTMP id');
      }

      return portal.unsubscribe(participant_id, options.id)
      .then(function() {
        safeCall(callback, 'success', {id: options.id});
      }, function(err) {
        log.info('portal.unsubscribe failed:', err.message);
        safeCall(callback, 'error', 'Invalid RTSP/RTMP id');
      }).catch(function(err) {
        log.info('portal.unsubscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('startRecorder', function(options, callback) {
      if (!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (options.recorderId && typeof options.recorderId !== 'string') {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      var unspecifiedStreamIds = (options.audioStreamId === undefined && options.videoStreamId === undefined);

      if ((options.audioStreamId || unspecifiedStreamIds) && (options.audioCodec !== undefined) && ((typeof options.audioCodec !== 'string') || options.audioCodec === '')) {
        return safeCall(callback, 'error', 'Invalid audio codec');
      }

      if ((options.videoStreamId || unspecifiedStreamIds) && (options.videoCodec !== undefined) && ((typeof options.videoCodec !== 'string') || options.videoCodec === '')) {
        return safeCall(callback, 'error', 'Invalid video codec');
      }

      var subscription_description = {audio: false, video: false};
      (options.audioStreamId || unspecifiedStreamIds) && (subscription_description.audio = {fromStream: options.audioStreamId || that.inRoom});
      (subscription_description.audio && (typeof options.audioCodec === 'string')) && (subscription_description.audio.codecs = [options.audioCodec]);
      subscription_description.audio && (subscription_description.audio.codecs = subscription_description.audio.codecs || ['pcmu']);
      (options.videoStreamId || unspecifiedStreamIds) && (subscription_description.video = {fromStream: options.videoStreamId || that.inRoom});
      (subscription_description.video && (typeof options.videoCodec === 'string')) && (subscription_description.video.codecs = [options.videoCodec]);
      subscription_description.video && (subscription_description.video.codecs = subscription_description.video.codecs || ['vp8']);
      options.path && (subscription_description.path = options.path);
      options.recorderId && (subscription_description.recorderId = options.recorderId);
      subscription_description.interval = (options.interval && options.interval > 0) ? options.interval : -1;

      var subscription_id, recording_file;
      return portal.subscribe(participant_id, 'recording', subscription_description, function(status) {
        if (status.type === 'failed') {
          safeCall(callback, 'error', status.reason);
        } else if (status.type === 'ready') {
          safeCall(callback, 'success', {recorderId: subscription_id, path: recording_file});
        }
      }).then(function(subscriptionId) {
        subscription_id = subscriptionId;
        recording_file = path.join(options.path || '', 'room_' + that.inRoom + '-' + subscription_id + '.mkv' );
      }).catch(function(err) {
        log.info('portal.subscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('stopRecorder', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (typeof options.recorderId !== 'string' || options.recorderId === '') {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      return portal.unsubscribe(participant_id, options.recorderId)
      .then(function() {
        safeCall(callback, 'success', {recorderId: options.recorderId});
      }, function(err) {
        log.info('portal.unsubscribe failed:', err.message);
        safeCall(callback, 'error', 'Invalid recorder id');
      }).catch(function(err) {
        log.info('portal.unsubscribe exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('getRegion', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (typeof options.id !== 'string' || options.id === '') {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      return portal.getRegion(participant_id, options.id)
      .then(function(regionId) {
        safeCall(callback, 'success', {region: regionId});
      }, function(err) {
        log.info('portal.getRegion failed:', err.message);
        safeCall(callback, 'error', 'Invalid stream id');
      }).catch(function(err) {
        log.info('portal.getRegion exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

    socket.on('setRegion', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (typeof options.id !== 'string' || options.id === '') {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      if (typeof options.region !== 'string' || options.region === '') {
        return safeCall(callback, 'error', 'Invalid region id');
      }

      return portal.setRegion(participant_id, options.id, options.region)
      .then(function() {
        safeCall(callback, 'success');
      }, function(err) {
        log.info('portal.setRegion failed:', err.message);
        safeCall(callback, 'error', err.message);
      }).catch(function(err) {
        log.info('portal.setRegion exception:', err.message);
        safeCall(callback, 'error', err.message);
      });
    });

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
            }, function(err) {
              log.info('portal.text failed:', err.message);
              safeCall(callback, 'error', err.message);
            }).catch(function(err) {
              log.info('portal.text exception:', err.message);
              safeCall(callback, 'error', err.message);
            });
        case 'control':
          if (typeof msg.payload !== 'object') {
            return safeCall(callback, 'error', 'Invalid payload');
          }

          if (typeof msg.payload.streamId !== 'string' || msg.payload.streamId === '') {
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
            }, function(err) {
              log.info('portal.mediaOnOff failed:', err.message);
              safeCall(callback, 'error', err.message);
            }).catch(function(err) {
              log.info('portal.mediaOnOff exception:', err.message);
              safeCall(callback, 'error', err.message);
            });
        default:
          return safeCall(callback, 'error', 'Invalid message type');
      }
    });

    socket.on('disconnect', function() {
      if (that.inRoom) {
        portal.leave(participant_id).catch(function(err) {
          log.info('portal.leave exception:', err.message);
        });
      }
      on_disconnect();
    });
  };

  that.notify = function(event, data) {
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

  return that;
};

module.exports = SocketIOServer;

