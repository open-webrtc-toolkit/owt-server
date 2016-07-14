/* global require */

var path = require('path');
var crypto = require('crypto');
var log = require('./logger').logger.getLogger('Portal');

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

var Portal = function(spec, rpcClient) {
  var that = {},
    token_key = spec.tokenKey,
    token_server = spec.tokenServer,
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId,
    permission_map = spec.permissionMap;

  /* {participantId: {userName: String(),
   *             role: String(),
   *             in_session: RoomId,
   *             controller: RpcId,
   *             connections: {ConnectionId: {locality: {agent: RpcIdOfAccessAgent, node: RpcIdOfAccessNode},
   *                                          type: 'webrtc' | 'rtsp' | 'rtmp' | 'recording' | ...,
   *                                          direction: 'out' | 'in',
   *                                          state: 'connecting' | 'connected'
   *                                         }
   *                          }
   *            }} */
  var participants = {};

  var isPermitted = function(role, act, track) {
    return permission_map[role]
           && ((permission_map[role][act] === true)
               || (typeof permission_map[role][act] === 'object' && permission_map[role][act][track] === true));
  };

  var isTextPermitted = function(role) {
    return permission_map[role] && (permission_map[role]['text'] !== false);
  };

  var constructConnectOptions = function(connectionId, connectionType, description, sessionId) {
    var options = {};
    if (!!description.audio) {
      if (typeof description.audio === 'object' && description.audio.codecs) {
        options.audio = {};
        options.audio.codecs = description.audio.codecs;
      } else {
        options.audio = true;
      }
    }

    if (!!description.video) {
      if (typeof description.video === 'object' && (description.video.codecs || description.video.resolution)) {
        options.video = {};
        description.video.codecs && (options.video.codecs = description.video.codecs);
        description.video.resolution && (options.video.resolution = description.video.resolution);
      } else {
        options.video = true;
      }
    }

    if (connectionType === 'avstream' && description.url) {
      options.url = path.join(description.url || '/', 'room_' + sessionId + '-' + connectionId + '.sdp');
    }

    if (connectionType === 'recording') {
      description.path && (options.path = description.path);
      options.filename = 'room_' + sessionId + '-' + connectionId + '.mkv';
      options.interval = description.interval;
    }

    return options;
  };

  var connectionObserver = function(onStatus, onConnectionReady, onConnectionFailed) {
    return function(status) {
      onStatus(status);
      if (status.type === 'failed') {
        return onConnectionFailed(status.reason);
      } else if (status.type === 'ready') {
        return onConnectionReady(status);
      } else {
        return Promise.resolve(status.type);
      }
    };
  };

  that.join = function(participantId, token) {
    log.debug('participant[', participantId, '] join with token:', JSON.stringify(token));
    var calculateSignature = function (token) {
      var toSign = token.tokenId + ',' + token.host,
        signed = crypto.createHmac('sha256', token_key).update(toSign).digest('hex');
      return (new Buffer(signed)).toString('base64');
    };

    var validateToken = function (token) {
      var signature = calculateSignature(token);

      if (signature !== token.signature) {
        return Promise.reject('Invalid token signature');
      } else {
        return Promise.resolve(token);
      }
    };

    var userName, role, session, session_controller;

    return validateToken(token)
      .then(function(validToken) {
        log.debug('token validation ok.');
        return rpcClient.tokenLogin(token_server, validToken.tokenId);
      })
      .then(function(loginResult) {
        log.debug('login ok.');
        userName = loginResult.userName;
        role = loginResult.role;
        session = loginResult.room;
        return rpcClient.getController(cluster_name, session);
      })
      .then(function(controller) {
        log.debug('got controller:', controller);
        session_controller = controller;
        return rpcClient.join(controller, session, {id: participantId, name: userName, role: role, portal: self_rpc_id});
      })
      .then(function(joinResult) {
        log.debug('join ok, result:', joinResult);
        participants[participantId] = {userName: userName,
                                       role: role,
                                       in_session: session,
                                       controller: session_controller,
                                       connections: {}};
        return {user: userName, role: role, session_id: session, participants: joinResult.participants, streams: joinResult.streams};
      });
  };

  that.leave = function(participantId) {
    log.debug('participant leave:', participantId);
    if (participants[participantId]) {
      for (var connection_id in participants[participantId].connections) {
        var connection = participants[participantId].connections[connection_id];
        if (connection.direction === 'in') {
          var stream_id = connection_id;
          if (connection.state === 'connected') {
              rpcClient.unpub2Session(participants[participantId].controller, participantId, stream_id);
          }
          rpcClient.unpublish(connection.locality.node, stream_id);
          rpcClient.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, consumer: connection_id});
        } else if (connection.direction === 'out') {
          var subscription_id = connection_id;
          if (connection.state === 'connected') {
              rpcClient.unsub2Session(participants[participantId].controller, participantId, subscription_id);
          }
          rpcClient.unsubscribe(connection.locality.node, subscription_id);
          rpcClient.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, consumer: connection_id});
        }
      }
      rpcClient.leave(participants[participantId].controller, participantId);
      delete participants[participantId];
      return Promise.resolve('ok');
    } else {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }
  };

  that.publish = function(participantId, connectionType, streamDescription, onConnectionStatus, notMix) {
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    if ((!isPermitted(participants[participantId].role, 'publish', 'audio') && streamDescription.audio)
        || (!isPermitted(participants[participantId].role, 'publish', 'video') && streamDescription.video)) {
      return Promise.reject('unauthorized');
    }

    var stream_id = Math.random() * 1000000000000000000 + '',
        connection_id = stream_id,
        locality;
    log.debug('publish, participantId:', participantId, 'connectionType:', connectionType, 'streamDescription:', streamDescription, 'notMix:', notMix, 'connection_id:', connection_id);

    var onConnectionReady = function(status) {
      var participant = participants[participantId];
      log.debug('publish::onConnectionReady, participantId:', participantId, 'connection_id:', connection_id);

      if (participant === undefined) {
        return Promise.reject('Participant ' + participantId + ' has left when the connection gets ready.');
      }

      if (streamDescription.audio && status.audio_codecs.length < 1) {
        rpcClient.unpublish(locality.node, connection_id);
        rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
        onConnectionStatus({type: 'failed', reason: 'No proper audio codec'});
        return Promise.reject('No proper audio codec');
      }

      if (streamDescription.video && status.video_codecs.length < 1) {
        rpcClient.unpublish(locality.node, connection_id);
        rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
        onConnectionStatus({type: 'failed', reason: 'No proper video codec'});
        return Promise.reject('No proper video codec');
      }

      var video_resolution = streamDescription.video && streamDescription.video.resolution;
      if (streamDescription.video) {
        if (streamDescription.video.resolution === 'unknown' && ((typeof status.video_resolution !== 'string') || (status.video_resolution === 'unknown'))) {
          rpcClient.unpublish(locality.node, connection_id);
          rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
          onConnectionStatus({type: 'failed', reason: 'Undetermined video resolution'});
          return Promise.reject('Undetermined video resolution');
        } else {
          video_resolution = ((typeof status.video_resolution === 'string' && status.video_resolution !== 'unknown') ? status.video_resolution : video_resolution);
        }
      }

      var stream_description = {audio: streamDescription.audio && {codec: status.audio_codecs[0]},
                                video: streamDescription.video && {resolution: video_resolution,
                                                                   device: streamDescription.video.device,
                                                                   codec: status.video_codecs[0]}
                               };
      (streamDescription.video && streamDescription.video.framerate) && (stream_description.video.framerate = streamDescription.video.framerate);
      return rpcClient.pub2Session(participant.controller, participantId, stream_id, locality, stream_description, notMix)
        .then(function(result) {
          log.debug('pub2Session ok, participantId:', participantId, 'connection_id:', connection_id);
          var p = participants[participantId];
          if (p === undefined) {
            rpcClient.unpub2Session(participant.controller, participantId, stream_id);
            return Promise.reject('Participant ' + participantId + ' has left when controller responds publish ok.');
          } else if (p.connections[stream_id] === undefined) {
            rpcClient.unpub2Session(participant.controller, participantId, stream_id);
            return Promise.reject('Connection ' + stream_id + ' has been released when controller responds publish ok.');
          }

          p.connections[stream_id].state = 'connected';
          return result;
        }).catch(function(err) {
          log.debug('pub2Session failed, participantId:', participantId, 'connection_id:', connection_id, 'err:', err);
          rpcClient.unpublish(locality.node, connection_id);
          rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
          onConnectionStatus({type: 'failed', reason: err.message});
          return Promise.reject(err);
        });
    };

    var onConnectionFailed = function(reason) {
      log.debug('publish::onConnectionFailed, participantId:', participantId, 'connection_id:', connection_id, 'reason:', reason);
      if (participants[participantId]) {
        if (participants[participantId].connections[connection_id]) {
          if (participants[participantId].connections[connection_id].state === 'connected') {
              rpcClient.unpub2Session(participants[participantId].controller, participantId, connection_id);
          }
          rpcClient.unpublish(locality.node, connection_id);
          rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
          delete participants[participantId].connections[connection_id];
        }
      }
      return Promise.reject(reason);
    };

    return rpcClient.getAccessNode(cluster_name, connectionType, {session: participants[participantId].in_session, consumer: connection_id})
      .then(function(accessNode) {
        log.debug('publish::getAccessNode ok, participantId:', participantId, 'connection_id:', connection_id, 'locality:', accessNode);
        locality = accessNode;
        var connect_options = constructConnectOptions(connection_id, connectionType, streamDescription, participants[participantId].in_session);
        return rpcClient.publish(locality.node,
                                 connection_id,
                                 connectionType,
                                 connect_options,
                                 connectionObserver(onConnectionStatus, onConnectionReady, onConnectionFailed));
      })
      .then(function() {
        log.debug('publish::pub2AccessNode ok, participantId:', participantId, 'connection_id:', connection_id);
        participants[participantId].connections[connection_id] = {locality: locality,
                                                                  type: connectionType,
                                                                  direction: 'in',
                                                                  state: 'connecting'};
        return stream_id;
      });
  };

  that.unpublish = function(participantId, streamId) {
    log.debug('unpublish, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId,
        connection = participants[participantId].connections[connection_id];
    if (connection === undefined) {
      return Promise.reject('stream does not exist');
    }

    if(connection.state === 'connected') {
      rpcClient.unpub2Session(participants[participantId].controller, participantId, streamId);
    }

    rpcClient.unpublish(connection.locality.node, connection_id);
    rpcClient.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, consumer: connection_id});

    delete participants[participantId].connections[connection_id];
    return Promise.resolve('ok');
  };

  that.mix = function(participantId, streamId) {
    log.debug('mix, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId;
    if (participants[participantId].connections[connection_id] === undefined) {
      return Promise.reject('stream does not exist');
    }

    return rpcClient.mix(participants[participantId].controller, participantId, streamId);
  };

  that.unmix = function(participantId, streamId) {
    log.debug('unmix, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId;
    if (participants[participantId].connections[connection_id] === undefined) {
      return Promise.reject('stream does not exist');
    }

    return rpcClient.unmix(participants[participantId].controller, participantId, streamId);
  };

  that.setVideoBitrate = function(participantId, streamId, bitrate) {
    log.debug('setVideoBitrate, participantId:', participantId, 'streamId:', streamId, 'bitrate:', bitrate);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId;
    if (participants[participantId].connections[connection_id] === undefined) {
      return Promise.reject('stream does not exist');
    }

    return rpcClient.setVideoBitrate(participants[participantId].connections[connection_id].locality.node, connection_id, bitrate);
  };

  that.subscribe = function(participantId, connectionType, subscriptionDescription, onConnectionStatus) {
    log.debug('subscribe, participantId:', participantId, 'connectionType:', connectionType, 'subscriptionDescription:', subscriptionDescription);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var act = (connectionType === 'recording' ? 'record' : 'subscribe');
    if ((!isPermitted(participants[participantId].role, act, 'audio') && subscriptionDescription.audio)
        || (!isPermitted(participants[participantId].role, act, 'video') && subscriptionDescription.video)) {
      return Promise.reject('unauthorized');
    }

    if ((subscriptionDescription.audio && subscriptionDescription.audio.fromStream && participants[participantId].connections[subscriptionDescription.audio.fromStream])
        ||(subscriptionDescription.video && subscriptionDescription.video.fromStream && participants[participantId].connections[subscriptionDescription.video.fromStream])) {
      return Promise.reject('Not allowed to subscribe a self-published stream');
    }

    var subscription_id;
    if (connectionType === 'webrtc') {
      //FIXME - a: use the target stream id as the subscription_id to keep compatible with client SDK, should be fixed and use random strings independently later.
      subscription_id = participantId + '-sub-' + ((subscriptionDescription.audio && subscriptionDescription.audio.fromStream) ||
                                                   (subscriptionDescription.video && subscriptionDescription.video.fromStream));
    } else if(connectionType === 'recording') {
      subscription_id = subscriptionDescription.recorderId || formatDate(new Date, 'yyyyMMddhhmmssSS');
    } else {
      subscription_id = Math.random() * 1000000000000000000 + '';
    }

    //FIXME : not allowed to subscribe an already subscribed stream, this is a limitation caused by FIXME - a.
    if ((subscriptionDescription.audio && subscriptionDescription.audio.fromStream && participants[participantId].connections[subscription_id])
        ||(subscriptionDescription.video && subscriptionDescription.video.fromStream && participants[participantId].connections[subscription_id])) {
      return Promise.reject('Not allowed to subscribe an already-subscribed stream');
    }

    var connection_id = subscription_id,
        locality;

    var onConnectionReady = function(status) {
      var participant = participants[participantId];
      log.debug('subscribe::onConnectionReady, participantId:', participantId, 'connection_id:', connection_id);

      if (participant === undefined) {
        return Promise.reject('Participant ' + participantId + ' has left when the connection gets ready.');
      }

      var subscription_description = {audio: subscriptionDescription.audio, video: subscriptionDescription.video};
      (subscription_description.audio) && (subscription_description.audio.codecs = status.audio_codecs);
      (subscription_description.video) && (subscription_description.video.codecs = status.video_codecs);
      (connectionType === 'recording') && (subscription_description.isRecording = true);

      return rpcClient.sub2Session(participant.controller, participantId, connection_id, locality, subscription_description)
        .then(function(result) {
          log.debug('sub2Session ok, participantId:', participantId, 'connection_id:', connection_id);
          var p = participants[participantId];
          if (p === undefined) {
            rpcClient.unsub2Session(participant.controller, participantId, connection_id);
            return Promise.reject('Participant ' + participantId + ' has left when controller responds subscribe ok.');
          } else if (p.connections[connection_id] === undefined) {
            rpcClient.unsub2Session(participant.controller, participantId, connection_id);
            return Promise.reject('Connection ' + connection_id + ' has been released when controller responds subscribe ok.');
          }

          p.connections[connection_id].state = 'connected';
          return result;
        }).catch(function(err) {
          log.debug('sub2Session failed, participantId:', participantId, 'connection_id:', connection_id, 'err:', err);
          rpcClient.unsubscribe(locality.node, connection_id);
          rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
          onConnectionStatus({type: 'failed', reason: err.message});
          return Promise.reject(err);
        });
    };

    var onConnectionFailed = function(reason) {
      log.debug('subscribe::onConnectionFailed, participantId:', participantId, 'connection_id:', connection_id, 'reason:', reason);
      if (participants[participantId]) {
        if (participants[participantId].connections[connection_id]) {
          if (participants[participantId].connections[connection_id].state === 'connected') {
              rpcClient.unsub2Session(participants[participantId].controller, participantId, connection_id);
          }
          rpcClient.unsubscribe(locality.node, connection_id);
          rpcClient.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, consumer: connection_id});
          delete participants[participantId].connections[connection_id];
        }
      }
      return Promise.reject(reason);
    };


    return rpcClient.getAccessNode(cluster_name, connectionType, {session: participants[participantId].in_session, consumer: connection_id})
      .then(function(accessNode) {
        log.debug('subscribe::getAccessNode ok, participantId:', participantId, 'connection_id:', connection_id, 'locality:', accessNode);
        locality = accessNode;
        var connect_options = constructConnectOptions(connection_id, connectionType, subscriptionDescription, participants[participantId].in_session);
        return rpcClient.subscribe(locality.node,
                                   connection_id,
                                   connectionType,
                                   connect_options,
                                   connectionObserver(onConnectionStatus, onConnectionReady, onConnectionFailed));
      })
      .then(function() {
        log.debug('subscribe::sub2AccessNode ok, participantId:', participantId, 'connection_id:', connection_id);
        participants[participantId].connections[connection_id] = {locality: locality,
                                                                  type: connectionType,
                                                                  direction: 'out',
                                                                  state: 'connecting'};
        return subscription_id;
      });
  };

  that.unsubscribe = function(participantId, subscriptionId) {
    log.debug('unsubscribe, participantId:', participantId, 'subscriptionId:', subscriptionId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = subscriptionId,
        connection = participants[participantId].connections[connection_id];
    if (connection === undefined) {
      return Promise.reject('subscription does not exist');
    }

    if(connection.state === 'connected') {
      rpcClient.unsub2Session(participants[participantId].controller, participantId, connection_id);
    }

    rpcClient.unsubscribe(connection.locality.node, connection_id);
    rpcClient.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, consumer: connection_id});

    delete participants[participantId].connections[connection_id];
    return Promise.resolve('ok');
  };

  that.onConnectionSignalling = function(participantId, connectionId, signaling) {
    var participant = participants[participantId];
    log.debug('onConnectionSignalling, participantId:', participantId, 'connectionId:', connectionId);

    var connection_id = ((participant.connections[connectionId] && participant.connections[connectionId].direction === 'in') ? connectionId : participantId + '-sub-' + connectionId);//FIXME: removed once FIXME - a is fixed.

    if (participant === undefined) {
      return Promise.reject('Participant ' + participantId + ' has left when receiving a signaling of its connection ' + connection_id + '.');
    } else if (participant.connections[connection_id] === undefined) {
      return Promise.reject('Connection ' + connection_id + ' of participant ' + participantId + ' does NOT exist when receives a signaling.');
    } else {
      return rpcClient.onConnectionSignalling(participant.connections[connection_id].locality.node, connection_id, signaling)
        .then(function(result) {
          return result;
        }, function(reason) {
          if (participants[participantId]) {
            var connection = participants[participantId].connections[connection_id];
            if (connection) {
              connection.direction === 'in' && rpcClient.unpublish(connection.locality.node, connection_id);
              connection.direction === 'out' && rpcClient.unsubscribe(connection.locality.node, connection_id);
              rpcClient.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, consumer: connection_id});
              delete participants[participantId].connections[connection_id];
            }
          }
          return Promise.reject(reason);
        });
    }
  };

  that.mediaOnOff = function(participantId, connectionId, track, direction, onOff) {
    var participant = participants[participantId];
    log.debug('mediaOnOff, participantId:', participantId, 'connectionId:', connectionId, 'track:', track, 'direction:', direction, 'onOff', onOff);
    if (participant === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = ((participant.connections[connectionId] && participant.connections[connectionId].direction === 'in') ? connectionId : participantId + '-sub-' + connectionId);//FIXME: removed once FIXME - a is fixed.

    var connection = participant.connections[connection_id];
    if (connection === undefined || connection.direction !== direction) {
      return Promise.reject('connection does not exist');
    }

    return rpcClient.mediaOnOff(connection.locality.node, connection_id, track, direction, onOff);
  };

  that.getRegion = function(participantId, subStreamId) {
    log.debug('getRegion, participantId:', participantId, 'subStreamId:', subStreamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    return rpcClient.getRegion(participants[participantId].controller, subStreamId);
  };

  that.setRegion = function(participantId, subStreamId, regionId) {
    log.debug('setRegion, participantId:', participantId, 'subStreamId:', subStreamId, 'regionId:', regionId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    return rpcClient.setRegion(participants[participantId].controller, subStreamId, regionId);
  };

  that.text = function(participantId, to, message) {
    log.debug('text, participantId:', participantId, 'to:', to, 'message:', message);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    if (!isTextPermitted(participants[participantId].role)) {
      return Promise.reject('unauthorized');
    }

    return rpcClient.text(participants[participantId].controller, participantId, to, message);
  };


  return that;
};


module.exports = Portal;

