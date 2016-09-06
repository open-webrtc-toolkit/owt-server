/*global require, module, GLOBAL, process*/
'use strict';

var amqper = require('./amqper');
var logger = require('./logger').logger;

var ST = require('./Stream');
var sessionController = require('./controller');

// Logger
var log = logger.getLogger('Session');


module.exports = function (amqper, selfRpcId) {
  var that = {},
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

  var rpcChannel = require('./rpcChannel')(amqper),
      rpcClient = require('./rpcClient')(rpcChannel);

  var initSession = function(sessionId) {
    if (session_id !== undefined) {
      if (session_id !== sessionId) {
        return Promise.reject('session id clash');
      } else {
        return Promise.resolve('ok');
      }
    } else {
      session_id = sessionId;
      return rpcClient.getSessionConfig('nuve'/*FIXME: hard coded*/, session_id)
        .then(function(config) {
            var room_config = config;

            user_limit = room_config.userLimit;
            if (user_limit === 0) {
              log.error('Room', roomID, 'disabled');
              return Promise.reject('Room' + roomID + 'disabled');
            }
            log.debug('initializing session:', session_id, 'got config:', config);
            room_config.enableAudioTranscoding = (room_config.enableAudioTranscoding === undefined ? true : room_config.enableAudioTranscoding);
            room_config.enableVideoTranscoding = (room_config.enableVideoTranscoding === undefined ? true : room_config.enableVideoTranscoding);

            return new Promise(function(resolve, reject) {
              controller = sessionController(
                {cluster: GLOBAL.config.cluster.name || 'woogeen-cluster',
                 rpcClient: rpcClient,
                 amqper: amqper,
                 room: session_id,
                 config: room_config,
                 observer: selfRpcId
                }, function (resolutions) {
                  log.debug('room controller init ok');
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
                  reject('controller init failed. reason: ' + reason);
                });
            });
        }, function(err) {
          log.error('Init session failed, reason:', err);
          return Promise.reject(err);
        });
    }
  };

  var deleteSession = function() {
    if (session_id) {
      controller && controller.destroy();
      controller = undefined;
      streams = {};
      session_id = undefined;
    }
    process.exit();
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
        rpcClient.sendMsg(participants[to].portal, to, msg, data);
      } else {
        log.warn('Can not send message to:', to);
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

        participants[participantInfo.id] = {name: participantInfo.name,
                                            role: participantInfo.role,
                                            portal: participantInfo.portal,
                                            published: [],
                                            subscribed: []
                                           };
        var current_participants = [],
            current_streams = [];

        for (var participant_id in participants) {
          current_participants.push({id: participant_id, name: participants[participant_id].name, role: participants[participant_id].role});
        }

        for (var stream_id in streams) {
          current_streams.push(streams[stream_id].getPublicStream());
        }

        callback('callback', {participants: current_participants, streams: current_streams});

        sendMsg(participantInfo.id, 'others', 'user_join', {user: {id: participantInfo.id, name: participantInfo.name, role: participantInfo.role}});
      }, function(err) {
        log.warn('Participant ' + participantInfo.id + ' join session ' + sessionId + ' failed, err:', err);
        callback('callback', 'error', 'Participant ' + participantInfo.id + ' join session ' + sessionId + ' failed');
      });
  };

  that.leave = function(participantId, callback) {
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

    callback('callback', 'ok');
    if (Object.keys(participants).length === 0) {
      log.info('Empty session ', session_id, '. Deleting it');
      deleteSession();
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
        rpcClient.dropUser(participants[participant_id].portal, participant_id, session_id);
        this.leave(participant_id, function(){});

        deleteNum += 1;
      };
    }

    callback('callback', deleteNum);
  };

  that.destroy = function(callback) {
    log.info('Destroy session:', session_id);
    deleteSession();
    callback('callback', 'Success');
  };


  return that;
};
