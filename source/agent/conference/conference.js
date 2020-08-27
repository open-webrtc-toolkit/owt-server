// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;

var AccessController = require('./accessController');
var RoomController = require('./roomController');
var dataAccess = require('./data_access');
var Participant = require('./participant');
//var Stream = require('./stream');

// Logger
var log = logger.getLogger('Conference');

const isAudioFmtCompatible = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec) && (fmt1.sampleRate === fmt2.sampleRate) && (fmt1.channelNum === fmt2.channelNum);
};

const isVideoFmtCompatible = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec && (!fmt1.profile || !fmt2.profile || fmt1.profile === fmt2.profile));
};

var calcResolution = (x, baseResolution) => {
  var floatToSize = (n) => {
    var x = Math.floor(n);
    return (x % 2 === 0) ? x : (x - 1);
  };

  switch (x) {
    case 'x3/4':
      return {width: floatToSize(baseResolution.width * 3 / 4), height: floatToSize(baseResolution.height * 3 / 4)};
    case 'x2/3':
      return {width: floatToSize(baseResolution.width * 2 / 3), height: floatToSize(baseResolution.height * 2 / 3)};
    case 'x1/2':
      return {width: floatToSize(baseResolution.width / 2), height: floatToSize(baseResolution.height / 2)};
    case 'x1/3':
      return {width: floatToSize(baseResolution.width / 3), height: floatToSize(baseResolution.height / 3)};
    case 'x1/4':
      return {width: floatToSize(baseResolution.width / 4), height: floatToSize(baseResolution.height / 4)};
    case 'xga':
      return {width: 1024, height: 768};
    case 'svga':
      return {width: 800, height: 600};
    case 'vga':
      return {width: 640, height: 480};
    case 'hvga':
      return {width: 480, height: 320};
    case 'cif':
      return {width: 352, height: 288};
    case 'qvga':
      return {width: 320, height: 240};
    case 'qcif':
      return {width: 176, height: 144};
    case 'hd720p':
      return {width: 1280, height: 720};
    case 'hd1080p':
      return {width: 1920, height: 1080}
    default:
      return {width: 65536, height: 65536};
  }
};

var calcBitrate = (x, baseBitrate) => {
  return Number(x.substring(1)) * baseBitrate;
};

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
   * {
   *   StreamId: {
   *     id: string(StreamId),
   *     type: 'forward' | 'mixed',
   *     media: {
   *       audio: {
   *         status: 'active' | 'inactive' | undefined,
   *         source: 'mic' | 'screen-cast' | 'raw-file' | 'encoded-file' | 'streaming',
   *         format: object(AudioFormat),
   *         optional: {
   *           format: [object(AudioFormat)] | undefined
   *         } | undefined
   *       } | undefined,
   *       video: {
   *         status: 'active' | 'inactive' | undefined,
   *         source: 'camera' | screen-cast' | 'raw-file' | 'encoded-file' | 'streaming',
   *         format: object(VideoFormat),
   *         parameters: {
   *           resolution: object(Resolution) | undefined,
   *           framerate: number(FramerateFPS) | undefined,
   *           bitrate: number(Kbps) | undefined,
   *           keyFrameInterval: number(Seconds) | undefined,
   *           } | undefined
   *         optional: {
   *           format: [object(VideoFormat)] | undefined,
   *           parameters: {
   *             resolution: [object(Resolution)] | undefined,
   *             framerate: [number(FramerateFPS)] | undefined,
   *             bitrate: [number(Kbps] | string('x' + Multiple) | undefined,
   *             keyFrameRate: [number(Seconds)] | undefined
   *           } | undefined
   *         } | undeinfed
   *       } | undefined
   *     },
   *     info: object(PublicationInfo):: {
   *       owner: string(ParticipantId),
   *       type: 'webrtc' | 'streaming' | 'sip',
   *       inViews: [string(ViewLabel)],
   *       attributes: object(ExternalDefinedObj)
   *     } | object(ViewInfo):: {
   *       label: string(ViewLabel),
   *       layout: [{
   *         stream: string(StreamId),
   *         region: object(Region)
   *       }]
   *     }
   *   }
   * }
   */
  var streams = {};

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
          sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready'});
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
          if (sessionInfo.info.location) {
            sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready', data: sessionInfo.info.location});
          } else {
            sendMsgTo(participantId, 'progress', {id: sessionId, status: 'ready'});
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
      sendMsgTo(participantId, 'progress', {id: sessionId, status: 'error', data: reason});
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

  var onLocalSessionSignaling = (participantId, sessionId, signaling) => {
    log.debug('onLocalSessionSignaling, participantId:', participantId, 'sessionId:', sessionId, 'signaling:', signaling);
    if (participants[participantId]) {
      sendMsgTo(participantId, 'progress', {id: sessionId, status: 'soac', data: signaling});
    }
  };

  var initRoom = function(roomId) {
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

            return new Promise(function(resolve, reject) {
              RoomController.create(
                {
                  cluster: global.config.cluster.name || 'owt-cluster',
                  rpcReq: rpcReq,
                  rpcClient: rpcClient,
                  room: roomId,
                  config: room_config,
                  selfRpcId: selfRpcId
                },
                function onOk(rmController) {
                  log.debug('room controller init ok');
                  roomController = rmController;
                  room_id = roomId;
                  is_initializing = false;

                  room_config.views.forEach((viewSettings) => {
                    var mixed_stream_id = room_id + '-' + viewSettings.label;
                    var av_capability = roomController.getViewCapability(viewSettings.label);

                    if (!av_capability) {
                      log.error('No audio/video capability for view: ' + viewSettings.label);
                      return;
                    }


                    var default_audio_fmt = viewSettings.audio.format;
                    var default_video_fmt = viewSettings.audio.format;

                    //FIXME: Validation defaultFormat/mediaOut against av_capability here is not complete.
                    if (viewSettings.audio) {
                      if (viewSettings.audio.format && (av_capability.audio.findIndex((f) => isAudioFmtCompatible(viewSettings.audio.format, f)) >= 0)) {
                        default_audio_fmt = viewSettings.audio.format;
                      } else {
                        for (var i in room_config.mediaOut.audio) {
                          var fmt = room_config.mediaOut.audio[i];
                          if (av_capability.audio.findIndex((f) => {return isAudioFmtCompatible(fmt,f);}) >= 0) {
                            default_audio_fmt = fmt;
                            break;
                          }
                        }
                      }

                      if (!default_audio_fmt) {
                        log.error('No capable audio format for view: ' + viewSettings.label);
                        return;
                      }
                    }

                    if (viewSettings.video) {
                      if (viewSettings.video.format && (av_capability.video.encode.findIndex((f) => {return isVideoFmtCompatible(viewSettings.video.format, f);}) >= 0)) {
                        default_video_fmt = viewSettings.video.format;
                      } else {
                        for (var i in room_config.mediaOut.video.format) {
                          var fmt = room_config.mediaOut.video.format[i];
                          if (av_capability.video.encode.findIndex((f) => {return isVideoFmtCompatible(fmt,f);}) >= 0) {
                            default_video_fmt = fmt;
                            break;
                          }
                        }
                      }

                      if (!default_video_fmt) {
                        log.error('No capable video format for view: ' + viewSettings.label);
                        return;
                      }
                    }

                    var mixed_stream_info = {
                      id: mixed_stream_id,
                      type: 'mixed',
                      media: {
                        audio: (viewSettings.audio) ? {
                          format: default_audio_fmt,
                          optional: {
                            format: room_config.mediaOut.audio.filter((fmt) => {
                              return (av_capability.audio.findIndex((f) => isAudioFmtCompatible(f, fmt)) >= 0
                                && !isAudioFmtCompatible(fmt, default_audio_fmt));
                            })
                          }
                        } : undefined,
                        video: (viewSettings.video) ? {
                          format: default_video_fmt,
                          optional: {
                            format: room_config.mediaOut.video.format.filter((fmt) => {
                              return (av_capability.video.encode.findIndex((f) => isVideoFmtCompatible(f, fmt)) >= 0
                                && !isVideoFmtCompatible(fmt, default_video_fmt));
                            }),
                            parameters: {
                              resolution: room_config.mediaOut.video.parameters.resolution.map((x) => {return calcResolution(x, viewSettings.video.parameters.resolution)}).filter((reso, pos, self) => {return ((reso.width < viewSettings.video.parameters.resolution.width) && (reso.height < viewSettings.video.parameters.resolution.height)) && (self.findIndex((r) => {return r.width === reso.width && r.height === reso.height;}) === pos);}),
                              framerate: room_config.mediaOut.video.parameters.framerate.filter((x) => {return x < viewSettings.video.parameters.framerate;}),
                              bitrate: room_config.mediaOut.video.parameters.bitrate,//.map((x) => {return calcBitrate(x, viewSettings.video.parameters.bitrate)}),
                              keyFrameInterval: room_config.mediaOut.video.parameters.keyFrameInterval.filter((x) => {return x < viewSettings.video.parameters.keyFrameInterval;})
                            }
                          },
                          parameters: {
                            resolution: viewSettings.video.parameters.resolution,
                            framerate: viewSettings.video.parameters.framerate,
                            bitrate: viewSettings.video.parameters.bitrate,
                            keyFrameInterval: viewSettings.video.parameters.keyFrameInterval,
                          }
                        } : undefined
                      },
                      info: {
                        label: viewSettings.label,
                        activeInput: 'unknown',
                        layout: []
                      }
                    };

                    streams[mixed_stream_id] = mixed_stream_info;
                    log.debug('Mixed stream info:', mixed_stream_info);
                    room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: mixed_stream_id, status: 'add', data: mixed_stream_info});
                  });

                  participants['admin'] = Participant({
                                                       id: 'admin',
                                                       user: 'admin',
                                                       role: 'admin',
                                                       portal: undefined,
                                                       origin: {isp: 'isp', region: 'region'},//FIXME: hard coded.
                                                       permission: {
                                                        subscribe: {audio: true, video: true},
                                                        publish: {audio: true, video: true}
                                                       }
                                                      }, rpcReq);

                  accessController = AccessController.create({clusterName: global.config.cluster.name || 'owt-cluster',
                                                              selfRpcId: selfRpcId,
                                                              inRoom: room_id,
                                                              mediaIn: room_config.mediaIn,
                                                              mediaOut: room_config.mediaOut},
                                                              rpcReq,
                                                              onSessionEstablished,
                                                              onSessionAborted,
                                                              onLocalSessionSignaling);
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
    var exit_delay = 0;
    if (room_id) {
      if (Object.keys(streams).length > 0 || Object.keys(subscriptions).length > 0) {
        exit_delay = 1000;
      }

      for (var pid in participants) {
        if (pid !== 'admin') {
          accessController && accessController.participantLeave(pid);
          removeParticipant(pid);
        }
      }
      accessController && accessController.participantLeave('admin');
      removeParticipant('admin');

      accessController && accessController.destroy();
      accessController = undefined;
      roomController && roomController.destroy();
      roomController = undefined;
      subscriptions = {};
      streams = {};
      participants = {};
      room_id = undefined;
    }
    setTimeout(() => {process.exit();}, exit_delay);
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

  const constructMediaInfo = (media) => {
    var result = {};

    if (media.audio) {
      result.audio = {
        status: 'active',
        format: extractAudioFormat(media.audio)
      };

      media.audio.source && (result.audio.source = media.audio.source);

      if (room_config.transcoding.audio) {
        result.audio.optional = {format: []};
        room_config.mediaOut.audio.forEach((fmt) => {
          if ((fmt.codec !== result.audio.format.codec)
              || (fmt.sampleRate !== result.audio.format.sampleRate)
              || (fmt.channelNum !== result.audio.format.channelNum)) {
            result.audio.optional.format.push(fmt);
          }
        });
      }
    }

    if (media.video) {
      result.video = {
        status: 'active',
        format: extractVideoFormat(media.video)
      };

      //FIXME: sometimes, streaming send invalid resolution { width: 0, height: 0}
      if (media.video.resolution && (!media.video.resolution.height && !media.video.resolution.width)) {
        delete media.video.resolution;
      }

      media.video.source && (result.video.source = media.video.source);
      media.video.resolution && (media.video.resolution.width !== 0) && (media.video.resolution.height !== 0) && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.resolution = media.video.resolution);
      media.video.framerate && (media.video.framerate !== 0) && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.framerate = Math.floor(media.video.framerate));
      media.video.bitrate && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.bitrate = media.video.bitrate);
      media.video.keyFrameInterval && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.keyFrameInterval = media.video.keyFrameInterval);

      if (room_config.transcoding.video && room_config.transcoding.video.format) {
        result.video.optional = {format: []};
        room_config.mediaOut.video.format.forEach((fmt) => {
          if ((fmt.codec !== result.video.format.codec)
              || (fmt.profile !== result.video.format.profile)) {
            result.video.optional.format.push(fmt);
          }
        });
      }

      if (room_config.transcoding.video && room_config.transcoding.video.parameters) {
        if (room_config.transcoding.video.parameters.resolution) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          if (result.video.parameters && result.video.parameters.resolution) {
            result.video.optional.parameters.resolution = room_config.mediaOut.video.parameters.resolution.map((x) => {return calcResolution(x, result.video.parameters.resolution)}).filter((reso, pos, self) => {return ((reso.width < result.video.parameters.resolution.width) && (reso.height < result.video.parameters.resolution.height)) && (self.findIndex((r) => {return r.width === reso.width && r.height === reso.height;}) === pos);});
          } else {
            result.video.optional.parameters.resolution = room_config.mediaOut.video.parameters.resolution
              .filter((x) => {return (x !== 'x3/4') && (x !== 'x2/3') && (x !== 'x1/2') && (x !== 'x1/3') && (x !== 'x1/4');})//FIXME: is auto-scaling possible?
              .map((x) => {return calcResolution(x)});
          }
        }

        if (room_config.transcoding.video.parameters.framerate) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          if (result.video.parameters && result.video.parameters.framerate) {
            result.video.optional.parameters.framerate = room_config.mediaOut.video.parameters.framerate.filter((fr) => {return fr < result.video.parameters.framerate;});
          } else {
            result.video.optional.parameters.framerate = room_config.mediaOut.video.parameters.framerate;
          }
        }

        if (room_config.transcoding.video.parameters.bitrate) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          result.video.optional.parameters.bitrate = room_config.mediaOut.video.parameters.bitrate.filter((x) => {return Number(x.substring(1)) < 1;});
        }

        if (room_config.transcoding.video.parameters.keyFrameInterval) {
          result.video.optional = (result.video.optional || {});
          result.video.optional.parameters = (result.video.optional.parameters || {});

          result.video.optional.parameters.keyFrameInterval = room_config.mediaOut.video.parameters.keyFrameInterval;
        }
      }
    }

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

  const isAudioFmtAcceptable = (audioIn, audioInList) => {
    for (var i in audioInList) {
      if (isAudioFmtCompatible(audioIn, audioInList[i])) {
        return true;
      }
    }
    return false;
  };

  const isVideoFmtAcceptable = (videoIn, videoInList) => {
    for (var i in videoInList) {
      if (isVideoFmtCompatible(videoIn, videoInList[i])) {
        return true;
      }
    }
    return false;
  };

  const addStream = (id, locality, media, info) => {
    if (media.audio && (!room_config.mediaIn.audio.length || !isAudioFmtAcceptable(extractAudioFormat(media.audio), room_config.mediaIn.audio))) {
      return Promise.reject('Audio format unacceptable');
    }

    if (media.video && (!room_config.mediaIn.video.length || !isVideoFmtAcceptable(extractVideoFormat(media.video), room_config.mediaIn.video))) {
      return Promise.reject('Video format unacceptable');
    }

    return new Promise((resolve, reject) => {
      roomController && roomController.publish(info.owner, id, locality, media, info.type, function() {
        if (participants[info.owner]) {
          var st = {
            id: id,
            type: 'forward',
            media: constructMediaInfo(media),
            info: info
          };
          st.info.inViews = [];
          streams[id] = st;
          setTimeout(() => {
            room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: id, status: 'add', data: st});
          }, 10);
          resolve('ok');
        } else {
          roomController && roomController.unpublish(info.owner, id);
          reject('Participant early left');
        }
      }, function(reason) {
        reject('roomController.publish failed, reason: ' + (reason.message ? reason.message : reason));
      });
    });
  };

  const updateStreamInfo = (streamId, info) => {
    if (streams[streamId] && streams[streamId].type === 'forward') {
      if (info) {
        if (info.video && streams[streamId].media.video) {
          if (info.video.parameters) {
            if (info.video.parameters.resolution) {
              streams[streamId].media.video.parameters = (streams[streamId].media.video.parameters || {});
              if (!streams[streamId].media.video.parameters.resolution ||
                  (streams[streamId].media.video.parameters.resolution.width !== info.video.parameters.resolution.width) ||
                  (streams[streamId].media.video.parameters.resolution.height !== info.video.parameters.resolution.height)) {
                streams[streamId].media.video.parameters.resolution = info.video.parameters.resolution;
                if (room_config.transcoding.video && room_config.transcoding.video.parameters && room_config.transcoding.video.parameters.resolution) {
                  streams[streamId].media.video.optional = (streams[streamId].media.video.optional || {});
                  streams[streamId].media.video.optional.parameters = (streams[streamId].media.video.optional.parameters || {});

                  streams[streamId].media.video.optional.parameters.resolution = room_config.mediaOut.video.parameters.resolution.map((x) => {return calcResolution(x, streams[streamId].media.video.parameters.resolution)}).filter((reso, pos, self) => {return ((reso.width < streams[streamId].media.video.parameters.resolution.width) && (reso.height < streams[streamId].media.video.parameters.resolution.height)) && (self.findIndex((r) => {return r.width === reso.width && r.height === reso.height;}) === pos);});
                }
              }
            }
          }
          room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: streamId, status: 'update', data: {field: '.', value: streams[streamId]}});
        }
      }
    }
  };

  const removeStream = (streamId) => {
    return new Promise((resolve, reject) => {
      if (streams[streamId]) {
        for (var sub_id in subscriptions) {
          if ((subscriptions[sub_id].media.audio && (subscriptions[sub_id].media.audio.from === streamId))
            || (subscriptions[sub_id].media.video && (subscriptions[sub_id].media.video.from === streamId))) {
            accessController && accessController.terminate(sub_id, 'out', 'Source stream loss');
          }
        }

        if (!streams[streamId].isInConnecting) {
          roomController && roomController.unpublish(streams[streamId].info.owner, streamId);
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

    var media = JSON.parse(JSON.stringify(mediaSpec));
    if (media.video) {
      if (streams[media.video.from] === undefined) {
        return Promise.reject('Video source early released');
      }

      var source = streams[media.video.from].media.video;

      media.video.format = (media.video.format || source.format);
      mediaSpec.video.format = media.video.format;
      mediaSpec.video.status = (media.video.status || 'active');

      if (streams[media.video.from].type === 'mixed') {
        if (media.video.parameters && (Object.keys(media.video.parameters).length > 0)) {
          if (media.video.parameters.bitrate) {
            if (source.parameters.bitrate && typeof media.video.parameters.bitrate === 'string') {
              media.video.parameters.bitrate = source.parameters.bitrate * Number(media.video.parameters.bitrate.substring(1));
            }
          } else {
            if (!media.video.parameters.resolution && !media.video.parameters.framerate) {
              media.video.parameters.bitrate = ((source.parameters && source.parameters.bitrate) || 'unspecified');
            } else {
              if (source.parameters && source.parameters.bitrate) {
                var reso_f = (media.video.parameters.resolution ? (media.video.parameters.resolution.width * media.video.parameters.resolution.height) / (source.parameters.resolution.width * source.parameters.resolution.height) : 1.0);
                var fr_f = (media.video.parameters.framerate ? media.video.parameters.framerate / source.parameters.framerate : 1.0);
                media.video.parameters.bitrate = source.parameters.bitrate * reso_f * fr_f;
              } else {
                media.video.parameters.bitrate = 'unspecified';
              }
            }
          }
          media.video.parameters.bitrate = (media.video.parameters.bitrate || 'unspecified');
          media.video.parameters.resolution = (media.video.parameters.resolution || source.parameters.resolution);
          media.video.parameters.framerate = (media.video.parameters.framerate || source.parameters.framerate);
          media.video.parameters.keyFrameInterval = (media.video.parameters.keyFrameInterval || source.parameters.keyFrameInterval);
        } else {
          media.video.parameters = source.parameters;
        }
      } else {
        if (media.video.parameters && (Object.keys(media.video.parameters).length > 0)) {
          if (!media.video.parameters.resolution && !media.video.parameters.framerate) {
            if (media.video.parameters.bitrate) {
              (source.parameters && source.parameters.bitrate) && (media.video.parameters.bitrate = source.parameters.bitrate * Number(media.video.parameters.bitrate.substring(1)));
            } else {
              media.video.parameters.bitrate = 'unspecified';
            }
          } else {
            media.video.parameters.bitrate = (media.video.parameters.bitrate || (source.parameters && source.parameters.bitrate) || 'unspecified');
          }

          media.video.parameters.resolution = (media.video.parameters.resolution || (source.parameters && source.parameters.resolution) || 'unspecified');
          media.video.parameters.framerate = (media.video.parameters.framerate || (source.parameters && source.parameters.framerate) || 'unspecified');
          media.video.parameters.keyFrameInterval = (media.video.parameters.keyFrameInterval || (source.parameters && source.parameters.keyFrameInterval) || 'unspecified');
        } else {
          media.video.parameters = {
            resolution: 'unspecified',
            framerate: 'unspecified',
            bitrate: 'unspecified',
            keyFrameInterval: 'unspecified'
          };
        }
      }
    }

    if (media.audio) {
      if (streams[media.audio.from] === undefined) {
        return Promise.reject('Audio source early released');
      }

      var source = streams[media.audio.from].media.audio;

      media.audio.format = (media.audio.format || source.format);
      mediaSpec.audio.format = media.audio.format;
      mediaSpec.audio.status = (media.audio.status || 'active');
    }

    const isAudioPubPermitted = !!participants[info.owner].isPublishPermitted('audio'); 
    return new Promise((resolve, reject) => {
      roomController && roomController.subscribe(info.owner, id, locality, media, info.type, isAudioPubPermitted, function() {
        if (participants[info.owner]) {
          var subscription = {
            id: id,
            locality: locality,
            media: mediaSpec,
            info: info
          };
          subscriptions[id] = subscription;
          resolve('ok');
        } else {
          roomController && roomController.unsubscribe(info.owner, id);
          reject('Participant early left');
        }
      }, function(reason) {
        log.info('roomController.subscribe failed, reason: ' + (reason.message ? reason.message : reason));
        reject('roomController.subscribe failed, reason: ' + (reason.message ? reason.message : reason));
      });
    });
  };

  const removeSubscription = (subscriptionId) => {
    return new Promise((resolve, reject) => {
      if (subscriptions[subscriptionId]) {
        if (!subscriptions[subscriptionId].isInConnecting) {
          roomController && roomController.unsubscribe(subscriptions[subscriptionId].info.owner, subscriptionId);
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

  const selfClean = () => {
    setTimeout(function() {
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

      if (!hasNonAdminParticipant && !hasPublication && !hasSubscription) {
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
    return initRoom(roomId)
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
          publish: my_role_def[0].publish,
          subscribe: my_role_def[0].subscribe
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
            current_streams.push(streams[stream_id]);
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
      .then((result) => {
        return removeParticipant(participantId);
      }).then((result) => {
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

    return accessController.onSessionSignaling(sessionId, signaling)
      .then((result) => {
        callback('callback', 'ok');
      }, (e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
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

    if (pubInfo.type !== 'analytics' && room_config.inputLimit >= 0 && (room_config.inputLimit <= currentInputCount())) {
      return callback('callback', 'error', 'Too many inputs');
    }

    if (streams[streamId]) {
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
      var format_preference;
      if (pubInfo.type === 'webrtc') {
        format_preference = {};
        if (pubInfo.media.audio) {
          format_preference.audio = {optional: room_config.mediaIn.audio};
        }

        if (pubInfo.media.video) {
          format_preference.video = {optional: room_config.mediaIn.video};
        }
      }

      initiateStream(streamId, {owner: participantId, type: pubInfo.type});
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

  const isAudioFmtAvailable = (streamAudio, fmt) => {
    //log.debug('streamAudio:', JSON.stringify(streamAudio), 'fmt:', fmt);
    if (isAudioFmtCompatible(streamAudio.format, fmt)) {
      return true;
    }
    if (streamAudio.optional && streamAudio.optional.format && (streamAudio.optional.format.findIndex((f) => {return isAudioFmtCompatible(f, fmt);}) >= 0)) {
      return true;
    }
    return false;
  };

  const validateAudioRequest = (type, req, err) => {
    if (!streams[req.from] || !streams[req.from].media.audio) {
      err && (err.message = 'Requested audio stream');
      return false;
    }

    if (req.format) {
      if (!isAudioFmtAvailable(streams[req.from].media.audio, req.format)) {
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

  const isResolutionEqual = (r1, r2) => {
    return (r1.width === r2.width) && (r1.height === r2.height);
  };

  const isResolutionAvailable = (streamVideo, resolution) => {
    if (streamVideo.parameters && streamVideo.parameters.resolution && isResolutionEqual(streamVideo.parameters.resolution, resolution)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.resolution && (streamVideo.optional.parameters.resolution.findIndex((r) => {return isResolutionEqual(r, resolution);}) >= 0)) {
      return true;
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
    if (!streams[req.from] || !streams[req.from].media.video) {
      err && (err.message = 'Requested video stream');
      return false;
    }

    if (req.format && !isVideoFmtAvailable(streams[req.from].media.video, req.format)) {
      err && (err.message = 'Format is not acceptable');
      return false;
    }

    if (req.parameters) {
      if (req.parameters.resolution && !isResolutionAvailable(streams[req.from].media.video, req.parameters.resolution)) {
        err && (err.message = 'Resolution is not acceptable');
        return false;
      }

      if (req.parameters.framerate && !isFramerateAvailable(streams[req.from].media.video, req.parameters.framerate)) {
        err && (err.message = 'Framerate is not acceptable');
        return false;
      }

      //FIXME: allow bitrate 1.0x for client-sdk
      if (req.parameters.bitrate === 'x1.0' || req.parameters.bitrate === 'x1') {
        req.parameters.bitrate = undefined;
      }
      if (req.parameters.bitrate && !isBitrateAvailable(streams[req.from].media.video, req.parameters.bitrate)) {
        err && (err.message = 'Bitrate is not acceptable');
        return false;
      }

      if (req.parameters.keyFrameInterval && !isKeyFrameIntervalAvailable(streams[req.from].media.video, req.parameters.keyFrameInterval)) {
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
        format_preference = {};
        if (subDesc.media.audio) {
          var source = streams[subDesc.media.audio.from].media.audio;
          if (streams[subDesc.media.audio.from].type === 'forward') {
            format_preference.audio = {preferred: source.format};
            source.optional && source.optional.format && (format_preference.audio.optional = source.optional.format);
          } else {
            format_preference.audio = {optional: [source.format]};
            source.optional && source.optional.format && (format_preference.audio.optional = format_preference.audio.optional.concat(source.optional.format));
          }
        }

        if (subDesc.media.video) {
          var source = streams[subDesc.media.video.from].media.video;
          if (streams[subDesc.media.video.from].type === 'forward') {
            format_preference.video = {preferred: source.format};
            source.optional && source.optional.format && (format_preference.video.optional = source.optional.format);
          } else {
            format_preference.video = {optional: [source.format]};
            source.optional && source.optional.format && (format_preference.video.optional = format_preference.video.optional.concat(source.optional.format));
          }
        }
      }

      if (subDesc.type === 'recording') {
        var audio_codec = 'none-aac', video_codec;
        if (subDesc.media.audio) {
          if (subDesc.media.audio.format) {
            audio_codec = subDesc.media.audio.format.codec;
          } else {
            audio_codec = streams[subDesc.media.audio.from].media.audio.format.codec;
          }

          //FIXME: To support codecs other than those in the following list.
          if ((audio_codec !== 'pcmu') && (audio_codec !== 'pcma') && (audio_codec !== 'opus') && (audio_codec !== 'aac')) {
            return Promise.reject('Audio codec invalid');
          }
        }

        if (subDesc.media.video) {
          video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || streams[subDesc.media.video.from].media.video.format.codec);
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

    const controllerMix = (resolve, reject) => {
      roomController.mix(streamId, toView, function() {
        if (streams[streamId].info.inViews.indexOf(toView) === -1) {
          streams[streamId].info.inViews.push(toView);
        }
        resolve('ok');
      }, function(reason) {
        log.info('roomController.mix failed, reason:', reason);
        reject(reason);
      });
    };

    if (streams[streamId].info.type === 'webrtc' && !streams[streamId].media.video) {
      log.debug('Select audio at local node:', streamId);
      const audioNode = roomController.getMixerNode(toView, 'audio');
      return accessController.addLocalAudioRank(streamId, streams[streamId].info.owner, audioNode)
        .then((ret) => {
          if (ret && ret.locality && Array.isArray(ret.rank)) {
            const rankedMedia = {audio: {codec: 'unknown'}};
            ret.rank.forEach(rankedId => {
              roomController.publish('admin', rankedId, ret.locality, rankedMedia, 'webrtc', function() {
                roomController.mix(rankedId, toView, function() {
                  log.debug('Controller mix ranked audio ok:', rankedId);
                }, function(reason) {
                  log.warn('Controller mix fail for ranked audio:', reason);
                });
              }, function(reason) {
                log.warn('Controller publish fail for ranked audio:', reason);
              });
            });
          } else {
            log.warn('Wrong return format for addLocalAudioRank:', ret);
          }
        }).catch((e) => {
          log.debug('RPC addLocalAudioRank fail:', e);
          return new Promise(controllerMix);
        });
    } else {
      return new Promise(controllerMix);
    }
  };

  const unmix = function(streamId, fromView) {
    if (streams[streamId].isInConnecting) {
      return Promise.reject('Stream is NOT ready');
    }

    const controllerUnmix = (resolve, reject) => {
      roomController.unmix(streamId, fromView, function() {
        streams[streamId].info.inViews.splice(streams[streamId].info.inViews.indexOf(fromView), 1);
        resolve('ok');
      }, function(reason) {
        log.info('roomController.unmix failed, reason:', reason);
        reject(reason);
      });
    };

    if (streams[streamId].info.type === 'webrtc' && !streams[streamId].media.video) {
      const audioNode = roomController.getMixerNode(toView, 'audio');
      return accessController.removeLocalAudioRank(streamId, audioNode)
        .catch((e) => {
          log.debug('RPC removeLocalAudioRank fail:', e);
          return new Promise(controllerUnmix);
        });
    } else {
      return new Promise(controllerUnmix);
    }
  };

  const setStreamMute = function(streamId, track, muted) {
    if (streams[streamId].type === 'mixed') {
      return Promise.reject('Stream is Mixed');
    }

    var audio = (track === 'audio' || track === 'av') ? true : false,
        video = (track === 'video' || track === 'av') ? true : false,
        status = (muted ? 'inactive' : 'active');

    if (audio && !streams[streamId].media.audio) {
      return Promise.reject('Stream does NOT contain audio track');
    }

    if (video && !streams[streamId].media.video) {
      return Promise.reject('Stream does NOT contain video track');
    }

    return accessController.setMute(streamId, track, muted)
      .then(() => {
        audio && (streams[streamId].media.audio.status = status);
        video && (streams[streamId].media.video.status = status);
        roomController.updateStream(streamId, track, status);
        var updateFields = (track === 'av') ? ['audio.status', 'video.status'] : [track + '.status'];
        room_config.notifying.streamChange && updateFields.forEach((fieldData) => {
          sendMsg('room', 'all', 'stream', {status: 'update', id: streamId, data: {field: fieldData, value: status}});
        });
        return 'ok';
      }, function(reason) {
        log.warn('accessController set mute failed:', reason);
        return Promise.reject(reason);
      });
  };

  const getRegion = function(streamId, inView) {
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
    return new Promise((resolve, reject) => {
      roomController.setRegion(streamId, regionId, inView, function() {
        resolve('ok');
      }, function(reason) {
        log.info('roomController.setRegion failed, reason:', reason);
        reject(reason);
      });
    });
  };

  const setLayout = function(streamId, layout) {
    return new Promise((resolve, reject) => {
      roomController.setLayout(streams[streamId].info.label, layout, function(updated) {
        if (streams[streamId]) {
          streams[streamId].info.layout = updated;
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
    var old_su = subscriptions[subscriptionId],
        new_su = JSON.parse(JSON.stringify(old_su));

    var effective = false;
    if (update.audio) {
      if (!old_su.media.audio) {
        return Promise.reject('Target audio stream does NOT satisfy');
      }

      new_su.media.audio = (new_su.media.audio || {});
      if (update.audio.from && (update.audio.from !== new_su.media.audio.from)) {
        new_su.media.audio.from = update.audio.from;
        effective = true;
      }
    }

    if (update.video) {
      if (!old_su.media.video) {
        return Promise.reject('Target video stream does NOT satisfy');
      }

      new_su.media.video = (new_su.media.video || {});
      if (update.video.from && (update.video.from !== new_su.media.video.from)) {
        new_su.media.video.from = update.video.from;
        effective = true;
      }

      if (update.video.parameters && (Object.keys(update.video.parameters).length > 0)) {
        new_su.media.video.parameters = (new_su.media.video.parameters || {});
        old_su.media.video.parameters = (old_su.media.video.parameters || {});
        old_su.media.video.parameters.resolution = (old_su.media.video.parameters.resolution || {});

        if (update.video.parameters.resolution && ((update.video.parameters.resolution.width !== old_su.media.video.parameters.resolution.width) || (update.video.parameters.resolution.height !== old_su.media.video.parameters.resolution.height))) {
          new_su.media.video.parameters.resolution = update.video.parameters.resolution;
          effective = true;
        }
        if (update.video.parameters.framerate && (update.video.parameters.framerate !== old_su.media.video.parameters.framerate)) {
          new_su.media.video.parameters.framerate = update.video.parameters.framerate;
          effective = true;
        }
        if (update.video.parameters.bitrate && (update.video.parameters.bitrate !== old_su.media.video.parameters.bitrate)) {
          new_su.media.video.parameters.bitrate = update.video.parameters.bitrate;
          effective = true;
        }
        if (update.video.parameters.keyFrameInterval && (update.video.parameters.keyFrameInterval !== old_su.media.video.parameters.keyFrameInterval)) {
          new_su.media.video.parameters.keyFrameInterval = update.video.parameters.keyFrameInterval;
          effective = true;
        }
      }
    }

    if (!effective) {
      return Promise.resolve('ok');
    } else {
      if (new_su.media.video && !validateVideoRequest(new_su.info.type, new_su.media.video)) {
        return Promise.reject('Target video stream does NOT satisfy');
      } else if (new_su.media.audio && !validateAudioRequest(new_su.info.type, new_su.media.audio)) {
        return Promise.reject('Target audio stream does NOT satisfy');
      } else {
        return removeSubscription(subscriptionId)
          .then((result) => {
            return addSubscription(subscriptionId, new_su.locality, new_su.media, new_su.info);
          }).catch((err) => {
            log.info('Update subscription failed:', err.message ? err.message : err);
            log.info('And is recovering the previous subscription:', JSON.stringify(old_su));
            return addSubscription(subscriptionId, old_su.locality, old_su.media, old_su.info)
              .then(() => {
                return Promise.reject('Update subscription failed');
              }, () => {
                return Promise.reject('Update subscription failed');
              });
          });
      }
    }
  };

  const setSubscriptionMute = (subscriptionId, track, muted) => {
    var audio = (track === 'audio' || track === 'av') ? true : false,
        video = (track === 'video' || track === 'av') ? true : false,
        status = (muted ? 'active' : 'inactive');

    if (audio && !subscriptions[subscriptionId].media.audio) {
      return Promise.reject('Subscription does NOT contain audio track');
    }

    if (video && !subscriptions[subscriptionId].media.video) {
      return Promise.reject('Subscription does NOT contain video track');
    }

    return accessController.setMute(subscriptionId, track, muted)
      .then(() => {
        audio && (subscriptions[subscriptionId].media.audio.status = status);
        video && (subscriptions[subscriptionId].media.video.status = status);
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

  that.onSessionProgress = (sessionId, direction, sessionStatus) => {
    log.debug('onSessionProgress, sessionId:', sessionId, 'direction:', direction, 'sessionStatus:', sessionStatus);
    accessController && accessController.onSessionStatus(sessionId, sessionStatus);
  };

  that.onMediaUpdate = (sessionId, direction, mediaUpdate) => {
    log.debug('onMediaUpdate, sessionId:', sessionId, 'direction:', direction, 'mediaUpdate:', mediaUpdate);
    if (direction === 'in' && streams[sessionId] && (streams[sessionId].type === 'forward')) {
      updateStreamInfo(sessionId, mediaUpdate);
      roomController && roomController.updateStreamInfo(sessionId, mediaUpdate);
    }
  };

  that.onVideoLayoutChange = function(roomId, layout, view, callback) {
    log.debug('onVideoLayoutChange, roomId:', roomId, 'layout:', layout, 'view:', view);
    if (room_id === roomId && roomController) {
      var streamId = roomController.getMixedStream(view);
      if (streams[streamId]) {
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

      var input = undefined;
      for (var id in streams) {
        if (streams[id].type === 'forward' && id === activeInputStream) {
          input = id;
          break;
        }
      }
      if (input) {
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
        result.push(streams[stream_id]);
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
      var stream_id = Math.round(Math.random() * 1000000000000000000) + '';
      return initRoom(roomId)
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

        initiateStream(stream_id, {owner: 'admin', type: pubInfo.type});
        return accessController.initiate('admin', stream_id, 'in', participants['admin'].getOrigin(), pubInfo);
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
        callback('callback', streams[stream_id]);
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
              if (streams[streamId].media.audio) {
                if (streams[streamId].media.audio.status !== cmd.value) {
                  muteReq.push({track: 'audio', mute: (cmd.value === 'inactive')});
                }
                exe = Promise.resolve('ok');
              } else {
                exe = Promise.reject('Track does NOT exist');
              }
            } else if ((cmd.path === '/media/video/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (streams[streamId].media.video) {
                if (streams[streamId].media.video.status !== cmd.value) {
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
      callback('callback', streams[streamId]);
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
    var result = {id: subId, media: subscriptions[subId].media};
    if (subscriptions[subId].info.type === 'streaming') {
      result.url = subscriptions[subId].info.url;
    } else if (subscriptions[subId].info.type === 'recording') {
      result.storage = subscriptions[subId].info.location;
    } else if (subscriptions[subId].info.type === 'analytics') {
      result.analytics = subscriptions[subId].info.analytics;
    }
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
      return initRoom(roomId)
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
              audio_codec = streams[subDesc.media.audio.from].media.audio.format.codec;
            }

            //FIXME: To support codecs other than those in the following list.
            if ((audio_codec !== 'pcmu') && (audio_codec !== 'pcma') && (audio_codec !== 'opus') && (audio_codec !== 'aac')) {
              return Promise.reject('Audio codec invalid');
            }
          }

          if (subDesc.media.video) {
            video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || streams[subDesc.media.video.from].media.video.format.codec);
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

          let sourceVideoOption = streams[subDesc.media.video.from].media.video;
          if (!subDesc.media.video.format) {
            // Subscribe source format
            subDesc.media.video.format = sourceVideoOption.format;
          }
          if (subDesc.media.video.parameters || sourceVideoOption.parameters) {
            subDesc.media.video.parameters = Object.assign(
              sourceVideoOption.parameters || {},
              subDesc.media.video.parameters || {});
          }
        }

        // Schedule preference for worker node
        var accessPreference = Object.assign({}, participants['admin'].getOrigin());
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
    return Promise.all(
      commands.map((cmd) => {
        var exe;
        switch (cmd.op) {
          case 'replace':
            if ((cmd.path === '/media/audio/status') && subscriptions[subId].media.audio && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (subscriptions[subId].media.audio.status !== cmd.value) {
                muteReqs.push({track: 'audio', mute: (cmd.value === 'inactive')});
              }
              exe = Promise.resolve('ok');
            } else if ((cmd.path === '/media/video/status') && subscriptions[subId].media.video && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (subscriptions[subId].media.video.status !== cmd.value) {
                muteReqs.push({track: 'video', mute: (cmd.value === 'inactive')});
              }
              exe = Promise.resolve('ok');
            } else if ((cmd.path === '/media/audio/from') && streams[cmd.value]) {
              subUpdate = (subUpdate || {});
              subUpdate.audio = (subUpdate.audio || {});
              subUpdate.audio.from = cmd.value
              exe = Promise.resolve('ok');
            } else if ((cmd.path === '/media/video/from') && streams[cmd.value]) {
              subUpdate = (subUpdate || {});
              subUpdate.video = (subUpdate.video || {});
              subUpdate.video.from = cmd.value;
              exe = Promise.resolve('ok');
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
              exe = Promise.resolve('ok');
            } else if (cmd.path === '/media/video/parameters/keyFrameInterval') {
              subUpdate = (subUpdate || {});
              subUpdate.video = (subUpdate.video || {});
              subUpdate.video.parameters = (subUpdate.video.parameters || {});
              subUpdate.video.parameters.keyFrameInterval = cmd.value;
              exe = Promise.resolve('ok');
            } else {
              exe = Promise.reject('Invalid path or value');
            }
            break;
          default:
            exe = Promise.reject('Invalid subscription control operation');
        }
        return exe;
      })
    ).then(() => {
      return Promise.all(muteReqs.map((r) => {
        return setSubscriptionMute(subId, r.track, r.mute);
      }));
    }).then(() => {
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
    for (var pid in participants) {
      if (participants[pid].getInfo().role === 'sip') {
        result.push(getSipCallInfo(pid));
      }
    }
    callback('callback', result);
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
    } else if (message.purpose === 'webrtc' ||
               message.purpose === 'recording' ||
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
    onMediaUpdate: conference.onMediaUpdate,
    onSessionAudit: conference.onSessionAudit,

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
    makeSipCall: conference.makeSipCall,
    controlSipCall: conference.controlSipCall,
    endSipCall: conference.endSipCall,
    drawText: conference.drawText,
    destroy: conference.destroy
  };

  that.onFaultDetected = conference.onFaultDetected;

  return that;
};

