// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;

var { RtcController } = require('./rtcController');
var AccessController = require('./accessController');
var RoomController = require('./roomController');
var dataAccess = require('./data_access');
var Participant = require('./participant');

// Logger
var log = logger.getLogger('Conference');

const {
  isAudioFmtEqual,
  isVideoFmtEqual,
  isVideoFmtCompatible,
  calcResolution,
  calcBitrate,
  isResolutionEqual,
} = require('./formatUtil');

const {
  ForwardStream,
  MixedStream,
  StreamConfigure,
} = require('./stream');

const {
  Subscription
} = require('./subscription');

/*
 * Definitions:
 *
 *
 *
   *    object(Role):: {
   *      role: string(RoleName),
   *      publish: {
   *        audio: true | false,
   *        video: true | false
   *      },
   *      subscribe: {
   *        audio: true | false,
   *        video: true | false
   *      }
   *    }
   *
   *    object(AudioFormat):: {
   *      codec: 'opus' | 'pcma' | 'pcmu' | 'isac' | 'ilbc' | 'g722' | 'aac' | 'ac3' | 'nellymoser',
   *      sampleRate: 8000 | 16000 | 32000 | 48000 | 44100 | undefined,
   *      channelNum: 1 | 2 | undefined
   *    }
   *
   *    object(VideoFormat):: {
   *      codec: 'h264' | 'vp8' | 'h265' | 'vp9',
   *      profile: 'constrained-baseline' | 'baseline' | 'main' | 'high' | undefined
   *    }
   *
   *    object(Resolution):: {
   *      width: number(WidthPX),
   *      height: number(HeightPX)
   *    },
   *
   *    object(Rational):: {
   *      numerator: number(Numerator),
   *      denominator: number(Denominator)
   *    },
   *
   *    object(Region):: {
   *      id: string(RegionID),
   *      shape: 'rectangle' | 'circle',
   *      area: object(Rectangle):: {
   *        left: object(Rational),
   *        top: object(Rational),
   *        width: object(Rational),
   *        height: object(Rational),
   *      } | object(Circle):: {
   *        centerW: object(Rational),
   *        centerH: object(Rational),
   *        radius: object(Rational),
   *      }
   *    }
   *
   *
   *
   *
   *
*/

var Conference = function (rpcClient, selfRpcId) {
  var that = {},
      is_initializing = false,
      room_id,
      roomController,
      accessController;

  var rtcController;

  /*
   * {
   *  _id: string(RoomID),
   *  name: string(RoomName),
   *  inputLimit: number(InputLimit),
   *  participantLimit: number(ParticipantLimit),
   *  roles: [object(Role)],
   *  views: [
   *    object(MixedViewSettings):: {
   *      label: string(ViewLabel),
   *      audio: {
   *        format: object(AudioFormat),
   *        vad: true | false
   *      } | false,
   *      video: {
   *        format: object(VideoFormat),
   *        motionFactor: Number(MotionFactor), //0.5~1.0, indicates the motion severity and affects the encoding bitrate, suggest to set 0.8 for conferencing scenario and 1.0 for sports scenario.
   *        parameters: {
   *          resolution: object(Resolution),
   *          framerate: number(FramerateFPS),
   *          bitrate: number(Kbps),
   *          keyFrameInterval: number(Seconds)
   *        }
   *        maxInput: number(MaxInput),
   *        bgColor: object(RGB)::{r: number(R), g: number(G), b: number(B)},
   *        layout: {
   *          fitPolicy: 'crop' | 'letterbox' | 'stretch',
   *          setRegionEffect: 'exchange' | 'insert',
   *          templates: [
   *            object(LayoutTemplate):: {
   *              primary: string(RegionID),
   *              regions: [object(Region)]
   *            }
   *          ]
   *        }
   *      } | false
   *  ],
   *  mediaIn: {
   *    audio: [object(AudioFormat)],
   *    video: [object(VideoFormat)]
   *  },
   *  mediaOut: {
   *    audio: [object(AudioFormat)],
   *    video: {
   *      format: [object(VideoFormat)],
   *      parameters: {
   *        resolution: ['x3/4' | 'x2/3' | 'x1/2' | 'x1/3' | 'x1/4' | 'xga' | 'svga' | 'vga' | 'hvga' | 'cif' | 'sif' | 'qcif'],
   *        framerate: [6 | 12 | 15 | 24 | 30 | 48 | 60],
   *        bitrate: ['x0.8' | 'x0.6' | 'x0.4' | 'x0.2'],
   *        keyFrameInterval: [100 | 30 | 5 | 2 | 1]
   *      }
   *    }
   *  },
   *  transcoding: {
   *    audio: true | false,
   *    video: {
   *      format: true | false,
   *      parameters: {
   *        resolution: true | false,
   *        framerate: true | false,
   *        bitrate: true | false,
   *        keyFrameInterval: true | false
   *      } | false
   *    } | false
   *  },
   *  notifying: {
   *    participantActivities: true | false,
   *    streamChange: true | false
   *  },
   *  sip: {
   *    sipServer: string(SipServerURL),
   *    username: string(SipUserID),
   *    password: string(SipUserPassword)
   *  } | false
   * }
   */
  var room_config;

  /*
   * {ParticipantId: object(Participant)
   * }
   */
  var participants = {};

  /*
   * {StreamId: object(Stream)}
   */
  var streams = {};

  /*
   * {TrackId: string(StreamId)}
   */
  var trackOwners = {};

  /*
   * {
   *   SubscriptionId: {
   *     id: string(SubscriptionId),
         locality: {agent: string(AgentRpcId), node: string(NodeRpcId)}
   *     media: {
   *       audio: {
   *         from: string(StreamId),
   *         status: 'active' | 'inactive' | undefined,
   *         format: object(AudioFormat),
   *       } | undefined,
   *       video: {
   *         from: string(StreamId),
   *         status: 'active' | 'inactive' | undefined,
   *         format: object(VideoFormat),
   *         parameters: {
   *           resolution: object(Resolution) | undefined,
   *           framerate: number(FramerateFPS) | undefined,
   *           bitrate: number(Kbps) | undefined,
   *           keyFrameInterval: number(Seconds) | undefined,
   *           } | undefined
   *       } | undefined
   *     },
   *     info: object(SubscriptionInfo):: {
   *       owner: string(ParticipantId),
   *       type: 'webrtc' | 'streaming' | 'recording' | 'sip' | 'analytics',
   *       location: {host: string(HostIPorDN), path: string(FileFullPath)} | undefined,
   *       url: string(URLofStreamingOut) | undefined
   *     }
   *   }
   * }
   */
  var subscriptions = {};

  var rpcChannel = require('./rpcChannel')(rpcClient),
      rpcReq = require('./rpcRequest')(rpcChannel);

  var onSessionEstablished = (participantId, sessionId, direction, sessionInfo) => {
    log.debug('onSessionEstablished, participantId:', participantId, 'sessionId:', sessionId, 'direction:', direction, 'sessionInfo:', JSON.stringify(sessionInfo));
    if (direction === 'in') {
      return addStream(sessionId, sessionInfo.locality, sessionInfo.media, sessionInfo.info
        ).then(() => {
          if (sessionInfo.info && sessionInfo.info.type !== 'webrtc') {
            sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready'});
          }
        }).catch((err) => {
          var err_msg = (err.message ? err.message : err);
          log.info('Exception:', err_msg);
          accessController && accessController.terminate(sessionId, 'in', err_msg).catch((e) => {
            log.info('Exception:', e.message ? e.message : e);
          });
        });
    } else if (direction === 'out') {
      return addSubscription(sessionId, sessionInfo.locality, sessionInfo.media, sessionInfo.info
        ).then(() => {
          if (sessionInfo.info && sessionInfo.info.type !== 'webrtc') {
            if (sessionInfo.info.location) {
              sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready', data: sessionInfo.info.location});
            } else {
              sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready'});
            }
          }
        }).catch((err) => {
          var err_msg = (err.message ? err.message : err);
          log.info('Exception:', err_msg);
          accessController && accessController.terminate(sessionId, 'out', err_msg).catch((e) => {
            log.info('Exception:', e.message ? e.message : e);
          });
        });
    } else {
      log.info('Unknown session direction:', direction);
    }
  };

  var onSessionAborted = (participantId, sessionId, direction, reason) => {
    log.debug('onSessionAborted, participantId:', participantId, 'sessionId:', sessionId, 'direction:', direction, 'reason:', reason);
    if (reason !== 'Participant terminate') {
      const rtcSession = rtcController.getOperation(sessionId);
      if (rtcSession) {
        // Session progress
        const transportId = rtcSession.transportId;
        sendMsgTo(participantId, 'progress', {id: transportId, sessionId, status: 'error', data: reason});
      } else {
        sendMsgTo(participantId, 'progress', {id: sessionId, status: 'error', data: reason});
      }
    }

    if (direction === 'in') {
      removeStream(sessionId)
        .catch((err) => {
          var err_msg = (err.message ? err.message : err);
          log.info(err_msg);
        });
    } else if (direction === 'out') {
      removeSubscription(sessionId)
       .catch((err) => {
         var err_msg = (err.message ? err.message : err);
         log.info(err_msg);
       });
    } else {
      log.info('Unknown session direction:', direction);
    }
  };

  var onLocalSessionSignaling = (participantId, transportId, signaling) => {
    log.debug('onLocalSessionSignaling, participantId:', participantId, 'transportId:', transportId, 'signaling:', signaling);
    if (participants[participantId]) {
      sendMsgTo(participantId, 'progress', {id: transportId, status: 'soac', data: signaling});
    }
  };

  var initRoom = function(roomId, origin) {
    if (is_initializing) {
      return new Promise(function(resolve, reject) {
        var interval = setInterval(function() {
          if (!is_initializing) {
            clearInterval(interval);
            if (room_id === roomId) {
              resolve('ok');
            } else {
              reject('room initialization failed');
            }
          }
        }, 50);
      });
    } else if (room_id !== undefined) {
      if (room_id !== roomId) {
        return Promise.reject('room id clash');
      } else {
        return Promise.resolve('ok');
      }
    } else {
      is_initializing = true;
      return dataAccess.room.config(roomId)
        .then(function(config) {
            //log.debug('initializing room:', roomId, 'got config:', JSON.stringify(config));
            room_config = config;
            room_config.internalConnProtocol = global.config.internal.protocol;
            StreamConfigure(room_config);

            return new Promise(function(resolve, reject) {
              RoomController.create(
                {
                  cluster: global.config.cluster.name || 'owt-cluster',
                  rpcReq: rpcReq,
                  rpcClient: rpcClient,
                  room: roomId,
                  config: room_config,
                  origin: origin,
                  selfRpcId: selfRpcId
                },
                function onOk(rmController) {
                  log.debug('room controller init ok');
                  roomController = rmController;
                  room_id = roomId;
                  is_initializing = false;

                  room_config.views.forEach((viewSettings) => {
                    var mixed_stream_id = room_id + '-' + viewSettings.label;
                    var mixed_stream_info = new MixedStream(mixed_stream_id, viewSettings.label);

                    streams[mixed_stream_id] = mixed_stream_info;
                    streams[mixed_stream_id].info.origin = origin;
                    log.debug('Mixed stream info:', mixed_stream_info);
                    room_config.notifying.streamChange &&
                      sendMsg('room', 'all', 'stream', {id: mixed_stream_id, status: 'add', data: mixed_stream_info.toPortalFormat()});
                  });

                  participants['admin'] = Participant({
                                                       id: 'admin',
                                                       user: 'admin',
                                                       role: 'admin',
                                                       portal: undefined,
                                                       origin: origin,
                                                       permission: {
                                                        subscribe: {audio: true, video: true},
                                                        publish: {audio: true, video: true}
                                                       }
                                                      }, rpcReq);

                  rtcController = new RtcController(room_id, rpcReq, selfRpcId, global.config.cluster.name || 'owt-cluster');
                  // Events
                  rtcController.on('transport-established', transportId => {
                    const transport = rtcController.getTransport(transportId);
                    sendMsgTo(transport.owner, 'progress', {id: transportId, status: 'ready'});
                  });
                  rtcController.on('transport-aborted', (transportId, reason) => {
                  });
                  rtcController.on('transport-signaling', (owner, transportId, message) => {
                    onLocalSessionSignaling(owner, transportId, message);
                  });
                  rtcController.on('session-established', (rtcInfo) => {
                    log.debug('New RTC session:', rtcInfo.id);
                    const sessionId = rtcInfo.id;
                    const media = { tracks: rtcInfo.tracks };
                    let direction = rtcInfo.direction;
                    const transport = rtcInfo.transport;
                    const sessionInfo = {
                      locality: transport.locality,
                      media: media,
                      info: { type: 'webrtc', owner: transport.owner }
                    };
                    sendMsgTo(transport.owner, 'progress', {id: transport.id, sessionId, status: 'ready'});
                    onSessionEstablished(transport.owner, sessionId, direction, sessionInfo);
                  });

                  rtcController.on('session-updated', (sessionId, data) => {
                    log.warn('Unexpected event session-updated');
                  });

                  rtcController.on('session-aborted', (sessionId, data) => {
                    onSessionAborted(data.owner, sessionId, data.direction, data.reason);
                  });

                  accessController = AccessController.create({clusterName: global.config.cluster.name || 'owt-cluster',
                                                              selfRpcId: selfRpcId,
                                                              inRoom: room_id,
                                                              mediaIn: room_config.mediaIn,
                                                              mediaOut: room_config.mediaOut},
                                                              rpcReq,
                                                              onSessionEstablished,
                                                              onSessionAborted,
                                                              onLocalSessionSignaling,
                                                              rtcController);
                  resolve('ok');
                },
                function onError(reason) {
                  log.error('roomController init failed.', reason);
                  is_initializing = false;
                  reject('roomController init failed. reason: ' + reason);
                });
            });
        }).catch(function(err) {
          log.error('Init room failed, reason:', err);
          is_initializing = false;
          setTimeout(() => {
            process.exit();
          }, 0);
          return Promise.reject(err);
        });
    }
  };

  var destroyRoom = function() {
    const doClean = () => {
      accessController && accessController.destroy();
      accessController = undefined;
      roomController && roomController.destroy();
      roomController = undefined;
      subscriptions = {};
      streams = {};
      participants = {};
      room_id = undefined;
    };

    var pl = [];
    for (var pid in participants) {
      if (pid !== 'admin' && accessController) {
        pl.push(accessController.participantLeave(pid));
      }
      if (pid !== 'admin' && rtcController) {
        pl.push(rtcController.terminateByOwner(pid));
      }
    }
    accessController && pl.push(accessController.participantLeave('admin'));
    rtcController && pl.push(rtcController.terminateByOwner('admin'));

    return Promise.all(pl)
      .then(() => {
        doClean();
        process.exit();
      });
  };

  const sendMsgTo = function(to, msg, data) {
    if (to !== 'admin') {
      if (participants[to]) {
        participants[to].notify(msg, data)
          .catch(function(reason) {
            log.debug('sendMsg fail:', reason);
          });
      } else {
        log.warn('Can not send message to:', to);
      }
    }
  };

  const sendMsg = function(from, to, msg, data) {
    log.debug('sendMsg, from:', from, 'to:', to, 'msg:', msg, 'data:', data);
    if (to === 'all') {
      for (var participant_id in participants) {
        sendMsgTo(participant_id, msg, data);
      }
    } else if (to === 'others') {
      for (var participant_id in participants) {
        if (participant_id !== from) {
          sendMsgTo(participant_id, msg, data);
        }
      }
    } else {
      sendMsgTo(to, msg, data);
    }
  };

  const addParticipant = function(participantInfo, permission) {
    participants[participantInfo.id] = Participant({
                                                    id: participantInfo.id,
                                                    user: participantInfo.user,
                                                    role: participantInfo.role,
                                                    portal: participantInfo.portal,
                                                    origin: participantInfo.origin,
                                                    permission: permission
                                                   }, rpcReq);
    room_config.notifying.participantActivities && sendMsg(participantInfo.id, 'others', 'participant', {action: 'join', data: {id: participantInfo.id, user: participantInfo.user, role: participantInfo.role}});
    return Promise.resolve('ok');
  };

  const removeParticipant = function(participantId) {
    if (participants[participantId]) {
      for (var sub_id in subscriptions) {
        if (subscriptions[sub_id].info.owner === participantId) {
          removeSubscription(sub_id);
        }
      }

      for (var stream_id in streams) {
        if (streams[stream_id].info.owner === participantId) {
          removeStream(stream_id);
        }
      }

      var participant = participants[participantId];
      var left_user = participant.getInfo();
      delete participants[participantId];
      room_config.notifying.participantActivities && sendMsg('room', 'all', 'participant', {action: 'leave', data: left_user.id});
    }
  };

  const dropParticipants = function(portal) {
    for (var participant_id in participants) {
      if (participants[participant_id].getPortal() === portal) {
        doDropParticipant(participant_id);
      }
    }
  };

  const extractAudioFormat = (audioInfo) => {
    var result = {codec: audioInfo.codec};
    audioInfo.sampleRate && (result.sampleRate = audioInfo.sampleRate);
    audioInfo.channelNum && (result.channelNum = audioInfo.channelNum);

    return result;
  };

  const extractVideoFormat = (videoInfo) => {
    var result = {codec: videoInfo.codec};
    videoInfo.profile && (result.profile = videoInfo.profile);

    return result;
  };

  const initiateStream = (id, info) => {
    if (streams[id]) {
      return Promise.reject('Stream already exists');
    }

    streams[id] = {
      id: id,
      type: 'forward',
      info: info,
      isInConnecting: true
    };

    return Promise.resolve('ok');
  };

  const addStream = (id, locality, media, info) => {
    info.origin = streams[id] ? streams[id].info.origin : {isp:"isp", region:"region"};
    if (info.analytics && subscriptions[info.analytics]) {
      const sourceTrack = subscriptions[info.analytics].media.tracks
        .find(t => t.type === 'video');
      const sourceId = streams[sourceTrack.from] ? sourceTrack.from : trackOwners[sourceTrack.from];
      if (streams[sourceId]) {
        info.origin = streams[sourceId].info.origin;
      } else {
        log.warn('Invalid analytics source when adding stream:', id);
      }
    }

    const fwdStream = new ForwardStream(id, media, info, locality);
    const errMsg = fwdStream.checkMediaError();
    if (errMsg) {
      return Promise.reject(errMsg);
    }

    const isReadded = !!(streams[id] && !streams[id].isInConnecting);
    const pubArgs = fwdStream.toRoomCtrlPubArgs();
    log.debug('PubArgs:', JSON.stringify(pubArgs));
    if (info.type === 'webrtc') {
      const pubs = pubArgs.map(pubArg => new Promise((resolve, reject) => {
        roomController && roomController.publish(
          pubArg.owner, pubArg.id, pubArg.locality, pubArg.media, pubArg.type, resolve, reject);
      }));
      return Promise.all(pubs).then(() => {
        if (participants[info.owner]) {
          streams[id] = fwdStream;
          pubArgs.forEach(pubArg => {
            trackOwners[pubArg.id] = id;
          });
          if (!isReadded) {
            setTimeout(() => {
              room_config.notifying.streamChange &&
                sendMsg('room', 'all', 'stream', {id: id, status: 'add', data: fwdStream.toPortalFormat()});
            }, 10);
          }
        } else {
          pubArgs.forEach(pubArg => {
            roomController && roomController.unpublish(info.owner, pubArg.id);
          });
          return Promise.reject('Participant early left');
        }
      });
    }

    // For non-webrtc stream
    const pubArg = pubArgs[0];
    return new Promise((resolve, reject) => {
      roomController && roomController.publish(
        pubArg.owner, pubArg.id, pubArg.locality, pubArg.media, pubArg.type,
        function() {
          if (participants[info.owner]) {
            streams[id] = fwdStream;
            if (!isReadded) {
              setTimeout(() => {
                room_config.notifying.streamChange &&
                  sendMsg('room', 'all', 'stream', {id: id, status: 'add', data: fwdStream.toPortalFormat()});
              }, 10);
            }
            resolve('ok');
          } else {
            roomController && roomController.unpublish(info.owner, pubArg.id);
            reject('Participant early left');
          }
        }, function(reason) {
          reject('roomController.publish failed, reason: ' + (reason.message ? reason.message : reason));
        });
    });
  };

  const updateStreamInfo = (streamId, info) => {
    if (!streams[streamId].isInConnecting && streams[streamId].update(info)) {
      room_config.notifying.streamChange &&
      sendMsg('room', 'all', 'stream', {
        id: streamId,
        status: 'update',
        data: {field: '.', value: streams[streamId].toPortalFormat()}
      });
    }
  };

  const removeStream = (streamId) => {
    return new Promise((resolve, reject) => {
      if (streams[streamId]) {
        for (var sub_id in subscriptions) {
          let subTrack = subscriptions[sub_id].media.tracks
            .find(t => (t.from === streamId || trackOwners[t.from] === streamId));
          if (subTrack) {
            accessController && accessController.terminate(sub_id, 'out', 'Source stream loss');
          }
        }

        if (!streams[streamId].isInConnecting) {
          // Only forward stream will be removed
          const pubArgs = streams[streamId].toRoomCtrlPubArgs();
          pubArgs.forEach(pubArg => {
            roomController && roomController.unpublish(streams[streamId].info.owner, pubArg.id);
          });
        }
        delete streams[streamId];
        setTimeout(() => {
          room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: streamId, status: 'remove'});
        }, 10);
      }
      resolve('ok');
    });
  };

  const initiateSubscription = (id, subSpec, info) => {
    if (subscriptions[id]) {
      return Promise.reject('Subscription already exists');
    }

    subscriptions[id] = {
      id: id,
      media: subSpec.media,
      info: info,
      isInConnecting: true
    };

    return Promise.resolve('ok');
  };

  const addSubscription = (id, locality, mediaSpec, info) => {
    if (!participants[info.owner]) {
      return Promise.reject('Participant early left');
    }
    const subscription = new Subscription(id, mediaSpec, locality, info);
    const pending = subscriptions[id];
    if (pending) {
      if (pending.isInConnecting) {
        // Assign pending parameters settings
        const tmp = new Subscription(id, pending.media, locality, info);
        subscription.media.tracks.forEach(t1 => {
          const mappedTrack = tmp.media.tracks
            .find(t2 => (t1.type === t2.type && t1.mid === t2.mid));
          t1.parameters = mappedTrack.parameters;
        });
      } else {
        log.warn('Add duplicate subscription:', id);
      }
    }
    for (const from of subscription.froms()) {
      const streamId = streams[from] ? from : trackOwners[from];
      if (streams[streamId]) {
        subscription.setSource(from, streams[streamId]);
      } else {
        return Promise.reject('Subscription source not found: ' + from);
      }
    }

    const isAudioPubPermitted = !!participants[info.owner].isPublishPermitted('audio'); 
    const subArgs = subscription.toRoomCtrlSubArgs();
    const subs = subArgs.map(subArg => new Promise((resolve, reject) => {
      if (roomController) {
        roomController.subscribe(
          subArg.owner, subArg.id, subArg.locality, subArg.media, subArg.type,
          isAudioPubPermitted, resolve, reject);
      } else {
        reject('RoomController is not ready');
      }
    }));
    return Promise.all(subs).then(() => {
      if (participants[info.owner]) {
        subscriptions[id] = subscription;
        return Promise.resolve('ok');
      } else {
        subArgs.forEach(subArg => {
          roomController && roomController.unsubscribe(info.owner, subArg.id);
        });
        return Promise.reject('Participant early left');
      }
    });
  };

  const removeSubscription = (subscriptionId) => {
    return new Promise((resolve, reject) => {
      if (subscriptions[subscriptionId]) {
        if (!subscriptions[subscriptionId].isInConnecting) {
          const subArgs = subscriptions[subscriptionId].toRoomCtrlSubArgs();
          subArgs.forEach(subArg => {
            roomController && roomController.unsubscribe(
              subscriptions[subscriptionId].info.owner, subArg.id);
          });
        }
        delete subscriptions[subscriptionId];
      }
      resolve('ok');
    });
  };

  const doUnpublish = (streamId) => {
    if (streams[streamId]) {
      if (streams[streamId].info.type === 'sip' || streams[streamId].info.type === 'analytics') {
        return removeStream(streamId);
      } else {
        return accessController.terminate(streamId, 'in', 'Participant terminate');
      }
    } else {
      return Promise.reject('Stream does NOT exist');
    }
  };

  const doUnsubscribe = (subId) => {
    if (subscriptions[subId]) {
      if (subscriptions[subId].info.type === 'sip') {
        return removeSubscription(subId);
      } else {
        return accessController.terminate(subId, 'out', 'Participant terminate');
      }
    } else {
      return Promise.reject('Subscription does NOT exist');
    }
  };

  const roomIsIdle = () => {
    var hasNonAdminParticipant = false,
      hasPublication = false,
      hasSubscription = false;

    for (let k in participants) {
      if (k !== 'admin') {
        hasNonAdminParticipant = true;
        break;
      }
    };

    if (!hasNonAdminParticipant) {
      for (let st in streams) {
        if (streams[st].type === 'forward') {
          hasPublication = true;
          break;
        }
      }
    }

    hasSubscription = (Object.keys(subscriptions).length > 0);

    return (!hasNonAdminParticipant && !hasPublication && !hasSubscription);
  };

  const selfClean = () => {
    setTimeout(function() {
      if (roomIsIdle()) {
        log.info('Empty room ', room_id, '. Deleting it');
        destroyRoom();
      }
    }, 30 * 1000);
  };

  const currentInputCount = () => {
    return Object.keys(streams).filter((stream_id) => {
      return (streams[stream_id].type === 'forward'
        && streams[stream_id].info.type !== 'analytics');
    }).length;
  };

  that.join = function(roomId, participantInfo, callback) {
    log.debug('participant:', participantInfo, 'join room:', roomId);
    var permission;
    return initRoom(roomId, participantInfo.origin)
      .then(function() {
        log.debug('room_config.participantLimit:', room_config.participantLimit, 'current participants count:', Object.keys(participants).length);
        if (room_config.participantLimit > 0 && (Object.keys(participants).length >= room_config.participantLimit + 1)) {
          log.warn('Room is full');
          callback('callback', 'error', 'Room is full');
          return Promise.reject('Room is full');
        }

        var my_role_def = room_config.roles.filter((roleDef) => {return roleDef.role === participantInfo.role;});
        if (my_role_def.length < 1) {
          callback('callback', 'error', 'Invalid role');
          return Promise.reject('Invalid role');
        }

        permission = {
          publish: {
            video: my_role_def[0].publish.video,
            audio: my_role_def[0].publish.audio
          },
          subscribe: {
            video: my_role_def[0].subscribe.video,
            audio: my_role_def[0].subscribe.audio
          }
        };

        return addParticipant(participantInfo, permission);
      }).then(function() {
        var current_participants = [],
            current_streams = [];

        for (var participant_id in participants) {
          if (participant_id !== 'admin') {
            current_participants.push(participants[participant_id].getInfo());
          }
        }

        for (var stream_id in streams) {
          if (!streams[stream_id].isInConnecting) {
            current_streams.push(streams[stream_id].toPortalFormat());
          }
        }

        callback('callback', {
          permission: permission,
          room: {
            id: room_id,
            views: room_config.views.map((viewSettings) => {return viewSettings.label;}),
            participants: current_participants,
            streams: current_streams
          }
        });
      }, function(err) {
        log.warn('Participant ' + participantInfo.id + ' join room ' + roomId + ' failed, err:', err);
        callback('callback', 'error', 'Joining room failed');
      });
  };

  that.leave = function(participantId, callback) {
    log.debug('leave, participantId:', participantId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    return accessController.participantLeave(participantId)
      .then(() => rtcController.terminateByOwner(participantId))
      .then((result) => {
        callback('callback', 'ok');
        selfClean();
      }, (e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  that.onSessionSignaling = function(sessionId, signaling, callback) {
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    return rtcController.onClientTransportSignaling(sessionId, signaling)
      .then((result) => {
        callback('callback', 'ok');
      }, (e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  const translateRtcPubIfNeeded = function (pubInfo) {
    if (pubInfo.tracks) {
      return pubInfo;
    }
    const rtcPubInfo = {
      type: pubInfo.type,
      transportId: pubInfo.transportId,
      tracks: pubInfo.media.tracks,
      legacy: pubInfo.legacy
    };
    return rtcPubInfo;
  };

  const translateRtcSubIfNeeded = function (subDesc) {
    if (subDesc.tracks) {
      return subDesc;
    }
    const rtcSubInfo = {
      type: subDesc.type,
      transportId: subDesc.transportId,
      tracks: subDesc.media.tracks,
      legacy: subDesc.legacy,
    };
    return rtcSubInfo;
  };


  that.publish = function(participantId, streamId, pubInfo, callback) {
    log.debug('publish, participantId:', participantId, 'streamId:', streamId, 'pubInfo:', JSON.stringify(pubInfo));
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      log.info('Participant ' + participantId + 'has not joined');
      return callback('callback', 'error', 'Participant has not joined');
    }

    if ((pubInfo.media.audio && !participants[participantId].isPublishPermitted('audio'))
        || (pubInfo.media.video && !participants[participantId].isPublishPermitted('video'))) {
      return callback('callback', 'error', 'unauthorized');
    }
    if (pubInfo.type === 'webrtc' && pubInfo.media.tracks) {
      if ((pubInfo.media.tracks.find(t => t.type === 'audio')
        && !participants[participantId].isPublishPermitted('audio'))
        || (pubInfo.media.tracks.find(t => t.type === 'video')
        && !participants[participantId].isPublishPermitted('video'))) {
        return callback('callback', 'error', 'unauthorized');
      }
    }

    if (pubInfo.type !== 'analytics' && room_config.inputLimit >= 0 && (room_config.inputLimit <= currentInputCount())) {
      return callback('callback', 'error', 'Too many inputs');
    }

    if (streams[streamId] && (pubInfo.type !== 'analytics')) {
      return callback('callback', 'error', 'Stream exists');
    }

    if (pubInfo.media.audio && !room_config.mediaIn.audio.length) {
      return callback('callback', 'error', 'Audio is forbiden');
    }

    if (pubInfo.media.video && !room_config.mediaIn.video.length) {
      return callback('callback', 'error', 'Video is forbiden');
    }

    if (pubInfo.type === 'sip') {
      return addStream(streamId, pubInfo.locality, pubInfo.media, {owner: participantId, type: 'sip'})
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
    } else if (pubInfo.type === 'analytics') {
      return addStream(streamId, pubInfo.locality,
        pubInfo.media,
        {owner: 'admin', type: 'analytics', analytics: pubInfo.analyticsId})
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
    } else {
      var origin = participants[participantId].getOrigin();
      var format_preference;
      if (pubInfo.type === 'webrtc') {
        const rtcPubInfo = translateRtcPubIfNeeded(pubInfo);
        // Set formatPreference
        rtcPubInfo.tracks.forEach(track => {
          track.formatPreference = {optional: room_config.mediaIn[track.type]};
        });
        initiateStream(streamId, {owner: participantId, type: pubInfo.type, origin});
        return rtcController.initiate(participantId, streamId, 'in', participants[participantId].getOrigin(), rtcPubInfo)
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          removeStream(streamId);
          callback('callback', 'error', e.message ? e.message : e);
        });
      }

      initiateStream(streamId, {owner: participantId, type: pubInfo.type, origin});
      return accessController.initiate(participantId, streamId, 'in', participants[participantId].getOrigin(), pubInfo, format_preference)
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        removeStream(streamId);
        callback('callback', 'error', e.message ? e.message : e);
      });
    }
  };

  that.unpublish = function(participantId, streamId, callback) {
    log.debug('unpublish, participantId:', participantId, 'streamId:', streamId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    //if (streams[streamId].info.owner !== participantId) {
    //  return callback('callback', 'error', 'unauthorized');
    //}

    return doUnpublish(streamId)
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  const getStreamTrack = (tsId, type) => {
    let track = null;
    if (trackOwners[tsId]) {
      track = streams[trackOwners[tsId]].media.tracks
        .find(t => (t.id === tsId && t.type === type));
    } else if (streams[tsId]) {
      track = streams[tsId].media.tracks
        .find(t => t.type === type);
    }
    return track;
  };

  const isAudioFmtAvailable = (streamAudio, fmt) => {
    //log.debug('streamAudio:', JSON.stringify(streamAudio), 'fmt:', fmt);
    if (isAudioFmtEqual(streamAudio.format, fmt)) {
      return true;
    }
    if (streamAudio.optional && streamAudio.optional.format && (streamAudio.optional.format.findIndex((f) => {return isAudioFmtEqual(f, fmt);}) >= 0)) {
      return true;
    }
    return false;
  };

  const validateAudioRequest = (type, req, err) => {
    let track = getStreamTrack(req.from, 'audio');
    if (!track) {
      err && (err.message = 'Requested audio stream');
      return false;
    }
    if (req.format) {
      if (!isAudioFmtAvailable(track, req.format)) {
        err && (err.message = 'Format is not acceptable');
        return false;
      }
    }
    return true;
  };

  const isVideoFmtAvailable = (streamVideo, fmt) => {
    //log.debug('streamVideo:', JSON.stringigy(streamVideo), 'fmt:', fmt);
    if (isVideoFmtCompatible(streamVideo.format, fmt)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.format && (streamVideo.optional.format.filter((f) => {return isVideoFmtCompatible(f, fmt);}).length > 0)) {
      return true;
    }
    return false;
  };

  const isResolutionAvailable = (streamVideo, resolution) => {
    if (streamVideo.parameters && streamVideo.parameters.resolution && isResolutionEqual(streamVideo.parameters.resolution, resolution)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.resolution && (streamVideo.optional.parameters.resolution.findIndex((r) => {return isResolutionEqual(r, resolution);}) >= 0)) {
      return true;
    }
    if (streamVideo.alternative) {
      for (const alt of streamVideo.alternative) {
        if (alt.parameters && alt.parameters.resolution &&
            isResolutionEqual(alt.parameters.resolution, resolution)) {
          return true;
        }
      }
    }
    return false;
  };

  const isFramerateAvailable = (streamVideo, framerate) => {
    if (streamVideo.parameters && streamVideo.parameters.framerate && (streamVideo.parameters.framerate === framerate)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.framerate && (streamVideo.optional.parameters.framerate.findIndex((f) => {return f === framerate;}) >= 0)) {
      return true;
    }
    return false;
  };

  const isBitrateAvailable = (streamVideo, bitrate) => {
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.bitrate && (streamVideo.optional.parameters.bitrate.findIndex((b) => {return b === bitrate;}) >= 0)) {
      return true;
    }
    return false;
  };

  const isKeyFrameIntervalAvailable = (streamVideo, keyFrameInterval) => {
    if (streamVideo.parameters && streamVideo.parameters.resolution && (streamVideo.parameters.keyFrameInterval === keyFrameInterval)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.keyFrameInterval && (streamVideo.optional.parameters.keyFrameInterval.findIndex((k) => {return k === keyFrameInterval;}) >= 0)) {
      return true;
    }
    return false;
  };

  const validateVideoRequest = (type, req, err) => {
    let track = getStreamTrack(req.from, 'video');
    if (!track) {
      err && (err.message = 'Requested video stream');
      return false;
    }
    if (req.format && !isVideoFmtAvailable(track, req.format)) {
      err && (err.message = 'Format is not acceptable');
      return false;
    }
    if (req.parameters) {
      if (req.parameters.resolution && !isResolutionAvailable(track, req.parameters.resolution)) {
        err && (err.message = 'Resolution is not acceptable');
        return false;
      }

      if (req.parameters.framerate && !isFramerateAvailable(track, req.parameters.framerate)) {
        err && (err.message = 'Framerate is not acceptable');
        return false;
      }
      //FIXME: allow bitrate 1.0x for client-sdk
      if (req.parameters.bitrate === 'x1.0' || req.parameters.bitrate === 'x1') {
        req.parameters.bitrate = undefined;
      }
      if (req.parameters.bitrate && !isBitrateAvailable(track, req.parameters.bitrate)) {
        err && (err.message = 'Bitrate is not acceptable');
        return false;
      }
      if (req.parameters.keyFrameInterval && !isKeyFrameIntervalAvailable(track, req.parameters.keyFrameInterval)) {
        err && (err.message = 'KeyFrameInterval is not acceptable');
        return false;
      }
    }

    return true;
  };

  that.subscribe = function(participantId, subscriptionId, subDesc, callback) {
    log.debug('subscribe, participantId:', participantId, 'subscriptionId:', subscriptionId, 'subDesc:', JSON.stringify(subDesc));
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      log.info('Participant ' + participantId + 'has not joined');
      return callback('callback', 'error', 'Participant has not joined');
    }

    if ((subDesc.media.audio && !participants[participantId].isSubscribePermitted('audio'))
        || (subDesc.media.video && !participants[participantId].isSubscribePermitted('video'))) {
      return callback('callback', 'error', 'unauthorized');
    }
    if (subDesc.type === 'webrtc' && subDesc.media.tracks) {
      if ((subDesc.media.tracks.find(t => t.type === 'audio')
        && !participants[participantId].isSubscribePermitted('audio'))
        || (subDesc.media.tracks.find(t => t.type === 'video')
        && !participants[participantId].isSubscribePermitted('video'))) {
        return callback('callback', 'error', 'unauthorized');
      }
    }

    if (subscriptions[subscriptionId]) {
      return callback('callback', 'error', 'Subscription exists');
    }

    var requestError = { message: '' };
    if (subDesc.media.audio && !validateAudioRequest(subDesc.type, subDesc.media.audio, requestError)) {
      return callback('callback', 'error', 'Target audio stream does NOT satisfy:' + requestError.message);
    }

    if (subDesc.media.video && !validateVideoRequest(subDesc.type, subDesc.media.video, requestError)) {
      return callback('callback', 'error', 'Target video stream does NOT satisfy:' + requestError.message);
    }

    if (subDesc.type === 'sip') {
      return addSubscription(subscriptionId, subDesc.locality, subDesc.media, {owner: participantId, type: 'sip'})
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
    } else {
      var format_preference;
      if (subDesc.type === 'webrtc') {
        const rtcSubInfo = translateRtcSubIfNeeded(subDesc);
        // Set formatPreference
        rtcSubInfo.tracks.forEach(track => {
          const source = streams[track.from].media.tracks.find(t => t.type === track.type);
          const formatPreference = {};
          if (streams[track.from].type === 'forward') {
            formatPreference.preferred = source.format;
            source.optional && source.optional.format && (formatPreference.optional = source.optional.format);
          } else {
            formatPreference.optional = [source.format];
            source.optional && source.optional.format && (formatPreference.optional = formatPreference.optional.concat(source.optional.format));
          }
          track.formatPreference = formatPreference;
        });

        initiateSubscription(subscriptionId, subDesc, {owner: participantId, type: subDesc.type});
        return rtcController.initiate(participantId, subscriptionId, 'out', participants[participantId].getOrigin(), rtcSubInfo)
        .then((result) => {
          const releasedSource = rtcSubInfo.tracks.find(track => {
            const sourceStreamId = trackOwners[track.from] || track.from;
            return !streams[sourceStreamId];
          });
          if (releasedSource) {
            rtcController.terminate(participantId, subscriptionId, 'Participant terminate');
            return Promise.reject('Target audio/video stream early released');
          }
          callback('callback', result);
        })
        .catch((e) => {
          removeSubscription(subscriptionId);
          callback('callback', 'error', e.message ? e.message : e);
        });
      }

      if (subDesc.type === 'recording') {
        var audio_codec = 'none-aac', video_codec;
        if (subDesc.media.audio) {
          if (subDesc.media.audio.format) {
            audio_codec = subDesc.media.audio.format.codec;
          } else {
            let track = getStreamTrack(subDesc.media.audio.from, 'audio');
            audio_codec = track.format.codec;
          }

          //FIXME: To support codecs other than those in the following list.
          if ((audio_codec !== 'pcmu') && (audio_codec !== 'pcma') && (audio_codec !== 'opus') && (audio_codec !== 'aac')) {
            return Promise.reject('Audio codec invalid');
          }
        }

        if (subDesc.media.video) {
          let track = getStreamTrack(subDesc.media.video.from, 'video');
          video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || track.format.codec);
        }

        if (!subDesc.connection.container || subDesc.connection.container === 'auto') {
          subDesc.connection.container = ((audio_codec === 'aac' && (!video_codec || (video_codec === 'h264') || (video_codec === 'h265'))) ? 'mp4' : 'mkv');
        }
      }

      if (subDesc.type === 'streaming') {
        if (subDesc.media.audio && !subDesc.media.audio.format) {
          var aacFmt = room_config.mediaOut.audio.find((a) => a.codec === 'aac');
          if (!aacFmt) {
            return Promise.reject('Audio codec aac not enabled');
          }
          subDesc.media.audio.format = aacFmt;
        }

        //FIXME: To support codecs other than those in the following list.
        if (subDesc.media.audio && (subDesc.media.audio.format.codec !== 'aac')) {
          return Promise.reject('Audio codec invalid');
        }

        if (subDesc.media.video && !subDesc.media.video.format) {
          subDesc.media.video.format = {codec: 'h264'};
        }

        //FIXME: To support codecs other than those in the following list.
        if (subDesc.media.video && (subDesc.media.video.format.codec !== 'h264')) {
          return Promise.reject('Video codec invalid');
        }
      }

      initiateSubscription(subscriptionId, subDesc, {owner: participantId, type: subDesc.type});
      return accessController.initiate(participantId, subscriptionId, 'out', participants[participantId].getOrigin(), subDesc, format_preference)
      .then((result) => {
        if ((subDesc.media.audio && !streams[subDesc.media.audio.from])
           ||(subDesc.media.video && !streams[subDesc.media.video.from])) {
          accessController.terminate(participantId, subscriptionId, 'Participant terminate');
          return Promise.reject('Target audio/video stream early released');
        }
        callback('callback', result);
      })
      .catch((e) => {
        removeSubscription(subscriptionId);
        callback('callback', 'error', e.message ? e.message : e);
      });
    }
  };

  that.unsubscribe = function(participantId, subscriptionId, callback) {
    log.debug('unsubscribe, participantId:', participantId, 'subscriptionId:', subscriptionId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    //if (subscriptions[subscriptionId].info.owner !== participantId) {
    //  return callback('callback', 'error', 'unauthorized');
    //}

    return doUnsubscribe(subscriptionId)
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  const mix = function(streamId, toView) {
    if (streams[streamId].isInConnecting) {
      return Promise.reject('Stream is NOT ready');
    }

    if (streams[streamId].info.inViews.indexOf(toView) !== -1) {
      return Promise.resolve('ok');
    }

    const trackIds = streams[streamId].media.tracks
      .map(t => t.id).filter(id => !!id);
    if (trackIds.length === 0) {
      // Mix the no-track-id stream
      trackIds.push(streamId);
    }

    const mixOps = trackIds.map(id => new Promise((resolve, reject) => {
      roomController.mix(id, toView, resolve, reject);
    }));
    return Promise.all(mixOps).then(() => {
      if (streams[streamId].info.inViews.indexOf(toView) === -1) {
        streams[streamId].info.inViews.push(toView);
      }
      return Promise.resolve('ok');
    }).catch(reason => {
      log.info('roomController.mix failed, reason:', reason);
      throw reason;
    });
  };

  const unmix = function(streamId, fromView) {
    if (streams[streamId].isInConnecting) {
      return Promise.reject('Stream is NOT ready');
    }

    const trackIds = streams[streamId].media.tracks
      .map(t => t.id).filter(id => !!id);
    if (trackIds.length === 0) {
      // Mix the no-track-id stream
      trackIds.push(streamId);
    }

    const unmixOps = trackIds.map(id => new Promise((resolve, reject) => {
      roomController.unmix(id, fromView, resolve, reject);
    }));
    return Promise.all(unmixOps).then(() => {
      streams[streamId].info.inViews.splice(streams[streamId].info.inViews.indexOf(fromView), 1);
      return Promise.resolve('ok');
    }).catch(reason => {
      log.info('roomController.unmix failed, reason:', reason);
      throw reason;
    });
  };

  const setStreamMute = function(streamId, track, muted) {
    if (streams[streamId].type === 'mixed') {
      return Promise.reject('Stream is Mixed');
    }

    const audio = (track === 'audio' || track === 'av') ? true : false;
    const video = (track === 'video' || track === 'av') ? true : false;
    const status = muted ? 'inactive' : 'active';

    const affectedTracks = streams[streamId].media.tracks
      .filter(t => ((audio && t.type === 'audio') || (video && t.type === 'video')))
      .map(t => t.id);
    if (affectedTracks.length === 0) {
      return Promise.reject('Stream does NOT contain valid track to mute/unmute');
    }
    return rtcController.setMute(streamId, affectedTracks, muted)
      .then(() => {
        streams[streamId].media.tracks.forEach(t => {
          if (affectedTracks.indexOf(t.id) > -1) {
            t.status = status;
            roomController.updateStream(t.id, track, status);
          }
        });
        var updateFields = (track === 'av') ? ['audio.status', 'video.status'] : [track + '.status'];
        room_config.notifying.streamChange && updateFields.forEach((fieldData) => {
          sendMsg('room', 'all', 'stream', {status: 'update', id: streamId, data: {field: fieldData, value: status}});
        });
        return 'ok';
      }, function(reason) {
        log.warn('rtcController set mute failed:', reason);
        return Promise.reject(reason);
      });
  };

  const getRegion = function(streamId, inView) {
    if (streams[streamId].info.type === 'webrtc') {
      streamId = getStreamTrack(streamId, 'video').id;
    }
    return new Promise((resolve, reject) => {
      roomController.getRegion(streamId, inView, function(region) {
        resolve({region: region});
      }, function(reason) {
        log.info('roomController.getRegion failed, reason:', reason);
        reject(reason);
      });
    });
  };

  const setRegion = function(streamId, regionId, inView) {
    if (streams[streamId].info.type === 'webrtc') {
      streamId = getStreamTrack(streamId, 'video').id;
    }
    return new Promise((resolve, reject) => {
      roomController.setRegion(streamId, regionId, inView, function() {
        resolve('ok');
      }, function(reason) {
        log.info('roomController.setRegion failed, reason:', reason);
        reject(reason);
      });
    });
  };

  // Convert trackId to streamId for webrtc stream in layout
  const convertLayout = function (layout) {
    if (layout) {
      layout = layout.map(mapRegion => {
        const streamId = mapRegion.stream;
        return {
          stream: streams[streamId] ? streamId : trackOwners[streamId],
          region: mapRegion.region
        };
      });
    }
    return layout;
  };

  const setLayout = function(streamId, layout) {
    if (layout) {
      layout.forEach(mapRegion => {
        const streamId = mapRegion.stream;
        if (streams[streamId] && streams[streamId].info.type === 'webrtc') {
          mapRegion.stream = getStreamTrack(streamId, 'video').id;
        }
      });
    }
    return new Promise((resolve, reject) => {
      roomController.setLayout(streams[streamId].info.label, layout, function(updated) {
        if (streams[streamId]) {
          streams[streamId].info.layout = convertLayout(updated);
          resolve('ok');
        } else {
          reject('stream early terminated');
        }
      }, function(reason) {
        log.info('roomController.setLayout failed, reason:', reason);
        reject(reason);
      });
    });
  };

  that.streamControl = (participantId, streamId, command, callback) => {
    log.debug('streamControl, participantId:', participantId, 'streamId:', streamId, 'command:', JSON.stringify(command));

    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    if (streams[streamId] === undefined) {
      log.info('Stream ' + streamId + ' does not exist');
      return callback('callback', 'error', 'Stream does NOT exist');
    }

    if ((streams[streamId].info.owner !== participantId)
        && (participants[participantId].getInfo().role !== 'admin') //FIXME: back-door for 3.4 client with 'admin' role
       ) {
      return callback('callback', 'error', 'unauthorized');
    }

    var op;
    switch (command.operation) {
      case 'mix':
        op = mix(streamId, command.data);
        break;
      case 'unmix':
        op = unmix(streamId, command.data);
        break;
      case 'set-region':
        op = setRegion(streamId, command.data.region, command.data.view);
        break;
      case 'get-region':
        op = getRegion(streamId, command.data);
        break;
      case 'pause':
        op = setStreamMute(streamId, command.data, true);
        break;
      case 'play':
        op = setStreamMute(streamId, command.data, false);
        break;
      default:
        op = Promise.reject('Invalid stream control operation');
    }

    return op.then((result) => {
        callback('callback', result);
      }, (err) => {
        log.info('streamControl failed', err);
        callback('callback', 'error', err.message ? err.message : err);
      });
  };

  const updateSubscription = (subscriptionId, update) => {
    const oldSub = subscriptions[subscriptionId];
    const newSubMedia = JSON.parse(JSON.stringify(oldSub.media));
    let audioTrack = null;
    let videoTrack = null;
    let effective = false;

    if (update.audio) {
      audioTrack = newSubMedia.tracks.find(t => t.type === 'audio');
      if (!audioTrack) {
        return Promise.reject('Target subscription does NOT have audio to update');
      }
      if (update.audio.from && update.audio.from !== audioTrack.from) {
        audioTrack.from = update.audio.from;
        effective = true;
      }
    }

    if (update.video) {
      videoTrack = newSubMedia.tracks.find(t => t.type === 'video');
      if (!videoTrack) {
        return Promise.reject('Target subscription does NOT have video to update');
      }
      if (update.video.from && update.video.from !== videoTrack.from) {
        videoTrack.from = update.video.from;
        effective = true;
      }
      if (update.video.parameters && (Object.keys(update.video.parameters).length > 0)) {
        videoTrack.parameters = (videoTrack.parameters || {});
        if (update.video.parameters.resolution &&
            !isResolutionEqual(update.video.parameters.resolution, videoTrack.parameters.resolution)) {
          videoTrack.parameters.resolution = update.video.parameters.resolution;
          effective = true;
        }
        if (update.video.parameters.framerate &&
            update.video.parameters.framerate !== videoTrack.parameters.framerate) {
          videoTrack.parameters.framerate = update.video.parameters.framerate;
          effective = true;
        }
        if (update.video.parameters.bitrate &&
            update.video.parameters.bitrate !== videoTrack.parameters.bitrate) {
          videoTrack.parameters.bitrate = update.video.parameters.bitrate;
          effective = true;
        }
        if (update.video.parameters.keyFrameInterval &&
            update.video.parameters.keyFrameInterval !== videoTrack.parameters.keyFrameInterval) {
          videoTrack.parameters.keyFrameInterval = update.video.parameters.keyFrameInterval;
          effective = true;
        }
      }
    }

    if (!effective) {
      return Promise.resolve('ok');
    }
    const err = {};
    if (videoTrack && !validateVideoRequest(oldSub.info.type, videoTrack, err)) {
      return Promise.reject('Target video stream does NOT satisfy:', (err && err.message));
    }
    if (audioTrack && !validateAudioRequest(oldSub.info.type, audioTrack, err)) {
      return Promise.reject('Target audio stream does NOT satisfy', (err && err.message));
    }

    return removeSubscription(subscriptionId)
      .then((result) => {
        return addSubscription(subscriptionId, oldSub.locality, newSubMedia, oldSub.info);
      }).catch((err) => {
        log.info('Update subscription failed:', err.message ? err.message : err);
        log.info('And is recovering the previous subscription:', JSON.stringify(old_su));
        return addSubscription(subscriptionId, oldSub.locality, oldSub.media, oldSub.info)
          .then(() => {
            return Promise.reject('Update subscription failed');
          }, () => {
            return Promise.reject('Update subscription failed');
          });
      });
  };

  const setSubscriptionMute = (subscriptionId, track, muted) => {
    const audio = (track === 'audio' || track === 'av') ? true : false;
    const video = (track === 'video' || track === 'av') ? true : false;
    const status = (muted ? 'active' : 'inactive');

    const affectedTracks = subscriptions[subscriptionId].media.tracks
      .filter(t => ((audio && t.type === 'audio') || (video && t.type === 'video')))
      .map(t => t.id);
    if (affectedTracks.length === 0) {
      return Promise.reject('Stream does NOT contain valid track to mute/unmute');
    }
    return rtcController.setMute(subscriptionId, affectedTracks, muted)
      .then(() => {
        subscriptions[subscriptionId].media.tracks.forEach(t => {
          if (affectedTracks.indexOf(t.id) > -1) {
            t.status = status;
          }
        });
        return 'ok';
      });
  };

  that.subscriptionControl = (participantId, subscriptionId, command, callback) => {
    log.debug('subscriptionControl, participantId:', participantId, 'subscriptionId:', subscriptionId, 'command:', JSON.stringify(command));

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    if (!subscriptions[subscriptionId]) {
      return callback('callback', 'error', 'Subscription does NOT exist');
    }

    if ((subscriptions[subscriptionId].info.owner !== participantId)
        && (participants[participantId].getInfo().role !== 'admin') //FIXME: back-door for 3.4 client with 'admin' role
       ) {
      return callback('callback', 'error', 'unauthorized');
    }


    var op;
    switch (command.operation) {
      case 'update':
        op = updateSubscription(subscriptionId, command.data);
        break;
      case 'pause':
        op = setSubscriptionMute(subscriptionId, command.data, true);
        break;
      case 'play':
        op = setSubscriptionMute(subscriptionId, command.data, false);
        break;
      default:
        op = Promise.reject('Invalid subscription control operation');
    }

    return op.then((result) => {
        callback('callback', result);
      }, (err) => {
        callback('callback', 'error', err.message ? err.message : err);
      });
  };

  that.text = function(fromParticipantId, toParticipantId, msg, callback) {
    if (participants[fromParticipantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    if ((toParticipantId !== 'all') && (participants[toParticipantId] === undefined)) {
      return callback('callback', 'error', 'Target participant does NOT exist: ' + toParticipantId);
    }

    sendMsg(fromParticipantId, toParticipantId, 'text', {from: fromParticipantId, to: (toParticipantId === 'all' ? 'all' : 'me'), message: msg});
    callback('callback', 'ok');
  };

  that.onSessionProgress = function (sessionId, direction, sessionStatus) {
    log.debug('onSessionProgress, sessionId:', sessionId, 'direction:', direction, 'sessionStatus:', sessionStatus);
    accessController && accessController.onSessionStatus(sessionId, sessionStatus);
  };

  that.onTransportProgress = function (transportId, status) {
    log.debug('onTransportProgress, transportId:', transportId, 'status:', status);
    rtcController && rtcController.onTransportProgress(transportId, status);
  };

  that.onMediaUpdate = function (trackId, direction, mediaUpdate) {
    log.debug('onMediaUpdate, trackId:', trackId, 'direction:', direction, 'mediaUpdate:', JSON.stringify(mediaUpdate));
    if (direction === 'in') {
      if (rtcController && rtcController.getTrack(trackId)) {
        // Update from webrtc
        const track = rtcController.getTrack(trackId);
        mediaUpdate.id = trackId;
        updateStreamInfo(track.operationId, mediaUpdate);
        roomController && roomController.updateStreamInfo(trackId, mediaUpdate);
      } else if (streams[trackId] && streams[trackId].type === 'forward'){
        // Update from others e.g SIP
        const sessionId = trackId;
        updateStreamInfo(sessionId, mediaUpdate);
        roomController && roomController.updateStreamInfo(sessionId, mediaUpdate);
      }
    }
  };

  that.onTrackUpdate = function (sessionId, trackUpdate) {
    log.debug('onTrackUpdate, sessionId:', sessionId, 'update:', JSON.stringify(trackUpdate));
    rtcController && rtcController.onTrackUpdate(sessionId, trackUpdate);
  };

  that.onVideoLayoutChange = function(roomId, layout, view, callback) {
    log.debug('onVideoLayoutChange, roomId:', roomId, 'layout:', layout, 'view:', view);
    if (room_id === roomId && roomController) {
      var streamId = roomController.getMixedStream(view);
      if (streams[streamId]) {
        layout = convertLayout(layout);
        streams[streamId].info.layout = layout;
        room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {status: 'update', id: streamId, data: {field: 'video.layout', value: layout}});
        callback('callback', 'ok');
      } else {
        callback('callback', 'error', 'no mixed stream.');
      }
    } else {
      callback('callback', 'error', 'room is not in service');
    }
  };

  that.onAudioActiveness = function(roomId, activeInputStream, view, callback) {
    log.debug('onAudioActiveness, roomId:', roomId, 'activeInputStream:', activeInputStream, 'view:', view);
    if ((room_id === roomId) && roomController) {
      room_config.views.forEach((viewSettings) => {
        if (viewSettings.label === view && viewSettings.video.keepActiveInputPrimary) {
          roomController.setPrimary(activeInputStream, view);
        }
      });

      var input = streams[activeInputStream] ? activeInputStream : trackOwners[activeInputStream];
      if (input && streams[input]) {
        for (var id in streams) {
          if (streams[id].type === 'mixed' && streams[id].info.label === view) {
            if (streams[id].info.activeInput !== input) {
              streams[id].info.activeInput = input;
              room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: id, status: 'update', data: {field: 'activeInput', value: input}});
            }
            break;
          }
        }
      }
      callback('callback', 'ok');
    } else {
      log.info('onAudioActiveness, room does not exist');
      callback('callback', 'error', 'room is not in service');
    }
  };

  //The following interfaces are reserved to serve management-api
  that.getParticipants = function(callback) {
    log.debug('getParticipants, room_id:', room_id);
    var result = [];
    for (var participant_id in participants) {
      (participant_id !== 'admin') && result.push(participants[participant_id].getDetail());
    }
    callback('callback', result);
  };

  //FIXME: Should handle updates other than authorities as well.
  that.controlParticipant = function(participantId, authorities, callback) {
    log.debug('controlParticipant', participantId, 'authorities:', authorities);
    if (participants[participantId] === undefined) {
      callback('callback', 'error', 'Participant does NOT exist');
    }

    return Promise.all(
      authorities.map((auth) => {
        if (participants[participantId]) {
          return participants[participantId].update(auth.op, auth.path, auth.value);
        } else {
          return Promise.reject('Participant left');
        }
      })
    ).then(() => {
      callback('callback', participants[participantId].getDetail());
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  const doDropParticipant = (participantId) => {
    log.debug('doDropParticipant', participantId);
    if (participants[participantId] && participantId !== 'admin') {
      var deleted = participants[participantId].getInfo();
      return participants[participantId].drop()
        .then(function(result) {
          removeParticipant(participantId);
          return deleted;
        }).catch(function(reason) {
          log.warn('doDropParticipant fail:', reason);
          return Promise.reject('Drop participant failed');
        });
    } else {
      return Promise.reject('Participant does NOT exist');
    }
  };

  that.dropParticipant = function(participantId, callback) {
    log.debug('dropParticipant', participantId);
    return doDropParticipant(participantId)
      .then((dropped) => {
        callback('callback', dropped);
      }).catch((reason) => {
        callback('callback', 'error', reason.message ? reason.message : reason);
      });
  };

  that.getStreams = function(callback) {
    log.debug('getStreams, room_id:', room_id);
    var result = [];
    for (var stream_id in streams) {
      if (!streams[stream_id].isInConnecting) {
        result.push(streams[stream_id].toPortalFormat());
      }
    }
    callback('callback', result);
  };

  that.getStreamInfo = function(streamId, callback) {
    log.debug('getStreamInfo, room_id:', room_id, 'streamId:', streamId);
    if (streams[streamId] && !streams[stream_id].isInConnecting) {
      callback('callback', streams[streamId]);
    } else {
      callback('callback', 'error', 'Stream does NOT exist');
    }
  };

  that.addStreamingIn = function(roomId, pubInfo, callback) {
    log.debug('addStreamingIn, roomId:', roomId, 'pubInfo:', JSON.stringify(pubInfo));

    if (pubInfo.type === 'streaming') {
      var origin = { isp:'isp', region:'region'};
      if(pubInfo.connection.origin != null) {
          origin = pubInfo.connection.origin;
      }
      var stream_id = Math.round(Math.random() * 1000000000000000000) + '';
      return initRoom(roomId, origin)
      .then(() => {
        if (room_config.inputLimit >= 0 && (room_config.inputLimit <= currentInputCount())) {
          return Promise.reject('Too many inputs');
        }

        if (pubInfo.media.audio && !room_config.mediaIn.audio.length) {
          return Promise.reject('Audio is forbiden');
        }

        if (pubInfo.media.video && !room_config.mediaIn.video.length) {
          return Promise.reject('Video is forbiden');
        }

        initiateStream(stream_id, {owner: 'admin', type: pubInfo.type, origin: origin});
        return accessController.initiate('admin', stream_id, 'in', origin, pubInfo);
      }).then((result) => {
        return 'ok';
      }).then(() => {
        return new Promise((resolve, reject) => {
          var count = 0, wait = 1420;
          var interval = setInterval(() => {
            if (count > wait || !streams[stream_id]) {
              clearInterval(interval);
              accessController.terminate('admin', stream_id, 'Participant terminate');
              removeStream(stream_id);
              reject('Access timeout');
            } else {
              if (streams[stream_id] && !streams[stream_id].isInConnecting) {
                clearInterval(interval);
                resolve('ok');
              } else {
                count = count + 1;
              }
            }
          }, 60);
        });
      }).then(() => {
        callback('callback', streams[stream_id].toPortalFormat());
      }).catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
        removeStream(stream_id);
        selfClean();
      });
    } else {
      callback('callback', 'error', 'Invalid publication type');
    }
  };

  const getRational = (str) => {
    if (str === '0') {
      return {numerator: 0, denominator: 1};
    } else if (str === '1') {
      return {numerator: 1, denominator: 1};
    } else {
      var s = str.split('/');
      if ((s.length === 2) && !isNaN(s[0]) && !isNaN(s[1]) && Number(s[0]) <= Number(s[1])) {
        return {numerator: Number(s[0]), denominator: Number(s[1])};
      } else {
        return null;
      }
    }
  };

  const getRegionObj = (region) => {
    if ((typeof region.id !== 'string' || region.id === '')
        || (region.shape !== 'rectangle')
        || (typeof region.area !== 'object')) {
          return null;
    }

    var left = getRational(region.area.left),
      top = getRational(region.area.top),
      width = getRational(region.area.width),
      height = getRational(region.area.height);

    if (left && top && width && height) {
      return {
        id: region.id,
        shape: region.shape,
        area: {left: left, top: top, width: width, height: height}
      };
    } else {
      return null;
    }
  };

  that.controlStream = function(streamId, commands, callback) {
    log.debug('controlStream', streamId, 'commands:', commands);
    if (streams[streamId] === undefined) {
      callback('callback', 'error', 'Stream does NOT exist');
    }

    var muteReq = [];
    return Promise.all(
      commands.map((cmd) => {
        if (streams[streamId] === undefined) {
          return Promise.reject('Stream does NOT exist');
        }

        var exe;
        switch (cmd.op) {
          case 'add':
            if (cmd.path === '/info/inViews') {
              exe = mix(streamId, cmd.value);
            } else {
              exe = Promise.reject('Invalid path');
            }
            break;
          case 'remove':
            if (cmd.path === '/info/inViews') {
              exe = unmix(streamId, cmd.value);
            } else {
              exe = Promise.reject('Invalid path');
            }
            break;
          case 'replace':
            if ((cmd.path === '/media/audio/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              const track = getStreamTrack(streamId, 'audio');
              if (track) {
                if (track.status !== cmd.value) {
                  muteReq.push({track: 'audio', mute: (cmd.value === 'inactive')});
                }
                exe = Promise.resolve('ok');
              } else {
                exe = Promise.reject('Track does NOT exist');
              }
            } else if ((cmd.path === '/media/video/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              const track = getStreamTrack(streamId, 'video');
              if (track) {
                if (track.status !== cmd.value) {
                  muteReq.push({track: 'video', mute: (cmd.value === 'inactive')});
                }
                exe = Promise.resolve('ok');
              } else {
                exe = Promise.reject('Track does NOT exist');
              }
            } else if (cmd.path === '/info/layout') {
              if (streams[streamId].type === 'mixed') {
                if (cmd.value instanceof Array) {
                  var first_absence = 65535;//FIXME: stream id hole is not allowed
                  var stream_id_hole = false;//FIXME: stream id hole is not allowed
                  var stream_id_dup = false;
                  var stream_ok = true;
                  var region_ok = true;
                  for (var i in cmd.value) {
                    if (first_absence === 65535 && !cmd.value[i].stream) {
                      first_absence = i;
                    }

                    if (cmd.value[i].stream && first_absence < i) {
                      stream_id_hole = true;
                      break;
                    }

                    for (var j = 0; j < i; j++) {
                      if (cmd.value[j].stream && cmd.value[j].stream === cmd.value[i].stream) {
                        stream_id_dup = true;
                      }
                    }

                    if (cmd.value[i].stream && (!streams[cmd.value[i].stream] || (streams[cmd.value[i].stream].type !== 'forward'))) {
                      stream_ok = false;
                      break;
                    }

                    var region_obj = getRegionObj(cmd.value[i].region);
                    if (!region_obj) {
                      region_ok = false;
                      break;
                    } else {
                      for (var j = 0; j < i; j++) {
                        if (cmd.value[j].region.id === region_obj.id) {
                          region_ok = false;
                          break;
                        }
                      }

                      if (region_ok) {
                        cmd.value[i].region = region_obj;
                      } else {
                        break;
                      }
                    }
                  }

                  if (stream_id_hole) {
                    exe = Promise.reject('Stream ID hole is not allowed');
                  } else if (stream_id_dup) {
                    exe = Promise.reject('Stream ID duplicates');
                  } else if (!stream_ok) {
                    exe = Promise.reject('Invalid input stream id');
                  } else if (!region_ok) {
                    exe = Promise.reject('Invalid region');
                  } else {
                    exe = setLayout(streamId, cmd.value);
                  }
                } else {
                  exe = Promise.reject('Invalid value');
                }
              } else {
                exe = Promise.reject('Not mixed stream');
              }
            } else if ((cmd.path.startsWith('/info/layout/') && streams[cmd.value] && (streams[cmd.value].type !== 'mixed'))) {
              var path = cmd.path.split('/');
              var layout = streams[streamId].info.layout;
              if (layout && layout[Number(path[3])]) {
                exe = setRegion(cmd.value, layout[Number(path[3])].region.id, streams[streamId].info.label);
              } else {
                exe = Promise.reject('Not mixed stream or invalid region');
              }
            } else {
              exe = Promise.reject('Invalid path or value');
            }
            break;
          default:
            exe = Promise.reject('Invalid stream control operation');
        }
        return exe;
      })
    ).then(() => {
      return Promise.all(muteReq.map((r) => {return setStreamMute(streamId, r.track, r.mute);}));
    }).then(() => {
      callback('callback', streams[streamId].toPortalFormat());
    }, (err) => {
      log.warn('failed in controlStream, reason:', err.message ? err.message : err);
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.deleteStream = function(streamId, callback) {
    log.debug('deleteStream, streamId:', streamId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (!streams[streamId]) {
      return callback('callback', 'error', 'Stream does NOT exist');
    }

    return doUnpublish(streamId)
      .then((result) => {
        callback('callback', result);
        selfClean();
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  const subscriptionAbstract = (subId) => {
    var result = {id: subId, media: {}};
    if (subscriptions[subId].info.type === 'streaming') {
      result.url = subscriptions[subId].info.url;
    } else if (subscriptions[subId].info.type === 'recording') {
      result.storage = subscriptions[subId].info.location;
    } else if (subscriptions[subId].info.type === 'analytics') {
      result.analytics = subscriptions[subId].info.analytics;
    }
    subscriptions[subId].media.tracks.forEach(t => {
      result.media[t.type] = {
        format: t.format,
        parameters: t.parameters,
        from: t.from,
        status: t.status
      };
    });
    return result;
  };

  that.getSubscriptions = function(type, callback) {
    log.debug('getSubscriptions, room_id:', room_id, 'type:', type);
    var result = [];
    for (var sub_id in subscriptions) {
      if (subscriptions[sub_id].info.type === type && !subscriptions[sub_id].isInConnecting) {
        result.push(subscriptionAbstract(sub_id));
      }
    }
    callback('callback', result);
  };

  that.getSubscriptionInfo = function(subId, callback) {
    log.debug('getSubscriptionInfo, room_id:', room_id, 'subId:', subId);
    if (subscriptions[subId] && !subscriptions[sub_id].isInConnecting) {
      callback('callback', subscriptionAbstract(subId));
    } else {
      callback('callback', 'error', 'Stream does NOT exist');
    }
  };

  that.addServerSideSubscription = function(roomId, subDesc, callback) {
    log.debug('addServerSideSubscription, roomId:', roomId, 'subDesc:', JSON.stringify(subDesc));

    if (subDesc.type === 'streaming' || subDesc.type === 'recording' || subDesc.type === 'analytics') {
      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      var origin = {isp: 'isp', region: 'region'};
      return initRoom(roomId, origin)
      .then(() => {
        if (subDesc.media.audio && !room_config.mediaOut.audio.length) {
          return Promise.reject('Audio is forbiden');
        }

        if (subDesc.media.video && !room_config.mediaOut.video.format.length) {
          return Promise.reject('Video is forbiden');
        }

        var requestError = { message: '' };
        if (subDesc.media.audio && !validateAudioRequest(subDesc.type, subDesc.media.audio, requestError)) {
          return Promise.reject('Target audio stream does NOT satisfy:' + requestError.message);
        }

        if (subDesc.media.video && !validateVideoRequest(subDesc.type, subDesc.media.video, requestError)) {
          return Promise.reject('Target video stream does NOT satisfy:' + requestError.message);
        }

        if (subDesc.type === 'recording') {
          var audio_codec = 'none-aac', video_codec;
          if (subDesc.media.audio) {
            if (subDesc.media.audio.format) {
              audio_codec = subDesc.media.audio.format.codec;
            } else {
              let track = getStreamTrack(subDesc.media.audio.from, 'audio');
              audio_codec = track.format.codec;
            }

            //FIXME: To support codecs other than those in the following list.
            if ((audio_codec !== 'pcmu') && (audio_codec !== 'pcma') && (audio_codec !== 'opus') && (audio_codec !== 'aac')) {
              return Promise.reject('Audio codec invalid');
            }
          }

          if (subDesc.media.video) {
            let track = getStreamTrack(subDesc.media.video.from, 'video');
            video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || track.format.codec);
          }

          if (!subDesc.connection.container || subDesc.connection.container === 'auto') {
            subDesc.connection.container = ((audio_codec === 'aac' && (!video_codec || (video_codec === 'h264') || (video_codec === 'h265'))) ? 'mp4' : 'mkv');
          }
        }

        if (subDesc.type === 'streaming') {
          if (subDesc.media.audio && !subDesc.media.audio.format) {
            var aacFmt = room_config.mediaOut.audio.find((a) => a.codec === 'aac');
            if (!aacFmt) {
              return Promise.reject('Audio codec aac not enabled');
            }
            subDesc.media.audio.format = aacFmt;
          }

          //FIXME: To support codecs other than those in the following list.
          if (subDesc.media.audio && (subDesc.media.audio.format.codec !== 'aac')) {
            return Promise.reject('Audio codec invalid');
          }

          if (subDesc.media.video && !subDesc.media.video.format) {
            subDesc.media.video.format = {codec: 'h264', profile: 'CB'};
          }

          //FIXME: To support codecs other than those in the following list.
          if (subDesc.media.video && (subDesc.media.video.format.codec !== 'h264')) {
            return Promise.reject('Video codec invalid');
          }
        }

        if (subDesc.type === 'analytics') {
          if (subDesc.media.audio) {
            // We don't analyze audio so far
            delete subDesc.media.audio;
          }
          if (!subDesc.media.video || !subDesc.media.video.from) {
            return Promise.reject('Video source not specified for analyzing');
          }
          if (!streams[subDesc.media.video.from]) {
            return Promise.reject('Video source not valid for analyzing');
          }

          let sourceVideoOption = getStreamTrack(subDesc.media.video.from, 'video');
          if (!subDesc.media.video.format) {
            // Subscribe source format
            subDesc.media.video.format = sourceVideoOption.format;
          }
          if (subDesc.media.video.parameters || sourceVideoOption.parameters) {
            subDesc.media.video.parameters = Object.assign(
              {}, sourceVideoOption.parameters || {},
              subDesc.media.video.parameters || {});
          }
          // Put output settings in connection.video to avoid external transcoding
          subDesc.connection.video = {
            format: subDesc.media.video.format,
            parameters: subDesc.media.video.parameters
          };
          subDesc.media.video.format = sourceVideoOption.format;
          delete subDesc.media.video.parameters;
        }

        // Schedule preference for worker node
        var streamFrom = undefined;
        if(subDesc.media.video && subDesc.media.video.from) {
          streamFrom = subDesc.media.video.from;
        } else if(subDesc.media.audio && subDesc.media.audio.from) {
          streamFrom = subDesc.media.audio.from;
        } else {
          return Promise.reject('No video or audio source to process');
        }

        var accessPreference = Object.assign({}, streams[streamFrom].info.origin);
        if (subDesc.type === 'analytics') {
          // Schedule analytics agent according to the algorithm
          accessPreference.algorithm = subDesc.connection.algorithm;
        }

        initiateSubscription(subscription_id, subDesc, {owner: 'admin', type: subDesc.type});
        return accessController.initiate('admin', subscription_id, 'out', accessPreference, subDesc);
      }).then(() => {
        if ((subDesc.media.audio && !streams[subDesc.media.audio.from])
           ||(subDesc.media.video && !streams[subDesc.media.video.from])) {
          accessController.terminate(participantId, subscriptionId, 'Participant terminate');
          return Promise.reject('Target audio/video stream early released');
        }
        return new Promise((resolve, reject) => {
          var count = 0, wait = 300;
          var interval = setInterval(() => {
            if (count > wait || !subscriptions[subscription_id]) {
              clearInterval(interval);
              accessController.terminate('admin', subscription_id, 'Participant terminate');
              removeSubscription(subscription_id);
              reject('Access timeout');
            } else {
              if (subscriptions[subscription_id] && !subscriptions[subscription_id].isInConnecting) {
                clearInterval(interval);
                resolve('ok');
              } else {
                count = count + 1;
              }
            }
          }, 60);
        });
      }).then(() => {
        callback('callback', subscriptionAbstract(subscription_id));
      }).catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
        removeSubscription(subscription_id);
        selfClean();
      });
    } else {
      callback('callback', 'error', 'Invalid subscription type');
    }
  };

  var doControlSubscription = function(subId, commands) {
    var subUpdate;
    var muteReqs = [];
    for (const cmd of commands) {
      switch (cmd.op) {
        case 'replace':
          if ((cmd.path === '/media/audio/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
            muteReqs.push({track: 'audio', mute: (cmd.value === 'inactive')});
          } else if ((cmd.path === '/media/video/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
            muteReqs.push({track: 'video', mute: (cmd.value === 'inactive')});
          } else if ((cmd.path === '/media/audio/from') && streams[cmd.value]) {
            subUpdate = (subUpdate || {});
            subUpdate.audio = (subUpdate.audio || {});
            subUpdate.audio.from = cmd.value
          } else if ((cmd.path === '/media/video/from') && streams[cmd.value]) {
            subUpdate = (subUpdate || {});
            subUpdate.video = (subUpdate.video || {});
            subUpdate.video.from = cmd.value;
          } else if (cmd.path === '/media/video/parameters/resolution') {
            subUpdate = (subUpdate || {});
            subUpdate.video = (subUpdate.video || {});
            subUpdate.video.parameters = (subUpdate.video.parameters || {});
            subUpdate.video.parameters.resolution = cmd.value;
            exe = Promise.resolve('ok');
          } else if (cmd.path === '/media/video/parameters/framerate') {
            subUpdate = (subUpdate || {});
            subUpdate.video = (subUpdate.video || {});
            subUpdate.video.parameters = (subUpdate.video.parameters || {});
            subUpdate.video.parameters.framerate = Number(cmd.value);
            exe = Promise.resolve('ok');
          } else if (cmd.path === '/media/video/parameters/bitrate') {
            subUpdate = (subUpdate || {});
            subUpdate.video = (subUpdate.video || {});
            subUpdate.video.parameters = (subUpdate.video.parameters || {});
            subUpdate.video.parameters.bitrate = cmd.value;
          } else if (cmd.path === '/media/video/parameters/keyFrameInterval') {
            subUpdate = (subUpdate || {});
            subUpdate.video = (subUpdate.video || {});
            subUpdate.video.parameters = (subUpdate.video.parameters || {});
            subUpdate.video.parameters.keyFrameInterval = cmd.value;
          } else {
            return Promise.reject('Invalid path or value');
          }
        default:
          return Promise.reject('Invalid subscription control operation');
      }
    }

    return Promise.all(muteReqs.map((r) => {
        return setSubscriptionMute(subId, r.track, r.mute);
    })).then(() => {
      if (subUpdate) {
        return updateSubscription(subId, subUpdate);
      } else {
        return 'ok';
      }
    });
  };

  that.controlSubscription = function(subId, commands, callback) {
    log.debug('controlSubscription', subId, 'commands:', commands);
    if (subscriptions[subId] === undefined) {
      callback('callback', 'error', 'Subscription does NOT exist');
    }

    return doControlSubscription(subId, commands)
    .then(() => {
      if (subscriptions[subId]) {
        callback('callback', subscriptionAbstract(subId));
      } else {
        callback('callback', 'error', 'Subscription does NOT exist');
      }
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.deleteSubscription = function(subId, type, callback) {
    log.debug('deleteSubscription, subId:', subId, type);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (subscriptions[subId] && subscriptions[subId].info.type !== type) {
      return callback('callback', 'error', 'Delete type not match');
    }

    return doUnsubscribe(subId)
      .then((result) => {
        callback('callback', result);
        selfClean();
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  var getSipCallInfo = function(sipCallId) {
      var pInfo = participants[sipCallId].getInfo();
      var sipcall = {id: pInfo.id, peer: pInfo.user}
      sipcall.type = (pInfo.id.startsWith('SipIn') ? 'dial-in' : 'dial-out');

      for (var stream_id in streams) {
        if (streams[stream_id].info.owner === pInfo.id) {
          sipcall.input = streams[stream_id];
          break;
        }
      }

      for (var subscription_id in subscriptions) {
        if (subscriptions[subscription_id].info.owner === pInfo.id) {
          sipcall.output = subscriptionAbstract(subscription_id);
          break;
        }
      }

      return sipcall;
  };

  that.getSipCalls = function(callback) {
    var result = [];
    log.debug('getSipCalls: ' , participants );
    for (var pid in participants) {
      if (participants[pid].getInfo().role === 'sip') {
        result.push(getSipCallInfo(pid));
      }
    }
    callback('callback', result);
  };

  that.getSipCall = function(participantId, callback) {
    if (participants[participantId]) {
      if (participants[participantId].getInfo().role === 'sip') {
        callback('callback', getSipCallInfo(participantId));
      } else {
        callback('callback', 'error', 'Not a sip call');
      }
    } else {
      callback('callback', 'error', 'Sip call does NOT exist');
    }
  };

  that.makeSipCall = function(roomId, options, callback) {
    return rpcReq.getSipConnectivity('sip-portal', roomId)
      .then((sipNode) => {
        return rpcReq.makeSipCall(sipNode, options.peerURI, options.mediaIn, options.mediaOut, selfRpcId);
      }).then((sipCallId) => {
        return new Promise((resolve, reject) => {
          var count = 0, wait = 880;
          var interval = setInterval(() => {
            if (count > wait || !participants[sipCallId]) {
              clearInterval(interval);
              rpcReq.endSipCall(sipCallId);
              reject('No answer timeout');
            } else {
              if (participants[sipCallId]) {
                clearInterval(interval);
                resolve(sipCallId);
              } else {
                count = count + 1;
              }
            }
          }, 100);
        });
      }).then((sipCallId) => {
        callback('callback', getSipCallInfo(sipCallId));
      }).catch((err) => {
        var reason = (err.message || err);
        log.error('makeSipCall failed, reason:', reason);
        callback('callback', 'error', reason);
      });
  };

  that.controlSipCall = function(sipCallId, cmds, callback) {
    log.debug('controlSipCall, sipCallId:', sipCallId, 'cmds:', JSON.stringify(cmds));
    if (participants[sipCallId] && participants[sipCallId].getInfo().role === 'sip') {
      var subscription_id;
      for (var sub_id in subscriptions) {
        if (subscriptions[sub_id].info.owner === sipCallId) {
          subscription_id = sub_id;
          break;
        }
      }

      if (!subscription_id) {
        return callback('callback', 'error', 'Sip call has no output');
      }

      cmds = cmds.map((cmd) => {cmd.path = cmd.path.replace(/^(\/output)/,""); return cmd;});
      return doControlSubscription(subscription_id, cmds)
        .then(() => {
          callback('callback', getSipCallInfo(sipCallId));
        }).catch((err) => {
          var reason = (err.message || err);
          log.error('controlSipCall failed, reason:', reason);
          callback('callback', 'error', reason);
        });
    } else {
      callback('callback', 'error', 'Sip call does NOT exist');
    }

  };

  that.endSipCall = function(sipCallId, callback) {
    log.debug('endSipCall, sipCallId:', sipCallId);
    if (participants[sipCallId] && participants[sipCallId].getInfo().role === 'sip') {
      rpcReq.endSipCall(participants[sipCallId].getPortal(), sipCallId);
      removeParticipant(sipCallId);
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Sip call does NOT exist');
    }
  };

  that.drawText = function(streamId, textSpec, duration, callback) {
    if (roomController) {
      roomController.drawText(streamId, textSpec, duration);
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Controllers are not ready');
    }
  };

  that.destroy = function(callback) {
    log.info('Destroy room:', room_id);
    destroyRoom();
    callback('callback', 'Success');
  };

  //This interface is for fault tolerance.
  that.onFaultDetected = function (message) {
    if (message.purpose === 'portal' || message.purpose === 'sip') {
      dropParticipants(message.id);
    } else if (message.purpose === 'webrtc') {
      rtcController && rtcController.terminateByLocality(message.type, message.id);
    } else if (message.purpose === 'recording' ||
               message.purpose === 'streaming' ||
               message.purpose === 'analytics') {
      accessController && accessController.onFaultDetected(message.type, message.id);
    } else if (message.purpose === 'audio' ||
               message.purpose === 'video') {
      roomController && roomController.onFaultDetected(message.purpose, message.type, message.id);
    }
  };

  return that;
};


module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
  var that = {
    agentID: parentRpcId,
    clusterIP: clusterWorkerIP
  };

  var conference = Conference(rpcClient, selfRpcId);

  that.rpcAPI = {
    // rpc from Portal and sip-node.
    join: conference.join,
    leave: conference.leave,
    text: conference.text,
    publish: conference.publish,
    unpublish: conference.unpublish,
    streamControl: conference.streamControl,
    subscribe: conference.subscribe,
    unsubscribe: conference.unsubscribe,
    subscriptionControl: conference.subscriptionControl,
    onSessionSignaling: conference.onSessionSignaling,

    //rpc from access nodes.
    onSessionProgress: conference.onSessionProgress,
    onTransportProgress: conference.onTransportProgress,
    onMediaUpdate: conference.onMediaUpdate,
    onTrackUpdate: conference.onTrackUpdate,
    // onSessionAudit: conference.onSessionAudit,

    // rpc from audio nodes.
    onAudioActiveness: conference.onAudioActiveness,

    // rpc from video nodes.
    onVideoLayoutChange: conference.onVideoLayoutChange,

    //rpc from OAM component.
    getParticipants: conference.getParticipants,
    controlParticipant: conference.controlParticipant,
    dropParticipant: conference.dropParticipant,
    getStreams: conference.getStreams,
    getStreamInfo: conference.getStreamInfo,
    addStreamingIn: conference.addStreamingIn,
    controlStream: conference.controlStream,
    deleteStream: conference.deleteStream,
    getSubscriptions: conference.getSubscriptions,
    getSubscriptionInfo: conference.getSubscriptionInfo,
    addServerSideSubscription: conference.addServerSideSubscription,
    controlSubscription: conference.controlSubscription,
    deleteSubscription: conference.deleteSubscription,
    getSipCalls: conference.getSipCalls,
    getSipCall: conference.getSipCall,
    makeSipCall: conference.makeSipCall,
    controlSipCall: conference.controlSipCall,
    endSipCall: conference.endSipCall,
    drawText: conference.drawText,
    destroy: conference.destroy
  };

  that.onFaultDetected = conference.onFaultDetected;

  return that;
};

