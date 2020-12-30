// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';
var validateReq = require('./restReqValidator').validate;
var rpc = require('./rpc/rpc');
var log = require('./logger').logger.getLogger('RequestHandler');
var cluster_name = ((global.config || {}).cluster || {}).name || 'owt-cluster';
var e = require('./errors');

const scheduleAgent = function(agentName, tokenCode, origin, attempts, callback) {
    var keepTrying = true;

    var tryFetchingAgent = function(attempts) {
        if (attempts <= 0) {
            return callback('timeout');
        }

        rpc.callRpc(cluster_name, 'schedule', [ agentName, tokenCode, origin, 60 * 1000 ], { callback : function(result) {
            if (result === 'timeout' || result === 'error') {
                if (keepTrying) {
                    log.info('Faild in scheduling ', agentName, ' tokenCode:', tokenCode, ', keep trying.');
                    setTimeout(function() { tryFetchingAgent(attempts - (result === 'timeout' ? 4 : 1)); }, 1000);
                }
            } else {
                callback(result.info);
                keepTrying = false;
            }
        } });
    };

    tryFetchingAgent(attempts);
};

exports.schedulePortal = function(tokenCode, origin, callback) {
    return scheduleAgent('portal', tokenCode, origin, 60, callback);
};

exports.scheduleQuicAgent = async function(tokenCode, origin, callback) {
    // QUIC agent may not be enabled in all deployment environment, at least before CI for QUIC SDK is ready. Attempts less times to reduce response time for creating tokens.
    return scheduleAgent('quic', tokenCode, origin, 5, callback);
};

const scheduleRoomController = function (roomId) {
  return new Promise((resolve, reject) => {
      rpc.callRpc(cluster_name, 'schedule', ['conference', roomId, 'preference', 30 * 1000], {callback: function (result) {
          if (result === 'timeout' || result === 'error') {
              reject('Error in scheduling room controller');
          } else {
              rpc.callRpc(result.id, 'getNode', [{room: roomId, task: roomId}], {callback: function (result) {
                  if (result === 'timeout' || result === 'error') {
                      reject('Error in scheduling room controller');
                  } else {
                      resolve(result);
                  }
              }});
          }
      }});
  });
};

const getRoomController = (roomId) => {
  return new Promise((resolve, reject) => {
    rpc.callRpc(cluster_name, 'getScheduled', ['conference', roomId], {callback: (agent) => {
      if (agent === 'timeout' || agent === 'error') {
        reject('Room is inactive');
      } else {
        rpc.callRpc(agent, 'queryNode', [roomId], {callback: (node) => {
          if (node === 'timeout' || node === 'error') {
            reject('Room is inactive');
          } else {
            resolve(node);
          }
        }});
      }
  }});
  });
};

const idPattern = /^[0-9a-zA-Z\-_]+$/;
const validateId = (type, id) => {
  if ((typeof id === 'string') && idPattern.test(id)) {
    return Promise.resolve(id);
  } else {
    return Promise.reject('Invalid ' + type);
  }
};

exports.deleteRoom = function (roomId, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'destroy', [], {callback: function (result) {
        callback(result);
      }});
    }).catch(() => {
      callback([]);
    });
};

exports.getParticipantsInRoom = function (roomId, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'getParticipants', [], {callback: function (participants) {
        log.debug('Got participants:', participants);
        if (participants === 'timeout' || participants === 'error') {
          callback('error');
        } else {
          callback(participants);
        }
      }});
    }).catch((err) => {
      log.info('getParticipantsInRoom failed, reason:', err.message ? err.message : err);
      if (err === 'Room is inactive') {
        callback([]);
      } else {
        callback('error');
      }
    });
};

exports.updateParticipant = function (roomId, participant, updates, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Participant ID', participant);
    }).then((ok) => {
      return validateReq('participant-update', updates);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'controlParticipant', [participant, updates], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.deleteParticipant = function (roomId, participant, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Participant ID', participant);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'dropParticipant', [participant], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.getStreamsInRoom = function (roomId, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'getStreams', [], {callback: function (streams) {
        log.debug('Got streams:', streams);
        if (streams === 'timeout' || streams === 'error') {
          callback('error');
        } else {
          callback(streams);
        }
      }});
    }).catch((err) => {
      log.info('getStreamsInRoom failed, reason:', err.message ? err.message : err);
      if (err === 'Room is inactive') {
        callback([]);
      } else {
        callback('error');
      }
    });
};

exports.addStreamingIn = function (roomId, pubReq, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateReq('streamingIn-req', pubReq);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      return controller;
    }, (e) => {
      if (e === 'Room is inactive') {
        return scheduleRoomController(roomId);
      } else {
        return Promise.reject('Validation failed');
      }
    }).then((controller) => {
      rpc.callRpc(controller, 'addStreamingIn', [roomId, pubReq], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }}, 90 * 1000);
    }).catch((err) => {
      callback('error');
    });
};

exports.controlStream = function (roomId, stream, cmds, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Stream ID', stream);
    }).then((ok) => {
      return validateReq('stream-update', cmds);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'controlStream', [stream, cmds], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.deleteStream = function (roomId, stream, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Stream ID', stream);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'deleteStream', [stream], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.getSubscriptionsInRoom = function (roomId, type, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      if (type === 'streaming' || type === 'recording' || type === 'webrtc' || type === 'analytics') {
        return Promise.resolve('ok');
      } else {
        return Promise.reject('Invalid type');
      }
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'getSubscriptions', [type], {callback: function (subscriptions) {
        log.debug('Got subscriptions:', subscriptions);
        if (subscriptions === 'timeout' || subscriptions === 'error') {
          callback('error');
        } else {
          callback(subscriptions);
        }
      }});
    }).catch((err) => {
      log.info('getSubscriptionsInRoom failed, reason:', err.message ? err.message : err);
      if (err === 'Room is inactive') {
        callback([]);
      } else {
        callback('error');
      }
    });
};

exports.addServerSideSubscription = function (roomId, subReq, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateReq('serverSideSub-req', subReq);
    }).then((ok) => {
      getRoomController(roomId)
        .catch((e) => {
          if (e === 'Room is inactive') {
            return scheduleRoomController(roomId);
          } else {
            return Promise.reject('Failed to get room controller');
          }
        })
        .then((controller) => {
          rpc.callRpc(controller, 'addServerSideSubscription', [roomId, subReq], {callback: function (result, edata) {
            if (result === 'error' || result === 'timeout') {
              log.info('RPC fail', result, edata);
              callback('error', new e.CloudError('RPC:addServerSideSubscription failed'));
            } else {
              callback(result);
            }
          }});
        })
        .catch((err) => {
          callback('error', new e.CloudError(err && err.message ? err.message : ''));
        });
    }).catch((err) => {
      var msg = err && err.message ? err.message : '';
      callback('error', new e.BadRequestError(msg));
    });
};

exports.controlSubscription = function (roomId, subId, cmds, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Subscription ID', subId);
    }).then((ok) => {
      return validateReq('subscription-update', cmds);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'controlSubscription', [subId, cmds], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.deleteSubscription = function (roomId, subId, type, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Subscription ID', subId);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'deleteSubscription', [subId, type], {callback: function (result, edata) {
        if (result === 'error' || result === 'timeout') {
          log.info('RPC fail', result, edata);
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      callback('error');
    });
};

exports.getSipCallsInRoom = function (roomId, callback) {
  return validateId('Room ID', roomId)
    .then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'getSipCalls', [], {callback: function (sipCalls) {
        log.debug('Got subscriptions:', sipCalls);
        if (sipCalls === 'timeout' || sipCalls === 'error') {
          callback('error');
        } else {
          callback(sipCalls);
        }
      }});
    }).catch((err) => {
      log.info('getSipCallsInRoom failed, reason:', err.message ? err.message : err);
      if (err === 'Room is inactive') {
        callback([]);
      } else {
        callback('error');
      }
    });
};

exports.addSipCall = function (roomId, options, callback) {
  log.debug('addSipCall, roomId:', roomId, 'options:', options);
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateReq('sipcall-add', options);
    }).then((ok) => {
      getRoomController(roomId)
        .catch((e) => {
          if (e === 'Room is inactive') {
            return scheduleRoomController(roomId);
          } else {
            return Promise.reject('Failed to get room controller');
          }
        })
        .then((controller) => {
          rpc.callRpc(controller, 'makeSipCall', [roomId, options], {callback: function (result, edata) {
            if (result === 'error' || result === 'timeout') {
              log.debug('RPC fail', result, edata);
              callback('error', new e.CloudError('RPC:makeSipCall failed'));
            } else {
              callback(result);
            }
          }}, 90 * 1000);
        })
        .catch((err) => {
          callback('error', new e.CloudError(err && err.message ? err.message : ''));
        });
    }).catch((err) => {
      var msg = err && err.message ? err.message : '';
      callback('error', new e.BadRequestError(msg));
    });
};

exports.updateSipCall = function (roomId, sipCallId, cmds, callback) {
  log.debug('updateSipCall, roomId:', roomId, 'sipCallId:', sipCallId, 'cmds:', cmds);
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Sip call ID', sipCallId);
    }).then((ok) => {
      return validateReq('sipcall-update', cmds);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'controlSipCall', [sipCallId, cmds], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      var msg = err && err.message ? err.message : '';
      callback('error', new e.BadRequestError(msg));
    });
};

exports.deleteSipCall = function (roomId, sipCallId, callback) {
  log.debug('deleteSipCall, roomId:', roomId, 'sipCallId:', sipCallId);
  return validateId('Room ID', roomId)
    .then((ok) => {
      return validateId('Sip call ID', sipCallId);
    }).then((ok) => {
      return getRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'endSipCall', [sipCallId], {callback: function (result) {
        if (result === 'error' || result === 'timeout') {
          callback('error');
        } else {
          callback(result);
        }
      }});
    }).catch((err) => {
      var msg = err && err.message ? err.message : '';
      callback('error', new e.BadRequestError(msg));
    });
};

exports.notifySipPortal = function (changeType, room, callback) {
        var arg = {
            type: changeType,
            room_id: room._id,
            sip: room.sip
        };
        rpc.callRpc('sip-portal', 'handleSipUpdate', [arg], {callback: function (r) {
        if (r === 'timeout' || r === 'error') {
            callback('Fail');
        } else {
            callback('Success');
        }
    }});
};


