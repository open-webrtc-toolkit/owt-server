// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const unpackOption = require('./grpcTools').unpackOption;

// Create GRPC interface for video agent
function createGrpcInterface(controller, streamingEmitter) {
  const that = {};

  // GRPC export
  that.grpcInterface = {
    init: function (call, callback) {
      const req = call.request;
      const option = unpackOption(req.type, req.option);
      controller.init(
          option.service, option.init, req.id, option.controller, option.label,
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
      const codec = req.media.video.format.codec;
      const pars = req.media.video.parameters;
      controller.generate(codec, pars.resolution, pars.framerate,
          pars.bitrate, pars.keyFrameInterval,
          (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          const reply = {};
          if (code) {
            reply.id = code.id;
            reply.media = {
              video: {
                parameters: code
              }
            }
          }
          callback(null, reply);
        }
      });
    },
    degenerate: function (call, callback) {
      controller.degenerate(call.request.id);
      callback(null, {});
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
    removeInput: function (call, callback) {
      controller.unpublish(call.request.id);
      callback(null, {});
    },
    */
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
    getRegion: function (call, callback) {
      controller.getRegion(call.request.id, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {message: code});
        }
      });
    },
    setRegion: function (call, callback) {
      const req = call.request;
      controller.setRegion(req.streamId, req.regionId, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {});
        }
      });
    },
    setLayout: function (call, callback) {
      const req = call.request;
      controller.setLayout(req.regions, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {regions: code});
        }
      });
    },
    setPrimary: function (call, callback) {
      controller.setPrimary(call.request.id, (n, code, data) => {
        if (code === 'error') {
          callback(new Error(data), null);
        } else {
          callback(null, {});
        }
      });
    },
    forceKeyFrame: function (call, callback) {
      controller.forceKeyFrame(call.request.id);
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
        req.id,
        req.from.audio && req.from.audio.id,
        req.from.video && req.from.video.id,
        req.from.data && req.from.data.id,
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
        const progress = {
          type: 'video',
          name: notification.name,
          data: notification.data,
        };
        call.write(progress);
      });
      streamingEmitter.on('close', () => {
        call.end();
      });
    },
    createInternalConnection: function (call, callback) {
      const req = call.request;
      controller.createInternalConnection(
        req.id,
        req.direction,
        req.internalOpt,
        (n, code, data) => {
          if (code === 'error') {
            callback(new Error(data), null);
          } else {
            callback(null, code);
          }
        });
    },
    destroyInternalConnection: function (call, callback) {
      const req = call.request;
      controller.destroyInternalConnection(
        req.id,
        req.direction,
        (n, code, data) => {
          if (code === 'error') {
            callback(new Error(data), null);
          } else {
            callback(null, {message: data});
          }
        });
    },
  };

  return that;
}

exports.createGrpcInterface = createGrpcInterface;
