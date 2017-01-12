/*global require, module, GLOBAL, process*/
'use strict';

var logger = require('./logger').logger;

var ST = require('./Stream');
var sessionController = require('./controller');

// Logger
var log = logger.getLogger('Session');


module.exports = function (rpcClient, selfRpcId) {
  var that = {},
      is_initializing = false,
      session_id,
      controller,
      user_limit = 0;

  /*
   * {StreamId: ST.Stream()}
   */
  var streams = {};

  /*
   * {ParticipantId: {name: String,
   *                  role: String,
   *                  portal: RpcId,
   *                  published: [StreamId],
   *                  subscribed: [SubscriptionId]
   *                 }
   * }
   */
  var participants = {};

  var rpcChannel = require('./rpcChannel')(rpcClient),
      rpcReq = require('./rpcRequest')(rpcChannel);

  var initSession = function(sessionId) {
    if (is_initializing) {
      return new Promise(function(resolve, reject) {
        var interval = setInterval(function() {
          if (!is_initializing) {
            clearInterval(interval);
            if (session_id === sessionId) {
              resolve('ok');
            } else {
              reject('session initialization failed');
            }
          }
        }, 50);
      });
    } else if (session_id !== undefined) {
      if (session_id !== sessionId) {
        return Promise.reject('session id clash');
      } else {
        return Promise.resolve('ok');
      }
    } else {
      is_initializing = true;
      return rpcReq.getSessionConfig('nuve'/*FIXME: hard coded*/, sessionId)
        .then(function(config) {
            var room_config = config;

            user_limit = room_config.userLimit;
            if (user_limit === 0) {
              log.error('Room', roomID, 'disabled');
              is_initializing = false;
              return Promise.reject('Room' + roomID + 'disabled');
            }
            log.debug('initializing session:', sessionId, 'got config:', config);
            room_config.enableAudioTranscoding = (room_config.enableAudioTranscoding === undefined ? true : room_config.enableAudioTranscoding);
            room_config.enableVideoTranscoding = (room_config.enableVideoTranscoding === undefined ? true : room_config.enableVideoTranscoding);

            return new Promise(function(resolve, reject) {
              controller = sessionController(
                {cluster: GLOBAL.config.cluster.name || 'woogeen-cluster',
                 rpcReq: rpcReq,
                 rpcClient: rpcClient,
                 room: sessionId,
                 config: room_config,
                 selfRpcId: selfRpcId
                }, function (resolutions) {
                  log.debug('room controller init ok');
                  session_id = sessionId;
                  is_initializing = false;
                  resolve('ok');
                  if (room_config.enableMixing) {
                    var mixed_stream = new ST.Stream({
                      id: session_id,
                      socket: '',
                      audio: true,
                      video: {
                        device: 'mcu',
                        resolutions: resolutions,
                        layout: []
                      },
                      from: '',
                      attributes: null
                    });
                    streams[session_id] = mixed_stream;
                    log.debug('Mixed stream info:', mixed_stream.getPublicStream(), 'resolutions:', mixed_stream.getPublicStream().video.resolutions);
                    sendMsg('room', 'all', 'add_stream', mixed_stream.getPublicStream());
                  }
                }, function (reason) {
                  log.error('controller init failed.', reason);
                  is_initializing = false;
                  reject('controller init failed. reason: ' + reason);
                });
            });
        }, function(err) {
          log.error('Init session failed, reason:', err);
          is_initializing = false;
          return Promise.reject(err);
        });
    }
  };

  var deleteSession = function(force) {
    var terminate = function () {
      if (session_id) {
        controller && controller.destroy();
        controller = undefined;
        streams = {};
        session_id = undefined;
      }
      process.exit();
    }

    if (!force) {
      setTimeout(function() {
        if (Object.keys(participants).length === 0) {
          log.info('Empty session ', session_id, '. Deleting it');
          terminate();
        }
      }, 6 * 1000);
    } else {
      terminate();
    }
  };

  var sendMsg = function(from, to, msg, data) {
    if (to === 'all') {
      for (var participant_id in participants) {
        sendMsg(from, participant_id, msg, data);
      }
    } else if (to === 'others') {
      for (var participant_id in participants) {
        if (participant_id !== from) {
          sendMsg(from, participant_id, msg, data);
        }
      }
    } else {
      if (participants[to]) {
        rpcReq.sendMsg(participants[to].portal, to, msg, data);
      } else {
        log.warn('Can not send message to:', to);
      }
    }
  };

  var addParticipant = function(participantInfo) {
    participants[participantInfo.id] = {name: participantInfo.name,
                                        role: participantInfo.role,
                                        portal: participantInfo.portal,
                                        published: [],
                                        subscribed: []
                                       };
    sendMsg(participantInfo.id, 'others', 'user_join', {user: {id: participantInfo.id, name: participantInfo.name, role: participantInfo.role}});
  };

  var removeParticipant = function(participantId) {
    if (participants[participantId]) {
      var participant = participants[participantId];

      participant.published.map(function(stream_id) {
        controller.unpublish(participantId, stream_id);
        delete streams[stream_id];
        sendMsg('room', 'all', 'remove_stream', {id: stream_id});
      });

      participant.subscribed.map(function(subscription_id) {
        controller.unsubscribe(participantId, subscription_id);
      });

      var left_user = {id: participantId, name: participant.name, role: participant.role};
      delete participants[participantId];
      sendMsg('room', 'all', 'user_leave', {user: left_user});
    }
  };

  var dropParticipants = function(portal) {
    for (var participant_id in participants) {
      if (participants[participant_id].portal === portal) {
        removeParticipant(participant_id);
      }
    }
  };

  that.join = function(sessionId, participantInfo, callback) {
    log.debug('participant:', participantInfo, 'join session:', sessionId);
    return initSession(sessionId)
      .then(function() {
        log.debug('user_limit:', user_limit, 'current users count:', Object.keys(participants).length);
        if (user_limit > 0 && (Object.keys(participants).length >= user_limit)) {
          log.warn('Room is full');
          return callback('callback', 'error', 'Room is full');
        }

        addParticipant(participantInfo);

        var current_participants = [],
            current_streams = [];

        for (var participant_id in participants) {
          current_participants.push({id: participant_id, name: participants[participant_id].name, role: participants[participant_id].role});
        }

        for (var stream_id in streams) {
          current_streams.push(streams[stream_id].getPublicStream());
        }

        callback('callback', {participants: current_participants, streams: current_streams});
      }, function(err) {
        log.warn('Participant ' + participantInfo.id + ' join session ' + sessionId + ' failed, err:', err);
        callback('callback', 'error', 'Participant ' + participantInfo.id + ' join session ' + sessionId + ' failed');
      });
  };

  that.leave = function(participantId, callback) {
    removeParticipant(participantId);
    callback('callback', 'ok');
    if (Object.keys(participants).length === 0) {
      deleteSession(false);
    }
  };

  that.publish = function(participantId, streamId, accessNode, streamInfo, unmix, callback) {
    if (participants[participantId]) {
      if (participants[participantId].published.indexOf(streamId) === -1) {
        if (streams[streamId] === undefined) {
          if (controller) {
            controller.publish(participantId, streamId, accessNode, streamInfo, unmix, function() {
              if (participants[participantId]) {
                var st = new ST.Stream({id: streamId, audio: streamInfo.audio, video: streamInfo.video, from: participantId});
                participants[participantId].published.push(streamId);
                streams[streamId] = st;
                sendMsg('room', 'all', 'add_stream', st.getPublicStream());
                callback('callback', 'ok');
              } else {
                controller && controller.unpublish(streamId);
                log.info('Participant ' + participantId + ' early left while publishing stream ' + streamId);
                callback('callback', 'error', 'Participant ' + participantId + ' early left while publishing stream ' + streamId);
              }
            }, function(reason) {
              log.info('controller.publish failed, reason: ' + reason);
              callback('callback', 'error', 'controller.publish failed, reason: ' + reason);
            });
          } else {
            log.info('Controller is not ready');
            callback('callback', 'error', 'Controller is not ready');
          }
        } else {
          log.info('Stream ' + streamId + ' exists');
          callback('callback', 'error', 'Stream ' + streamId + ' exists');
        }
      } else {
        log.info('Participant ' + participantId + ' has published stream ' + streamId + ' previously');
        callback('callback', 'error', 'Participant ' + participantId + ' has published stream ' + streamId + ' previously');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.unpublish = function(participantId, streamId, callback) {
    if (participants[participantId]) {
      var index = participants[participantId].published.indexOf(streamId);
      if (index !== -1 && streams[streamId]) {
          if (controller) {
            controller.unpublish(participantId, streamId);
            participants[participantId].published.splice(index, 1);
            delete streams[streamId];
            sendMsg('room', 'all', 'remove_stream', {id: streamId});
            callback('callback', 'ok');
          } else {
            log.info('Controller is not ready');
            callback('callback', 'error', 'Controller is not ready');
          }
      } else {
        log.info('Stream ' + streamId + ' does not exists');
        callback('callback', 'error', 'Stream ' + streamId + 'does not exists');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.subscribe = function(participantId, subscriptionId, accessNode, subInfo, callback) {
    if (participants[participantId]) {
      if (participants[participantId].subscribed.indexOf(subscriptionId) === -1) {
        if ((!subInfo.audio || streams[subInfo.audio.fromStream])
            && (!subInfo.video || streams[subInfo.video.fromStream])) {
          if (controller) {
            controller.subscribe(participantId, subscriptionId, accessNode, subInfo, function() {
              if (participants[participantId]) {
                participants[participantId].subscribed.push(subscriptionId);
                callback('callback', 'ok');
              } else {
                controller && controller.unsubscribe(participantId, subscriptionId);
                log.info('Participant ' + participantId + ' early left while subscribing');
                callback('callback', 'error', 'Participant ' + participantId + ' early left while subscribing');
              }
            }, function(reason) {
              log.info('controller.subscribe failed, reason:' + reason);
              callback('callback', 'error', 'controller.subscribe failed, reason:' + reason);
            });
          } else {
            log.info('Controller is not ready');
            callback('callback', 'error', 'Controller is not ready');
          }
        } else {
          log.info('Streams in does not exist');
          callback('callback', 'error', 'Streams do not exist');
        }
      } else {
        log.info('Participant ' + participantId + ' has subscribed ' + subscriptionId + ' previously');
        callback('callback', 'error', 'Participant ' + participantId + ' has subscribed ' + subscriptionId + ' previously');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.unsubscribe = function(participantId, subscriptionId, callback) {
    if (participants[participantId]) {
      var index = participants[participantId].subscribed.indexOf(subscriptionId);
      if (controller) {
        controller.unsubscribe(participantId, subscriptionId);
        participants[participantId].subscribed.splice(index, 1);
        callback('callback', 'ok');
      } else {
        log.info('Controller is not ready');
        callback('callback', 'error', 'Controller is not ready');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.mix = function(participantId, streamId, callback) {
    log.debug('mix, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId]) {
      var index = participants[participantId].published.indexOf(streamId);
      if (index !== -1 && streams[streamId]) {
        if (controller) {
          controller.mix(streamId, function() {
            callback('callback', 'ok');
          }, function(reason) {
            log.info('Controller.mix failed, reason:', reason);
            callback('callback', 'error', reason);
          });
        } else {
          log.info('Controller is not ready');
          callback('callback', 'error', 'Controller is not ready');
        }
      } else {
        log.info('Stream ' + streamId + ' does not exists');
        callback('callback', 'error', 'Stream ' + streamId + 'does not exists');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.unmix = function(participantId, streamId, callback) {
    log.debug('unmix, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId]) {
      var index = participants[participantId].published.indexOf(streamId);
      if (index !== -1 && streams[streamId]) {
        if (controller) {
          controller.unmix(streamId, function() {
            callback('callback', 'ok');
          }, function(reason) {
            log.info('Controller.unmix failed, reason:', reason);
            callback('callback', 'error', reason);
          });
        } else {
          log.info('Controller is not ready');
          callback('callback', 'error', 'Controller is not ready');
        }
      } else {
        log.info('Stream ' + streamId + ' does not exists');
        callback('callback', 'error', 'Stream ' + streamId + 'does not exists');
      }
    } else {
      log.info('Participant ' + participantId + ' has not joined');
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.updateStream = function(streamId, track, status, callback) {
    log.info('updateStream:', streamId, status);
    sendMsg('room', 'all', 'update_stream', {event: 'StateChange', id: streamId, state: status});

    if (controller) {
      controller.updateStream(streamId, track, status);
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Controller is not ready');
    }
  };

  that.setMute = function(streamId, muted, callback) {
    log.debug('set mute:', streamId, muted);

    if (!streams[streamId]) {
      log.info('Stream ' + streamId + ' does not exists');
      callback('callback', 'error', 'Stream ' + streamId + 'does not exist');
      return;
    }

    if (streams[streamId].isMixed()) {
      log.warn('Stream ' + streamId + ' is Mixed');
      callback('callback', 'error', 'Stream ' + streamId + ' is Mixed');
      return;
    }

    var stream_owner = streams[streamId].getOwner();
    var index = participants[stream_owner].published.indexOf(streamId);
    if (index === -1) {
      log.warn('Stream ' + streamId + ' not published by ' + stream_owner);
      callback('callback', 'error', 'Stream ' + streamId + ' not published by ' + stream_owner);
      return;
    }

    rpcReq.setMute(participants[stream_owner].portal, stream_owner, streamId, muted)
    .then(function() {
      callback('callback', 'ok');
    }).catch(function(reason) {
      log.warn('Session set mute rpc fail:', reason);
      callback('callback', 'error', reason)
    });
  };

  that.setPermission = function(targetId, act, value, callback) {
    log.debug('set permission:', targetId, act, value);

    if (!participants[targetId]) {
      callback('callback', 'error', 'Target user does not exist');
      return;
    }

    rpcReq.setPermission(participants[targetId].portal, targetId, act, value)
    .then(function() {
      callback('callback', 'ok');
    }).catch(function(reason) {
      log.warn('Session set permission rpc fail:', reason);
      callback('callback', 'error', reason)
    });
  };


  that.getRegion = function(streamId, callback) {
    if (streams[streamId]) {
      if (controller) {
        controller.getRegion(streamId, function(region) {
          callback('callback', region);
        }, function(reason) {
          log.info('controller.getRegion failed, reason:', reason);
          callback('callback', 'error', reason);
        });
      } else {
        log.info('Controller is not ready');
        callback('callback', 'error', 'Controller is not ready');
      }
    } else {
      log.info('Stream ' + streamId + ' does not exists');
      callback('callback', 'error', 'Stream ' + streamId + 'does not exists');
    }
  };

  that.setRegion = function(streamId, regionId, callback) {
    if (streams[streamId]) {
      if (controller) {
        controller.setRegion(streamId, regionId, function() {
          callback('callback', 'ok');
        }, function(reason) {
          log.info('controller.setRegion failed, reason:', reason);
          callback('callback', 'error', reason);
        });
      } else {
        log.info('Controller is not ready');
        callback('callback', 'error', 'Controller is not ready');
      }
    } else {
      log.info('Stream ' + streamId + ' does not exists');
      callback('callback', 'error', 'Stream ' + streamId + 'does not exists');
    }
  };

  that.text = function(fromParticipantId, toParticipantId, data, callback) {
    if (participants[fromParticipantId]) {
      sendMsg(fromParticipantId, toParticipantId, 'custom_message', {from: fromParticipantId, to: toParticipantId, data: data});
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Participant ' + participantId + ' has not joined');
    }
  };

  that.onVideoLayoutChange = function(sessionId, layout, callback) {
    if (session_id === sessionId) {
      if (streams[sessionId]) {
        streams[sessionId].setLayout(layout);
        sendMsg('room', 'all', 'update_stream', {event: 'VideoLayoutChanged', id: session_id, data: layout});
        callback('callback', 'ok');
      } else {
        callback('callback', 'error', 'no mixed stream.');
      }
    } else {
      callback('callback', 'error', 'session is not in service');
    }
  };

  that.onAudioActiveParticipant = function(sessionId, activeParticipantId, callback) {
    log.debug('onAudioActiveParticipant, sessionId:', sessionId, 'active:', activeParticipantId);
    if ((session_id === sessionId) && controller) {
      controller.setPrimary(activeParticipantId);
      callback('callback', 'ok');
    } else {
      log.info('onAudioActiveParticipant, session does not exist');
      callback('callback', 'error', 'session is not in service');
    }
  };

  //The following interfaces are reserved to serve nuve
  that.getUsers = function(callback) {
    log.debug('getUsers, session_id:', session_id);
    var result = [];
    for (var participant_id in participants) {
      result.push({name: participants[participant_id].name, role: participants[participant_id].role, id: participant_id});
    }
    callback('callback', result);
  };

  that.deleteUser = function(user, callback) {
    var deleteNum = 0;
    for (var participant_id in participants) {
      if (participants[participant_id].name === user) {
        rpcReq.dropUser(participants[participant_id].portal, participant_id, session_id);
        this.leave(participant_id, function(){});

        deleteNum += 1;
      };
    }

    callback('callback', deleteNum);
  };

  that.destroy = function(callback) {
    log.info('Destroy session:', session_id);
    deleteSession(true);
    callback('callback', 'Success');
  };

  that.onFaultDetected = function (message) {
    if (message.purpose === 'portal' || message.purpose === 'sip') {
      dropParticipants(message.id);
    } else {
      controller && controller.onFaultDetected(message.purpose, message.type, message.id);
    }
  };

  return that;
};
