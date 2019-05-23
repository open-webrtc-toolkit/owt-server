// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var path = require('path');
var url = require('url');
var log = require('./logger').logger.getLogger('AccessController');

module.exports.create = function(spec, rpcReq, on_session_established, on_session_aborted, on_session_signaling, rtc_controller) {
  var that = {},
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId,
    in_room = spec.inRoom;

  /*
   *sessions: {
   *    SessionId: {
   *        owner: ParticipantId,
   *        locality: { agent: RpcIdOfAccessAgent, node: RpcIdOfAccessNode },
   *        direction: 'out' | 'in',
   *        options: object(pubOptions) | object(subOptions),
   *        state: 'initialized' | 'connecting' | 'connected'
   *    }
   *},
   */
  var sessions = {};


  //Should terminateSession always succeed?
  const terminateSession = (sessionId) => {
    var session = sessions[sessionId];
    if (session && session.state === 'connecting' || session.state === 'connected') {
      return rpcReq.terminate(session.locality.node, sessionId, session.direction)
        .then(function() {
          log.debug('to recycleWorkerNode:', session.locality, 'task:', sessionId);
          return rpcReq.recycleWorkerNode(session.locality.agent, session.locality.node, {room: in_room, task: sessionId})
        }).then(function() {
          delete sessions[sessionId];
        })
        .catch(function(reason) {
          log.debug('AccessNode not recycled', session.locality);
        });
    } else {
      delete sessions[sessionId];
      return Promise.resolve('ok');
    }
  };

  const isImpacted = (locality, type, id) => {
    return (type === 'worker' && locality.agent === id) || (type === 'node' && locality.node === id);
  };

  const onReady = (sessionId, status) => {
    var audio = status.audio, video = status.video;
    var simulcast = status.simulcast;
    var session = sessions[sessionId];

    if (session.options.type === 'webrtc') {
      if (!!session.options.media.audio && !audio) {
        var owner = session.owner, direction = session.direction;
        terminateSession(sessionId).catch((whatever) => {});
        on_session_aborted(owner, sessionId, direction, 'No proper audio codec');
        log.error('No proper audio codec');
        return;
      }

      if (!!session.options.media.video && !video) {
        var owner = session.owner, direction = session.direction;
        terminateSession(sessionId).catch((whatever) => {});
        on_session_aborted(owner, sessionId, direction, 'No proper video codec');
        log.error('No proper video codec');
        return;
      }
    }

    session.state = 'connected';
    var media = {}, info = {type: session.options.type, owner: session.owner};

    if (session.direction === 'in') {
      session.options.media.audio && (media.audio = (!!audio ? (audio || {}) : false));
      media.audio && session.options.media.audio && session.options.media.audio.source && (media.audio.source = session.options.media.audio.source);
      session.options.media.video && (media.video = (!!video ? (video || {}) : false));
      media.video && session.options.media.video && session.options.media.video.source && (media.video.source = session.options.media.video.source);
      media.video && session.options.media.video && session.options.media.video.parameters && session.options.media.video.parameters.resolution && (media.video.resolution = session.options.media.video.parameters.resolution);
      media.video && session.options.media.video && session.options.media.video.parameters && session.options.media.video.parameters.framerate && (media.video.framerate = session.options.media.video.parameters.framerate);

      simulcast && simulcast.video && (media.video.simulcast = true);
      session.options.attributes && (info.attributes = session.options.attributes);
    } else {
      if (session.options.media.audio) {
        media.audio = {from: session.options.media.audio.from};
        session.options.media.audio.format && (media.audio.format = session.options.media.audio.format);
        (session.options.type === 'webrtc') && (media.audio.format = audio);
      }

      if (session.options.media.video) {
        media.video = {from: session.options.media.video.from};
        session.options.media.video.format && (media.video.format = session.options.media.video.format);
        (session.options.type === 'webrtc') && (media.video.format = video);
        session.options.media.video.parameters && (media.video.parameters = session.options.media.video.parameters);
        session.options.media.video.simulcastRid && (media.video.simulcastRid = session.options.media.video.simulcastRid);
      }

      if (session.options.type === 'recording') {
        info.location = status.info;
      }

      if (session.options.type === 'streaming') {
        info.url = status.info
      }

      if (session.options.type === 'analytics') {
        info.analytics = status.info;
      }
    }

    var session_info = {
      locality: session.locality,
      media: media,
      info: info
    };

    on_session_established(session.owner, sessionId, session.direction, session_info);
  };

  const onFailed = (sessionId, reason) => {
    log.info('onFailed, sessionId:', sessionId, 'reason:', reason);
    var owner = sessions[sessionId].owner,
        direction = sessions[sessionId].direction;
    terminateSession(sessionId).catch((whatever) => {});
    on_session_aborted(owner, sessionId, direction, reason);
  };

  const onSignaling = (sessionId, signaling) => {
    on_session_signaling(sessions[sessionId].owner, sessionId, signaling);
  };

  that.getSessionState = (sessionId) => {
    return sessions[sessionId] && sessions[sessionId].state;
  };

  that.onSessionStatus = (sessionId, status) => {
    if (!sessions[sessionId]) {
      log.error('Session does NOT exist');
      return Promise.reject('Session does NOT exist');
    }

    if (status.type === 'ready') {
      onReady(sessionId, status);
    } else if (status.type === 'failed') {
      onFailed(sessionId, status.reason);
    } else if (status.type === 'offer' || status.type === 'answer' || status.type === 'candidate' || status.type === 'quic-p2p-server-parameters') {
      onSignaling(sessionId, status);
    } else {
      log.error('Irrispective status:' + status.type);
      return Promise.reject('Irrispective status');
    }

    return Promise.resolve('ok');
  };

  that.onSessionSignaling = (sessionId, signaling) => {
    if (!sessions[sessionId]) {
      return Promise.reject('Session ' + sessionId + ' does NOT exist');
    }

    return rpcReq.onSessionSignaling(sessions[sessionId].locality.node, sessionId, signaling)
      .catch((e) => {
        terminateSession(sessionId).catch((whatever) => {});
        return Promise.reject(e.message ? e.message : e);
      });
  };

  that.participantLeave = function(participantId) {
    log.debug('participantLeave, participantId:', participantId);
    var pl = [];
    for (var session_id in sessions) {
      if (sessions[session_id].owner === participantId) {
        var direction = sessions[session_id].direction;
        pl.push(terminateSession(session_id));
        on_session_aborted(participantId, session_id, direction, 'Participant leave');
      }
    }
    return Promise.all(pl);
  };

  that.initiate = function(participantId, sessionId, direction, origin, sessionOptions, formatPreference) {
    log.debug('initiate, participantId:', participantId, 'sessionId:', sessionId, 'direction:', direction, 'origin:', origin, 'sessionOptions:', sessionOptions);
    if (sessionOptions.type === 'webrtc') {
      return rtc_controller.initiate(participantId, sessionId, direction, origin, sessionOptions, formatPreference);
    }
    if (sessions[sessionId]) {
      return Promise.reject('Session exists');
    }

    sessions[sessionId] = {owner: participantId,
                           direction: direction,
                           options: sessionOptions,
                           origin: origin,
                           preference: formatPreference,
                           state: 'initialized'};

    var locality;
    log.debug('session type: '+sessionOptions.type);
    return rpcReq.getWorkerNode(cluster_name, sessionOptions.type, {room: in_room, task: sessionId}, origin)
      .then(function(accessLocality) {
        locality = accessLocality;
        log.debug('getWorkerNode ok, participantId:', participantId, 'sessionId:', sessionId, 'locality:', locality);
        if (sessions[sessionId] === undefined) {
          log.debug('Session has been aborted, sessionId:', sessionId);
          rpcReq.recycleWorkerNode(locality.agent, locality.node, {room: in_room, task: sessionId})
            .catch(function(reason) {
              log.debug('AccessNode not recycled', locality);
            });
          return Promise.reject('Session has been aborted');
        }
        sessions[sessionId].locality = locality;
        var options = {
          controller: self_rpc_id
        };
        sessionOptions.connection && (options.connection = sessionOptions.connection);
        sessionOptions.media && (options.media = sessionOptions.media);
        sessionOptions.transport && (options.transport = sessionOptions.transport);
        formatPreference && (options.formatPreference = formatPreference);
        return rpcReq.initiate(locality.node,
                               sessionId,
                               sessionOptions.type,
                               direction,
                               options);
      })
      .then(function() {
        log.debug('Initiate ok, participantId:', participantId, 'sessionId:', sessionId);
        if (sessions[sessionId] === undefined) {
          log.debug('Session has been aborted, sessionId:', sessionId);
          rpcReq.terminate(locality.node, sessionId, direction)
            .catch(function(reason) {
              log.debug('Terminate fail:', reason);
            });
          rpcReq.recycleWorkerNode(locality.agent, locality.node, {room: in_room, task: sessionId})
            .catch(function(reason) {
              log.debug('AccessNode not recycled', locality);
            });
          return Promise.reject('Session has been aborted');
        }
        sessions[sessionId].state = 'connecting';
        return 'ok';
      }, (e) => {
        delete sessions[sessionId];
        return Promise.reject(e.message ? e.message : e);
      });
  };

  that.terminate = function(sessionId, direction, reason) {
    log.debug('terminate, sessionId:', sessionId, 'direction:', direction);
    if (!sessions[sessionId]) {
      return rtc_controller.terminate(sessionId, direction, reason);
    }

    var session = sessions[sessionId];
    if (session === undefined || (sessions[sessionId].direction !== direction)) {
      return Promise.reject('Session does NOT exist');
    }

    terminateSession(sessionId).catch((whatever) => {});
    on_session_aborted(session.owner, sessionId, session.direction, reason);
    return Promise.resolve('ok');
  };

  that.setMute = function(sessionId, track, muted) {
    log.debug('setMute, sessionId:', sessionId, 'muted:', muted);

    if (!sessions[sessionId]) {
      return Promise.reject('Session does NOT exist');
    }

    var session = sessions[sessionId],
        onOff = (muted ? 'off' : 'on');

    if (session.options.type !== 'webrtc') {
      return Promise.reject('Session does NOT support muting');
    }

    return rpcReq.mediaOnOff(session.locality.node, sessionId, track, session.direction, onOff);
  };

  const rebuildAnalyticsSubscriber = sessionId => {
    log.debug('rebuildAnalyticsSubscriber, sessionId:', sessionId);

    let session = sessions[sessionId];
    session.state = 'initialized';

    var locality;
    return rpcReq.getWorkerNode(cluster_name, session.options.type, {room: in_room, task: sessionId}, session.origin)
      .then(function(accessLocality) {
        locality = accessLocality;
        log.debug('getWorkerNode ok, participantId:', session.owner, 'sessionId:', sessionId, 'locality:', locality);
        if (sessions[sessionId] === undefined) {
          log.debug('Session has been aborted, sessionId:', sessionId);
          rpcReq.recycleWorkerNode(locality.agent, locality.node, {room: in_room, task: sessionId})
            .catch(function(reason) {
              log.debug('AccessNode not recycled', locality);
            });
          return Promise.reject('Session has been aborted');
        }
        session.locality = locality;
        var options = {
          controller: self_rpc_id
        };
        session.options.connection && (options.connection = session.options.connection);
        session.options.media && (options.media = session.options.media);
        session.preference && (options.formatPreference = session.preference);
        return rpcReq.initiate(locality.node,
                               sessionId,
                               session.options.type,
                               session.direction,
                               options);
      })
      .then(function() {
        log.debug('Initiate ok, participantId:', session.owner, 'sessionId:', sessionId);
        if (sessions[sessionId] === undefined) {
          log.debug('Session has been aborted, sessionId:', sessionId);
          rpcReq.terminate(locality.node, sessionId, session.direction)
            .catch(function(reason) {
              log.debug('Terminate fail:', reason);
            });
          rpcReq.recycleWorkerNode(locality.agent, locality.node, {room: in_room, task: sessionId})
            .catch(function(reason) {
              log.debug('AccessNode not recycled', locality);
            });
          return Promise.reject('Session has been aborted');
        }
        sessions[sessionId].state = 'connecting';
        return 'ok';
      }, (e) => {
        delete sessions[sessionId];
        return Promise.reject(e.message ? e.message : e);
      });
  };

  that.onFaultDetected = function (faultType, faultId) {
    for (var session_id in sessions) {
      var locality = sessions[session_id].locality;
      if (locality && isImpacted(locality, faultType, faultId)) {
        var owner = sessions[session_id].owner,
            direction = sessions[session_id].direction;

        log.info('Fault detected on node:', locality);
        on_session_aborted(owner, session_id, direction, 'Access node exited unexpectedly');

        if (sessions[session_id].options.type === 'analytics') {
          rebuildAnalyticsSubscriber(session_id);
        } else {
          terminateSession(session_id).catch((whatever) => {});
        }
      }
    }
    return Promise.resolve('ok');
  };

  that.destroy = () => {
    for (var session_id in sessions) {
      var owner = sessions[session_id].owner;
      var direction = sessions[session_id].direction;
      terminateSession(session_id).catch((whatever) => {});
      on_session_aborted(owner, session_id, direction, 'Room destruction');
    }
  };

  return that;
};

