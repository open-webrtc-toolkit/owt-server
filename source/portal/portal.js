/* global require */

var path = require('path');
var url = require('url');
var crypto = require('crypto');
var log = require('./logger').logger.getLogger('Portal');

var Portal = function(spec, rpcReq) {
  var that = {},
    token_key = spec.tokenKey,
    token_server = spec.tokenServer,
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId,
    permission_map = spec.permissionMap;

  /*
   * {participantId: {
   *     tokenCode: String(),
   *     userName: String(),
   *     role: String(),
   *     origin: {isp: String(), region: String()},
   *     in_session: RoomId,
   *     controller: RpcId,
   *     connections: {
   *         ConnectionId: {
   *             locality: { agent: RpcIdOfAccessAgent, node: RpcIdOfAccessNode },
   *             type: 'webrtc' | 'avstream' | 'recording' | ...,
   *             direction: 'out' | 'in',
   *             audio_codecs: [AudioCodecName],
   *             video_codecs: [VideoCodecName],
   *             state: 'initialized' | 'connecting' | 'connected',
   *             status_observer: function(status) {...}
   *         }
   *     },
   *     permissions: { permissionMap got by role }
   * }}
   */
  var participants = {};

  var newPermissions = function(role) {
    var deepClone = function(obj) {
      if (typeof obj !== 'object') {
        return obj;
      }

      var result;
      if (Array.isArray(obj)) {
        result = [];
        obj.forEach(function(value) {
          result.push(deepClone(value));
        });
      } else {
        result = {};
        Object.keys(obj).forEach(function(key) {
            result[key] = deepClone(obj[key]);
        });
      }

      return result;
    };

    var permissions = deepClone(permission_map[role]);

    return permissions;
  };

  var isPermitted = function(participantId, act, track) {
    if (!participants[participantId]) {
      return false;
    }

    var permissions = participants[participantId].permissions;
    return permissions
           && ((permissions[act] === true)
               || (typeof permissions[act] === 'object' && permissions[act][track] === true));
  };

  var isTextPermitted = function(participantId) {
    if (!participants[participantId]) {
      return false;
    }

    var permissions = participants[participantId].permissions;
    return permissions && (permissions['text'] !== false);
  };

  var constructConnectOptions = function(connectionId, connectionType, direction, description, sessionId) {
    var options = {controller: self_rpc_id};
    if (!!description.audio) {
      if (typeof description.audio === 'object' && description.audio.codecs) {
        options.audio = {};
        options.audio.codecs = description.audio.codecs;
      } else {
        options.audio = true;
      }
    }

    if (!!description.video) {
      if (typeof description.video === 'object' && (description.video.codecs || description.video.resolution || description.video.quality_level)) {
        options.video = {};
        description.video.codecs && (options.video.codecs = description.video.codecs);
        description.video.resolution && (options.video.resolution = description.video.resolution);
        (typeof description.video.quality_level === 'string') && (options.video.quality_level = description.video.quality_level.toLowerCase());
      } else {
        options.video = true;
      }
    }

    if (connectionType === 'avstream' && description.url) {
      var url_obj = url.parse(description.url);
      options.url = url_obj.format();
      if (direction === 'in') {
        options.transport = description.transport;
        options.buffer_size = description.bufferSize;
      }
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
      if (status.type === 'failed') {
        return onConnectionFailed(status.reason);
      } else if (status.type === 'ready') {
        return onConnectionReady(status);
      } else {
        onStatus(status);
        return Promise.resolve(status.type);
      }
    };
  };

  var isImpacted = function(locality, type, id) {
    return (type === 'worker' && locality.agent === id) || (type === 'node' && locality.node === id);
  };

  var onAccessNodeFault = function(type, id) {
    var impacted_connections = [];
    for (var participant_id in participants) {
      for (var connection_id in participants[participant_id].connections) {
        var locality = participants[participant_id].connections[connection_id].locality;
        if (locality && isImpacted(locality, type, id)) {
          log.info('Fault detected on node:', locality);
          impacted_connections.push(participants[participant_id].connections[connection_id].status_observer({type: 'failed', reason: 'access node exited unexpectedly'}));
        }
      }
    }
    return Promise.all(impacted_connections);
  };

  that.updateTokenKey = function(tokenKey) {
    token_key = tokenKey;
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

    var tokenCode, userName, role, origin, session, session_controller;

    return validateToken(token)
      .then(function(validToken) {
        log.debug('token validation ok.');
        return rpcReq.tokenLogin(token_server, validToken.tokenId);
      })
      .then(function(loginResult) {
        log.debug('login ok.');
        tokenCode = loginResult.code;
        userName = loginResult.userName;
        role = loginResult.role;
        origin = loginResult.origin;
        session = loginResult.room;
        return rpcReq.getController(cluster_name, session);
      })
      .then(function(controller) {
        log.debug('got controller:', controller);
        session_controller = controller;
        return rpcReq.join(controller, session, {id: participantId, name: userName, role: role, portal: self_rpc_id});
      })
      .then(function(joinResult) {
        log.debug('join ok, result:', joinResult);
        participants[participantId] = {
          tokenCode: tokenCode,
          userName: userName,
          role: role,
          origin: origin,
          in_session: session,
          controller: session_controller,
          connections: {},
          permissions: newPermissions(role)
        };

        return {
          tokenCode: tokenCode,
          user: userName,
          role: role,
          session_id: session,
          participants: joinResult.participants,
          streams: joinResult.streams,
        };
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
            rpcReq.unpub2Session(participants[participantId].controller, participantId, stream_id);
            connection.state = 'connecting';
          }
          if (connection.state === 'connecting') {
            rpcReq.unpublish(connection.locality.node, stream_id);
            rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, task: connection_id})
            .catch(function(reason) {
                log.warn('AccessNode not recycled', connection.locality);
            });
          }
        } else if (connection.direction === 'out') {
          var subscription_id = connection_id;
          if (connection.state === 'connected') {
            rpcReq.unsub2Session(participants[participantId].controller, participantId, subscription_id);
            connection.state = 'connecting';
          }
          if (connection.state === 'connecting') {
            rpcReq.unsubscribe(connection.locality.node, subscription_id);
            rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, task: connection_id})
            .catch(function(reason) {
                log.warn('AccessNode not recycled', connection.locality);
            });
          }
        }
      }
      rpcReq.leave(participants[participantId].controller, participantId);
      delete participants[participantId];
      return Promise.resolve('ok');
    } else {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }
  };

  that.publish = function(participantId, connectionId, connectionType, streamDescription, onConnectionStatus, notMix) {
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    if ((!isPermitted(participantId, 'publish', 'audio') && streamDescription.audio)
        || (!isPermitted(participantId, 'publish', 'video') && streamDescription.video)) {
      return Promise.reject('unauthorized');
    }

    var stream_id = connectionId,
        connection_id = stream_id,
        locality;
    log.debug('publish, participantId:', participantId, 'connectionType:', connectionType, 'streamDescription:', streamDescription, 'notMix:', notMix, 'connection_id:', connection_id);

    var onConnectionReady = function(status) {
      var participant = participants[participantId];
      log.debug('publish::onConnectionReady, participantId:', participantId, 'connection_id:', connection_id, 'status:', status);

      if (participant === undefined) {
        return Promise.reject('Participant ' + participantId + ' has left when the connection gets ready.');
      }

      if (streamDescription.audio && (!status.audio_codecs || status.audio_codecs.length < 1)) {
        rpcReq.unpublish(locality.node, connection_id);
        rpcReq.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, task: connection_id})
        .catch(function(reason) {
            log.warn('AccessNode not recycled', locality);
        });
        onConnectionStatus({type: 'failed', reason: 'No proper audio codec'});
        return Promise.reject('No proper audio codec');
      }

      if (streamDescription.video && (!status.video_codecs || status.video_codecs.length < 1)) {
        rpcReq.unpublish(locality.node, connection_id);
        rpcReq.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, task: connection_id})
        .catch(function(reason) {
            log.warn('AccessNode not recycled', locality);
        });
        onConnectionStatus({type: 'failed', reason: 'No proper video codec'});
        return Promise.reject('No proper video codec');
      }

      var video_resolution = (streamDescription.video && streamDescription.video.resolution);
      video_resolution = ((typeof status.video_resolution === 'string' && status.video_resolution !== 'unknown') ? status.video_resolution : video_resolution);

      var stream_description = {audio: streamDescription.audio && {codec: status.audio_codecs[0]},
                                video: streamDescription.video && {resolution: video_resolution,
                                                                   device: streamDescription.video.device,
                                                                   codec: status.video_codecs[0]},
                                type: connectionType
                               };
      (streamDescription.video && streamDescription.video.framerate) && (stream_description.video.framerate = streamDescription.video.framerate);

      var controller = participant.controller;
      return rpcReq.pub2Session(controller, participantId, stream_id, locality, stream_description, notMix)
        .then(function(result) {
          log.debug('pub2Session ok, participantId:', participantId, 'connection_id:', connection_id);
          var participant = participants[participantId];
          if (participant === undefined) {
            rpcReq.unpub2Session(controller, participantId, stream_id);
            return Promise.reject('Participant ' + participantId + ' has left when controller responds publish ok.');
          } else if (participant.connections[stream_id] === undefined) {
            rpcReq.unpub2Session(controller, participantId, stream_id);
            return Promise.reject('Connection ' + stream_id + ' has been released when controller responds publish ok.');
          }

          participant.connections[stream_id].state = 'connected';
          onConnectionStatus(status);
          return result;
        }).catch(function(err) {
          log.debug('pub2Session failed, participantId:', participantId, 'connection_id:', connection_id, 'err:', err);
          var participant = participants[participantId];
          if (participant) {
            var connection = participant.connections[connection_id];
            if (connection) {
              if (connection.state === 'connected') {
                rpcReq.unpub2Session(p.controller, participantId, stream_id);
                connection.state = 'connecting';
              }
              if (connection.state === 'connecting') {
                rpcReq.unpublish(connection.locality.node, connection_id);
                rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participant.in_session, task: connection_id})
                .catch(function(reason) {
                    log.warn('AccessNode not recycled', connection.locality);
                });
              }
            }
            delete participant.connections[connection_id];
          }
          onConnectionStatus({type: 'failed', reason: (typeof err === 'string' ? err : err.message)});
          return Promise.reject(err);
        });
    };

    var onConnectionFailed = function(reason) {
      log.debug('publish::onConnectionFailed, participantId:', participantId, 'connection_id:', connection_id, 'reason:', reason);
      if (participants[participantId]) {
        if (participants[participantId].connections[connection_id]) {
          if (participants[participantId].connections[connection_id].state === 'connected') {
            rpcReq.unpub2Session(participants[participantId].controller, participantId, connection_id);
            participants[participantId].connections[connection_id].state = 'connecting';
          }
          if (participants[participantId].connections[connection_id].state === 'connecting') {
            rpcReq.unpublish(locality.node, connection_id);
            rpcReq.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, task: connection_id})
            .catch(function(reason) {
                log.warn('AccessNode not recycled', locality);
            });
          }
          delete participants[participantId].connections[connection_id];
        }
      }
      onConnectionStatus({type: 'failed', reason: reason});
      return Promise.reject(reason);
    };

    var connection_observer = connectionObserver(onConnectionStatus, onConnectionReady, onConnectionFailed),
      in_session = participants[participantId].in_session;

    participants[participantId].connections[connection_id] = {type: connectionType,
                                                              direction: 'in',
                                                              state: 'initialized',
                                                              status_observer: connection_observer};

    return rpcReq.getAccessNode(cluster_name, connectionType, {session: in_session, task: connection_id}, participants[participantId].origin)
      .then(function(accessNode) {
        log.debug('publish::getAccessNode ok, participantId:', participantId, 'connection_id:', connection_id, 'locality:', accessNode);
        locality = accessNode;
        if (participants[participantId] === undefined || participants[participantId].connections[connection_id] === undefined) {
          log.debug('aborting publishing because of early leave, participantId:', participantId, 'connection_id:', connection_id);
          rpcReq.recycleAccessNode(locality.agent, locality.node, {session: in_session, task: connection_id})
          .catch(function(reason) {
              log.warn('AccessNode not recycled', locality);
          });
          return Promise.reject('publishing is aborted because of early leave');
        }
        participants[participantId].connections[connection_id].locality = locality;
        var connect_options = constructConnectOptions(connection_id, connectionType, 'in', streamDescription, in_session);
        return rpcReq.publish(locality.node,
                                 connection_id,
                                 connectionType,
                                 connect_options,
                                 connection_observer);
      })
      .then(function() {
        log.debug('publish::pub2AccessNode ok, participantId:', participantId, 'connection_id:', connection_id);
        if (participants[participantId] === undefined || participants[participantId].connections[connection_id] === undefined) {
          log.debug('canceling publishing because of early leave, participantId:', participantId, 'connection_id:', connection_id);
          rpcReq.unpublish(locality.node, connection_id);
          rpcReq.recycleAccessNode(locality.agent, locality.node, {session: in_session, task: connection_id})
          .catch(function(reason) {
              log.warn('AccessNode not recycled', locality);
          });
          return Promise.reject('publishing is canceled because of early leave');
        }
        participants[participantId].connections[connection_id].state = 'connecting';
        return locality;
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
      rpcReq.unpub2Session(participants[participantId].controller, participantId, streamId);
      connection.state = 'connecting';
    }

    if (connection.state === 'connecting') {
      rpcReq.unpublish(connection.locality.node, connection_id);
      rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, task: connection_id})
      .catch(function(reason) {
          log.warn('AccessNode not recycled', connection.locality);
      });
    }

    delete participants[participantId].connections[connection_id];
    return Promise.resolve('ok');
  };

  that.mix = function(participantId, streamId, mixStreams) {
    log.debug('mix, participantId:', participantId, 'streamId:', streamId, 'mixStreams:', mixStreams);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId;
    if (participants[participantId].connections[connection_id] === undefined) {
      return Promise.reject('stream does not exist');
    }

    return rpcReq.mix(participants[participantId].controller, participantId, streamId, mixStreams);
  };

  that.unmix = function(participantId, streamId, mixStreams) {
    log.debug('unmix, participantId:', participantId, 'streamId:', 'mixStreams:', mixStreams);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId;
    if (participants[participantId].connections[connection_id] === undefined) {
      return Promise.reject('stream does not exist');
    }

    return rpcReq.unmix(participants[participantId].controller, participantId, streamId, mixStreams);
  };

  that.setVideoBitrate = function(participantId, streamId, bitrate) {
    log.debug('setVideoBitrate, participantId:', participantId, 'streamId:', streamId, 'bitrate:', bitrate);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var connection_id = streamId,
      connection = participants[participantId].connections[connection_id];
    if (connection === undefined) {
      return Promise.reject('stream does not exist');
    }

    if (connection.type !== 'webrtc' || connection.direction !== 'in') {
      return Promise.reject('operation not allowed');
    }

    return rpcReq.setVideoBitrate(connection.locality.node, connection_id, bitrate);
  };

  that.subscribe = function(participantId, connectionId, connectionType, subscriptionDescription, onConnectionStatus) {
    log.debug('subscribe, participantId:', participantId, 'connectionType:', connectionType, 'subscriptionDescription:', subscriptionDescription);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    var act = 'subscribe';
    if (connectionType === 'recording') {
      act = 'record';
    } else if (connectionType === 'avstream') {
      act = 'addExternalOutput';
    }

    if ((!isPermitted(participantId, act, 'audio') && subscriptionDescription.audio)
        || (!isPermitted(participantId, act, 'video') && subscriptionDescription.video)) {
      return Promise.reject('unauthorized');
    }

    var subscription_id = connectionId;

    //FIXME : not allowed to subscribe an already subscribed stream, this is a limitation caused by FIXME - a.
    if (participants[participantId].connections[subscription_id]) {
      if (connectionType !== 'recording') {
        return Promise.reject('Not allowed to subscribe an already-subscribed stream');
      }
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
      subscription_description.type = connectionType;

      var controller = participant.controller;
      return rpcReq.sub2Session(controller, participantId, connection_id, locality, subscription_description)
        .then(function(result) {
          log.debug('sub2Session ok, participantId:', participantId, 'connection_id:', connection_id);
          var participant = participants[participantId];
          if (participant === undefined) {
            rpcReq.unsub2Session(controller, participantId, connection_id);
            return Promise.reject('Participant ' + participantId + ' has left when controller responds subscribe ok.');
          } else if (participant.connections[connection_id] === undefined) {
            rpcReq.unsub2Session(controller, participantId, connection_id);
            return Promise.reject('Connection ' + connection_id + ' has been released when controller responds subscribe ok.');
          }

          participant.connections[connection_id].state = 'connected';
          participant.connections[connection_id].audio_codecs = (subscription_description.audio ? subscription_description.audio.codecs : []);
          participant.connections[connection_id].video_codecs = (subscription_description.video ? subscription_description.video.codecs : []);
          onConnectionStatus(status);
          return result;
        }).catch(function(err) {
          log.debug('sub2Session failed, participantId:', participantId, 'connection_id:', connection_id, 'err:', err);
          var participant = participants[participantId];
          if (participant) {
            var connection = participant.connections[connection_id];
            if (connection) {
              if (connection.state === 'connected') {
                rpcReq.unsub2Session(participant.controller, participantId, connection_id);
                connection.state = 'connecting';
              }
              if (connection.state === 'connecting') {
                rpcReq.unsubscribe(connection.locality.node, connection_id);
                rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participant.in_session, task: connection_id})
                .catch(function(reason) {
                    log.warn('AccessNode not recycled', connection.locality);
                });
              }
            }
            delete participant.connections[connection_id];
          }
          onConnectionStatus({type: 'failed', reason: (typeof err === 'string' ? err : err.message)});
          return Promise.reject(err);
        });
    };

    var onConnectionFailed = function(reason) {
      log.debug('subscribe::onConnectionFailed, participantId:', participantId, 'connection_id:', connection_id, 'reason:', reason);
      if (participants[participantId]) {
        if (participants[participantId].connections[connection_id]) {
          if (participants[participantId].connections[connection_id].state === 'connected') {
            rpcReq.unsub2Session(participants[participantId].controller, participantId, connection_id);
            participants[participantId].connections[connection_id].state = 'connecting';
          }
          if (participants[participantId].connections[connection_id].state === 'connecting') {
            rpcReq.unsubscribe(locality.node, connection_id);
            rpcReq.recycleAccessNode(locality.agent, locality.node, {session: participants[participantId].in_session, task: connection_id})
            .catch(function(reason) {
                log.warn('AccessNode not recycled', locality);
            });
          }
          delete participants[participantId].connections[connection_id];
        }
      }
      onConnectionStatus({type: 'failed', reason: reason});
      return Promise.reject(reason);
    };

    var connection = participants[participantId].connections[connection_id];
    if (connection) {
      if (connection.state === 'connected') {
          rpcReq.unsub2Session(participants[participantId].controller, participantId, connection_id);
          setTimeout(function() {
            locality = connection.locality;
            onConnectionReady({type: 'ready', audio_codecs: connection.audio_codecs, video_codecs: connection.video_codecs});
          }, 0);
          connection.state === 'connecting';
      }
      //TODO: notify user about 'recorder-continued'? Does it really neccesary?
      return Promise.resolve(connection.locality);
    } else {
      var connection_observer = connectionObserver(onConnectionStatus, onConnectionReady, onConnectionFailed),
        in_session = participants[participantId].in_session;

      participants[participantId].connections[connection_id] = {type: connectionType,
                                                                direction: 'out',
                                                                state: 'initialized',
                                                                status_observer: connection_observer};

      return rpcReq.getAccessNode(cluster_name, connectionType, {session: in_session, task: connection_id}, participants[participantId].origin)
        .then(function(accessNode) {
          log.debug('subscribe::getAccessNode ok, participantId:', participantId, 'connection_id:', connection_id, 'locality:', accessNode);
          locality = accessNode;
          if (participants[participantId] === undefined || participants[participantId].connections[connection_id] === undefined) {
            log.debug('aborting subscribing because of early leave, participantId:', participantId, 'connection_id:', connection_id);
            rpcReq.recycleAccessNode(locality.agent, locality.node, {session: in_session, task: connection_id}).catch(function(reason) {
                log.warn('AccessNode not recycled', locality);
            });
            return Promise.reject('subscribing is aborted because of early leave');
          }
          participants[participantId].connections[connection_id].locality = locality;
          var connect_options = constructConnectOptions(connection_id, connectionType, 'out', subscriptionDescription, in_session);
          return rpcReq.subscribe(locality.node,
                                     connection_id,
                                     connectionType,
                                     connect_options,
                                     connection_observer);
        })
        .then(function() {
          log.debug('subscribe::sub2AccessNode ok, participantId:', participantId, 'connection_id:', connection_id);
          if (participants[participantId] === undefined || participants[participantId].connections[connection_id] === undefined) {
            log.debug('canceling subscribing because of early leave, participantId:', participantId, 'connection_id:', connection_id);
            rpcReq.unsubscribe(locality.node, connection_id);
            rpcReq.recycleAccessNode(locality.agent, locality.node, {session: in_session, task: connection_id}).catch(function(reason) {
                log.warn('AccessNode not recycled', locality);
            });
            return Promise.reject('subscribing is canceled because of early leave');
          }
          participants[participantId].connections[connection_id].state = 'connecting';
          return locality;
        });
    }
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

    if (connection.state === 'connected') {
      rpcReq.unsub2Session(participants[participantId].controller, participantId, connection_id);
      connection.state = 'connecting';
    }

    if (connection.state === 'connecting' && connection.locality) {
      rpcReq.unsubscribe(connection.locality.node, connection_id);
      rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, task: connection_id})
      .catch(function(reason) {
          log.warn('AccessNode not recycled', connection.locality);
      });
    }

    delete participants[participantId].connections[connection_id];
    return Promise.resolve('ok');
  };

  that.onConnectionSignalling = function(participantId, connectionId, signaling) {
    var participant = participants[participantId];
    log.debug('onConnectionSignalling, participantId:', participantId, 'connectionId:', connectionId);

    if (participant === undefined) {
      return Promise.reject('Participant ' + participantId + ' has left when receiving a signaling of its connection ' + connection_id + '.');
    }

    var subscription_id = participantId + '-sub-' + connectionId,
        stream_id = connectionId;
    var connection_id = ((participant.connections[subscription_id] && participant.connections[subscription_id].direction === 'out') ? subscription_id : stream_id);//FIXME: removed once FIXME - a is fixed.

    if (participant.connections[connection_id] === undefined) {
      return Promise.reject('Connection does NOT exist when receiving a signaling.');
    } else if (participant.connections[connection_id].locality === undefined){
      return Promise.reject('Connection has been tore down.');
    } else {
      return rpcReq.onConnectionSignalling(participant.connections[connection_id].locality.node, connection_id, signaling)
        .then(function(result) {
          return result;
        }, function(reason) {
          if (participants[participantId]) {
            var connection = participants[participantId].connections[connection_id];
            if (connection) {
              connection.direction === 'in' && rpcReq.unpublish(connection.locality.node, connection_id);
              connection.direction === 'out' && rpcReq.unsubscribe(connection.locality.node, connection_id);
              rpcReq.recycleAccessNode(connection.locality.agent, connection.locality.node, {session: participants[participantId].in_session, task: connection_id})
              .catch(function(reason) {
                  log.warn('AccessNode not recycled', connection.locality);
              });
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

    if (onOff === 'on' && !isPermitted(participantId, 'publish', track)) {
      return Promise.reject('start stream permission denied');
    }

    var subscription_id = participantId + '-sub-' + connectionId,
        stream_id = connectionId;

    var targetConnectionId = (direction === 'out')? subscription_id : stream_id;

    var targetConnection;
    if (participant.connections[targetConnectionId] && participant.connections[targetConnectionId].direction === direction) {
      targetConnection = participant.connections[targetConnectionId];
    }

    if (targetConnection === undefined) {
      return Promise.reject('connection does not exist');
    }

    if (targetConnection.type !== 'webrtc') {
      return Promise.reject(targetConnection.type + ' connection does not support mediaOnOff');
    }

    var status = (onOff === 'on')? 'active':'inactive';

    if (direction === 'in') {
      rpcReq.updateStream(participant.controller, targetConnectionId, track, status);
    }

    return rpcReq.mediaOnOff(targetConnection.locality.node, targetConnectionId, track, direction, onOff);
  };

  that.setMute = function(participantId, streamId, muted) {
    var participant = participants[participantId];
    log.debug('setMute, participantId:', participantId, 'streamId:', streamId, 'muted:', muted);

    if (participant === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    if (!isPermitted(participantId, 'manage')) {
      return Promise.reject('Mute/Unmute Permission Denied');
    }

    return rpcReq.setMute(participant.controller, streamId, muted);
  };

  that.setPermission = function(participantId, targetId, act, value, fromSession) {
    log.debug('setPermission, participantId:', participantId, targetId, act, value, fromSession);
    var target = participants[targetId];

    if (target === undefined) {
      return Promise.reject('Target ' + targetId + ' does NOT exist.')
    }

    if (fromSession) {
      // Set the permission from session RPC
      target.permissions[act] = value;
      return Promise.resolve('ok');
    } else {
      // Notify session controller
      var participant = participants[participantId];
      if (participant === undefined) {
        return Promise.reject('Participant ' + participantId + ' does NOT exist.');
      }

      if (!isPermitted(participantId, 'manage')) {
        return Promise.reject('setPermission Permission Denied');
      }

      return rpcReq.setPermission(participant.controller, targetId, act, value);
    }
  };

  that.getRegion = function(participantId, subStreamId, mixStreamId) {
    log.debug('getRegion, participantId:', participantId, 'subStreamId:', subStreamId, 'mixStreamId:', mixStreamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    return rpcReq.getRegion(participants[participantId].controller, subStreamId, mixStreamId);
  };

  that.setRegion = function(participantId, subStreamId, regionId, mixStreamId) {
    log.debug('setRegion, participantId:', participantId, 'subStreamId:', subStreamId, 'regionId:', regionId, 'mixStreamId:', mixStreamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    return rpcReq.setRegion(participants[participantId].controller, subStreamId, regionId, mixStreamId);
  };

  that.text = function(participantId, to, message) {
    log.debug('text, participantId:', participantId, 'to:', to, 'message:', message);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant ' + participantId + ' does NOT exist.');
    }

    if (!isTextPermitted(participantId)) {
      return Promise.reject('unauthorized');
    }

    return rpcReq.text(participants[participantId].controller, participantId, to, message);
  };

  that.onFaultDetected = function (message) {
    if (message.purpose === 'webrtc' ||
               message.purpose === 'recording' ||
               message.purpose === 'avstream') {
      return onAccessNodeFault(message.type, message.id);
    }
    return Promise.resolve('ok');
  };

  that.getParticipantsByController = function (type, id) {
    var result = [];
    for (var participant_id in participants) {
      if ((type === 'node' && participants[participant_id].controller === id) ||
          (type === 'worker' && participants[participant_id].controller.startsWith(id))) {
        result.push(participant_id);
      }
    }
    return Promise.resolve(result);
  };

  return that;
};


module.exports = Portal;

