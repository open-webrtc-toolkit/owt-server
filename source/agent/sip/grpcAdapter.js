// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const unpackOption = require('./grpcTools').unpackOption;
const packNotification = require('./grpcTools').packNotification;

// Create GRPC interface for sip agent
function createGrpcInterface(controller, streamingEmitter) {
  const that = {};

  // GRPC export
  that.grpcInterface = {
    init: function (call, callback) {
      const req = call.request;
      const options = {
        sip_server: req.sipServer,
        sip_user: req.sipUser,
        sip_passwd: req.sipPassword,
        room_id: req.room
      };
      controller.init(options, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: data});
        }
      });
    },
    keepAlive: function (call, callback) {
      controller.keepAlive((n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: code});
        }
      });
    },
    clean: function (call, callback) {
      controller.clean();
      callback(null, {});
    },
    drop: function (call, callback) {
      const req = call.request;
      controller.drop(req.id, req.room);
      callback(null, {});
    },
    makeCall: function (call, callback) {
      const req = call.request;
      controller.makeCall(
          req.peerUri, req.mediaIn, req.mediaOut, req.controller,
          (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: code});
        }
      });
    },
    endCall: function (call, callback) {
      controller.endCall(call.request.id, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: code});
        }
      });
    },
    linkup: function (call, callback) {
      const req = call.request;
      controller.linkup(
        req.id, req.from,
        (n, code, data) => {
          if (code === 'error') {
            callback(new Error(data), null);
          } else {
            callback(null, {message: data});
          }
        });
    },
    cutoff: function (call, callback) {
      controller.cutoff(call.request.id, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {});
        }
      });
    },
    listenToNotifications: function (call, callback) {
      streamingEmitter.on('notification', (notification) => {
        const progress = packNotification({
          type: 'sip',
          name: notification.name,
          data: notification.data,
        });
        call.write(progress);
      });
      streamingEmitter.on('close', () => {
        call.end();
      });
    },
    getInternalAddress: function (call, callback) {
      controller.getInternalAddress((n, code, data) => {
          if (code === 'error') {
            callback(new Error(data), null);
          } else {
            callback(null, code);
          }
        });
    },
  };

  return that;
}

exports.createGrpcInterface = createGrpcInterface;
