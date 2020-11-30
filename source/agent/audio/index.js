// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var internalIO = require('../internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;
var MediaFrameMulticaster = require('../mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
var AudioMixer = require('../audioMixer/build/Release/audioMixer');

var { SelectiveMixer } = require('./SelectiveMixer');

var logger = require('../logger').logger;

// Logger
var log = logger.getLogger('AudioNode');
var InternalConnectionFactory = require('./InternalConnectionFactory');

module.exports = function (rpcClient, selfRpcId, parentRpcId, clusterWorkerIP) {
    var that = {
      agentID: parentRpcId,
      clusterIP: clusterWorkerIP
    },
        engine,
        belong_to_room,
        controller,
        observer,
        view,

        supported_codecs = [],

        /*{StreamID : {for_whom: EndpointID,
                       codec: 'pcmu' | 'pcma' | 'isac_16000' | 'isac_32000' | 'opus_48000_2' |...,
                       dispatcher: MediaFrameMulticaster,
                       }}*/
        outputs = {},

        /*{StreamID : {owner: EndpointID,
                       connection: InternalIn}
          }*/
        inputs = {},

        /*{ConnectionID: {audioFrom: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {},

        // For internal SCTP connection creation
        internalConnFactory = new InternalConnectionFactory;

    var addInput = function (stream_id, owner, codec, options, on_ok, on_error) {
        if (engine) {
            var conn = internalConnFactory.fetch(stream_id, 'in');
            if (!conn) {
                on_error('Create internal connection failed.');
                return;
            }
            conn.connect(options);

            if (engine.addInput(owner, stream_id, codec, conn)) {
                inputs[stream_id] = {owner: owner,
                                     connection: conn};
                log.debug('addInput ok, for:', owner, 'codec:', codec, 'options:', options);
                on_ok(stream_id);
            } else {
                on_error('Failed in adding input to audio-engine.');
            }
        } else {
            on_error('Audio-mixer engine is not ready.');
        }
    };

    var removeInput = function (stream_id) {
        if (inputs[stream_id]) {
            engine.removeInput(inputs[stream_id].owner, stream_id);
            internalConnFactory.destroy(stream_id, 'in');
            delete inputs[stream_id];
        }
    };

    var addOutput = function (for_whom, codec, on_ok, on_error) {
        var stream_id = Math.random() * 1000000000000000000 + '';
        if (engine) {
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(for_whom, stream_id, codec, dispatcher)) {
                outputs[stream_id] = {for_whom: for_whom,
                                      codec: codec,
                                      dispatcher: dispatcher,
                                      connections: {}};
                on_ok(stream_id);
            } else {
                on_error('Failed in adding output to audio-engine');
            }
        } else {
            on_error('Audio-mixer engine is not ready.');
        }
    };

    var removeOutput = function (stream_id) {
        if (outputs[stream_id]) {
            var output = outputs[stream_id];
            for (var connectionId in output.connections) {
                 output.dispatcher.removeDestination('audio', output.connections[connectionId].receiver());
                 output.connections[connectionId].close();
                 delete output.connections[connectionId];
            }
            engine.removeOutput(output.for_whom, stream_id);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    var initEngine = function (config, belongToRoom, ctrlr, callback) {
        var topK = global.config.mix.top_k;
        if (view && topK > 0) {
            engine = new SelectiveMixer(topK, JSON.stringify(config));
        } else {
            engine = new AudioMixer(JSON.stringify(config));
        }
        belong_to_room = belongToRoom;
        controller = ctrlr;

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['pcmu', 'opus_48000_2', 'pcma', 'ilbc', 'isac_16000', 'isac_32000', 'g722_16000_1', 'g722_16000_2', 'aac', 'aac_48000_2', 'ac3', 'nellymoser'];

        log.debug('AudioMixer.init OK');
        callback('callback', {codecs: supported_codecs});
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
        belong_to_room = undefined;
        controller = undefined;
    };

    that.generate = function (for_whom, codec, callback) {
        codec = codec.toLowerCase();
        log.debug('generate, for_whom:', for_whom, 'codec:', codec);
        for (var stream_id in outputs) {
            if (outputs[stream_id].for_whom === for_whom && outputs[stream_id].codec === codec) {
                log.debug('generate, got stream:', stream_id);
                callback('callback', stream_id);
                return;
            }
        }

        addOutput(for_whom, codec, function (stream_id) {
            log.debug('generate, got stream:', stream_id);
            callback('callback', stream_id);
        }, function (error_reason) {
            log.error(error_reason);
            callback('callback', 'error', error_reason);
        });
    };

    that.degenerate = function (stream_id) {
        removeOutput(stream_id);
    };

    that.createInternalConnection = function (connectionId, direction, internalOpt, callback) {
        internalOpt.minport = global.config.internal.minport;
        internalOpt.maxport = global.config.internal.maxport;
        var portInfo = internalConnFactory.create(connectionId, direction, internalOpt);
        callback('callback', {ip: global.config.internal.ip_address, port: portInfo});
    };

    that.destroyInternalConnection = function (connectionId, direction, callback) {
        internalConnFactory.destroy(connectionId, direction);
        callback('callback', 'ok');
    };

    that.publish = function (stream_id, stream_type, options, callback) {
        log.debug('publish stream:', stream_id, 'stream_type:', stream_type, 'options:', options);
        if (stream_type !== 'internal') {
            return callback('callback', 'error', 'can not publish a stream to audio engine through a non-internal connection');
        }

        if (options.audio === undefined || options.audio.codec === undefined) {
            return callback('callback', 'error', 'can not publish a stream to audio engine without proper audio codec');
        }

        if (inputs[stream_id] === undefined) {
            addInput(stream_id, options.publisher, options.audio.codec, options, function () {
                callback('callback', 'ok');
            }, function (error_reason) {
                log.error(error_reason);
                callback('callback', 'error', error_reason);
            });
        } else {
            callback('callback', 'error', 'Connection Already exist.');
        }
    };

    that.unpublish = function (stream_id) {
        log.debug('unpublish, stream_id:', stream_id);
        removeInput(stream_id);
    };

    that.setInputActive = function (stream_id, active, callback) {
        log.debug('setInputActive, stream_id:', stream_id, 'active:', active);
        if (inputs[stream_id]) {
            engine.setInputActive(inputs[stream_id].owner, stream_id, !!active);
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'Connection does NOT exist.');
        }
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
        if (connectionType !== 'internal') {
            return callback('callback', 'error', 'can not subscribe a stream from audio engine through a non-internal connection');
        }

        var conn = internalConnFactory.fetch(connectionId, 'out');
        if (!conn) {
            callback('callback', 'error', 'Create internal connection failed.');
            return;
        }
        conn.connect(options);


        if (connections[connectionId] === undefined) {
            connections[connectionId] = {audioFrom: undefined,
                                         connection: conn
                                        };
        }
        callback('callback', 'ok');
    };

    that.unsubscribe = function (connectionId) {
        log.debug('unsubscribe, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].audioFrom) {
            if (outputs[connections[connectionId].audioFrom]) {
                outputs[connections[connectionId].audioFrom].dispatcher.removeDestination('audio', connections[connectionId].connection.receiver());
            }
            internalConnFactory.destroy(connectionId, 'out');
            delete connections[connectionId];
        }
    };

    that.linkup = function (connectionId, audio_stream_id, video_stream_id, data_stream_id, callback) {
        log.debug('linkup, connectionId:', connectionId, 'audio_stream_id:', audio_stream_id);
        if (connections[connectionId] === undefined) {
            return callback('callback', 'error', 'connection does not exist');
        }

        if (outputs[audio_stream_id]) {
            outputs[audio_stream_id].dispatcher.addDestination('audio', connections[connectionId].connection.receiver());
            connections[connectionId].audioFrom = audio_stream_id;
            callback('callback', 'ok');
        } else {
            log.error('Stream does not exist!', audio_stream_id);
            callback('callback', 'error', 'Stream does not exist!');
        }
    };

    that.cutoff = function (connectionId) {
        log.debug('cutoff, connectionId:', connectionId);
        if (connections[connectionId] && connections[connectionId].audioFrom) {
            if (outputs[connections[connectionId].audioFrom]) {
                outputs[connections[connectionId].audioFrom].dispatcher.removeDestination('audio', connections[connectionId].connection.receiver());
            }
            connections[connectionId].audioFrom = undefined;
        }
    };

    that.enableVAD = function (periodMS) {
        log.debug('enableVAD, periodMS:', periodMS);
        engine.enableVAD(periodMS, function (activeInput) {
            log.debug('enableVAD, activeInput:', activeInput);
            controller && rpcClient.remoteCall(controller, 'onAudioActiveness', [belong_to_room, activeInput, view], {callback: function(){}});
        });
    };

    that.resetVAD = function () {
        engine.resetVAD();
    };

    that.init = function (service, config, belongToRoom, controller, mixView, callback) {
        var audioConfig = global.config.audio || {};
        log.debug('init, audioConfig:', audioConfig);

        if (service === 'mixing') {
            if (typeof config === 'object') {
                // Merge media mixing audio property
                audioConfig = Object.assign(audioConfig, config);
            }
            view = mixView;
            initEngine(audioConfig, belongToRoom, controller, callback);
        } else if (service === 'transcoding') {
            initEngine(audioConfig, belongToRoom, controller, callback);
        } else {
            log.error('Unknown service type to init an audio node:', service);
            callback('callback', 'error', 'Unknown service type to init an audio node.');
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

    return that;
};
