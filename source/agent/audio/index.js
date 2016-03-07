/*global require, exports*/
'use strict';

var woogeenInternalIO = require('../../../bindings/internalIO/build/Release/internalIO');
var InternalIn = woogeenInternalIO.InternalIn;
var InternalOut = woogeenInternalIO.InternalOut;
var MediaFrameMulticaster = require('../../../bindings/mediaFrameMulticaster/build/Release/mediaFrameMulticaster').MediaFrameMulticaster;
var AudioMixer = require('../../../bindings/audioMixer/build/Release/audioMixer').AudioMixer;

var logger = require('../../../common/logger').logger;
var amqper = require('../../../common/amqper');

// Logger
var log = logger.getLogger('AudioNode');

var AudioEngine = function () {
    var that = {},
        engine,
        observer,

        supported_codecs = [],

        /*{StreamID : {owner: TerminalID,
                       codec: 'pcmu' | 'pcma' | 'isac_16000' | 'isac_32000' | 'opus_48000_2' |...,
                       dispatcher: MediaFrameMulticaster,
                       subscriptions: {SubscriptionID: InternalOut}
                       }}*/
        outputs = {},

        /*{StreamID : {owner: TerminalID,
                       connection: InternalIn}
          }*/
        inputs = {};

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
                                      subscriptions: {}};
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
            for (var subscription_id in output.subscriptions) {
                 output.dispatcher.removeDestination('audio', output.subscriptions[subscription_id]);
                 output.subscriptions[subscription_id].close();
                 delete output.subscriptions[subscription_id];
            }
            engine.removeOutput(output.owner);
            output.dispatcher.close();
            delete outputs[stream_id];
        }
    };

    that.initEngine = function (config, callback) {
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
        if (inputs[stream_id] === undefined) {
            log.debug('publish stream:', stream_id, 'stream_type:', stream_type);
            addInput(stream_id, options.owner, options.audio_codec, options.protocol, function () {
                callback('callback', inputs[stream_id].connection.getListeningPort());
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

    that.subscribe = function (subscription_id, subscription_type, audio_stream_id, video_stream_id, options, callback) {
        if (outputs[audio_stream_id]) {
            log.debug('subscribe, subscription_id:', subscription_id, 'subscription_type:', subscription_type, 'audio_stream_id:', audio_stream_id, 'options:', options);
            var conn = new InternalOut(options.protocol, options.dest_ip, options.dest_port);
            outputs[audio_stream_id].dispatcher.addDestination('audio', conn);
            outputs[audio_stream_id].subscriptions[subscription_id] = conn;
            callback('callback', 'ok');
        } else {
            log.error('Stream does not exist!', audio_stream_id);
            callback('callback', 'error', 'Stream does not exist!');
        }
    };

    that.unsubscribe = function (subscription_id) {
        for (var stream_id in outputs) {
            if (outputs[stream_id].subscriptions[subscription_id]) {
                outputs[stream_id].dispatcher.removeDestination('audio', outputs[stream_id].subscriptions[subscription_id]);
                delete outputs[stream_id].subscriptions[subscription_id];
            }
        }
    };

    that.enableVAD = function (periodMS, roomId, observer) {
        engine.enableVAD(periodMS, function (activeParticipant) {
            log.debug('enableVAD, activeParticipant:', activeParticipant);
            amqper.callRpc(observer, 'eventReport', ['vad', roomId, {active_terminal: activeParticipant, data: null}]);
        });
    };

    return that;
};

exports.AudioNode = function () {
    var that = new AudioEngine();

    that.init = function (service, config, callback) {
        if (service === 'mixing') {
            that.initEngine(config, callback);
        } else if (service === 'transcoding') {
            var audioConfig = {};
            that.initEngine(audioConfig, callback);
        } else {
            log.error('Unknown service type to init an audio node:', service);
            callback('callback', 'error', 'Unknown service type to init an audio node.');
        }
    };

    return that;
};
