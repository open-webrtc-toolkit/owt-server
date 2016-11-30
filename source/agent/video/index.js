/*global require, module, GLOBAL, process*/
'use strict';

var internalIO = require('./internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;
var MediaFrameMulticaster = require('./mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

var logger = require('./logger').logger;

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
    r720x720: ['r720x720', 'r480x480', 'r360x360']
};

function calculateResolutions(rootResolution, useMultistreaming) {
    var base = VideoResolutionMap[rootResolution.toLowerCase()];
    if (!base) return [];
    if (!useMultistreaming) return [base[0]];
    return base.map(function (r) {
        return r;
    });
}

module.exports = function (rpcClient) {
    var that = {},
        engine,
        belong_to,
        controller,

        supported_codecs = [],
        supported_resolutions = [],
        // Map to enum QualityLevel defined in MediaFramePipeline.h
        supported_qualities = {
            bestquality: 0,
            betterquality: 1,
            standard: 2,
            betterspeed: 3,
            bestspeed: 4
        };

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
                       resolution: 'cif' | 'vga' | 'svga' | 'xga' | 'hd720p' | 'sif' | 'hvga' | 'r640x360' | 'r480x360' | 'qcif' | 'r192x144' | 'hd1080p' | 'uhd_4k' | 'r720x720' ...,
                       framerate: Number(1~120) | undefined,
                       dispatcher: Dispather,
                       }}*/
        outputs = {},

        /*{StreamID : InternalIn}*/
        inputs = {},
        maxInputNum = 0,

        useHardware = GLOBAL.config.video.hardwareAccelerated,
        openh264Enabled = GLOBAL.config.video.openh264Enabled,

        /*{ConnectionID: {video: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {};

    var VideoMixer;
    try {
        if (useHardware) {
            VideoMixer = require('./videoMixer_hw/build/Release/videoMixer-hw');
        } else {
            VideoMixer = require('./videoMixer_sw/build/Release/videoMixer-sw');
        }
    } catch (e) {
        log.error(e);
        process.exit(1);
    }

    var addInput = function (stream_id, codec, protocol, avatar, on_ok, on_error) {
        if (engine) {
            if (!useHardware && !openh264Enabled && codec === 'h264') {
                on_error('Codec ' + codec + ' is not supported by video engine.');
            } else {
                var conn = new InternalIn(protocol, GLOBAL.config.internal.minport, GLOBAL.config.internal.maxport);

                // Use default avatar if it is not set
                avatar = avatar || GLOBAL.config.avatar.location;
                if (engine.addInput(stream_id, codec, conn, avatar)) {
                    inputs[stream_id] = conn;
                    log.debug('addInput ok, stream_id:', stream_id, 'codec:', codec, 'protocol:', protocol);
                    on_ok(stream_id);
                    notifyLayoutChange();
                } else {
                    conn.close();
                    on_error('Failed in adding input to video-engine.');
                }
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

            notifyLayoutChange();
        }
    };

    var addOutput = function (codec, resolution, quality, on_ok, on_error) {
        if (engine) {
            for (var id in outputs) {
                if (outputs[id].codec === codec && outputs[id].resolution === resolution && outputs[id].quality === quality) {
                    return on_ok(id);
                }
            }

            log.debug('addOutput: codec', codec, 'resolution', resolution, 'quality', quality);

            var stream_id = Math.random() * 1000000000000000000 + '';
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(stream_id, codec,
                                 ((!resolution || resolution === 'unspecified' || VideoResolutionMap[resolution] === undefined) ? supported_resolutions[0] : resolution),
                                 quality,
                                 dispatcher)) {
                outputs[stream_id] = {codec: codec,
                                      resolution: resolution,
                                      quality: quality,
                                      dispatcher: dispatcher,
                                      connections: {}};
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
            for (var connectionId in output.connections) {
                 output.dispatcher.removeDestination('video', output.connections[connectionId]);
                 output.connections[connectionId].close();
                 delete output.connections[connectionId];
            }
            engine.removeOutput(stream_id);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    var getSortedRegions = function () {
        // TODO: implement the layout processor in node.js layer.
        var regions = (engine.getCurrentRegions() || []);
        // Sort by region ID to save work with overlap in client side.
        regions.sort(function(regionA, regionB) {
            return regionA.id.localeCompare(regionB.id);
        });

        return regions;
    };

    var notifyLayoutChange = function () {
        if (controller) {
            var regions = getSortedRegions();
            rpcClient.remoteCall(
                controller,
                'onVideoLayoutChange',
                [belong_to, regions]);
        }
    };

    var initEngine = function (videoConfig, belongTo, layoutcontroller, callback) {
        log.debug('initEngine, videoConfig:', videoConfig);
        var config = {
            'hardware': useHardware,
            'maxinput': videoConfig.maxInput,
            'resolution': videoConfig.resolution,
            'backgroundcolor': videoConfig.bkColor,
            'layout': videoConfig.layout,
            'simulcast': videoConfig.multistreaming,
            'crop': videoConfig.crop
        };

        engine = new VideoMixer(JSON.stringify(config));
        belong_to = belongTo;
        controller = layoutcontroller;
        maxInputNum = videoConfig.maxInput;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['vp8'];
        if (useHardware || openh264Enabled) {
            supported_codecs.push('h264');
        }

        if (useHardware) {
            supported_codecs.push('h265');
        }

        supported_codecs.push('vp9');

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

        if (engine) {
            engine.close();
            engine = undefined;
        }
    };

    that.generate = function (codec, resolution, quality, callback) {
        codec = codec || supported_codecs[0];
        codec = codec.toLowerCase();
        resolution = ((!resolution || resolution === 'unspecified' || VideoResolutionMap[resolution] === undefined) ? supported_resolutions[0] : resolution);
        resolution = resolution.toLowerCase();
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'qualityLevel:', quality);

        // Map to qualityLevel enum
        var qualityLevel = supported_qualities[quality.toLowerCase()];
        if (qualityLevel === undefined) {
            qualityLevel = 2; // use 'standard' if not found
            log.warn('Not supported quality:', quality, ', use level:', qualityLevel);
        }

        for (var stream_id in outputs) {
            if (outputs[stream_id].codec === codec &&
                outputs[stream_id].resolution === resolution &&
                outputs[stream_id].quality === qualityLevel) {
                callback('callback', stream_id);
                return;
            }
        }

        if (supported_codecs.indexOf(codec) === -1 || supported_resolutions.indexOf(resolution) === -1) {
            log.error('Not supported codec or resolution:'+codec+', '+resolution);
            callback('callback', 'error', 'Not supported codec or resolution.');
            return;
        }

        addOutput(codec, resolution, qualityLevel, function (stream_id) {
            callback('callback', stream_id);
        }, function (error_reason) {
            log.error(error_reason);
            callback('callback', 'error', error_reason);
        });
    };

    that.degenerate = function (stream_id) {
        removeOutput(stream_id);
    };

    that.setInputActive = function (stream_id, active, callback) {
        if (inputs[stream_id]) {
            engine.setInputActive(stream_id, !!active);
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'No stream:' + stream_id);
        }
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        log.debug('publish, stream_id:', stream_id, 'stream_type:', stream_type, 'options:', options);
        if (stream_type !== 'internal') {
            return callback('callback', 'error', 'can not publish a stream to video engine through a non-internal connection');
        }

        if (options.video === undefined || options.video.codec === undefined) {
            return callback('callback', 'error', 'can not publish a stream to video engine without proper video codec');
        }

        if (inputs[stream_id] === undefined) {
            log.debug('publish 1, inputs.length:', Object.keys(inputs).length, 'maxInputNum:', maxInputNum);
            if (Object.keys(inputs).length < maxInputNum) {
                addInput(stream_id, options.video.codec, options.protocol, options.avatar, function () {
                    callback('callback', {ip: that.clusterIP, port: inputs[stream_id].getListeningPort()});
                }, function (error_reason) {
                    log.error(error_reason);
                    callback('callback', 'error', error_reason);
                });
            } else {
                log.error('Too many inputs in video-engine.');
                callback('callback', 'error', 'Too many inputs in video-engine.');
            }
        } else {
            callback('callback', {ip: that.clusterIP, port: inputs[stream_id].getListeningPort()});
        }
    };

    that.unpublish = function (stream_id) {
        log.debug('unpublish, stream_id:', stream_id);
        removeInput(stream_id);
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connectionType !== 'internal') {
            return callback('callback', 'error', 'can not subscribe a stream from video engine through a non-internal connection');
        }

        if (connections[connectionId] === undefined) {
            connections[connectionId] = {videoFrom: undefined,
                                         connection: new InternalOut(options.protocol, options.dest_ip, options.dest_port)
                                        };
        }
        callback('callback', 'ok');
    };

    that.unsubscribe = function (connectionId) {
        log.debug('unsubscribe, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].videoFrom) {
            if (outputs[connections[connectionId].videoFrom]) {
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection);
            }
            //connections[connectionId].connection.close();
            delete connections[connectionId];
        }
    };

    that.linkup = function (connectionId, audio_stream_id, video_stream_id, callback) {
        log.debug('linkup, connectionId:', connectionId, 'video_stream_id:', video_stream_id);
        if (connections[connectionId] === undefined) {
            return callback('callback', 'error', 'connection does not exist');
        }

        if (outputs[video_stream_id]) {
            outputs[video_stream_id].dispatcher.addDestination('video', connections[connectionId].connection);
            connections[connectionId].videoFrom = video_stream_id;
            callback('callback', 'ok');
        } else {
            log.error('Stream does not exist!', video_stream_id);
            callback('callback', 'error', 'Stream does not exist!');
        }
    };

    that.cutoff = function (connectionId) {
        log.debug('cutoff, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].videoFrom) {
            if (outputs[connections[connectionId].videoFrom]) {
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection);
            }
            connections[connectionId].videoFrom = undefined;
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
            notifyLayoutChange();
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.setRegion = function (stream_id, region_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id]) {
            var old_region_id = engine.getRegion(stream_id);
            if (old_region_id !== '' && (old_region_id === region_id)) {
                callback('callback', 'ok');
            } else if (engine.setRegion(stream_id, region_id)) {
                callback('callback', 'ok');
                notifyLayoutChange();
            } else {
                callback('callback', 'error', 'Invalid region_id.');
            }
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.getRegion = function (stream_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id]) {
            var region_id = engine.getRegion(stream_id);
            callback('callback', region_id);
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.init = function (service, config, belongTo, controller, callback) {
        if (service === 'mixing') {
            initEngine(config, belongTo, controller, callback);
        } else if (service === 'transcoding') {
            var videoConfig = {maxInput: 1,
                               bitrate: 0,
                               resolution: (config.resolution === 'unknown' || VideoResolutionMap[config.resolution] === undefined) ? 'hd1080p' : config.resolution,
                               bkColor: 'black',
                               multistreaming: true,
                               layout: [{region: [{id: '1', left: 0, top: 0, relativesize: 1}]}],
                               crop: false};
            initEngine(videoConfig, belongTo, controller, callback);
        } else {
            log.error('Unknown service type to init a video node:', service);
            callback('callback', 'error', 'Unknown service type to init a video node.');
        }
    };

    that.onFaultDetected = function (message) {
        if (message.purpose === 'session') {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Session controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
                that.deinit();
                process.exit();
            }
        }
    };

    return that;
};
