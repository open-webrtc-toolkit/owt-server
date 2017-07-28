/*global require, module, global, process*/
'use strict';
require = require('module')._load('./AgentLoader');

var MediaFrameMulticaster = require('./mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('VideoNode');
var InternalConnectionFactory = require('./InternalConnectionFactory');
var internalConnFactory = new InternalConnectionFactory;

const { LayoutProcessor } = require('./layout');

var useHardware = global.config.video.hardwareAccelerated,
    openh264Enabled = global.config.video.openh264Enabled,
    yamiEnabled = global.config.video.yamiEnabled,
    gaccPluginEnabled = global.config.video.enableBetterHEVCQuality;

var VideoMixer, VideoTranscoder;
try {
    if (useHardware && yamiEnabled) {
        VideoMixer = require('./videoMixer_yami/build/Release/videoMixer-yami');
        VideoTranscoder = require('./videoMixer_yami/build/Release/videoTranscoder-yami');
    } else if (useHardware && !yamiEnabled) {
        VideoMixer = require('./videoMixer_msdk/build/Release/videoMixer-msdk');
        VideoTranscoder = require('./videoTranscoder_msdk/build/Release/videoTranscoder-msdk');
    } else {
        VideoMixer = require('./videoMixer_sw/build/Release/videoMixer-sw');
        VideoTranscoder = require('./videoTranscoder_sw/build/Release/videoTranscoder-sw');
    }
} catch (e) {
    log.error(e);
    process.exit(-2);
}

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

// String(streamId) - Number(inputId) Map
class StreamMap {
    constructor(maxInput) {
        this.map = {};
        this.freeIndex = [];
        // Default to 100 if not specified
        if (typeof maxInput !== 'number' || maxInput <= 0) maxInput = 100;
        for (let i = 0; i < maxInput; i++) {
            this.freeIndex.push(i);
        }
        this.freeIndex.reverse();
    }

    // Has the streamId
    has(streamId) {
        return (this.map[streamId] !== undefined);
    }

    // Get input from stream
    get(streamId) {
        if (this.map[streamId] === undefined) return -1;
        return this.map[streamId];
    }

    // Given String streamId, return Number inputId
    add(streamId) {
        if (this.map[streamId] !== undefined) {
            return -1;
        }
        if (this.freeIndex.length > 0) {
            this.map[streamId] = this.freeIndex.pop();
        }
        return this.map[streamId];
    }

    // Remove streamId, return Number inputId
    remove(streamId) {
        var inputId = this.map[streamId]
        if (inputId !== undefined) {
            this.freeIndex.push(inputId);
            delete this.map[streamId];
            return inputId;
        }
        return -1;
    }

    // Return all the streams
    streams() {
        return Object.keys(this.map);
    }

    // Get stream from input
    getStreamFromInput(inputId) {
        for (let streamId in this.map) {
            if (this.map[streamId] === inputId) {
                return streamId;
            }
        }
        return null;
    }
}

function VMixer(rpcClient, clusterIP) {
    var that = {},
        engine,
        layoutProcessor,
        belong_to,
        controller,
        view,
        defaultQuality,

        supported_codecs = [],
        supported_resolutions = [],
        // Map to enum QualityLevel defined in MediaFramePipeline.h
        supported_qualities = {
            bestquality: 0,
            betterquality: 1,
            standard: 2,
            betterspeed: 3,
            bestspeed: 4
        },

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
                       resolution: 'cif' | 'vga' | 'svga' | 'xga' | 'hd720p' | 'sif' | 'hvga' | 'r640x360' | 'r480x360' | 'qcif' | 'r192x144' | 'hd1080p' | 'uhd_4k' | 'r720x720' ...,
                       framerate: Number(1~120) | undefined,
                       dispatcher: Dispather,
                       }}*/
        outputs = {},

        /*{StreamID : InternalIn}*/
        inputs = {},
        maxInputNum = 0,

        /*{ConnectionID: {video: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {};

    // Map streamID {string} to {number}
    var streamMap;

    var addInput = function (stream_id, codec, options, avatar, on_ok, on_error) {
        log.debug('add input', stream_id);
        if (engine) {
            if (!useHardware && !openh264Enabled && codec === 'h264') {
                on_error('Codec ' + codec + ' is not supported by video engine.');
            } else {
                var conn = internalConnFactory.fetch(stream_id, 'in');
                if (!conn) {
                    on_error('Create internal connection failed.');
                    return;
                }
                conn.connect(options);

                // Use default avatar if it is not set
                avatar = avatar || global.config.avatar.location;
                let inputId = streamMap.add(stream_id);
                if (inputId >= 0 && engine.addInput(inputId, codec, conn, avatar)) {
                    layoutProcessor.addInput(inputId);
                    inputs[stream_id] = conn;
                    log.debug('addInput ok, stream_id:', stream_id, 'codec:', codec, 'options:', options);
                    on_ok(stream_id);
                } else {
                    internalConnFactory.destroy(stream_id, 'in');
                    on_error('Failed in adding input to video-engine.');
                }
            }
        } else {
            on_error('Video-mixer engine is not ready.');
        }
    };

    var removeInput = function (stream_id) {
        log.debug('remove input', stream_id);
        if (inputs[stream_id] && streamMap.has(stream_id)) {
            let inputId = streamMap.remove(stream_id);
            engine.removeInput(inputId);
            layoutProcessor.removeInput(inputId);
            internalConnFactory.destroy(stream_id, 'in');
            delete inputs[stream_id];
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
                                 ((!resolution || resolution === 'unspecified' || supported_resolutions.indexOf(resolution) < 0) ? supported_resolutions[0] : resolution),
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
                 output.dispatcher.removeDestination('video', output.connections[connectionId].receiver());
                 output.connections[connectionId].close();
                 delete output.connections[connectionId];
            }
            engine.removeOutput(stream_id);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    that.initialize = function (videoConfig, belongTo, layoutcontroller, mixView, callback) {
        log.debug('initEngine, videoConfig:', videoConfig);
        var config = {
            'hardware': useHardware,
            'maxinput': videoConfig.maxInput,
            'resolution': videoConfig.resolution,
            'backgroundcolor': videoConfig.bkColor,
            'layout': videoConfig.layout,
            'simulcast': videoConfig.multistreaming,
            'crop': videoConfig.crop,
            'gaccplugin': gaccPluginEnabled,
        };

        streamMap = new StreamMap(videoConfig.maxInput);
        engine = new VideoMixer(JSON.stringify(config));
        layoutProcessor = new LayoutProcessor(videoConfig.layout);
        layoutProcessor.on('error', function (e) {
            log.warn('layout error:', e);
        });
        layoutProcessor.on('layoutChange', function (layoutSolution) {
            log.debug('layoutChange', layoutSolution);
            if (typeof engine.updateLayoutSolution === 'function') {
               engine.updateLayoutSolution(layoutSolution);
            } else {
                log.warn('No native method: updateLayoutSolution');
            }

            var streamRegions = layoutSolution.map((inputRegion) => {
                return {
                    streamID: streamMap.getStreamFromInput(inputRegion.input),
                    id: inputRegion.region.id,
                    left: inputRegion.region.left,
                    top: inputRegion.region.top,
                    relativeSize: inputRegion.region.relativeSize
                };
            });
            var layoutChangeArgs = [belong_to, streamRegions, view];
            rpcClient.remoteCall(controller, 'onVideoLayoutChange', layoutChangeArgs);
        });
        belong_to = belongTo;
        controller = layoutcontroller;
        maxInputNum = videoConfig.maxInput;
        view = mixView;
        defaultQuality = videoConfig.quality_level;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['vp8', 'vp9'];
        if (useHardware || openh264Enabled) {
            supported_codecs.push('h264');
        }

        if (useHardware) {
            supported_codecs.push('h265');
        }

        supported_resolutions = calculateResolutions(videoConfig.resolution, videoConfig.multistreaming);

        log.debug('Video engine init OK');
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

    that.close = function() {
        if (engine) {
            that.deinit();
        }
    };

    that.onFaultDetected = function (message) {
        if (message.purpose === 'session' && controller) {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Session controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
                that.deinit();
                process.exit();
            }
        }
    };

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = global.config.internal.minport;
        internalOpt.maxport = global.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: clusterIP, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

    that.generate = function (codec, resolution, quality, callback) {
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'qualityLevel:', quality);
        codec = codec || supported_codecs[0];
        codec = codec.toLowerCase();
        resolution = ((!resolution || resolution === 'unspecified' || supported_resolutions.indexOf(resolution) < 0) ? supported_resolutions[0] : resolution);
        resolution = resolution.toLowerCase();

        // Map to qualityLevel enum
        if (quality === 'unspecified') quality = defaultQuality;
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
        log.debug('degenerate, stream_id:', stream_id);
        removeOutput(stream_id);
    };

    that.setInputActive = function (stream_id, active, callback) {
        log.debug('setInputActive, stream_id:', stream_id, 'active:', active);
        if (inputs[stream_id] && streamMap.has(stream_id)) {
            let inputId = streamMap.get(stream_id);
            engine.setInputActive(inputId, !!active);
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
                addInput(stream_id, options.video.codec, options, options.avatar, function () {
                    callback('callback', {ip: clusterIP, port: inputs[stream_id].getListeningPort()});
                }, function (error_reason) {
                    log.error(error_reason);
                    callback('callback', 'error', error_reason);
                });
            } else {
                log.error('Too many inputs in video-engine.');
                callback('callback', 'error', 'Too many inputs in video-engine.');
            }
        } else {
            callback('callback', {ip: clusterIP, port: inputs[stream_id].getListeningPort()});
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

        var conn = internalConnFactory.fetch(connectionId, 'out');
        if (!conn) {
            callback('callback', 'error', 'Create internal connection failed.');
            return;
        }
        conn.connect(options);

        if (connections[connectionId] === undefined) {
            connections[connectionId] = {videoFrom: undefined,
                                         connection: conn
                                        };
        }
        callback('callback', 'ok');
    };

    that.unsubscribe = function (connectionId) {
        log.debug('unsubscribe, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].videoFrom) {
            if (outputs[connections[connectionId].videoFrom]) {
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection.receiver());
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
            outputs[video_stream_id].dispatcher.addDestination('video', connections[connectionId].connection.receiver());
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
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection.receiver());
            }
            connections[connectionId].videoFrom = undefined;
        }
    };

    that.setLayoutTemplates = function (templates, callback) {
        //TODO: implement the layout processor in node.js layer.
    };

    that.setPrimary = function (stream_id, callback) {
        if (inputs[stream_id] && streamMap.has(stream_id)) {
            let inputId = streamMap.get(stream_id);
            //TODO: re-implement the setPrimary method
            layoutProcessor.setPrimary(inputId);
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.setRegion = function (stream_id, region_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id] && streamMap.has(stream_id)) {
            let inputId = streamMap.get(stream_id);
            if (layoutProcessor.specifyInputRegion(inputId, region_id)) {
                callback('callback', 'ok');
            } else {
                callback('callback', 'error', 'Invalid region_id.');
            }
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.getRegion = function (stream_id, callback) {
        //TODO: implement the layout processor in node.js layer.
        if (inputs[stream_id] && streamMap.has(stream_id)) {
            let inputId = streamMap.get(stream_id);
            let region = layoutProcessor.getRegion(inputId);
            let regionId = region ? region.id : '';
            callback('callback', regionId);
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    return that;
};

function VTranscoder(rpcClient, clusterIP) {
    var that = {},
        engine,
        controller,

        supported_codecs = [],

        input_id = undefined,
        input_conn = undefined,

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
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
        if (engine) {
            if (!useHardware && !openh264Enabled && codec === 'h264') {
                on_error('Codec ' + codec + ' is not supported by video engine.');
            } else {
                var conn = internalConnFactory.fetch(stream_id, 'in');
                if (!conn) {
                    on_error('Create internal connection failed.');
                    return;
                }
                conn.connect(options);

                if (engine.setInput(stream_id, codec, conn)) {
                    input_id = stream_id;
                    input_conn = conn;
                    log.debug('setInput ok, stream_id:', stream_id, 'codec:', codec, 'options:', options);
                    on_ok(stream_id);
                } else {
                    internalConnFactory.destroy(stream_id, 'in');
                    on_error('Failed in setting input to video-engine.');
                }
            }
        } else {
            on_error('Video-transcoder engine is not ready.');
        }
    };

    var unsetInput = function () {
        if (input_id) {
            engine.unsetInput(input_id);
            internalConnFactory.destroy(input_id, 'in');
            input_id = undefined;
            input_conn = undefined;
        }
    };

    var addOutput = function (codec, on_ok, on_error) {
        if (engine) {
            for (var id in outputs) {
                if (outputs[id].codec === codec) {
                    return on_ok(id);
                }
            }

            log.debug('addOutput: codec', codec);

            var stream_id = Math.random() * 1000000000000000000 + '';
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(stream_id, codec, dispatcher)) {
                outputs[stream_id] = {codec: codec,
                                      dispatcher: dispatcher,
                                      connections: {}};
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
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    that.initialize = function (ctrlr, callback) {
        log.debug('to initialize video transcoder');
        var config = {
            'hardware': useHardware,
            'simulcast': false,
            'crop': false,
            'gaccplugin': gaccPluginEnabled
        };

        controller = ctrlr;

        engine = new VideoTranscoder(JSON.stringify(config));

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['vp8', 'vp9'];
        if (useHardware || openh264Enabled) {
            supported_codecs.push('h264');
        }

        if (useHardware) {
            supported_codecs.push('h265');
        }

        log.debug('Video transcoding engine init OK');
        callback('callback', {codecs: supported_codecs});
    };

    that.deinit = function () {
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
        if (message.purpose === 'session' && controller) {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Session controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
                that.deinit();
                process.exit();
            }
        }
    };

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = global.config.internal.minport;
        internalOpt.maxport = global.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: clusterIP, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

    that.generate = function (codec, callback) {
        log.debug('generate, codec:', codec);
        codec = codec || supported_codecs[0];
        codec = codec.toLowerCase();

        for (var stream_id in outputs) {
            if (outputs[stream_id].codec === codec) {
                callback('callback', stream_id);
                return;
            }
        }

        if (supported_codecs.indexOf(codec) === -1) {
            log.error('Not supported codec:'+codec);
            callback('callback', 'error', 'Not supported codec.');
            return;
        }

        addOutput(codec, function (stream_id) {
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
        log.debug('publish, stream_id:', stream_id, 'stream_type:', stream_type, 'options:', options);
        if (stream_type !== 'internal') {
            return callback('callback', 'error', 'can not publish a stream to video engine through a non-internal connection');
        }

        if (options.video === undefined || options.video.codec === undefined) {
            return callback('callback', 'error', 'can not publish a stream to video engine without proper video codec');
        }

        if (input_id === undefined) {
            setInput(stream_id, options.video.codec, options, function () {
                callback('callback', {ip: clusterIP, port: input_conn.getListeningPort()});
            }, function (error_reason) {
                log.error(error_reason);
                callback('callback', 'error', error_reason);
            });
        } else if (input_id === stream_id) {
            callback('callback', {ip: clusterIP, port: input_conn.getListeningPort()});
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
        if (connectionType !== 'internal') {
            return callback('callback', 'error', 'can not subscribe a stream from video engine through a non-internal connection');
        }

        var conn = internalConnFactory.fetch(connectionId, 'out');
        if (!conn) {
            callback('callback', 'error', 'Create internal connection failed.');
            return;
        }
        conn.connect(options);

        if (connections[connectionId] === undefined) {
            connections[connectionId] = {videoFrom: undefined,
                                         connection: conn
                                        };
        }
        callback('callback', 'ok');
    };

    that.unsubscribe = function (connectionId) {
        log.debug('unsubscribe, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].videoFrom) {
            if (outputs[connections[connectionId].videoFrom]) {
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection.receiver());
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
            outputs[video_stream_id].dispatcher.addDestination('video', connections[connectionId].connection.receiver());
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
                outputs[connections[connectionId].videoFrom].dispatcher.removeDestination('video', connections[connectionId].connection.receiver());
            }
            connections[connectionId].videoFrom = undefined;
        }
    };

    return that;
};

module.exports = function (rpcClient, clusterIP) {
    var that = {},
        processor = undefined;

    that.init = function (service, config, belongTo, controller, mixView, callback) {
        if (processor === undefined) {
            if (service === 'mixing') {
                processor = new VMixer(rpcClient, clusterIP);
                processor.initialize(config, belongTo, controller, mixView, callback);
                that.__proto__ = processor;
            } else if (service === 'transcoding') {
                processor = new VTranscoder(rpcClient, clusterIP);
                processor.initialize(controller, callback);
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

    return that;
};
