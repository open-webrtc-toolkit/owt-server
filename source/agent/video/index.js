// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

const logger = require('../logger').logger;

// Logger
const log = logger.getLogger('VideoNode');

const {InternalConnectionRouter} = require('./internalConnectionRouter');
const router = new InternalConnectionRouter(global.config.internal);

// Setup GRPC server
var createGrpcInterface = require('./grpcAdapter').createGrpcInterface;
var enableGRPC = global.config.agent.enable_grpc || false;

var EventEmitter = require('events').EventEmitter;

var useHardware = global.config.video.hardwareAccelerated,
    gaccPluginEnabled = global.config.video.enableBetterHEVCQuality || false,
    MFE_timeout = global.config.video.MFE_timeout || 0,
    supported_codecs = global.config.video.codecs;

var VideoMixer, VideoTranscoder;
try {
    if (useHardware) {
        VideoMixer = require('../videoMixer_msdk/build/Release/videoMixer-msdk');
        VideoTranscoder = require('../videoTranscoder_msdk/build/Release/videoTranscoder-msdk');
    } else {
        VideoMixer = require('../videoMixer_sw/build/Release/videoMixer-sw');
        VideoTranscoder = require('../videoTranscoder_sw/build/Release/videoTranscoder-sw');
    }
} catch (e) {
    log.error(e);
    process.exit(-2);
}

const {VMixer} = require('./vmixer');
const {VTranscoder} = require('./vtranscoder');

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    const that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    };
    let processor = undefined;

    // For GRPC notifications
    var streamingEmitter = new EventEmitter();

    that.init = function (service, config, belongTo, controller, mixView, callback) {
        if (processor === undefined) {
            if (service === 'mixing') {
                processor = new VMixer(rpcClient, clusterWorkerIP, VideoMixer,
                    router, streamingEmitter);
                processor.initialize(config, belongTo, controller, mixView, callback);
                that.__proto__ = processor;
            } else if (service === 'transcoding') {
                processor = new VTranscoder(rpcClient, clusterWorkerIP, VideoTranscoder,
                    router, streamingEmitter);
                processor.initialize(config.motionFactor, controller, callback);
                that.__proto__ = processor;
            } else {
                log.error('Unknown service type to init a video node:', service);
                callback('callback', 'error', 'Unknown service type to init a video node.');
            }
        } else {
            log.error('Repetitive initialization, service:', service);
            callback('callback', 'error', 'Repetitive initialization.');
        }
    };

    that.deinit = function () {
        processor && processor.deinit();
        processor = undefined;
    };

    that.getInternalAddress = function(callback) {
        const ip = global.config.internal.ip_address;
        const port = router.internalPort;
        callback('callback', {ip, port});
    };
    if (enableGRPC) {
        // Export GRPC interface.
        return createGrpcInterface(that, streamingEmitter);
    }

    return that;
};
