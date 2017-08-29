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

var resolutionValue2Name = {
    'r352x288': 'cif',
    'r640x480': 'vga',
    'r800x600': 'svga',
    'r1024x768': 'xga',
    'r640x360': 'r640x360',
    'r1280x720': 'hd720p',
    'r320x240': 'sif',
    'r480x320': 'hvga',
    'r480x360': 'r480x360',
    'r176x144': 'qcif',
    'r192x144': 'r192x144',
    'r1920x1080': 'hd1080p',
    'r3840x2160': 'uhd_4k',
    'r360x360': 'r360x360',
    'r480x480': 'r480x480',
    'r720x720': 'r720x720'
};

var resolution2String = (r) => {
  var k = 'r' + r.width + 'x' + r.height;
  return resolutionValue2Name[k] ? resolutionValue2Name[k] : k;
};

function isResolutionEqual(r1, r2) {
  return (r1.width === r2.width) && (r1.height === r2.height);
}

const partial_linear_bitrate = [
  {size: 0, bitrate: 0},
  {size: 76800, bitrate: 400},  //320*240, 30fps
  {size: 307200, bitrate: 800}, //640*480, 30fps
  {size: 921600, bitrate: 2000},  //1280*720, 30fps
  {size: 2073600, bitrate: 4000}, //1920*1080, 30fps
  {size: 8294400, bitrate: 16000} //3840*2160, 30fps
];

const standardBitrate = (width, height, framerate) => {
  let bitrate = 0;
  let prev = 0;
  let next = 0;
  let portion = 0.0;
  let def = width * height * framerate / 30;

  // find the partial linear section and calculate bitrate
  for (var i = 0; i < partial_linear_bitrate.length - 1; i++) {
    prev = partial_linear_bitrate[i].size;
    next = partial_linear_bitrate[i+1].size;
    if (def > prev && def <= next) {
      portion = (def - prev) / (next - prev);
      bitrate = partial_linear_bitrate[i].bitrate + (partial_linear_bitrate[i+1].bitrate - partial_linear_bitrate[i].bitrate) * portion;
      break;
    }
  }

  // set default bitrate for over large resolution
  if (0 == bitrate) {
    bitrate = 8000;
  }

  return bitrate;
}

const calcDefaultBitrate = (codec, resolution, framerate) => {
  let factor = (codec === 'vp8' ? 0.9 : 1.0);
  return standardBitrate(resolution.width, resolution.height, framerate) * factor;
};

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
        defaultframerate, bitrate, keyFrameInterval,

        supported_codecs = [],
        default_resolution = {width: 640, height: 480},
        default_framerate = 30,
        default_kfi = 1000,

        /*{StreamID : {codec: 'vp8' | 'h264' |...,
                       resolution: {width: Number(Width), height: Number(Height)},
                       framerate: Number(FPS),
                       bitrate: Number(Kbps),
                       kfi: Number(KeyFrameIntervalSeconds),
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

    that.initialize = function (videoConfig, belongTo, layoutcontroller, mixView, callback) {
        //log.debug('initEngine, videoConfig:', JSON.stringify(videoConfig));
        var config = {
            'hardware': useHardware,
            'maxinput': videoConfig.maxInput,
            'resolution': resolution2String(videoConfig.parameters.resolution),
            'backgroundcolor': videoConfig.bgColor,
            'layout': videoConfig.layout.templates,
            'crop': videoConfig.layout.crop,
            'gaccplugin': gaccPluginEnabled,
        };

        streamMap = new StreamMap(videoConfig.maxInput);
        engine = new VideoMixer(JSON.stringify(config));
        layoutProcessor = new LayoutProcessor(videoConfig.layout.templates);
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
                    stream: streamMap.getStreamFromInput(inputRegion.input),
                    region: inputRegion.region
                };
            });
            var layoutChangeArgs = [belong_to, streamRegions, view];
            rpcClient.remoteCall(controller, 'onVideoLayoutChange', layoutChangeArgs);
        });
        belong_to = belongTo;
        controller = layoutcontroller;
        maxInputNum = videoConfig.maxInput;
        view = mixView;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['vp8', 'vp9'];
        default_resolution = (videoConfig.parameters.resolution || {width: 640, height: 480});
        default_framerate = (videoConfig.parameters.framerate || 30);
        default_kfi = (videoConfig.parameters.keyFrameInterval || 1000);

        if (useHardware || openh264Enabled) {
            supported_codecs.push('h264');
        }

        if (useHardware) {
            supported_codecs.push('h265');
        }

        log.debug('Video engine init OK');
        callback('callback', {codecs: supported_codecs});
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
        if (message.purpose === 'conference' && controller) {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Conference controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
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

    that.generate = function (codec, resolution, framerate, bitrate, keyFrameInterval, callback) {
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'framerate:', framerate, 'bitrate:', bitrate, 'keyFrameInterval:', keyFrameInterval);
        codec = (codec || supported_codecs[0]).toLowerCase();
        resolution = (resolution === 'unspecified' ? default_resolution : resolution);
        framerate = (framerate === 'unspecified' ? default_framerate : framerate);
        bitrate = (bitrate === 'unspecified' ? calcDefaultBitrate(codec, resolution, framerate) : bitrate);
        keyFrameInterval = (keyFrameInterval === 'unspecified' ? default_kfi : keyFrameInterval);

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

        if (supported_codecs.indexOf(codec) === -1) {
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
        default_resolution = {width: 0, height: 0},
        default_framerate = 30,
        default_kfi = -1,

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
        if (message.purpose === 'conference' && controller) {
            if ((message.type === 'node' && message.id === controller) ||
                (message.type === 'worker' && controller.startsWith(message.id))) {
                log.error('Conference controller (type:', message.type, 'id:', message.id, ') fault is detected, exit.');
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

    that.generate = function (codec, resolution, framerate, bitrate, keyFrameInterval, callback) {
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'framerate:', framerate, 'bitrate:', bitrate, 'keyFrameInterval:', keyFrameInterval);
        codec = (codec || supported_codecs[0]).toLowerCase();
        resolution = (resolution === 'unspecified' ? default_resolution : resolution);
        framerate = (framerate === 'unspecified' ? default_framerate : framerate);
        var bitrate_factor = (typeof bitrate === 'string' ? (bitrate === 'unspecified' ? 1.0 : Number(bitrate.substring(1))) : 0);
        bitrate = (bitrate_factor ? calcDefaultBitrate(codec, resolution, framerate) * bitrate_factor : bitrate);
        keyFrameInterval = (keyFrameInterval === 'unspecified' ? default_kfi : keyFrameInterval);

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

        if (supported_codecs.indexOf(codec) === -1) {
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
