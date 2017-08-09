/* global require */

var path = require('path');
var url = require('url');
var log = require('./logger').logger.getLogger('AccessController');

module.exports.create = function(spec, rpcReq, onSessionEstablished, onSessionAborted, onLocalSessionSignaling) {
  var that = {},
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId,
    in_room = spec.inRoom,
    on_session_established = onSessionEstablished,
    on_session_aborted = onSessionAborted,
    on_session_signaling = onLocalSessionSignaling;

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
    if (session.state === 'connecting' || session.state === 'connected') {
      rpcReq.terminate(session.locality.node, sessionId, session.direction)
        .catch(function(reason) {
          log.debug('rpcRequest.terminate fail:', reason);
        });
      rpcReq.recycleWorkerNode(session.locality.agent, session.locality.node, {room: in_room, task: sessionId})
        .catch(function(reason) {
          log.debug('AccessNode not recycled', session.locality);
        });
      session.state = 'initialized';
    }

    delete sessions[sessionId];
  };

  const isImpacted = (locality, type, id) => {
    return (type === 'worker' && locality.agent === id) || (type === 'node' && locality.node === id);
  };

  const onReady = (sessionId, audio, video) => {
    var session = sessions[sessionId];

    if (session.options.type === 'webrtc' && !!session.options.media.audio && !audio) {
      var owner = session.owner, direction = session.direction;
      terminateSession(sessionId);
      on_session_aborted(owner, sessionId, direction, 'No proper audio codec');
      return Promise.reject('No proper audio codec');
    }

    if (session.options.type === 'webrtc' && !!session.options.media.video && !video) {
      var owner = session.owner, direction = session.direction;
      terminateSession(sessionId);
      on_session_aborted(owner, sessionId, direction, 'No proper video codec');
      return Promise.reject('No proper video codec');
    }

    session.state = 'connected';
    var media = {}, info = {type: session.options.type, owner: session.owner};

    session.options.media.audio && (media.audio = (audio || {}));
    session.options.media.video && (media.video = (video || {}));

    if (session.direction === 'in') {
      media.audio && session.options.media.audio && session.options.media.audio.source && (media.audio.source = session.options.media.audio.source);
      media.video && session.options.media.video && session.options.media.video.source && (media.video.source = session.options.media.video.source);

      session.options.attributes && (info.attributes = session.options.attributes);
    } else {
      //FIXME: should verify whether the negotiated audio/video codec and parameters satisfy the original specification.
      media.audio && session.options.media.audio && session.options.media.audio.from && (media.audio.from = session.options.media.audio.from);
      media.audio && session.options.media.audio && session.options.media.audio.spec && (media.audio.spec = session.options.media.audio.spec);
      media.video && session.options.media.video && session.options.media.video.from && (media.video.from = session.options.media.video.from);
      media.video && session.options.media.video && session.options.media.video.spec && (media.video.spec = session.options.media.video.spec);
    }

    var session_info = {
      locality: session.locality,
      media: media,
      info: info
    };

    on_session_established(session.owner, sessionId, session.direction, session_info);
    return Promise.resolve('ok');
  };

  const onFailed = (sessionId, reason) => {
    log.info('onFailed, sessionId:', sessionId, 'reason:', reason);
    var owner = sessions[sessionId].owner,
        direction = sessions[sessionId].direction;
    terminateSession(sessionId);
    on_session_aborted(owner, sessionId, direction, reason);
    return Promise.reject(reason);
  };

  const onSignaling = (sessionId, signaling) => {
    on_session_signaling(sessions[sessionId].owner, sessionId, signaling);
    return Promise.resolve('ok');
  };

  that.getSessionState = (sessionId) => {
    return sessions[sessionId] && sessions[sessionId].state;
  };

  that.onSessionStatus = (sessionId, status) => {
    if (!sessions[sessionId]) {
      return Promise.reject('Session does NOT exist');
    }

    if (status.type === 'ready') {
      return onReady(sessionId, status.audio, status.video);
    } else if (status.type === 'failed') {
      return onFailed(sessionId, status.reason);
    } else if (status.type === 'offer' || status.type === 'answer' || status.type === 'candidate') {
      return onSignaling(sessionId, status);
    } else {
      return Promise.reject('Irrispective status:' + status.type);
    }
  };

  that.onSessionSignaling = (sessionId, signaling) => {
    if (!sessions[sessionId]) {
      return Promise.reject('Session ' + sessionId + ' does NOT exist');
    }

    return rpcReq.onSessionSignaling(sessions[sessionId].locality.node, sessionId, signaling)
      .catch((e) => {
        terminateSession(sessionId);
        return Promise.reject(e.message ? e.message : e);
      });
  };

  that.participantLeave = function(participantId) {
    for (var session_id in sessions) {
      if (sessions[session_id].owner === participantId) {
        var direction = sessions[session_id].direction;
        terminateSession(session_id);
        on_session_aborted(participantId, session_id, direction, 'Participant leave');
      }
    }
    return Promise.resolve('ok');
  };

  that.initiate = (participantId, sessionId, direction, origin, sessionOptions) => {
    log.debug('initiate, participantId:', participantId, 'sessionId:', sessionId, 'direction:', direction, 'origin:', origin, 'sessionOptions:', sessionOptions);
    if (sessions[sessionId]) {
      return Promise.reject('Session exists');
    }

    sessions[sessionId] = {owner: participantId,
                           direction: direction,
                           options: sessionOptions,
                           state: 'initialized'};

    var locality;
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
        //FIXME: should specify the desired audio/video format in the options passed to access nodes.
        sessionOptions.media && (options.media = sessionOptions.media);
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

  that.terminate = function(sessionId) {
    log.debug('terminate, streamId:', sessionId);

    var session = sessions[sessionId];
    if (session === undefined) {
      return Promise.reject('Session does NOT exist');
    }

    terminateSession(sessionId);
    on_session_aborted(session.owner, sessionId, session.direction, 'Participant terminate');
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

  that.onFaultDetected = function (faultType, faultId) {
    for (var session_id in sessions) {
      var locality = sessions[session_id].locality;
      if (locality && isImpacted(locality, faultType, faultId)) {
        var owner = sessions[session_id].owner,
            direction = sessions[session_id].direction;

        log.info('Fault detected on node:', locality);
        terminateSession(session_id);
        on_session_aborted(owner, session_id, direction, 'Access node exited unexpectedly');
      }
    }
    return Promise.resolve('ok');
  };

  that.destroy = () => {
    for (var session_id in sessions) {
      terminateSession(session_id);
      on_session_aborted(owner, session_id, direction, 'Participant terminate');
    }
  };

  return that;
};

