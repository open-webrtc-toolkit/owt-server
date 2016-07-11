/*global require, module*/
'use strict';

var internalIO = require('./internalIO/build/Release/internalIO');
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;
var MediaFrameMulticaster = require('./mediaFrameMulticaster/build/Release/mediaFrameMulticaster');
var AudioMixer = require('./audioMixer/build/Release/audioMixer');

var logger = require('./logger').logger;
var amqper = require('./amqper');

// Logger
var log = logger.getLogger('AudioNode');

module.exports = function () {
    var that = {},
        engine,
        observer,

        supported_codecs = [],

        /*{StreamID : {owner: ParticipantID,
                       codec: 'pcmu' | 'pcma' | 'isac_16000' | 'isac_32000' | 'opus_48000_2' |...,
                       dispatcher: MediaFrameMulticaster,
                       }}*/
        outputs = {},

        /*{StreamID : {owner: TerminalID,
                       connection: InternalIn}
          }*/
        inputs = {},

        /*{ConnectionID: {audioFrom: StreamID | undefined,
                          connection: InternalOut}
                         }
         }
         */
        connections = {};

    var addInput = function (stream_id, for_whom, codec, protocol, on_ok, on_error) {
        if (engine) {
            var conn = new InternalIn(protocol);
            if (engine.addInput(for_whom, codec, conn)) {
                inputs[stream_id] = {owner: for_whom,
                                     connection: conn};
                log.debug('addInput ok, for:', for_whom, 'codec:', codec, 'protocol:', protocol);
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
            engine.removeInput(inputs[stream_id].owner);
            inputs[stream_id].connection.close();
            delete inputs[stream_id];
        }
    };

    var addOutput = function (for_whom, codec, on_ok, on_error) {
        var stream_id = Math.random() * 1000000000000000000 + '';
        if (engine) {
            var dispatcher = new MediaFrameMulticaster();
            if (engine.addOutput(for_whom, codec, dispatcher)) {
                outputs[stream_id] = {owner: for_whom,
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
                 output.dispatcher.removeDestination('audio', output.connections[connectionId]);
                 output.connections[connectionId].close();
                 delete output.connections[connectionId];
            }
            engine.removeOutput(output.owner);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    var initEngine = function (config, callback) {
        engine = new AudioMixer(JSON.stringify(config));

        // FIXME: The supported codec list should be a sub-list of those querried from the engine
        // and filterred out according to config.
        supported_codecs = ['pcmu', 'opus_48000_2', 'pcm_raw'/*, 'pcma', 'isac_16000', 'isac_32000'*/];

        log.info('AudioMixer.init OK');
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
        observer = undefined;
    };

    that.generate = function (for_whom, codec, callback) {
        codec = codec.toLowerCase();
        log.debug('generate, for_whom:', for_whom, 'codec:', codec);
        for (var stream_id in outputs) {
            if (outputs[stream_id].owner === for_whom && outputs[stream_id].codec === codec) {
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

    that.publish = function (stream_id, stream_type, options, callback) {
        if (stream_type !== 'internal') {
            return callback('callback', 'error', 'can not publish a stream to audio engine through a non-internal connection');
        }

        if (options.audio === undefined || options.audio.codec === undefined) {
            return callback('callback', 'error', 'can not publish a stream to audio engine without proper audio codec');
        }

        if (inputs[stream_id] === undefined) {
            log.debug('publish stream:', stream_id);
            addInput(stream_id, options.owner, options.audio.codec, options.protocol, function () {
                callback('callback', {ip: that.clusterIP, port: inputs[stream_id].connection.getListeningPort()});
            }, function (error_reason) {
                log.error(error_reason);
                callback('callback', 'error', error_reason);
            });
        } else {
            log.error('Stream:'+stream_id+' already input in audio-engine.');
            callback('callback', 'error', 'Stream:'+stream_id+' already input in audio-engine.');
        }
    };

    that.unpublish = function (stream_id) {
        removeInput(stream_id);
    };

    that.subscribe = function (connectionId, connectionType, options, callback) {
        if (connectionType !== 'internal') {
            return callback('callback', 'error', 'can not subscribe a stream from audio engine through a non-internal connection');
        }
        log.debug('subscribe, connectionId:', connectionId, 'options:', options);

        if (connections[connectionId] === undefined) {
            connections[connectionId] = {audioFrom: undefined,
                                         connection: new InternalOut(options.protocol, options.dest_ip, options.dest_port)
                                        };
            callback('callback', 'ok');
        } else {
            callback('callback', 'error', 'connection already exists');
        }
    };

    that.unsubscribe = function (connectionId) {
        if (connections[connectionId] && connections[connectionId].audioFrom) {
            if (outputs[connections[connectionId].audioFrom]) {
                outputs[connections[connectionId].audioFrom].dispatcher.removeDestination('audio', connections[connectionId].connection);
            }
            //connections[connectionId].connection.close();
            delete connections[connectionId];
        }
    };

    that.linkup = function (connectionId, audio_stream_id, video_stream_id, callback) {
        if (connections[connectionId] === undefined) {
            return callback('callback', 'error', 'connection does not exist');
        }

        if (outputs[audio_stream_id]) {
            log.debug('linkup, connectionId:', connectionId, 'audio_stream_id:', audio_stream_id);
            outputs[audio_stream_id].dispatcher.addDestination('audio', connections[connectionId].connection);
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
                outputs[connections[connectionId].audioFrom].dispatcher.removeDestination('audio', connections[connectionId].connection);
            }
            connections[connectionId].audioFrom = undefined;
        }
    };

    that.enableVAD = function (periodMS, roomId, observer) {
        engine.enableVAD(periodMS, function (activeParticipant) {
            log.debug('enableVAD, activeParticipant:', activeParticipant);
            amqper.callRpc(observer, 'onAudioActiveParticipant', [roomId, activeParticipant]);
        });
    };

    that.resetVAD = function () {
        engine.resetVAD();
    };

    that.init = function (service, config, callback) {
        if (service === 'mixing') {
            initEngine(config, callback);
        } else if (service === 'transcoding') {
            var audioConfig = {};
            initEngine(audioConfig, callback);
        } else {
            log.error('Unknown service type to init an audio node:', service);
            callback('callback', 'error', 'Unknown service type to init an audio node.');
        }
    };

    return that;
};
