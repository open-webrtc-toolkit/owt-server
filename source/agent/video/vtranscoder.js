// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

const log = require('../logger').logger.getLogger('VTranscoderJS');

const {
    calcDefaultBitrate,
    resolution2String,
    isResolutionEqual,
} = require('../mediaUtil');

const isUnspecified = (value) => (value === undefined || value === 'unspecified');

const useHardware = global.config.video.hardwareAccelerated;
const gaccPluginEnabled = global.config.video.enableBetterHEVCQuality || false;
const MFE_timeout = global.config.video.MFE_timeout || 0;
const supported_codecs = global.config.video.codecs;

function VTranscoder(rpcClient, clusterIP, VideoTranscoder, router) {
    var that = {},
        engine,
        controller,

        drawing_text_tmr,

        motion_factor = 1.0,
        default_resolution = {width: 0, height: 0},
        default_framerate = 30,
        default_kfi = 1000,

        input_id = undefined,
        input_conn = undefined,

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
                       resolution: {width: Number(Width), height: Number(Height)},
                       framerate: Number(FPS),
                       bitrate: Number(Kbps),
                       kfi: Number(KeyFrameIntervalSeconds),
                       dispatcher: Dispather,
                       }}*/
        outputs = {},

        /*{ConnectionID: {video: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {};

    var setInput = function (stream_id, codec, options, on_ok, on_error) {
        // FIXME: filter profile setting for native layer
        codec = codec.indexOf('h264') > -1 ? 'h264' : codec;
        if (engine) {
            var conn = router.getOrCreateRemoteSource({
                id: stream_id,
                ip: options.ip,
                port: options.port
            });
            if (engine.setInput(stream_id, codec, conn)) {
                input_id = stream_id;
                input_conn = conn;
                log.debug('setInput ok, stream_id:', stream_id, 'codec:', codec, 'options:', options);
                on_ok(stream_id);
            } else {
                router.destroyRemoteSource(stream_id);
                on_error('Failed in setting input to video-engine.');
            }
        } else {
            on_error('Video-transcoder engine is not ready.');
        }
    };

    var unsetInput = function () {
        if (input_id) {
            engine.unsetInput(input_id);
            router.destroyRemoteSource(input_id);
            input_id = undefined;
            input_conn = undefined;
        }
    };

    var addOutput = function (codec, resolution, framerate, bitrate, keyFrameInterval, on_ok, on_error) {
        log.debug('addOutput: codec', codec, 'resolution:', resolution, 'framerate:', framerate, 'bitrate:', bitrate, 'keyFrameInterval:', keyFrameInterval);
        if (engine) {
            var stream_id = Math.random() * 1000000000000000000 + '';
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(stream_id,
                                 codec,
                                 resolution2String(resolution),
                                 framerate,
                                 bitrate,
                                 keyFrameInterval,
                                 dispatcher)) {
                outputs[stream_id] = {codec: codec,
                                      resolution: resolution,
                                      framerate: framerate,
                                      bitrate: bitrate,
                                      kfi: keyFrameInterval,
                                      dispatcher: dispatcher,
                                      connections: {}};
                router.addLocalSource(stream_id, 'vtrasncoder', dispatcher.source());
                log.debug('addOutput ok, stream_id:', stream_id);
                on_ok(stream_id);
            } else {
                on_error('Failed in yielding output');
            }
        } else {
            on_error('Video-transcoder engine is not ready.');
        }
    };

    var removeOutput = function (stream_id) {
        if (outputs[stream_id]) {
            var output = outputs[stream_id];
            for (var connectionId in output.connections) {
                 output.dispatcher.removeDestination('video', output.connections[connectionId].receiver());
                 output.connections[connectionId].close();
                 delete output.connections[connectionId];
            }
            engine.removeOutput(stream_id);
            router.removeLocalSource(stream_id);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    var getOutput = function (stream_id) {
        if (outputs[stream_id]) {
            return {
                id: stream_id,
                resolution: outputs[stream_id].resolution,
                framerate: outputs[stream_id].framerate,
                bitrate: outputs[stream_id].bitrate,
                keyFrameInterval: outputs[stream_id].kfi
            };
        } else {
            return undefined;
        }
    };

    that.initialize = function (motionFactor, ctrlr, callback) {
        log.debug('to initialize video transcoder');
        var config = {
            'hardware': useHardware,
            'simulcast': false,
            'crop': false,
            'gaccplugin': gaccPluginEnabled,
            'MFE_timeout': MFE_timeout
        };

        controller = ctrlr;

        engine = new VideoTranscoder(config);

        motion_factor = (motionFactor || 1.0);
        log.debug('Video transcoding engine init OK, supported_codecs:', supported_codecs);
        callback('callback', {codecs: supported_codecs});
    };

    that.deinit = function () {
        if (drawing_text_tmr) {
          clearTimeout(drawing_text_tmr);
          drawing_text_tmr = undefined;
        }

        for (var stream_id in outputs) {
            removeOutput(stream_id);
        }
        input_id && unsetInput(input_id);

        if (engine) {
            engine.close();
            engine = undefined;
        }
    };

    that.close = function() {
        if (engine) {
            that.deinit();
        }
    };

    that.onFaultDetected = function (message) {
        if (message.purpose === 'conference' && controller) {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Conference controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
                that.deinit();
                process.exit();
            }
        }
    };

    that.generate = function (codec, resolution, framerate, bitrate, keyFrameInterval, callback) {
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'framerate:', framerate, 'bitrate:', bitrate, 'keyFrameInterval:', keyFrameInterval);
        codec = (codec || supported_codecs.encode[0]).toLowerCase();
        resolution = (isUnspecified(resolution) ? default_resolution : resolution);
        framerate = (isUnspecified(framerate) ? default_framerate : framerate);
        var bitrate_factor = (isUnspecified(bitrate) ? 1.0
            : (typeof bitrate === 'string' ? Number(bitrate.replace('x', '')) : 0));
        bitrate = (bitrate_factor ? calcDefaultBitrate(codec, resolution, framerate, motion_factor) * bitrate_factor : bitrate);
        keyFrameInterval = (isUnspecified(keyFrameInterval) ? default_kfi : keyFrameInterval);

        for (var stream_id in outputs) {
            if (outputs[stream_id].codec === codec &&
                isResolutionEqual(outputs[stream_id].resolution, resolution) &&
                outputs[stream_id].framerate === framerate &&
                outputs[stream_id].bitrate === bitrate &&
                outputs[stream_id].kfi === keyFrameInterval) {
                callback('callback', getOutput(stream_id));
                return;
            }
        }

        if (supported_codecs.encode.findIndex((c) => (c.toLowerCase() === codec)) < 0) {
            log.error('Not supported codec: '+codec);
            callback('callback', 'error', 'Not supported codec.');
            return;
        }

        addOutput(codec, resolution, framerate, bitrate, keyFrameInterval, function (stream_id) {
            callback('callback', getOutput(stream_id));
        }, function (error_reason) {
            log.error(error_reason);
            callback('callback', 'error', error_reason);
        });
    };

    that.degenerate = function (stream_id) {
        removeOutput(stream_id);
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        log.debug('publish, stream_id:', stream_id, 'stream_type:', stream_type, 'options:', options);
        if (stream_type !== 'internal') {
            return callback('callback', 'error', 'can not publish a stream to video engine through a non-internal connection');
        }

        if (options.video === undefined || options.video.codec === undefined) {
            return callback('callback', 'error', 'can not publish a stream to video engine without proper video codec');
        }

        if (input_id === undefined) {
            setInput(stream_id, options.video.codec, options, function () {
                callback('callback', {ip: clusterIP});
            }, function (error_reason) {
                log.error(error_reason);
                callback('callback', 'error', error_reason);
            });
        } else if (input_id === stream_id) {
            callback('callback', {ip: clusterIP});
        } else {
            log.error('input already exists.');
            callback('callback', 'error', 'input already exists.');
        }
    };

    that.unpublish = function (stream_id) {
        log.debug('unpublish, stream_id:', stream_id);
        unsetInput(stream_id);
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        callback('callback', 'ok');
    };

    that.unsubscribe = function (connectionId) {
        log.debug('unsubscribe, connectionId:', connectionId);
    };

    that.linkup = function (connectionId, audio_stream_id, callback) {
        callback('callback', 'ok');
    };

    that.cutoff = function (connectionId) {
        log.debug('cutoff, connectionId:', connectionId);
    };

    that.forceKeyFrame = function (stream_id) {
        if (outputs[stream_id] && engine) {
            engine.forceKeyFrame(stream_id);
        }
    };

    that.drawText = function (textSpec, duration) {
        log.debug('drawText, textSpec:', textSpec, 'duration:', duration);
        if (drawing_text_tmr) {
          clearTimeout(drawing_text_tmr);
          drawing_text_tmr = undefined;
        }

        engine.drawText(textSpec);

        if (duration > 0) {
            drawing_text_tmr = setTimeout(() => {engine.clearText();}, duration);
        }
    };

    return that;
};

exports.VTranscoder = VTranscoder;
