/*global require, exports, global*/
'use strict';
var rpc = require('./rpc/rpc');
var log = require('./logger').logger.getLogger('CloudHandler');
var cluster_name = ((global.config || {}).cluster || {}).name || 'woogeen-cluster';

exports.schedulePortal = function (tokenCode, origin, callback) {
    var keepTrying = true;

    var tryFetchingPortal = function (attempts) {
        if (attempts <= 0) {
            return callback('timeout');
        }

        rpc.callRpc(cluster_name, 'schedule', ['portal', tokenCode, origin, 60 * 1000], {callback: function (result) {
            if (result === 'timeout' || result === 'error') {
                if (keepTrying) {
                    log.info('Faild in scheduling portal, tokenCode:', tokenCode, ', keep trying.');
                    setTimeout(function () {tryFetchingPortal(attempts - (result === 'timeout' ? 4 : 1));}, 1000);
                }
            } else {
                callback(result.info);
                keepTrying = false;
            }
        }});
    };

    tryFetchingPortal(60);
};

var scheduleRoomController = function (roomId) {
  return new Promise((resolve, reject) => {
      rpc.callRpc(cluster_name, 'schedule', ['conference', roomId, 'preference', 30 * 1000], {callback: function (result) {
          if (result === 'timeout' || result === 'error') {
              reject('Error in scheduling room controller');
          } else {
              rpc.callRpc(result, 'getNode', [{room: roomId, task: roomId}], {callback: function (result) {
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

var getRoomController = (roomId) => {
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

exports.deleteRoom = function (roomId, callback) {
  return getRoomController(roomId)
    .then((controller) => {
      rpc.callRpc(controller, 'destroy', [], {callback: function (result) {
        callback(result);
      }});
    }).catch(() => {
      callback([]);
    });
};

exports.getParticipantsInRoom = function (roomId, callback) {
  return getRoomController(roomId)
    .then((controller) => {
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
      callback('error');
    });
};

exports.updateParticipant = function (roomId, participant, updates, callback) {
  return getRoomController(roomId)
    .then((controller) => {
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
  return getRoomController(roomId)
    .then((controller) => {
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
  return getRoomController(roomId)
    .then((controller) => {
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
      callback('error');
    });
};

exports.addStreamingIn = function (roomId, pubReq, callback) {
  return getRoomController(roomId)
    .then((controller) => {
      return controller;
    }, (e) => {
      return scheduleRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'addStreamingIn', [roomId, pubReq], {callback: function (result) {
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

exports.controlStream = function (roomId, stream, cmds, callback) {
  return getRoomController(roomId)
    .then((controller) => {
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
  return getRoomController(roomId)
    .then((controller) => {
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
  return getRoomController(roomId)
    .then((controller) => {
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
      callback('error');
    });
};

exports.addServerSideSubscription = function (roomId, subReq, callback) {
  return getRoomController(roomId)
    .then((controller) => {
      return controller;
    }, (e) => {
      return scheduleRoomController(roomId);
    }).then((controller) => {
      rpc.callRpc(controller, 'addServerSideSubscription', [roomId, subReq], {callback: function (result) {
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

exports.controlSubscription = function (roomId, subId, cmds, callback) {
  return getRoomController(roomId)
    .then((controller) => {
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

exports.deleteSubscription = function (roomId, subId, callback) {
  return getRoomController(roomId)
    .then((controller) => {
      rpc.callRpc(controller, 'deleteSubscription', [subId], {callback: function (result) {
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

exports.notifySipPortal = function (changeType, room, callback) {
        var arg = {
            type: changeType,
            room_id: room._id,
            sipInfo: room.sipInfo
        };
        rpc.callRpc('sip-portal', 'handleSipUpdate', [arg], {callback: function (r) {
        if (r === 'timeout' || r === 'error') {
            callback('Fail');
        } else {
            callback('Success');
        }
    }});
};


