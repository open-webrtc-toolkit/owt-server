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
        that.inRoom = result.session_id;
        that.tokenCode = result.tokenCode;
        observer.onJoin(result.tokenCode);
        return {clientId: participant_id,
                id: result.session_id,
                streams: result.streams.map(function(st) {
                  if (st.view === 'common') {
                    that.commonViewStream = st.id;
                  }
                  st.video && (st.video.resolutions instanceof Array) && (st.video.resolutions = st.video.resolutions.map(resolution2WidthHeight));
                  return st;
                }),
                users: result.participants};
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
      }).catch(function(error) {
        joinPortalFailed(error, callback);
      });
    });

    // "token" event is deprecated. It has been replace with "login". All clients use "token" will be treated as legacy version (<3.3).
    socket.on('token', function(token, callback) {
      joinPortal(participant_id, token).then(function(room_info){
        safeCall(callback, 'success', room_info);
      }).catch(function(error){
        joinPortalFailed(error, callback);
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
        for(let client of old_clients){
          client.replaceSocket(socket);
        }
      }).then(function(){
        reconnection_enabled=true;
        const ticket = generateReconnectionTicket();
        safeCall(callback, 'success', ticket);
      }).catch(function(error){
        safeCall(callback, 'error', error);
      }).then(function(){
        for(let message of pending_messages){
          notify(message.event, message.data);
        }
        pending_messages=[];
      });
    });

    socket.on('refreshReconnectionTicket', function(callback){
      if(!reconnection_enabled)
        safeCall(callback,'error','Reconnection is not enabled.');
      const ticket = generateReconnectionTicket();
      safeCall(callback, 'success', ticket);
    });

    socket.on('publish', function(options, url, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      var stream_id = Math.round(Math.random() * 1000000000000000000) + '',
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
      options.attributes && (stream_description.attributes = options.attributes);
      var unmix = (options.unmix === true || (stream_description.video && (stream_description.video.device === 'screen'))) ? true : false;

      var has_responded = false;
      return portal.publish(participant_id, stream_id, connection_type, stream_description, function(status) {
        if (status.type === 'failed') {
          if (has_responded) {
            log.info('emitted connection_failed, reason:', status.reason);
            send_msg('connection_failed', {streamId: stream_id});
          } else {
            safeCall(callback, 'error', status.reason);
          }
        } else {
          if (connection_type === 'webrtc') {
            if (status.type === 'initializing') {
              safeCall(callback, 'initializing', stream_id);
              has_responded = true;
            } else {
              send_msg('signaling_message_erizo', {streamId: stream_id, mess: status});
            }
          } else {
            if (status.type === 'ready') {
              safeCall(callback, 'success', stream_id);
              has_responded = true;
            } else if (status.type !== 'initializing') {
              safeCall(callback, status);
              has_responded = true;
            }
          }
        }
      }, unmix).then(function(connectionLocality) {
        log.debug('portal.publish succeeded, connection locality:', connectionLocality);
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.publish failed:', err_message);
        if (has_responded) {
          send_msg('connection_failed', {streamId: stream_id});
        } else {
          safeCall(callback, 'error', err_message);
        }
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
        const err_message = getErrorMessage(err);
        log.info('portal.unpublish failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('mix', function(options, callback) {
      var streamId = options.streamId;
      var mixStreams = options.mixStreams;

      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.mix(participant_id, streamId, mixStreams)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.mix failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('unmix', function(options, callback) {
      var streamId = options.streamId;
      var mixStreams = options.mixStreams;

      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unmix(participant_id, streamId, mixStreams)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.unmix failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    // To be delete after clients updated
    socket.on('addToMixer', function(streamId, mixStreams, callback) {
      if (typeof mixStreams === 'function') {
        // Shift the arguments with old clients
        callback = mixStreams;
        mixStreams = undefined;
      }

      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.mix(participant_id, streamId, mixStreams)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.mix failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    // To be delete after clients updated
    socket.on('removeFromMixer', function(streamId, mixStreams, callback) {
      if (typeof mixStreams === 'function') {
        // Shift the arguments with old clients
        callback = mixStreams;
        mixStreams = undefined;
      }

      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      };

      return portal.unmix(participant_id, streamId, mixStreams)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
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
        const err_message = getErrorMessage(err);
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
      (options.video && (typeof options.video.resolution === 'string')) && (subscription_description.video.resolution = options.video.resolution);
      (options.video && options.video.quality_level && (subscription_description.video.quality_level = options.video.quality_level));

      if (!subscription_description.audio && !subscription_description.video) {
          return safeCall(callback, 'error', 'bad options');
      }

      //FIXME - a: use the target stream id as the subscription_id to keep compatible with client SDK, should be fixed and use random strings independently later.
      var subscription_id = participant_id + '-sub-' + ((subscription_description.audio && subscription_description.audio.fromStream) ||
                                                        (subscription_description.video && subscription_description.video.fromStream));

      return portal.subscribe(participant_id, subscription_id, 'webrtc', subscription_description, function(status) {
        if (status.type === 'failed') {
          safeCall(callback, 'error', status.reason);
        } else if (status.type === 'initializing') {
          safeCall(callback, 'initializing', options.streamId/*FIXME -a */);
        } else {
          send_msg('signaling_message_erizo', {peerId: options.streamId/*FIXME -a */, mess: status});
        }
      }).then(function(connectionLocality) {
        log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
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
        const err_message = getErrorMessage(err);
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
        options.streamId = that.commonViewStream;
      }

      var subscription_description = {};
      (options.audio || options.audio === undefined) && (subscription_description.audio = {fromStream: options.streamId, codecs: (options.audio && options.audio.codecs) || ['aac']});
      (options.video || options.video === undefined) && (subscription_description.video = {fromStream: options.streamId, codecs: (options.video && options.video.codecs) || ['h264']});
      ((subscription_description.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (subscription_description.video.resolution = widthHeight2Resolution(options.resolution.width, options.resolution.height));
      (subscription_description.video && (typeof options.resolution === 'string')) && (subscription_description.video.resolution = options.resolution);
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
        const err_message = getErrorMessage(err);
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
        options.streamId = that.commonViewStream;
      }

      var subscription_description = {};
      (options.audio || options.audio === undefined) && (subscription_description.audio = {fromStream: options.streamId, codecs: (options.audio && options.audio.codecs) || ['aac']});
      (options.video || options.video === undefined) && (subscription_description.video = {fromStream: options.streamId, codecs: (options.video && options.video.codecs) || ['h264']});
      ((subscription_description.video) && options.resolution && (typeof options.resolution.width === 'number') && (typeof options.resolution.height === 'number')) &&
      (subscription_description.video.resolution = widthHeight2Resolution(options.resolution.width, options.resolution.height));
      (subscription_description.video && (typeof options.resolution === 'string')) && (subscription_description.video.resolution = options.resolution);
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
        const err_message = getErrorMessage(err);
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
      (options.audioStreamId || unspecifiedStreamIds) && (subscription_description.audio = {fromStream: options.audioStreamId || that.commonViewStream});
      (subscription_description.audio && (typeof options.audioCodec === 'string')) && (subscription_description.audio.codecs = [options.audioCodec]);
      subscription_description.audio && (subscription_description.audio.codecs = (subscription_description.audio.codecs || ['opus']).map(function(c) {return (c === 'opus' ? 'opus_48000_2' : c);}));
      (options.videoStreamId || unspecifiedStreamIds) && (subscription_description.video = {fromStream: options.videoStreamId || that.commonViewStream});
      (subscription_description.video && (typeof options.videoCodec === 'string')) && (subscription_description.video.codecs = [options.videoCodec]);
      subscription_description.video && (subscription_description.video.codecs = subscription_description.video.codecs || ['vp8']);
      options.path && (subscription_description.path = options.path);
      subscription_description.interval = (options.interval && options.interval > 0) ? options.interval : -1;

      var recorder_id = (options.recorderId || formatDate(new Date, 'yyyyMMddhhmmssSS'));
      var subscription_id = participant_id + '-' + recorder_id;
      var recording_file, recorder_added = false;
      return portal.subscribe(participant_id, subscription_id, 'recording', subscription_description, function(status) {
        if (status.type === 'failed') {
          if (recorder_added) {
            that.notify('remove_recorder', {id: recorder_id});
          } else {
            safeCall(callback, 'error', status.reason);
          }
        } else if (status.type === 'ready') {
          recorder_added = true;
          safeCall(callback, 'success', {recorderId: recorder_id, path: recording_file, host: 'unknown'});
        }
      }).then(function(connectionLocality) {
        log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
        recording_file = path.join(options.path || '', 'room_' + that.inRoom + '-' + subscription_id + '.mkv' );
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.subscribe failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('stopRecorder', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (options.recorderId === undefined || !isValidIdString(options.recorderId)) {
        return safeCall(callback, 'error', 'Invalid recorder id');
      }

      var subscription_id = (options.recorderId.startsWith(participant_id) ? options.recorderId : participant_id + '-' + options.recorderId);
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
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!isValidIdString(options.id)) {
        return safeCall(callback, 'error', 'Invalid stream id');
      }

      return portal.getRegion(participant_id, options.id, options.mixStreamId)
      .then(function(regionId) {
        safeCall(callback, 'success', {region: regionId});
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
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

      return portal.setRegion(participant_id, options.id, options.region, options.mixStreamId)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
        log.info('portal.setRegion failed:', err_message);
        safeCall(callback, 'error', err_message);
      });
    });

    socket.on('mute', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!options.streamId) {
        return safeCall(callback, 'error', 'no stream ID');
      }

      if (['video', 'audio', 'av'].indexOf(options.track) < 0) {
        return safeCall(callback, 'error', `invalid track ${options.track}`);
      }

      return portal.setMute(participant_id, options.streamId, options.track, true)
      .then(function() {
          safeCall(callback, 'success');
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
          log.info('portal.setMute failed:', err_message);
          safeCall(callback, 'error', err_message);
        });
    });

    socket.on('unmute', function(options, callback) {
      if(!that.inRoom) {
        return safeCall(callback, 'error', 'unauthorized');
      }

      if (!options.streamId) {
        return safeCall(callback, 'error', 'no stream ID');
      }

      if (['video', 'audio', 'av'].indexOf(options.track) < 0) {
        return safeCall(callback, 'error', `invalid track ${options.track}`);
      }

      return portal.setMute(participant_id, options.streamId, options.track, false)
      .then(function() {
          safeCall(callback, 'success');
        }).catch(function(err) {
          const err_message = getErrorMessage(err);
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

      if (!options.action) {
        return safeCall(callback, 'error', 'no action specified');
      }

      return portal.setPermission(participant_id, options.targetId, options.action, options.update)
      .then(function() {
        safeCall(callback, 'success');
      }).catch(function(err) {
        const err_message = getErrorMessage(err);
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
              on_off = cmdOpts[2];

          return portal.mediaOnOff(participant_id, msg.payload.streamId, track, direction, on_off)
            .then(function() {
              safeCall(callback, 'success');
            }).catch(function(err) {
              const err_message = getErrorMessage(err);
              log.info('portal.mediaOnOff failed:', err_message);
              safeCall(callback, 'error', err_message);
            });
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

  that.notify = function(event, data) {
    log.debug('notify, event:', event, 'data:', data);
    send_msg(event, data);
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
      return {pendingMessages:pending_messages, oldClients:old_clients};
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
          return Promise.resolve({pendingStreams:old_client_info.pending_messages, roomId:client.inRoom, participantId:ticket.participantId, oldClients:old_client_info.oldClients});
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

  that.drop = function(participantId, fromRoom) {
    if (participantId === 'all') {
      for(var pid in clients) {
        clients[pid].drop();
      }
      return Promise.resolve('ok');
    } else if (clients[participantId] && (fromRoom === undefined || clients[participantId].inRoom === fromRoom)) {
      clients[participantId].drop();
      return Promise.resolve('ok');
    } else {
      return Promise.reject('user not in room');
    }
  };

  return that;
};



module.exports = SocketIOServer;

