// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var path = require('path');
var url = require('url');
var crypto = require('crypto');
var log = require('./logger').logger.getLogger('Portal');
var dataAccess = require('./data_access');

var Portal = function(spec, rpcReq) {
  var that = {},
    token_key = spec.tokenKey,
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId;

  /*
   * {participantId: {
   *     in_room: RoomId,
   *     controller: RpcId
   * }}
   */
  var participants = {};

  that.updateTokenKey = function(tokenKey) {
    token_key = tokenKey;
  };

  that.join = function(participantId, token) {
    log.debug('participant[', participantId, '] join with token:', JSON.stringify(token));
    if (participants[participantId]) {
      return Promise.reject('Participant already in room');
    }

    var calculateSignature = function (token) {
      var toSign = token.tokenId + ',' + token.host + ',' + token.webTransportUrl,
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

    var tokenCode, userInfo, role, origin, room_id, room_controller;

    return validateToken(token)
      .then(function(validToken) {
        log.debug('token validation ok.');
        return dataAccess.token.delete(validToken.tokenId);
      })
      .then(function(deleteTokenResult) {
        log.debug('login ok.', deleteTokenResult);
        tokenCode = deleteTokenResult.code;
        userInfo = deleteTokenResult.user;
        role = deleteTokenResult.role;
        origin = deleteTokenResult.origin;
        room_id = deleteTokenResult.room;
        return rpcReq.getController(cluster_name, room_id);
      })
      .then(function(controller) {
        log.debug('got controller:', controller);
        room_controller = controller;
        return rpcReq.join(controller, room_id, {id: participantId, user: userInfo, role: role, portal: self_rpc_id, origin: origin});
      })
      .then(function(joinResult) {
        log.debug('join ok, result:', joinResult);
        participants[participantId] = {
          in_room: room_id,
          controller: room_controller
        };

        return {
          tokenCode: tokenCode,
          data: {
            user: userInfo,
            role: role,
            permission: joinResult.permission,
            room: joinResult.room
          }
        };
      });
  };

  that.leave = function(participantId) {
    log.debug('participant leave:', participantId);
    if (participants[participantId]) {
      rpcReq.leave(participants[participantId].controller, participantId)
        .catch(function(reason) {
          log.info('Failed in leaving, ', reason.message ? reason.message : reason);
        });
      delete participants[participantId];
      return Promise.resolve('ok');
    } else {
      return Promise.reject('Participant has NOT joined');
    }
  };

  that.publish = function(participantId, streamId, pubInfo) {
    log.debug('publish, participantId:', participantId, 'streamId:', streamId, 'pubInfo:', pubInfo);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.publish(participants[participantId].controller,
                          participantId,
                          streamId,
                          pubInfo);
  };

  that.unpublish = function(participantId, streamId) {
    log.debug('unpublish, participantId:', participantId, 'streamId:', streamId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.unpublish(participants[participantId].controller,
                            participantId,
                            streamId);
  };

  that.streamControl = function(participantId, streamId, commandInfo) {
    log.debug('streamControl, participantId:', participantId, 'streamId:', streamId, 'command:', commandInfo);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.streamControl(participants[participantId].controller,
                                participantId,
                                streamId,
                                commandInfo);
  };

  that.subscribe = function(participantId, subscriptionId, subDesc) {
    log.debug('subscribe, participantId:', participantId, 'subscriptionId:', subscriptionId, 'subDesc:', subDesc);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.subscribe(participants[participantId].controller,
                            participantId,
                            subscriptionId,
                            subDesc);
  };

  that.unsubscribe = function(participantId, subscriptionId) {
    log.debug('unsubscribe, participantId:', participantId, 'subscriptionId:', subscriptionId);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.unsubscribe(participants[participantId].controller, participantId, subscriptionId);
  };

  that.subscriptionControl = function(participantId, subscriptionId, commandInfo) {
    log.debug('subscriptionControl, participantId:', participantId, 'subscriptionId:', subscriptionId, 'command:', commandInfo);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.subscriptionControl(participants[participantId].controller,
                                      participantId,
                                      subscriptionId,
                                      commandInfo);
  };

  that.onSessionSignaling = function(participantId, sessionId, signaling) {
    log.debug('onSessionSignaling, participantId:', participantId, 'sessionId:', sessionId, 'signaling:', signaling);

    var participant = participants[participantId];
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }
 
    return rpcReq.onSessionSignaling(participants[participantId].controller, sessionId, signaling);
  };

  that.text = function(participantId, to, msg) {
    log.debug('text, participantId:', participantId, 'to:', to, 'msg:', msg);
    if (participants[participantId] === undefined) {
      return Promise.reject('Participant has NOT joined');
    }

    return rpcReq.text(participants[participantId].controller, participantId, to, msg);
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

