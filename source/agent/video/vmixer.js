// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const MediaFrameMulticaster = require(
    '../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');

const log = require('../logger').logger.getLogger('VMixerJS');

const {LayoutProcessor} = require('./layout');

const {
    calcDefaultBitrate,
    resolution2String,
    isResolutionEqual,
} = require('../mediaUtil');

const colorMap = {
    'white': { r: 255, g: 255, b: 255 },
    'black': { r: 0, g: 0, b: 0 }
};

const isUnspecified = (value) => (value === undefined || value === 'unspecified');

const useHardware = global.config.video.hardwareAccelerated;
const gaccPluginEnabled = global.config.video.enableBetterHEVCQuality || false;
const MFE_timeout = global.config.video.MFE_timeout || 0;
const supported_codecs = global.config.video.codecs;

/*
 * This class manage `maxInput` number of inputs to video mixer.
 * When there're already `maxInput` inputs in mixer, later inputs
 * try to go into mixer will be in pending state. Those pending inputs
 * can be added into mixer if the input number in mixer becomes
 * less than `maxInput`.
 */
class InputManager {

    constructor(maxInput) {
        // Key: stream ID, Value: input-object
        this.inputs = {};
        this.pendingInputs = {};
        this.freeIndex = [];

        // Default to 100 if not specified
        if (typeof maxInput !== 'number' || maxInput <= 0) maxInput = 100;
        for (let i = 0; i < maxInput; i++) {
            this.freeIndex.push(i);
        }
        this.freeIndex.reverse();
    }

    // Reset maxInput
    reset(maxInput) {
      constructor(maxInput);
    }

    // Has the streamId
    has(streamId) {
        return (!!this.inputs[streamId] || !!this.pendingInputs[streamId]);
    }

    // Check if the stream with `streamId` is in pending state
    isPending(streamId) {
        return !!this.pendingInputs[streamId];
    }


    /* Return the InputInfo of the give streamId
     * InputInfo = {
     *   id: number,
     *   codec: string,
     *   conn: AddonObject,
     *   avatar: string,
     *   primaryCount: number,
     * }
     */
    get(streamId) {
        return (this.inputs[streamId] || this.pendingInputs[streamId]);
    }

    // Add a input with related info
    add(streamId, codec, conn, avatar) {
        if (this.inputs[streamId])
            return -1;

        var input = {
            id: this.freeIndex.pop(),
            codec: codec,
            conn: conn,
            avatar: avatar,
            primaryCount: 0
        };
        if (input.id === undefined) {
            this.pendingInputs[streamId] = input;
            return -1;
        } else {
            this.inputs[streamId] = input;
            return input.id;
        }
    }

    // Remove a input with streamId
    remove(streamId) {
        var input = null;
        if (this.inputs[streamId]) {
            input = this.inputs[streamId];
            delete this.inputs[streamId];
            this.freeIndex.push(input.id);
        } else if (this.pendingInputs[streamId]) {
            input = this.pendingInputs[streamId];
            delete this.pendingInputs[streamId];
        }
        return input;
    }

    // Activate the pending input with streamId,
    // Change input with ID toPending to pending if specified.
    enable(streamId, toPending) {
        if (!this.pendingInputs[streamId] || streamId === toPending)
            return -1;

        // Move least setPrimary input to pending
        var k;
        var minPrimary = Number.MAX_SAFE_INTEGER;

        if (toPending === undefined) {
            for (let k in this.inputs) {
                if (this.inputs[k].primaryCount < minPrimary || !toPending) {
                    minPrimary = this.inputs[k].primaryCount;
                    toPending = k;
                }
            }
        }

        if (toPending !== undefined) {
            this.pendingInputs[toPending] = this.inputs[toPending];
            delete this.inputs[toPending];
            this.inputs[streamId] = this.pendingInputs[streamId];
            delete this.pendingInputs[streamId];
            // Swap their input id
            this.inputs[streamId].id = this.pendingInputs[toPending].id;
            this.pendingInputs[toPending].id = undefined;
            // Return the changed input id
            return this.inputs[streamId].id;
        }

        return -1;
    }

    // Activate first pending stream and place it in input with `inputId` 
    promotePendingStream(inputId) {
        var pos = this.freeIndex.indexOf(inputId)
        if (pos < 0)
            return;

        var streamId;
        for (streamId in this.pendingInputs) {
            break;
        }
        if (streamId) {
            this.inputs[streamId] = this.pendingInputs[streamId];
            this.inputs[streamId].id = inputId;
            delete this.pendingInputs[streamId];
            this.freeIndex.splice(pos, 1);
            return this.inputs[streamId];
        }
        return null;
    }

    size() {
        return Object.keys(this.inputs).length;
    }

    getStreamList() {
        return Object.keys(this.inputs).concat(Object.keys(this.pendingInputs));
    }

    // Get stream from input
    getStreamFromInput(inputId) {
        for (let streamId in this.inputs) {
            if (this.inputs[streamId].id === inputId) {
                return streamId;
            }
        }
        return null;
    }
}

function VMixer(rpcClient, clusterIP, VideoMixer, router, streamingEmitter) {
    var that = {},
        engine,
        layoutProcessor,
        belong_to,
        controller,
        view,

        drawing_text_tmr,

        motion_factor = 0.8,
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
        inputManager,
        maxInputNum = 0,

        /*{ConnectionID: {video: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {};

    var addInput = function (stream_id, codec, options, avatar, on_ok, on_error) {
        log.debug('add input', stream_id);
        // FIXME: filter profile setting for native layer
        codec = codec.indexOf('h264') > -1 ? 'h264' : codec;
        if (engine) {
            var conn = router.getOrCreateRemoteSource({
                id: stream_id,
                ip: options.ip,
                port: options.port
            }, function onStats(stat) {
                if (stat === 'disconnected') {
                    log.debug('Internal connection closed:', stream_id);
                    removeInput(stream_id);
                }
            });
            // Use default avatar if it is not set
            avatar = avatar || global.config.avatar.location;

            let inputId = inputManager.add(stream_id, codec, conn, avatar);
            if (inputId >= 0) {
                if (engine.addInput(inputId, codec, conn, avatar)) {
                    layoutProcessor.addInput(inputId);
                    log.debug('addInput ok, stream_id:', stream_id, 'codec:', codec, 'options:', options);
                    on_ok(stream_id);
                } else {
                    router.destroyRemoteSource(stream_id);
                    on_error('Failed in adding input to video-engine.');
                }
            } else {
                log.debug('addInput pending', stream_id);
                on_ok(stream_id);
            }
        } else {
            on_error('Video-mixer engine is not ready.');
        }
    };

    var removeInput = function (stream_id) {
        log.debug('remove input', stream_id);
        if (inputManager.has(stream_id)) {
            let input = inputManager.remove(stream_id);
            if (input.id >= 0) {
                // Remove activate input
                engine.removeInput(input.id);
                let newInput = inputManager.promotePendingStream(input.id);
                if (newInput) {
                    // If there's pending input
                    let newStreamId = inputManager.getStreamFromInput(newInput.id);
                    if (!engine.addInput(newInput.id, newInput.codec, newInput.conn, newInput.avatar)) {
                        layoutProcessor.removeInput(newInput.id);
                        log.error('engine fail to add pending input', newStreamId);
                    }
                    log.debug('add pending input', newStreamId);
                } else {
                    layoutProcessor.removeInput(input.id);
                }
            }
            router.destroyRemoteSource(stream_id);
        } else {
            log.error('no such input to remove:', stream_id);
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
                router.addLocalSource(stream_id, 'vmixer', dispatcher.source());
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

    var formatLayoutSolution = function (layoutSolution) {
        return layoutSolution.map((inputRegion) => {
            var rationalString = (obj) => (obj.numerator + '/' + obj.denominator);
            var curRegion = inputRegion.region;
            return {
                stream: inputManager.getStreamFromInput(inputRegion.input),
                region: {
                    id: curRegion.id,
                    shape: curRegion.shape,
                    area: curRegion.shape === 'rectangle' ?
                        {
                            left: rationalString(curRegion.area.left),
                            top: rationalString(curRegion.area.top),
                            width: rationalString(curRegion.area.width),
                            height: rationalString(curRegion.area.height),
                        } :
                        {
                            centerW: rationalString(curRegion.area.centerW),
                            centerH: rationalString(curRegion.area.centerH),
                            radius: rationalString(curRegion.area.radius),
                        }
                }
            };
        });
    };

    that.initialize = function (videoConfig, belongTo, layoutcontroller, mixView, callback) {
        log.debug('initEngine, videoConfig:', JSON.stringify(videoConfig));
        var config = {
            'hardware': useHardware,
            'maxinput': videoConfig.maxInput || 16,
            'resolution': resolution2String(videoConfig.parameters.resolution),
            'backgroundcolor': (typeof videoConfig.bgColor === 'string') ? colorMap[videoConfig.bgColor] : videoConfig.bgColor,
            'layout': videoConfig.layout.templates,
            'crop': (videoConfig.layout.fitPolicy === 'crop' ? true : false),
            'gaccplugin': gaccPluginEnabled,
            'MFE_timeout': MFE_timeout
        };

        inputManager = new InputManager(videoConfig.maxInput);
        engine = new VideoMixer(config);
        layoutProcessor = new LayoutProcessor(videoConfig.layout.templates);
        layoutProcessor.on('error', function (e) {
            log.warn('layout error:', e);
        });
        layoutProcessor.on('layoutChange', function (layoutSolution) {
            log.debug('layoutChange', layoutSolution);
            if (typeof engine.updateLayoutSolution === 'function') {
                engine.updateLayoutSolution(layoutSolution.filter((obj) => {return obj.input !== undefined;}));
            } else {
                log.warn('No native method: updateLayoutSolution');
            }

            var streamRegions = formatLayoutSolution(layoutSolution);
            var layoutChangeArgs = [belong_to, streamRegions, view];
            rpcClient.remoteCall(controller, 'onVideoLayoutChange', layoutChangeArgs);
            // Emit GRPC notifications
            const notification = {
                name: 'onVideoLayoutChange',
                data: {
                    owner: belongTo,
                    label: view,
                    regions: streamRegions
                }
            };
            streamingEmitter.emit('notification', notification);
        });
        belong_to = belongTo;
        controller = layoutcontroller;
        maxInputNum = videoConfig.maxInput;
        view = mixView;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        motion_factor = (videoConfig.motionFactor || 0.8);
        default_resolution = (videoConfig.parameters.resolution || {width: 640, height: 480});
        default_framerate = (videoConfig.parameters.framerate || 30);
        default_kfi = (videoConfig.parameters.keyFrameInterval || 1000);

        log.debug('Video engine init OK, supported_codecs:', supported_codecs);
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

        // Remove pending stream first
        for (let stream_id of inputManager.getStreamList()) {
            if (inputManager.isPending(stream_id)) {
                removeInput(stream_id);
            }
        }
        for (let stream_id of inputManager.getStreamList()) {
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

    that.generate = function (codec, resolution, framerate, bitrate, keyFrameInterval, callback) {
        log.debug('generate, codec:', codec, 'resolution:', resolution, 'framerate:', framerate, 'bitrate:', bitrate, 'keyFrameInterval:', keyFrameInterval);
        codec = (codec || supported_codecs.encode[0]).toLowerCase();
        resolution = (isUnspecified(resolution) ? default_resolution : resolution);
        framerate = (isUnspecified(framerate) ? default_framerate : framerate);
        var bitrate_factor = (typeof bitrate === 'string' ? (bitrate === 'unspecified' ? 1.0 : (Number(bitrate.replace('x', '')) || 0)) : 0);
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
        log.debug('degenerate, stream_id:', stream_id);
        removeOutput(stream_id);
    };

    that.setInputActive = function (stream_id, active, callback) {
        log.debug('setInputActive, stream_id:', stream_id, 'active:', active);
        if (inputManager.has(stream_id) && !inputManager.isPending(stream_id)) {
            let inputId = inputManager.get(stream_id).id;
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

        if (!inputManager.has(stream_id)) {
            log.debug('publish 1, inputs.length:', inputManager.size(), 'maxInputNum:', maxInputNum);
            addInput(stream_id, options.video.codec, options, options.avatar, function () {
                callback('callback', {ip: clusterIP});
            }, function (error_reason) {
                log.error(error_reason);
                callback('callback', 'error', error_reason);
            });
        } else {
            callback('callback', {ip: clusterIP});
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

    that.setPrimary = function (stream_id, callback) {
        log.debug('setPrimary, stream_id:', stream_id);
        if (inputManager.has(stream_id)) {
            let input;
            if (inputManager.isPending(stream_id)) {
                inputManager.enable(stream_id);
                input = inputManager.get(stream_id);
                engine.removeInput(input.id);
                if (!engine.addInput(input.id, input.codec, input.conn, input.avatar)) {
                    layoutProcessor.removeInput(input.id);
                    callback('callback', 'error', 'Switch input failed.');
                    return;
                }
            } else {
                input = inputManager.get(stream_id);
            }
            //TODO: re-implement the setPrimary method
            layoutProcessor.setPrimary(input.id);
            input.primaryCount++;
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.setRegion = function (stream_id, region_id, callback) {
        log.debug('setRegion, stream_id:', stream_id, 'region_id:', region_id);
        if (inputManager.has(stream_id)) {
            if (inputManager.isPending(stream_id)) {
                let originInput = layoutProcessor.getInput(region_id);
                if (originInput < 0) {
                    callback('callback', 'error', 'Invalid region');
                    return;
                }
                inputManager.enable(stream_id, inputManager.getStreamFromInput(originInput));
                let input = inputManager.get(stream_id);
                engine.removeInput(input.id);
                if (!engine.addInput(input.id, input.codec, input.conn, input.avatar)) {
                    layoutProcessor.removeInput(input.id);
                    callback('callback', 'error', 'Switch input failed.');
                } else {
                    callback('callback', 'ok');
                }
            } else {
                let inputId = inputManager.get(stream_id).id;
                if (layoutProcessor.specifyInputRegion(inputId, region_id)) {
                    callback('callback', 'ok');
                } else {
                    callback('callback', 'error', 'Invalid region_id.');
                }
            }
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.getRegion = function (stream_id, callback) {
        log.debug('getRegion, stream_id:', stream_id);
        if (inputManager.has(stream_id) && !inputManager.isPending(stream_id)) {
            let inputId = inputManager.get(stream_id).id;
            let region = layoutProcessor.getRegion(inputId);
            let regionId = region ? region.id : '';
            callback('callback', regionId);
        } else {
            callback('callback', 'error', 'Invalid input stream_id.');
        }
    };

    that.setLayout = function (layout, callback) {
        log.debug('setLayout, layout:', JSON.stringify(layout));

        var specified_streams = layout.map((obj) => {return obj.stream ? obj.stream : null;}).filter((st) => { return st;});
        var current_streams = [];

        inputManager.getStreamList().map((stream_id) => {
            let input = inputManager.remove(stream_id);
            if (input.id >= 0) {
                engine.removeInput(input.id);
            }

            input.stream = stream_id;

            if (specified_streams.indexOf(stream_id) >= 0) {
              current_streams.unshift(input);
            } else {
              current_streams.push(input);
            }
        });

        inputManager.reset(layout.length);

        current_streams.forEach((obj) => {
          let input = inputManager.add(obj.stream, obj.codec, obj.conn, obj.avatar);
          if (input >= 0) {
            engine.addInput(input, obj.codec, obj.conn, obj.avatar);
            if (specified_streams.indexOf(obj.stream) < 0) {
              for (var i in layout) {
                if (!layout[i].stream) {
                  layout[i].stream = obj.stream;
                  break;
                }
              }
            }
          }
        });

        var inputLayout = layout.map((obj) => {
          if (obj.stream) {
            return {input: inputManager.get(obj.stream).id, region: obj.region};
          } else {
            return {region: obj.region};
          }
        });

        layoutProcessor.setLayout(inputLayout, function(layoutSolution) {
          callback('callback', formatLayoutSolution(layoutSolution));
        }, function(err) {
          callback('callback', 'error', 'layoutProcessor failed');
        });
    };

    that.forceKeyFrame = function (stream_id) {
        log.debug('forceKeyFrame, stream_id:', stream_id);
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

exports.VMixer = VMixer;
