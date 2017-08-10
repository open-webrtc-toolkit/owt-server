/*global require, module, global, process*/
'use strict';

var logger = require('./logger').logger;

var AccessController = require('./accessController');
var RoomController = require('./roomController');
var dbAccess = require('./dataBaseAccess');
var Participant = require('./participant');
//var Stream = require('./stream');

// Logger
var log = logger.getLogger('Conference');

var translateOldRoomConfig = (oldConfig) => {
  var config = {
    _id: oldConfig.id,
    name: oldConfig.name,
    publishLimit: oldConfig.publishLimit,
    userLimit: oldConfig.userLimit,
    avatar: 'local',
    roles: [],
    views: [],
    mediaIn: {
      audio: [{
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        }, {
          codec: 'pcmu'
        }, {
          codec: 'pcma'
        }, {
          codec: 'aac'
        }, {
          codec: 'ac3'
        }, {
          codec: 'nellymoser'
        }
      ],
      video: [{
          codec: 'h264',
          profile: 'high'
        }, {
          codec: 'vp8'
        }, {
          codec: 'h265'
        }, {
          codec: 'vp9'
        }
      ]
    },
    mediaOut: {
      audio: [{
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        }, {
          codec: 'pcmu'
        }, {
          codec: 'pcma'
        }, {
          codec: 'aac',
          samepleRate: 48000,
          channelNum: 2
        }, {
          codec: 'ac3',
          samepleRate: 48000,
          channelNum: 2
        }, {
          codec: 'nellymoser',
          samepleRate: 48000,
          channelNum: 2
        }
      ],
      video: {
        format: [{
            codec: 'h264',
            profile: 'constrained-baseline'
          }, {
            codec: 'h264',
            profile: 'high'
          }, {
            codec: 'vp8'
          }, {
            codec: 'h265'
          }, {
            codec: 'vp9'
          }
        ],
        parameters: {
          resolution: [/*'0.75x', '0.667x',*/ '0.5x', '0.333x', '0.25x', 'xga', 'svga', 'vga'/*, 'hvga', 'cif', 'sif', 'qcif'*/],
          framerate: [5, 15, 24, 30, 48, 60],
          bitrate: ['1.6x', '1.4x', '1.2x', '0.8x', '0.6x', '0.4x'],
          keyFrameInterval: [100, 30, 5, 2, 1]
        }
      }
    },
    transcoding: {
      audio: true,
      video: {
        format: true,
        parameters: false
      }
    },
    notifying: {
      participantActivities: true,
      streamChange: true
    },
    sip: !!oldConfig.sip ? oldConfig.sip : false
  };

  // load local configurated roles definition.
  for (var r in global.config.conference.roles) {
    var role_def = {
      role: r,
      publish: !!global.config.conference.roles[r].publish ? {type: ['webrtc', 'streaming'], media: {audio: true, video: true}} : false,
      subscribe: !!global.config.conference.roles[r].subscribe ? {type: ['webrtc'], media: {audio: true, video: true}} : false,
      text: !!global.config.conference.roles[r].text,
      manage: !!global.config.conference.roles[r].manage
    };
    if (typeof global.config.conference.roles[r].publish === 'object') {
      role_def.publish.media.audio = !!global.config.conference.roles[r].publish.audio;
      role_def.publish.media.video = !!global.config.conference.roles[r].publish.video;
    }
    if (typeof global.config.conference.roles[r].subscribe === 'object') {
      role_def.subscribe.media.audio = !!global.config.conference.roles[r].subscribe.audio;
      role_def.subscribe.media.video = !!global.config.conference.roles[r].subscribe.video;
    }
    if (!!global.config.conference.roles[r].record) {
      role_def.subscribe || (role_def.subscribe = {type: [], media:{audio: true, video: true}});
      role_def.subscribe.type.push('recording');
    }
    if (!!global.config.conference.roles[r].addExternalOutput) {
      role_def.subscribe || (role_def.subscribe = {type: [], media:{audio: true, video: true}});
      role_def.subscribe.type.push('streaming');
    }
    config.roles.push(role_def);
  }

  if (config.sip) {
    config.roles.push({
      role: 'sip',
      publish: {type: ['sip'], media: {audio: true, video: true}},
      subscribe: {type: ['sip'], media: {audio: true, video: true}},
      text: false,
      manage: false
    });
  }

  //translate view definition.
  var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720}
  };
  var resolutionName2StandardBitrateKbps = {
    'cif': 100,
    'vga': 800,
    'svga': 1000,
    'xga': 1200,
    'r640x360': 600,
    'hd720p': 2000,
    'sif': 200,
    'hvga': 500,
    'r480x360': 600,
    'qcif': 100,
    'r192x144': 100,
    'hd1080p': 4000,
    'uhd_4k': 16000,
    'r360x360': 500,
    'r480x480': 700,
    'r720x720': 800
  };
  var qualityLevel2Factor = {
    'best_quality': 1.4,
    'better_quality': 1.2,
    'standart': 1.0,
    'better_speed': 0.8,
    'best_speed': 0.6
  };
  function getResolution(old_resolution) {
     return resolutionName2Value[old_resolution] || {width: 640, height: 480};
  }

  function getBitrate(old_quality_level, old_resolution) {
    var standard_bitrate = resolutionName2StandardBitrateKbps[old_resolution] || 0.8;
    var mul_factor = qualityLevel2Factor[old_quality_level] || 1.0;
    return standard_bitrate * mul_factor;
  }

  for (var v in oldConfig.views) {
    var old_view = oldConfig.views[v].mediaMixing;
    config.views.push({
      label: v,
      audio: {
        format: {
          codec: 'opus',
          sampleRate: 48000,
          channelNum: 2
        },
        vad: (!!old_view.video && old_view.video.avCoordinated) ? {detectInterval: 1000} : false
      },
      video: (!!old_view.video ? {
        format: {
          codec: 'h264',
          profile: 'high'
        },
        parameters: {
          resolution: getResolution(old_view.video.resolution),
          framerate: 30,
          bitrate: getBitrate(old_view.video.quality_level, old_view.video.resolution),
          keyFrameInterval: 100
        },
        maxInput: old_view.video.maxInput,
        bgColor: old_view.video.bkColor,
        layout: {
          crop: old_view.video.crop,
          setRegionEffect: 'insert',
          templates: old_view.video.layout
        }
      } : false)
    });
  }

  return config;
};

var calcResolution = (x, baseResolution) => {
  switch (x) {
    case '0.75x':
      return {width: Math.floor(baseResolution.width * 3 / 4), height: Math.floor(baseResolution.height * 3 / 4)};
    case '0.667x':
      return {width: Math.floor(baseResolution.width * 2 / 3), height: Math.floor(baseResolution.height * 2 / 3)};
    case '0.5x':
      return {width: Math.floor(baseResolution.width / 2), height: Math.floor(baseResolution.height / 2)};
    case '0.333x':
      return {width: Math.floor(baseResolution.width / 3), height: Math.floor(baseResolution.height / 3)};
    case '0.25x':
      return {width: Math.floor(baseResolution.width / 4), height: Math.floor(baseResolution.height / 4)};
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
    default:
      return {width: 65536, height: 65536};
  }
};

var calcBitrate = (x, baseBitrate) => {
  return Number(x.substring(0, x.length - 1)) * baseBitrate;
};

/*
 * Definitions:
 *
 *
   *    object(Role):: {
   *      role: string(RoleName),
   *      publish: {
   *        type: ['webrtc' | 'streaming'],
   *        media: {
   *          audio: true | false,
   *          video: true | false
   *        }
   *      } | false,
   *      subscribe: {
   *        type: ['webrtc' | 'streaming' | 'recording'],
   *        media: {
   *          audio: true | false,
   *          video: true | false
   *        }
   *      } | false,
   *      text: 'to-peer' | 'to-all' | false,
   *      manage: true | false
   *    }
   *
   *    object(AudioFormat):: {
   *      codec: 'opus' | 'pcma' | 'pcmu' | 'aac' | 'ac3' | 'nellymoser',
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
   *    object(Region):: {
   *      id: string(RegionID),
   *      shape: 'rectangle' | 'circle',
   *      area: object(Rectangle):: {
   *        left: number(LeftRatio),
   *        top: nmber(TopRatio),
   *        width: number(WidthRatio),
   *        height: number(HeightRatio)
   *      } | object(Circle):: {
   *        centerW: number(CenterWRatio),
   *        centerH: number(CenterHRatio),
   *        radius: number(RadiusWRatio)
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
      current_publication_count = 0,
      roomController,
      accessController;

  /*
   * {
   *  _id: string(RoomID),
   *  name: string(RoomName),
   *  publishLimit: number(PublishLimit),
   *  userLimit: number(UserLimit),
   *  avatar: string(AvatarStorageURL),
   *  roles: [object(Role)],
   *  views: [
   *    object(MixedViewSettings):: {
   *      label: string(ViewLabel),
   *      audio: {
   *        format: object(AudioFormat),
   *        vad: {
   *          detectInterval: number(Milliseconds)
   *        } | false
   *      },
   *      video: {
   *        format: object(VideoFormat),
   *        parameters: {
   *          resolution: object(Resolution),
   *          framerate: number(FramerateFPS),
   *          bitrate: number(Kbps),
   *          keyFrameInterval: number(Seconds)
   *        }
   *        maxInput: number(MaxInput),
   *        bkColor: 'black' | 'white' | 'blue' | 'green',
   *        layout: {
   *          fillInEffect: 'crop' | 'margin' | 'stretching',
   *          setRegionEffect: 'exchange' | 'insert',
   *          templates: [
   *            object(LayoutTemplate):: {
   *              primary: string(RegionID),
   *              regions: [object(Region)]
   *            }
   *          ]
   *        }
   *      }
   *  ],
   *  mediaIn: {
   *    audio: [object(AudioFormat)] | false,
   *    video: [object(VideoFormat)] | false
   *  },
   *  mediaOut: {
   *    audio: [object(AudioFormat)] | false,
   *    video: {
   *      format: [object(VideoFormat)],
   *      parameters: {
   *        resolution: ['0.75x' | '0.667x' | '0.5x' | '0.333x' | '0.25x' | 'xga' | 'svga' | 'vga' | 'hvga' | 'cif' | 'sif' | 'qcif'],
   *        framerate: [5 | 15 | 24 | 30 | 48 | 60],
   *        bitrate: ['1.6x' | '1.4x' | '1.2x' | '0.8x' | '0.6x' | '0.4x'],
   *        keyFrameInterval: [100 | 30 | 5 | 2 | 1]
   *      }
   *    } | false
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
   *         }
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
   *             bitrate: [number(Kbps] | string(Multiple + 'x') | undefined,
   *             keyFrameRate: [number(Seconds)] | undefined
   *           } | undefined
   *         }
   *       } | undefined
   *     },
   *     info: object(PublicationInfo):: {
   *       owner: string(ParticipantId),
   *       type: 'webrtc' | 'streaming' | 'sip',
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
   *       type: 'webrtc' | 'streaming' | 'recording' | 'sip'
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
          sendMsgTo(participantId, 'progress', {id, sessionId, status: 'error', data: err_msg});
          accessController && accessController.terminate(sessionId).catch((e) => {
            log.info('Exception:', e.message ? e.message : e);
          });
        });
    } else if (direction === 'out') {
      return addSubscription(sessionId, sessionInfo.locality, sessionInfo.media, sessionInfo.info
        ).catch((err) => {
          var err_msg = (err.message ? err.message : err);
          log.info('Exception:', err_msg);
          sendMsgTo(participantId, 'progress', {id: sessionId, status: 'error', data: err_msg});
          accessController && accessController.terminate(sessionId).catch((e) => {
            log.info('Exception:', e.message ? e.message : e);
          });
        });
    } else {
      log.info('Unknown session direction:', direction);
    }
  };

  var onSessionAborted = (participantId, sessionId, direction, reason) => {
    log.debug('onSessionAborted, participantId:', participantId, 'sessionId:', sessionId, 'direction:', direction, 'reason:', reason);
    if (direction === 'in') {
      current_publication_count -= 1;
      if (reason !== 'Participant terminate') {
        sendMsgTo(participantId, 'progress', {id: sessionId, status: 'error', data: reason});
      }

      if (streams[sessionId]) {
        removeStream(participantId, sessionId)
          .catch((e) => {
            var err_msg = (err.message ? err.message : err);
            log.info(err_msg);
          });
      }
    } else if (direction === 'out') {
      removeSubscription(sessionId)
       .catch((e) => {
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
      return dbAccess.getRoomConfig(roomId)
        .then(function(config) {
            //log.debug('initializing room:', roomId, 'got config:', JSON.stringify(config));
            //FIXME: To refine the configuration data structure and remove the following translating logic later.
            //room_config = config;
            room_config = translateOldRoomConfig(config);
            //log.debug('room_config:', JSON.stringify(room_config));

            room_config.internalConnProtocol = global.config.internal.protocol;

            if (room_config.userLimit === 0) {
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
                    var mixed_stream_info = {
                      id: mixed_stream_id,
                      type: 'mixed',
                      media: {
                        audio: (viewSettings.audio && room_config.mediaOut.audio) ? {
                          format: viewSettings.audio.format,
                          optional: {
                            format: room_config.mediaOut.audio
                          }
                        } : undefined,
                        video: (viewSettings.video && room_config.mediaOut.video) ? {
                          format: viewSettings.video.format,
                          optional: {
                            format: room_config.mediaOut.video.format,
                            parameters: {
                              resolution: room_config.mediaOut.video.parameters.resolution.map((x) => {return calcResolution(x, viewSettings.video.parameters.resolution)}).filter((reso) => {return reso.width < viewSettings.video.parameters.resolution.width && reso.height < viewSettings.video.parameters.resolution.height;}),
                              framerate: room_config.mediaOut.video.parameters.framerate.filter((x) => {return x < viewSettings.video.parameters.framerate;}),
                              bitrate: room_config.mediaOut.video.parameters.bitrate.map((x) => {calcBitrate(x, viewSettings.video.parameters.bitrate)}),
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
                        layout: []
                      }
                    };

                    streams[mixed_stream_id] = mixed_stream_info;
                    log.debug('Mixed stream info:', mixed_stream_info);
                    room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: mixed_stream_id, status: 'add', data: mixed_stream_info});
                  });

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

  var destroyRoom = function(force) {
    var terminate = function () {
      if (room_id) {
        accessController && accessController.destroy();
        accessController = undefined;
        roomController && roomController.destroy();
        roomController = undefined;
        streams = {};
        room_id = undefined;
      }
      process.exit();
    }

    if (!force) {
      setTimeout(function() {
        if (Object.keys(participants).length === 0) {
          log.info('Empty room ', room_id, '. Deleting it');
          terminate();
        }
      }, 6 * 1000);
    } else {
      terminate();
    }
  };

  const sendMsgTo = function(to, msg, data) {
    if (participants[to]) {
      rpcReq.sendMsg(participants[to].getPortal(), to, msg, data)
        .catch(function(reason) {
          log.debug('sendMsg fail:', reason);
        });
    } else {
      log.warn('Can not send message to:', to);
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
      room_config.notifying.participantActivities && sendMsg('room', 'all', 'participant', {action: 'leave', data: left_user});
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
    return addStream(id, pubInfo.locality, pubInfo.media, {owner: owner, type: 'sip'});
  };

  const constructMediaInfo = (media) => {
    var result = {};

    if (media.audio) {
      result.audio = {
        status: 'active',
        format: {codec: media.audio.codec}
      };

      media.audio.source && (result.audio.source = media.audio.source);
      media.audio.sampleRate && (result.audio.format.sampleRate = media.audio.sampleRate);
      media.audio.channelNum && (result.audio.format.channelNum = media.audio.channelNum);

      if (room_config.transcoding.audio) {
        result.audio.optional = {format: []};
        room_config.mediaOut.audio.forEach((x) => {
          if ((x.codec !== result.audio.format.codec)
              || (x.sampleRate !== result.audio.format.sampleRate)
              || (x.channelNum !== result.audio.format.channelNum)) {
            result.audio.optional.format.push(x);
          }
        });
      }
    }

    if (media.video) {
      result.video = {
        status: 'active',
        format: {codec: media.video.codec}
      };

      media.video.source && (result.video.source = media.video.source);
      media.video.profile && (result.video.format.profile = media.video.profile);
      //media.video.resolution && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.resolution = media.video.resolution);
      //media.video.framerate && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.framerate = media.video.framerate);
      //media.video.bitrate && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.resolution = media.video.bitrate);
      //media.video.keyFrameInterval && (result.video.parameters || (result.video.parameters = {})) && (result.video.parameters.resolution = media.video.keyFrameInterval);

      if (room_config.transcoding.video && room_config.transcoding.video.format) {
        result.video.optional = {format: []};
        room_config.mediaOut.video.format.forEach((x) => {
          if ((x.codec !== result.video.format.codec)
              || (x.profile !== result.video.format.profile)) {
            result.video.optional.format.push(x);
          }
        });
      }
    }

    return result;
  };

  const addStream = (id, locality, media, info) => {
    return new Promise((resolve, reject) => {
      roomController && roomController.publish(info.owner, id, locality, media, function() {
        if (participants[info.owner]) {
          var st = {
            id: id,
            type: 'forward',
            media: constructMediaInfo(media),
            info: info
          };
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
      roomController && roomController.unpublish(participantId, streamId);
      if (streams[streamId]) {
        delete streams[streamId];
        setTimeout(() => {
          room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {id: streamId, status: 'remove'});
        }, 10);
      }
      resolve('ok');
    });
  };

  const addSIPSubscription = (id, owner, subDesc) => {
    return addSubscription(id, subDesc.locality, subDesc.media, {owner: owner, type: 'sip'});
  };

  const addSubscription = (id, locality, media, info) => {
    return new Promise((resolve, reject) => {
      roomController && roomController.subscribe(info.owner, id, locality, media, function() {
        if (participants[info.owner]) {
          var su = {
            id: id,
            locality: locality,
            media: media,
            info: info
          };
          subscriptions[id] = su;
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

  that.join = function(roomId, participantInfo, callback) {
    log.debug('participant:', participantInfo, 'join room:', roomId);
    var permission;
    return initRoom(roomId)
      .then(function() {
        log.debug('room_config.userLimit:', room_config.userLimit, 'current users count:', Object.keys(participants).length);
        if (room_config.userLimit > 0 && (Object.keys(participants).length >= room_config.userLimit)) {
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
          subscribe: my_role_def[0].subscribe,
          text: my_role_def[0].text,
          manage: my_role_def[0].manage
        };

        return addParticipant(participantInfo, permission);
      }).then(function() {
        var current_participants = [],
            current_streams = [];

        for (var participant_id in participants) {
          current_participants.push(participants[participant_id].getInfo());
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

    return accessController.participantLeave(participantId)
      .then((result) => {
        return removeParticipant(participantId);
      }).then((result) => {
        callback('callback', 'ok');
        if (Object.keys(participants).length === 0) {
          destroyRoom(false);
        }
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

    if ((pubInfo.media.audio && !participants[participantId].isPublishPermitted('audio', pubInfo.type))
        || (pubInfo.media.video && !participants[participantId].isPublishPermitted('video', pubInfo.type))) {
      return callback('callback', 'error', 'unauthorized');
    }

    if (room_config.publishLimit >= 0 && (room_config.publishLimit <= current_publication_count)) {
      return callback('callback', 'error', 'Too many publications');
    }

    if (streams[streamId]) {
      return callback('callback', 'error', 'Stream exists');
    }

    if (pubInfo.type === 'sip') {
      return addSIPStream(participantId, streamId, pubInfo)
      .then((result) => {
        current_publication_count += 1;
        callback('callback', result);
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
    } else {
      return accessController.initiate(participantId, streamId, 'in', participants[participantId].getOrigin(), pubInfo)
      .then((result) => {
        current_publication_count += 1;
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
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    } else {
      return accessController.terminate(streamId)
        .then((result) => {
          log.debug('accessController.terminate result:', result);
          return removeStream(participantId, streamId);
        }, (e) => {
          return Promise.reject('Failed in terminating session');
        })
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    }
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

    if ((subDesc.media.audio && !participants[participantId].isSubscribePermitted('audio', subDesc.type))
        || (subDesc.media.video && !participants[participantId].isSubscribePermitted('video', subDesc.type))) {
      return callback('callback', 'error', 'unauthorized');
    }

    if (subscriptions[subscriptionId]) {
      return callback('callback', 'error', 'Subscription exists');
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
      return accessController.initiate(participantId, subscriptionId, 'out', participants[participantId].getOrigin(), subDesc)
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
    } else {
      return accessController.terminate(subscriptionId)
        .then((result) => {
          return removeSubscription(subscriptionId);
        }, (e) => {
          return Promise.reject('Failed in terminating session');
        })
        .then((result) => {
          callback('callback', result);
        })
        .catch((e) => {
          callback('callback', 'error', e.message ? e.message : e);
        });
    }
  };

  const mix = function(streamId, toView) {
    return new Promise((resolve, reject) => {
      roomController.mix(streamId, toView, function() {
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
        status = (muted ? 'active' : 'inactive');

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
        room_config.notifying.streamChange && sendMsg('room', 'all', 'stream', {status: 'update', id: streamId, data: {field: track + '.status', value: status}});
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

    if ((streams[streamId].info.owner !== participantId)
        && !participants[participantId].isManagePermitted()) {
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
        callback('callback', 'error', err.message ? err.message : err);
      });
  };

  const updateSubscription = (subscriptionId, update, callback) => {
    var old_su = subscriptions[subscriptionId];
        new_su = JSON.parse(JSON.stringify(old_su));

    var effective = false;
    if (update.audio) {
      new_su.media.audio = (new_su.media.audio || {});
      if (update.audio.from && (update.audio.from !== new_su.media.audio.from)) {
        new_su.media.audio.from = update.audio.from;
        effective = true;
      }
    }

    if (update.video) {
      new_su.media.video = (new_su.media.video || {});
      if (update.video.from && (update.video.from !== new_su.media.video.from)) {
        new_su.media.video.from = update.video.from;
        effective = true;
      }

      /*
      if (update.video.spec) {
        new_su.media.video.spec = (new_su.media.video.spec || {});
        if (update.video.spec.resolution && ((update.video.spec.resolution.width !== new_su.media.video.spec.resolution.width) || (update.video.spec.resolution.height !== new_su.media.video.spec.resolution.height))) {
          new_su.media.video.spec.resolution = update.video.spec.resolution;
          effective = true;
        }
        if (update.video.spec.framerate && (update.video.spec.framerate !== new_su.media.video.spec.framerate)) {
          new_su.media.video.spec.framerate = update.video.spec.framerate;
          effective = true;
        }
        if (update.video.spec.bitrate && (update.video.spec.bitrate !== new_su.media.video.spec.bitrate)) {
          new_su.media.video.spec.bitrate = update.video.spec.bitrate;
          effective = true;
        }
        if (update.video.spec.keyFrameInterval && (update.video.spec.keyFrameInterval !== new_su.media.video.spec.keyFrameInterval)) {
          new_su.media.video.spec.keyFrameInterval = update.video.spec.keyFrameInterval;
          effective = true;
        }
      }
      */
    }

    if (!effective) {
      return Promise.resolve('ok');
    } else {
      return removeSubscription(old_su.info.owner, subscriptionId)
        .then((result) => {
          return addSubscription(subscriptionId, new_su.locality, new_su.media, new_su.info);
        }).catch((err) => {
          log.info('Update subscription failed:', err.message ? err.message : err);
          log.info('And is recovering the previous subscription:', JSON.stringify(old_su));
          return addSubscription(subscriptionId, old_su.locality, old_su.media, old_su.info);
        });
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

    return accessController.setMute(subscriptionId, track, muted);
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
        && !participants[participantId].isManagePermitted()) {
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

  that.setPermission = (participantId, anotherParticipantId, authorities, callback) => {
    if (participants[participantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    if (!participants[participantId].isManagePermitted()) {
      return callback('callback', 'error', 'unauthorized');
    }

    if (participants[anotherParticipantId] === undefined) {
      return callback('callback', 'error', 'Target participant does NOT exist')
    }

    return Promise.all(
      authorities.map((auth) => {
        return participants[anotherParticipantId].setPermission(auth.operation, auth.field, auth.value);
      })
    ).then(() => {
      callback('callback', 'ok');
    }, (err) => {
      callback('callback', 'error', err.message ? err.message : err);
    });
  };

  that.text = function(fromParticipantId, toParticipantId, msg, callback) {
    if (participants[fromParticipantId] === undefined) {
      return callback('callback', 'error', 'Participant has not joined');
    }

    if (!participants[fromParticipantId].isTextPermitted(toParticipantId)) {
      return callback('callback', 'error', 'unauthorized');
    }

    if ((data.to !== 'all') && (participants[toParticipantId] === undefined)) {
      return callback('callback', 'error', 'Target participant does NOT exist')
    }

    sendMsg(fromParticipantId, toParticipantId, 'custom_message', {from: fromParticipantId, to: toParticipantId, data: msg});
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
        sendMsg('room', 'all', 'stream', {status: 'update', id: streamId, data: {field: 'video.layout', value: layout}});
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
      roomController.setPrimary(activeParticipantId, view);
      callback('callback', 'ok');
    } else {
      log.info('onAudioActiveness, room does not exist');
      callback('callback', 'error', 'room is not in service');
    }
  };

  //The following interfaces are reserved to serve nuve
  that.getUsers = function(callback) {
    log.debug('getUsers, room_id:', room_id);
    var result = [];
    for (var participant_id in participants) {
      result.push(participants[participant_id].getInfo());
    }
    callback('callback', result);
  };

  that.deleteUser = function(user, callback) {
    log.debug('deleteUser', user);
    var deleted = null;
    for (var participant_id in participants) {
      if (participant_id === user) {
        deleted = participants[participant_id];
        rpcReq.dropUser(participants[participant_id].getPortal(), participant_id)
          .then(function(result) {
            return removeParticipant(participant_id);
          }).catch(function(reason) {
            log.debug('dropUser fail:', reason);
          });
        break;
      }
    }

    callback('callback', deleted);
  };

  that.destroy = function(callback) {
    log.info('Destroy room:', room_id);
    destroyRoom(true);
    callback('callback', 'Success');
  };

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
    setPermission: conference.setPermission,

    //rpc from access nodes.
    onSessionProgress: conference.onSessionProgress,
    onSessionAudit: conference.onSessionAudit,

    // rpc from audio nodes.
    onAudioActiveParticipant: conference.onAudioActiveness,

    // rpc from video nodes.
    onVideoLayoutChange: conference.onVideoLayoutChange,

    //rpc from OAM component.
    deleteUser: conference.deleteUser,
    getUsers: conference.getUsers,
    destroy: conference.destroy
  };

  that.onFaultDetected = conference.onFaultDetected;

  return that;
};

