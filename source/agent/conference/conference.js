/*global require, module, global, process*/
'use strict';

var Fraction = require('fraction.js');
var logger = require('./logger').logger;

var AccessController = require('./accessController');
var RoomController = require('./roomController');
var dataAccess = require('./data_access');
var Participant = require('./participant');
//var Stream = require('./stream');

// Logger
var log = logger.getLogger('Conference');

const partial_linear_bitrate = [
  {size: 0, bitrate: 0},
  {size: 76800, bitrate: 400},  //320*240, 30fps
  {size: 307200, bitrate: 800}, //640*480, 30fps
  {size: 921600, bitrate: 2000},  //1280*720, 30fps
  {size: 2073600, bitrate: 4000}, //1920*1080, 30fps
  {size: 8294400, bitrate: 16000} //3840*2160, 30fps
];

const standardBitrate = (width, height, framerate) => {
  let bitrate = -1;
  let prev = 0;
  let next = 0;
  let portion = 0.0;
  let def = width * height * framerate / 30;

  // find the partial linear section and calculate bitrate
  for (var i = 0; i < partial_linear_bitrate.length - 1; i++) {
    prev = partial_linear_bitrate[i].size;
    next = partial_linear_bitrate[i+1].size;
    if (def > prev && def <= next) {
      portion = (def - prev) / (next - prev);
      bitrate = partial_linear_bitrate[i].bitrate + (partial_linear_bitrate[i+1].bitrate - partial_linear_bitrate[i].bitrate) * portion;
      break;
    }
  }

  // set default bitrate for over large resolution
  if (-1 == bitrate) {
    bitrate = 16000;
  }

  return bitrate;
}

const calcDefaultBitrate = (codec, resolution, framerate, motionFactor) => {
  let codec_factor = 1.0;
  switch (codec) {
    case 'h264':
      codec_factor = 1.0;
      break;
    case 'vp8':
      codec_factor = 1.0;
      break;
    case 'vp9':
      codec_factor = 0.8;//FIXME: Theoretically it should be 0.5, not appliable before encoder is improved.
      break;
    case 'h265':
      codec_factor = 0.9;//FIXME: Theoretically it should be 0.5, not appliable before encoder is improved.
      break;
    default:
      break;
  }
  return standardBitrate(resolution.width, resolution.height, framerate) * codec_factor * motionFactor;
};

const isAudioFmtCompatible = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec) && (fmt1.sampleRate === fmt2.sampleRate) && (fmt1.channelNum === fmt2.channelNum);
};

const isVideoFmtCompatible = (fmt1, fmt2) => {
  return (fmt1.codec === fmt2.codec); //FIXME: fmt.profile should be considered.
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
    case 'sif':
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
      current_input_count = 0,
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
   *       type: 'webrtc' | 'streaming' | 'recording' | 'sip',
   *       location: {host: string(HostIPorDN), path: string(FileFullPath)} | undefined,
   *       url: string(URLofStreamingOut) | undefined
   *     }
   *   }
   * }
   */
  var subscriptions = {};

  /*
   * ParticipantID => [ StreamID ]
   */
  var sipPublications = {};
  /*
   * ParticipantID => [ SubscriptionID ]
   */
  var sipSubscriptions = {};


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
      current_input_count -= 1;
      removeStream(participantId, sessionId)
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
            //FIXME: To refine the configuration data structure and remove the following translating logic later.
            room_config = config;
            room_config.internalConnProtocol = global.config.internal.protocol;

            if (room_config.participantLimit === 0) {
              log.error('Room', roomID, 'disabled');
              is_initializing = false;
              return Promise.reject('Room' + roomID + 'disabled');
            }

            return new Promise(function(resolve, reject) {
              RoomController.create(
                {
                  cluster: global.config.cluster.name || 'woogeen-cluster',
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

                    //FIXME: Validation defaultFormat/mediaOut against av_capability here is not complete.
                    var default_audio_fmt = false, default_video_fmt = false;
                    if (viewSettings.audio) {
                      if (viewSettings.audio.format && (av_capability.audio.findIndex((f) => {return isAudioFmtCompatible(viewSettings.audio.format, f);}) >= 0)) {
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
                                                       permission: {}
                                                      }, rpcReq);

                  accessController = AccessController.create({clusterName: global.config.cluster.name || 'woogeen-cluster',
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
        }, function(err) {
          log.error('Init room failed, reason:', err);
          is_initializing = false;
          return Promise.reject(err);
        });
    }
  };

  var destroyRoom = function() {
    if (room_id) {
      accessController && accessController.destroy();
      accessController = undefined;
      roomController && roomController.destroy();
      roomController = undefined;
      subscriptions = {};
      streams = {};
      participants = {};
      room_id = undefined;
    }
    process.exit();
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
      var participant = participants[participantId];

      var left_user = participant.getInfo();
      delete participants[participantId];
      room_config.notifying.participantActivities && sendMsg('room', 'all', 'participant', {action: 'leave', data: left_user.id});
    }
  };

  const dropParticipants = function(portal) {
    for (var participant_id in participants) {
      if (participants[participant_id].getPortal() === portal) {
        removeParticipant(participant_id);
      }
    }
  };

  const addSIPStream = (id, owner, pubInfo) => {
    if (!sipPublications[owner]) {
      sipPublications[owner] = [];
    }
    sipPublications[owner].push(id);
    return addStream(id, pubInfo.locality, pubInfo.media, {owner: owner, type: 'sip'});
  };

  const extractAudioFormat = (audioInfo) => {
    var result = {codec: audioInfo.codec};
    audioInfo.sampleRate && (result.sampleRate = audioInfo.sampleRate);
    audioInfo.channelNum && (result.channelNum = audioInfo.channelNum);

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
        format: {codec: media.video.codec}
      };

      //FIXME: sometimes, streaming send invalid resolution { width: 0, height: 0}
      if (media.video.resolution && (!media.video.resolution.height && !media.video.resolution.width)) {
        delete media.video.resolution;
      }

      media.video.source && (result.video.source = media.video.source);
      media.video.profile && (result.video.format.profile = media.video.profile);
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

  const addStream = (id, locality, media, info) => {
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

  const removeStream = (participantId, streamId) => {
    return new Promise((resolve, reject) => {
      if (streams[streamId]) {
        for (var sub_id in subscriptions) {
          if ((subscriptions[sub_id].media.audio && (subscriptions[sub_id].media.audio.from === streamId))
            || (subscriptions[sub_id].media.video && (subscriptions[sub_id].media.video.from === streamId))) {
            accessController && accessController.terminate(sub_id, 'out', 'Source stream loss');
          }
        }

        roomController && roomController.unpublish(participantId, streamId);
        delete streams[streamId];
        setTimeout(() => {
          room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: streamId, status: 'remove'});
        }, 10);
      }
      resolve('ok');
    });
  };

  const addSIPSubscription = (id, owner, subDesc) => {
    if (!sipSubscriptions[owner]) {
      sipSubscriptions[owner] = [];
    }
    sipSubscriptions[owner].push(id);
    return addSubscription(id, subDesc.locality, subDesc.media, {owner: owner, type: 'sip'});
  };

  const addSubscription = (id, locality, mediaSpec, info) => {
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
        if (media.video.parameters) {
          if (!media.video.parameters.resolution && !media.video.parameters.framerate) {
            if (media.video.parameters.bitrate) {
              media.video.parameters.bitrate = source.parameters.bitrate * Number(media.video.parameters.bitrate.substring(1));
            } else {
              media.video.parameters.bitrate = source.parameters.bitrate;
            }
          } else {
            media.video.parameters.bitrate = (media.video.parameters.bitrate || 'unspecified');//Should use 'unspecified' rather than source.parameters.bitrate
          }

          media.video.parameters.resolution = (media.video.parameters.resolution || source.parameters.resolution);
          media.video.parameters.framerate = (media.video.parameters.framerate || source.parameters.framerate);
          media.video.parameters.keyFrameInterval = (media.video.parameters.keyFrameInterval || source.parameters.keyFrameInterval);
        } else {
          media.video.parameters = source.parameters;
        }
      } else {
        if (media.video.parameters) {
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

    return new Promise((resolve, reject) => {
      roomController && roomController.subscribe(info.owner, id, locality, media, info.type, function() {
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
        roomController && roomController.unsubscribe(subscriptions[subscriptionId].info.owner, subscriptionId);
        delete subscriptions[subscriptionId];
      }
      resolve('ok');
    });
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
    }, 6 * 1000);
  };

  that.join = function(roomId, participantInfo, callback) {
    log.debug('participant:', participantInfo, 'join room:', roomId);
    var permission;
    return initRoom(roomId)
      .then(function() {
        log.debug('room_config.participantLimit:', room_config.participantLimit, 'current participants count:', Object.keys(participants).length);
        if (room_config.participantLimit > 0 && (Object.keys(participants).length >= room_config.participantLimit)) {
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
          current_streams.push(streams[stream_id]);
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
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    // Remove SIP streams
    if (sipPublications[participantId]) {
      for (let i = 0; i < sipPublications[participantId].length; i++) {
        removeStream(participantId, sipPublications[participantId][i]);
      }
      delete sipPublications[participantId];
    }
    // Remove SIP subscriptions
    if (sipSubscriptions[participantId]) {
      for (let i = 0; i < sipSubscriptions[participantId].length; i++) {
        removeSubscription(sipSubscriptions[participantId][i]);
      }
      delete sipSubscriptions[participantId];
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

    if (room_config.inputLimit >= 0 && (room_config.inputLimit <= current_input_count)) {
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
      return addSIPStream(streamId, participantId, pubInfo)
      .then((result) => {
        current_input_count += 1;
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

      return accessController.initiate(participantId, streamId, 'in', participants[participantId].getOrigin(), pubInfo, format_preference)
      .then((result) => {
        current_input_count += 1;
        callback('callback', result);
      })
      .catch((e) => {
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

    //if (!streams[streamId]) {
    //  return callback('callback', 'error', 'Stream does NOT exist');
    //}

    //if (streams[streamId].info.owner !== participantId) {
    //  return callback('callback', 'error', 'unauthorized');
    //}

    if (streams[streamId] && (streams[streamId].info.type === 'sip')) {
      return removeStream(participantId, streamId)
        .then((result) => {
          current_input_count -= 1;
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    } else if (streams[streamId]) {
      return accessController.terminate(streamId, 'in', 'Participant terminate')
        .then((result) => {
          log.debug('accessController.terminate result:', result);
          //return removeStream(participantId, streamId);
          return 'ok';
        }, (e) => {
          return Promise.reject('Failed in terminating session');
        })
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    } else {
      callback('callback', 'ok');
    }
  };

  const isAudioFmtAcceptable = (streamAudio, fmt) => {
    //log.debug('streamAudio:', JSON.stringify(streamAudio), 'fmt:', fmt);
    if (isAudioFmtCompatible(streamAudio.format, fmt)) {
      return true;
    }
    if (streamAudio.optional && streamAudio.optional.format && (streamAudio.optional.format.findIndex((f) => {return isAudioFmtCompatible(f, fmt);}) >= 0)) {
      return true;
    }
    return false;
  };

  const validateAudioRequest = (type, req) => {
    if (!streams[req.from] || !streams[req.from].media.audio) {
      return false;
    }

    if (req.format) {
      if (!isAudioFmtAcceptable(streams[req.from].media.audio, req.format)) {
        return false;
      }
    }

    return true;
  };

  const isVideoFmtAcceptable = (streamVideo, fmt) => {
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

  const isResolutionAcceptable = (streamVideo, resolution) => {
    if (streamVideo.parameters && streamVideo.parameters.resolution && isResolutionEqual(streamVideo.parameters.resolution, resolution)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.resolution && (streamVideo.optional.parameters.resolution.findIndex((r) => {return isResolutionEqual(r, resolution);}) >= 0)) {
      return true;
    }
    return false;
  };

  const isFramerateAcceptable = (streamVideo, framerate) => {
    if (streamVideo.parameters && streamVideo.parameters.framerate && (streamVideo.parameters.framerate === framerate)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.framerate && (streamVideo.optional.parameters.framerate.findIndex((f) => {return f === framerate;}) >= 0)) {
      return true;
    }
    return false;
  };

  const isBitrateAcceptable = (streamVideo, bitrate) => {
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.bitrate && (streamVideo.optional.parameters.bitrate.findIndex((b) => {return b === bitrate;}) >= 0)) {
      return true;
    }
    return false;
  };

  const isKeyFrameIntervalAcceptable = (streamVideo, keyFrameInterval) => {
    if (streamVideo.parameters && streamVideo.parameters.resolution && (streamVideo.parameters.keyFrameInterval === keyFrameInterval)) {
      return true;
    }
    if (streamVideo.optional && streamVideo.optional.parameters && streamVideo.optional.parameters.keyFrameInterval && (streamVideo.optional.parameters.keyFrameInterval.findIndex((k) => {return k === keyFrameInterval;}) >= 0)) {
      return true;
    }
    return false;
  };

  const validateVideoRequest = (type, req) => {
    if (!streams[req.from] || !streams[req.from].media.video) {
      return false;
    }

    if (req.format && !isVideoFmtAcceptable(streams[req.from].media.video, req.format)) {
      return false;
    }

    if (req.parameters) {
      if (req.parameters.resolution && !isResolutionAcceptable(streams[req.from].media.video, req.parameters.resolution)) {
        return false;
      }

      if (req.parameters.framerate && !isFramerateAcceptable(streams[req.from].media.video, req.parameters.framerate)) {
        return false;
      }

      //FIXME: allow bitrate 1.0x for client-sdk
      if (req.parameters.bitrate === 'x1.0' || req.parameters.bitrate === 'x1') {
        req.parameters.bitrate = undefined;
      }
      if (req.parameters.bitrate && !isBitrateAcceptable(streams[req.from].media.video, req.parameters.bitrate)) {
        return false;
      }

      if (req.parameters.keyFrameInterval && !isKeyFrameIntervalAcceptable(streams[req.from].media.video, req.parameters.keyFrameInterval)) {
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

    if (subDesc.media.audio && !validateAudioRequest(subDesc.type, subDesc.media.audio)) {
      return callback('callback', 'error', 'Target audio stream does NOT satisfy');
    }

    if (subDesc.media.video && !validateVideoRequest(subDesc.type, subDesc.media.video)) {
      return callback('callback', 'error', 'Target video stream does NOT satisfy');
    }

    if (subDesc.type === 'sip') {
      return addSIPSubscription(participantId, subscriptionId, subDesc)
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

      if (subDesc.type === 'recording' && (!subDesc.connection.container || subDesc.connection.container === 'auto')) {
        var audio_codec = 'none-aac', video_codec;
        if (subDesc.media.audio) {
          if (subDesc.media.audio.format) {
            audio_codec = subDesc.media.audio.format.codec;
          } else {
            audio_codec = streams[subDesc.media.audio.from].media.audio.format.codec;
          }
        }

        if (subDesc.media.video) {
          video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || streams[subDesc.media.video.from].media.video.format.codec);
        }

        subDesc.connection.container = ((audio_codec === 'aac' && (!video_codec || (video_codec === 'h264') || (video_codec === 'h265'))) ? 'mp4' : 'mkv');
      }

      if (subDesc.type === 'streaming' && subDesc.media.audio && !subDesc.media.audio.format) {//FIXME: To support audio formats other than aac_48000_2.
        subDesc.media.audio.format = {codec: 'aac', sampleRate: 48000, channelNum: 2};
      }

      return accessController.initiate(participantId, subscriptionId, 'out', participants[participantId].getOrigin(), subDesc, format_preference)
      .then((result) => {
        callback('callback', result);
      })
      .catch((e) => {
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

    //if (!subscriptions[subscriptionId]) {
    //  return callback('callback', 'error', 'Subscription does NOT exist');
    //}

    //if (subscriptions[subscriptionId].info.owner !== participantId) {
    //  return callback('callback', 'error', 'unauthorized');
    //}

    if (subscriptions[subscriptionId] && (subscriptions[subscriptionId].info.type === 'sip')) {
      return removeSubscription(subscriptionId)
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    } else if (subscriptions[subscriptionId]) {
      return accessController.terminate(subscriptionId, 'out', 'Participant terminate')
        .then((result) => {
          //return removeSubscription(subscriptionId);
          return 'ok';
        }, (e) => {
          return Promise.reject('Failed in terminating session');
        })
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    } else {
        callback('callback', 'ok');
    }
  };

  const mix = function(streamId, toView) {
    return new Promise((resolve, reject) => {
      roomController.mix(streamId, toView, function() {
        streams[streamId].info.inViews.push(toView);
        resolve('ok');
      }, function(reason) {
        log.info('roomController.mix failed, reason:', reason);
        reject(reason);
      });
    });
  };

  const unmix = function(streamId, fromView) {
    return new Promise((resolve, reject) => {
      roomController.unmix(streamId, fromView, function() {
        streams[streamId].info.inViews.splice(streams[streamId].info.inViews.indexOf(fromView), 1);
        resolve('ok');
      }, function(reason) {
        log.info('roomController.unmix failed, reason:', reason);
        reject(reason);
      });
    });
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
        log.info('roomController.setRegion failed, reason:', reason);
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

    if (streams[streamId].info.owner !== participantId) {
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

      if (update.video.parameters) {
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

    if (subscriptions[subscriptionId].info.owner !== participantId) {
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

    sendMsg(fromParticipantId, toParticipantId, 'text', {from: fromParticipantId, to: toParticipantId, message: msg});
    callback('callback', 'ok');
  };

  that.onSessionProgress = (sessionId, direction, sessionStatus) => {
    log.debug('onSessionProgress, sessionId:', sessionId, 'direction:', direction, 'sessionStatus:', sessionStatus);
    accessController && accessController.onSessionStatus(sessionId, sessionStatus);
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

  that.onAudioActiveness = function(roomId, activeParticipantId, view, callback) {
    log.debug('onAudioActiveness, roomId:', roomId, 'activeParticipantId:', activeParticipantId, 'view:', view);
    if ((room_id === roomId) && roomController) {
      room_config.views.forEach((viewSettings) => {
        if (viewSettings.label === view && viewSettings.video.keepActiveInputPrimary) {
          roomController.setPrimary(activeParticipantId, view);
        }
      });

      var input = undefined;
      for (var id in streams) {
        if (streams[id].type === 'forward' && streams[id].info.owner === activeParticipantId) {
          input = id;
          break;
        }
      }
      if (input) {
        for (var id in streams) {
          if (streams[id].type === 'mixed' && streams[id].info.label === view) {
            streams[id].info.activeInput = input;
            room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: id, status: 'update', data: {field: 'activeInput', value: input}});
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

  //The following interfaces are reserved to serve nuve
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
        return participants[participantId].update(auth.op, auth.path, auth.value);
      })
    ).then(() => {
      callback('callback', participants[participantId].getDetail());
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.dropParticipant = function(participantId, callback) {
    log.debug('dropParticipant', participantId);
    var deleted = null;
    if (participants[participantId] && participantId !== 'admin') {
      deleted = participants[participantId];
      participants[participantId].drop()
        .then(function(result) {
          removeParticipant(participantId);
          callback('callback', deleted);
        }).catch(function(reason) {
          log.debug('dropParticipant fail:', reason);
          callback('callback', 'error', 'Drop participant failed');
        });
    } else {
      callback('callback', 'error', 'Participant does NOT exist');
    }
  };

  that.getStreams = function(callback) {
    log.debug('getStreams, room_id:', room_id);
    var result = [];
    for (var stream_id in streams) {
      result.push(streams[stream_id]);
    }
    callback('callback', result);
  };

  that.getStreamInfo = function(streamId, callback) {
    log.debug('getStreamInfo, room_id:', room_id, 'streamId:', streamId);
    if (streams[streamId]) {
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
        if (room_config.inputLimit >= 0 && (room_config.inputLimit <= current_input_count)) {
          return Promise.reject('Too many inputs');
        }

        if (pubInfo.media.audio && !room_config.mediaIn.audio.length) {
          return Promise.reject('Audio is forbiden');
        }

        if (pubInfo.media.video && !room_config.mediaIn.video.length) {
          return Promise.reject('Video is forbiden');
        }

        return accessController.initiate('admin', stream_id, 'in', participants['admin'].getOrigin(), pubInfo);
      }).then((result) => {
        current_input_count += 1;
        return 'ok';
      }).then(() => {
        return new Promise((resolve, reject) => {
          var count = 0, wait = 200;
          var interval = setInterval(() => {
            if (count > wait) {
              clearInterval(interval);
              accessController.terminate('admin', stream_id, 'Participant terminate');
              reject('Access timeout');
            } else {
              if (streams[stream_id]) {
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
        selfClean();
      });
    } else {
      callback('callback', 'error', 'Invalid publication type');
    }
  };

  that.controlStream = function(streamId, commands, callback) {
    log.debug('controlStream', streamId, 'commands:', commands);
    if (streams[streamId] === undefined) {
      callback('callback', 'error', 'Stream does NOT exist');
    }

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
              if (streams[streamId].media.audio.status !== cmd.value) {
                exe = setStreamMute(streamId, 'audio', (cmd.value === 'inactive'));
              } else {
                exe = Promise.resolve('ok');
              }
            } else if ((cmd.path === '/media/video/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (streams[streamId].media.video.status !== cmd.value) {
                exe = setStreamMute(streamId, 'video', (cmd.value === 'inactive'));
              } else {
                exe = Promise.resolve('ok');
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
      callback('callback', streams[streamId]);
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.deleteStream = function(streamId, callback) {
    log.debug('deleteStream, streamId:', streamId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (!streams[streamId]) {
      return callback('callback', 'ok');
    }

    return accessController.terminate(streamId, 'in', 'Participant terminate')
      .then((result) => {
        log.debug('accessController.terminate result:', result);
        //return removeStream('admin', streamId);
        return 'ok';
      }, (e) => {
        return Promise.reject('Failed in terminating session');
      })
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
    }
    return result;
  };

  that.getSubscriptions = function(type, callback) {
    log.debug('getSubscriptions, room_id:', room_id, 'type:', type);
    var result = [];
    for (var sub_id in subscriptions) {
      if (subscriptions[sub_id].info.type === type) {
        result.push(subscriptionAbstract(sub_id));
      }
    }
    callback('callback', result);
  };

  that.getSubscriptionInfo = function(subId, callback) {
    log.debug('getSubscriptionInfo, room_id:', room_id, 'subId:', subId);
    if (subscriptions[subId]) {
      callback('callback', subscriptionAbstract(subId));
    } else {
      callback('callback', 'error', 'Stream does NOT exist');
    }
  };

  that.addServerSideSubscription = function(roomId, subDesc, callback) {
    log.debug('addServerSideSubscription, roomId:', roomId, 'subDesc:', JSON.stringify(subDesc));

    if (subDesc.type === 'streaming' || subDesc.type === 'recording') {
      var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';
      return initRoom(roomId)
      .then(() => {
        if (subDesc.media.audio && !room_config.mediaOut.audio.length) {
          return Promise.reject('Audio is forbiden');
        }

        if (subDesc.media.video && !room_config.mediaOut.video.format.length) {
          return Promise.reject('Video is forbiden');
        }

        if (subDesc.media.audio && !validateAudioRequest(subDesc.type, subDesc.media.audio)) {
          return Promise.reject('Target audio stream does NOT satisfy');
        }

        if (subDesc.media.video && !validateVideoRequest(subDesc.type, subDesc.media.video)) {
          return Promise.reject('Target video stream does NOT satisfy');
        }

        if (subDesc.type === 'recording' && (!subDesc.connection.container || subDesc.connection.container === 'auto')) {
          var audio_codec = 'none-aac', video_codec;
          if (subDesc.media.audio) {
            if (subDesc.media.audio.format) {
              audio_codec = subDesc.media.audio.format.codec;
            } else {
              audio_codec = streams[subDesc.media.audio.from].media.audio.format.codec;
            }
          }

          if (subDesc.media.video) {
            video_codec = ((subDesc.media.video.format && subDesc.media.video.format.codec) || streams[subDesc.media.video.from].media.video.format.codec);
          }

          subDesc.connection.container = ((audio_codec === 'aac' && (!video_codec || (video_codec === 'h264') || (video_codec === 'h265'))) ? 'mp4' : 'mkv');
        }

        if (subDesc.type === 'streaming' && subDesc.media.audio && !subDesc.media.audio.format) {//FIXME: To support audio formats other than aac_48000_2.
          subDesc.media.audio.format = {codec: 'aac', sampleRate: 48000, channelNum: 2};
        }

        return accessController.initiate('admin', subscription_id, 'out', participants['admin'].getOrigin(), subDesc);
      }).then(() => {
        return new Promise((resolve, reject) => {
          var count = 0, wait = 200;
          var interval = setInterval(() => {
            if (count > wait) {
              clearInterval(interval);
              accessController.terminate('admin', subscription_id, 'Participant terminate');
              reject('Access timeout');
            } else {
              if (subscriptions[subscription_id]) {
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
        selfClean();
      });
    } else {
      callback('callback', 'error', 'Invalid subscription type');
    }
  };

  that.controlSubscription = function(subId, commands, callback) {
    log.debug('controlSubscription', subId, 'commands:', commands);
    if (subscriptions[subId] === undefined) {
      callback('callback', 'error', 'Subscription does NOT exist');
    }

    var subUpdate;
    return Promise.all(
      commands.map((cmd) => {
        var exe;
        switch (cmd.op) {
          case'replace':
            if ((cmd.path === '/media/audio/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (subscriptions[subId].media.audio.status !== cmd.value) {
                exe = setSubscriptionMute(subId, 'audio', (cmd.value === 'inactive'));
              } else {
                exe = Promise.resolve('ok');
              }
            } else if ((cmd.path === '/media/video/status') && (cmd.value === 'inactive' || cmd.value === 'active')) {
              if (subscriptions[subId].media.video.status !== cmd.value) {
                exe = setSubscriptionMute(subId, 'video', (cmd.value === 'inactive'));
              } else {
                exe = Promise.resolve('ok');
              }
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
      if (subUpdate) {
        return updateSubscription(subId, subUpdate);
      } else {
        return 'ok';
      }
    }).then(() => {
      if (subscriptions[subId]) {
        callback('callback', subscriptionAbstract(subId));
      } else {
        callback('callback', 'error', 'Subscription does NOT exist');
      }
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.deleteSubscription = function(subId, callback) {
    log.debug('deleteSubscription, subId:', subId);
    if (!accessController || !roomController) {
      return callback('callback', 'error', 'Controllers are not ready');
    }

    if (!subscriptions[subId]) {
      return callback('callback', 'ok');
    }

    return accessController.terminate(subId, 'out', 'Participant terminate')
      .then((result) => {
        log.debug('accessController.terminate result:', result);
        //return removeSubscription(subId);
        return 'ok';
      }, (e) => {
        return Promise.reject('Failed in terminating session');
      })
      .then((result) => {
        callback('callback', result);
        selfClean();
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
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
               message.purpose === 'streaming') {
      accessController && accessController.onFaultDetected(message.type, message.id);
    } else if (message.purpose === 'audio' ||
               message.purpose === 'video') {
      roomController && roomController.onFaultDetected(message.purpose, message.type, message.id);
    }
  };

  return that;
};


module.exports = function (rpcClient, selfRpcId) {
  var that = {};

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
    onSessionAudit: conference.onSessionAudit,

    // rpc from audio nodes.
    onAudioActiveParticipant: conference.onAudioActiveness,

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
    destroy: conference.destroy
  };

  that.onFaultDetected = conference.onFaultDetected;

  return that;
};

