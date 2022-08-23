// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const unpackOption = require('./grpcTools').unpackOption;
const packNotification = require('./grpcTools').packNotification;

// Create GRPC interface for audio agent
function createGrpcInterface(controller, streamingEmitter) {
  const that = {};

  // GRPC export
  that.grpcInterface = {
    init: function (call, callback) {
      const req = call.request;
      const option = unpackOption(req.type, req.option);
      controller.init(
          option.service, {}, req.id, option.controller, option.label,
          (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, code);
        }
      });
    },
    deinit: function (call, callback) {
      controller.deinit();
      callback(null, {});
    },
    generate: function (call, callback) {
      const req = call.request;
      const forWhom = req.id;
      const codec = req.media.audio.format.codec;
      controller.generate(forWhom, codec, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {id: code});
        }
      });
    },
    degenerate: function (call, callback) {
      controller.degenerate(call.request.id);
      callback(null, {});
    },
    setInputActive: function (call, callback) {
      const req = call.request;
      controller.setInputActive(req.id, req.active, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {});
        }
      });
    },
    /* Replace publish/subscribe
    addInput: function (call, callback) {
      const req = call.request;
      const option = unpackOption(req.type, req.option);
      const pubOpt = {
        audio: {codec: req.media.audio.format.codec},
        publisher: option.owner
      }
      controller.publish(req.id, req.type, pubOpt, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: data});
        }
      });
    },
    */
    removeInput: function (call, callback) {
      controller.unpublish(call.request.id);
      callback(null, {});
    },
    enableVad: function (call, callback) {
      controller.enableVAD(call.request.periodMs);
      callback(null, {});
    },
    resetVad: function (call, callback) {
      controller.resetVAD();
      callback(null, {});
    },
    publish: function (call, callback) {
      const req = call.request;
      const option = unpackOption(req.type, req.option);
      controller.publish(req.id, req.type, option, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {id: req.id});
        }
      });
    },
    unpublish: function (call, callback) {
      controller.unpublish(call.request.id);
      callback(null, {});
    },
    subscribe: function (call, callback) {
      const req = call.request;
      const option = unpackOption(req.type, req.option);
      controller.subscribe(req.id, req.type, option, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {id: req.id});
        }
      });
    },
    unsubscribe: function (call, callback) {
      controller.unsubscribe(call.request.id);
      callback(null, {});
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
      controller.cutoff(call.request.id);
      callback(null, {});
    },
    listenToNotifications: function (call, callback) {
      streamingEmitter.on('notification', (notification) => {
        const progress = packNotification({
          type: 'audio',
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
