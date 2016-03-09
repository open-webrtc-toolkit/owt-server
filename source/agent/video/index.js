/*global require, exports, GLOBAL*/
'use strict';

var woogeenInternalIO = require('../../../bindings/internalIO/build/Release/internalIO');
var InternalIn = woogeenInternalIO.InternalIn;
var InternalOut = woogeenInternalIO.InternalOut;
var MediaFrameMulticaster = require('../../../bindings/mediaFrameMulticaster/build/Release/mediaFrameMulticaster').MediaFrameMulticaster;
var VideoMixer = require('../../../bindings/videoMixer/build/Release/videoMixer').VideoMixer;

var logger = require('../../../common/logger').logger;

// Logger
var log = logger.getLogger('VideoNode');

var VideoResolutionMap = { // definition adopted from VideoLayout.h
    cif:      ['cif'],
    vga:      ['vga', 'sif'],
    svga:     ['svga', 'sif'],
    xga:      ['xga', 'sif'],
    r640x360: ['r640x360'],
    hd720p:   ['hd720p', 'vga', 'r640x360'],
    sif:      ['sif'],
    hvga:     ['hvga'],
    r480x360: ['r480x360', 'sif'],
    qcif:     ['qcif'],
    r192x144: ['r192x144'],
    hd1080p:  ['hd1080p', 'hd720p', 'svga', 'vga', 'r640x360'],
    uhd_4k:   ['uhd_4k', 'hd1080p', 'hd720p', 'svga', 'vga', 'r640x360'],
    r720x720: ['r720x720']
};

function calculateResolutions(rootResolution, useMultistreaming) {
    var base = VideoResolutionMap[rootResolution.toLowerCase()];
    if (!base) return [];
    if (!useMultistreaming) return [base[0]];
    return base.map(function (r) {
        return r;
    });
}

var VideoEngine = function () {
    var that = {},
        engine,

        supported_codecs = [],
        supported_resolutions = [],

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
                       resolution: 'cif' | 'vga' | 'svga' | 'xga' | 'hd720p' | 'sif' | 'hvga' | 'r640x360' | 'r480x360' | 'qcif' | 'r192x144' | 'hd1080p' | 'uhd_4k' | 'r720x720' ...,
                       framerate: Number(1~120) | undefined,
                       dispatcher: Dispather,
                       subscriptions: {SubscriptionID: InternalOut}
                       }}*/
        outputs = {},

        /*{StreamID : InternalIn}*/
        inputs = {},
        maxInputNum = 0,

        useHardware = GLOBAL.config.erizo.hardwareAccelerated,
        openh264Enabled = GLOBAL.config.erizo.openh264Enabled;

    var addInput = function (stream_id, codec, protocol, on_ok, on_error) {
        if (engine) {
            var conn = new InternalIn(protocol);
            if (engine.addInput(stream_id, codec, conn)) {
                inputs[stream_id] = conn;
                log.debug('addInput ok, stream_id:', stream_id, 'codec:', codec, 'protocol:', protocol);
                on_ok(stream_id);
            } else {
                conn.close();
                on_error('Failed in adding input to video-engine.');
            }
        } else {
            on_error('Video-mixer engine is not ready.');
        }
    };

    var removeInput = function (stream_id) {
        if (inputs[stream_id]) {
            engine.removeInput(stream_id);
            inputs[stream_id].close();
            delete inputs[stream_id];
        }
    };

    var addOutput = function (codec, resolution, on_ok, on_error) {
        if (engine) {
            for (var id in outputs) {
                if (outputs[id].codec === codec && outputs[id].resolution === resolution) {
                    return on_ok(id);
                }
            }

            var stream_id = Math.random() * 1000000000000000000 + '';
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(stream_id, codec, resolution, dispatcher)) {
                outputs[stream_id] = {codec: codec,
                                      resolution: resolution,
                                      dispatcher: dispatcher,
                                      subscriptions: {}};
                log.debug('addOutput ok, stream_id:', stream_id);
                on_ok(stream_id);
            } else {
                on_error('Failed in adding output to video-engine');
            }
        } else {
            on_error('Video-mixer engine is not ready.');
        }
    };

    var removeOutput = function (stream_id) {
        if (outputs[stream_id]) {
            var output = outputs[stream_id];
            for (var subscription_id in output.subscriptions) {
                 output.dispatcher.removeDestination('video', output.subscriptions[subscription_id]);
                 output.subscriptions[subscription_id].close();
                 delete output.subscriptions[subscription_id];
            }
            engine.removeOutput(stream_id);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    that.initEngine = function (videoConfig, callback) {
        log.debug('initEngine, videoConfig:', videoConfig);
        var config = {
            'hardware': useHardware,
            'maxinput': videoConfig.maxInput,
            'bitrate': videoConfig.bitrate,
            'resolution': videoConfig.resolution,
            'backgroundcolor': videoConfig.bkColor,
            'layout': videoConfig.layout,
            'simulcast': videoConfig.multistreaming,
            'crop': videoConfig.crop
        };

        engine = new VideoMixer(JSON.stringify(config));
        maxInputNum = videoConfig.maxInput;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['vp8'];
        if (useHardware || openh264Enabled) {
            supported_codecs.push('h264');
        }

        supported_resolutions = calculateResolutions(videoConfig.resolution, videoConfig.multistreaming);

        log.info('Video engine init OK');
        callback('callback', {codecs: supported_codecs, resolutions: supported_resolutions});
    };

    that.deinit = function () {
        for (var stream_id in outputs) {
            removeOutput(stream_id);
        }

        for (var stream_id in inputs) {
            removeInput(stream_id);
        }

        engine.close();
        engine = undefined;
    };

    that.generate = function (codec, resolution, callback) {
        codec = codec.toLowerCase();
        resolution = resolution.toLowerCase();
        log.debug('generate, codec:', codec, 'resolution:', resolution);
        resolution = resolution || supported_resolutions[0];
        for (var stream_id in outputs) {
            if (outputs[stream_id].codec === codec && outputs[stream_id].resolution === resolution) {
                callback('callback', stream_id);
                return;
            }
        }

        if (supported_codecs.indexOf(codec) === -1 || supported_resolutions.indexOf(resolution) === -1) {
            log.error('Not supported codec or resolution:'+codec+', '+resolution);
            callback('callback', 'error', 'Not supported codec or resolution.');
            return;
        }

        addOutput(codec, resolution, function (stream_id) {
            callback('callback', stream_id);
        }, function (error_reason) {
            log.error(error_reason);
            callback('callback', 'error', error_reason);
        });
    };

    that.degenerate = function (stream_id) {
        removeOutput(stream_id);
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        if (inputs[stream_id] === undefined) {
            log.debug('inputs.length:', Object.keys(inputs).length, 'maxInputNum:', maxInputNum);
            if (Object.keys(inputs).length < maxInputNum) {
                addInput(stream_id, options.video_codec, options.protocol, function () {
                    callback('callback', inputs[stream_id].getListeningPort());
                }, function (error_reason) {
                    log.error(error_reason);
                    callback('callback', 'error', error_reason);
                });
            } else {
                log.error('Too many inputs in video-engine.');
                callback('callback', 'error', 'Too many inputs in video-engine.');
            }
        } else {
            log.error('Stream:'+stream_id+' already input in video-engine.');
            callback('callback', 'error', 'Stream:'+stream_id+' already input in video-engine.');
        }
    };

    that.unpublish = function (stream_id) {
        removeInput(stream_id);
    };

    that.subscribe = function (subscription_id, subscription_type, audio_stream_id, video_stream_id, options, callback) {
        if (outputs[video_stream_id]) {
            var conn = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
            outputs[video_stream_id].dispatcher.addDestination('video', conn);
            outputs[video_stream_id].subscriptions[subscription_id] = conn;
            callback('callback', 'ok');
        } else {
            log.error('Stream does not exist!', audio_stream_id);
            callback('callback', 'error', 'Stream does not exist!');
        }
    };

    that.unsubscribe = function (subscription_id) {
        for (var stream_id in outputs) {
            if (outputs[stream_id].subscriptions[subscription_id]) {
                outputs[stream_id].dispatcher.removeDestination('video', outputs[stream_id].subscriptions[subscription_id]);
                delete outputs[stream_id].subscriptions[subscription_id];
            }
        }
    };

    that.setLayoutTemplates = function (templates, callback) {
        //TODO: implement the layout processor in node.js layer.
    };

    that.setPrimary = function (stream_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id]) {
            engine.setPrimary(stream_id);
            callback('callback', 'ok');
        }
        callback('callback', 'error', 'Invalid input stream_id.');
    };

    that.setRegion = function (stream_id, region_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id]) {
            engine.setRegion(stream_id, region_id);
            callback('callback', 'ok');
        }
        callback('callback', 'error', 'Invalid input stream_id.');
    };

    that.getRegion = function (stream_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id]) {
            var region_id = engine.getRegion(stream_id);
            callback('callback', region_id);
        }
        callback('callback', 'error', 'Invalid input stream_id.');
    };

    return that;
};

exports.VideoNode = function () {
    var that = new VideoEngine();

    that.init = function (service, config, callback) {
        if (service === 'mixing') {
            that.initEngine(config, callback);
        } else if (service === 'transcoding') {
            var videoConfig = {maxInput: 1,
                               bitrate: 0,
                               resolution: config.resolution,
                               bkColor: 'black',
                               multistreaming: true,
                               layout: [{region: [{id: '1', left: 0, top: 0, relativesize: 1}]}]};
            that.initEngine(videoConfig, callback);
        } else {
            log.error('Unknown service type to init a video node:', service);
            callback('callback', 'error', 'Unknown service type to init a video node.');
        }
    };

    return that;
};
