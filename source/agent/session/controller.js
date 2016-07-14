/*global require, module*/
'use strict';

var logger = require('./logger').logger;
var makeRPC = require('./makeRPC').makeRPC;

// Logger
var log = logger.getLogger('Controller');

module.exports = function (spec, on_init_ok, on_init_failed) {

    var that = {};

    var cluster = spec.cluster,
        rpc_client = spec.rpcClient,
        amqper = spec.amqper,
        config = spec.config,
        room_id = spec.room,
        observer = spec.observer,
        mixed_stream_id = spec.room,
        supported_audio_codecs = [],
        supported_video_codecs = [],
        supported_video_resolutions = [],
        enable_audio_transcoding = config.enableAudioTranscoding,
        enable_video_transcoding = config.enableVideoTranscoding,
        audio_mixer,
        video_mixer;

    /*
    {terminalID: {owner: ParticipantID | RoomId(for amixer and vmixer),
                  type: 'participant' | 'amixer' | 'axcoder' | 'vmixer' | 'vxcoder',
                  locality: {agent: AgentRpcID, node: NodeRpcID},
                  published: [StreamID],
                  subscribed: {SubscriptionID: {audio: StreamID, video: StreamID}}
                 }
    }
    */
    var terminals = {};

    /*
    {StreamID: {owner: terminalID,
                audio: {codec: 'pcmu' | 'pcma' | 'isac_16000' | 'isac_32000' | 'opus_48000_2' |...
                        subscribers: [terminalID]} | undefined,
                video: {codec: 'h264' | 'vp8' |...,
                        resolution: 'cif' | 'vga' | 'svga' | 'xga' | 'hd720p' | 'sif' | 'hvga' | 'r640x360' | 'r480x360' | 'qcif' | 'hd1080p' | 'uhd_4k' |...,
                        framerate: Number(1~120) | undefined,
                        subscribers: [terminalID]
                       } | undefined,
                spread: [NodeRpcID]
               }
     }
    */
    var streams = {};

    var enableAVCoordination = function () {
        log.debug('enableAVCoordination');
        if (config.enableMixing && audio_mixer && video_mixer) {
            makeRPC(
                amqper,
                terminals[audio_mixer].locality.node,
                'enableVAD',
                [1000, room_id, observer]);
        }
    };

    var resetVAD = function () {
        if (config.enableMixing &&
            config.mediaMixing &&
            config.mediaMixing.video &&
            config.mediaMixing.video.avCoordinated &&
            audio_mixer &&
            video_mixer) {
            makeRPC(
                amqper,
                terminals[audio_mixer].locality.node,
                'resetVAD',
                []);
        }
    };

    var initialize = function () {
        log.debug('initialize room', room_id);
        if (config.enableMixing) {
            audio_mixer = Math.random() * 1000000000000000000 + '';
            newTerminal(audio_mixer, 'amixer', room_id, false, function () {
                log.debug('new audio mixer ok. audio_mixer:', audio_mixer, 'room_id:', room_id);
                makeRPC(
                    amqper,
                    terminals[audio_mixer].locality.node,
                    'init',
                    ['mixing', config.mediaMixing.audio],
                    function (supported_audio) {
                        log.debug('init audio mixer ok. room_id:', room_id);
                        video_mixer = Math.random() * 1000000000000000000 + '';
                        newTerminal(video_mixer, 'vmixer', room_id, false, function () {
                            log.debug('new video mixer ok. video_mixer:', video_mixer, 'room_id:', room_id);
                            makeRPC(
                                amqper,
                                terminals[video_mixer].locality.node,
                                'init',
                                ['mixing', config.mediaMixing.video, room_id, observer],
                                function (supported_video) {
                                    log.debug('init video mixer ok. room_id:', room_id);
                                    supported_audio_codecs = supported_audio.codecs;
                                    supported_video_codecs = supported_video.codecs;
                                    supported_video_resolutions = supported_video.resolutions;

                                    if (typeof on_init_ok === 'function') on_init_ok(supported_video_resolutions);
                                    if (config.mediaMixing.video.avCoordinated) {
                                        enableAVCoordination();
                                    }
                                }, function (error_reason) {
                                    log.error('init video_mixer failed. room_id:', room_id, 'reason:', error_reason);
                                    deleteTerminal(video_mixer);
                                    makeRPC(
                                        amqper,
                                        terminals[audio_mixer].locality.node,
                                        'deinit',
                                        [audio_mixer]);
                                    deleteTerminal(audio_mixer);
                                    audio_mixer = undefined;
                                    video_mixer = undefined;
                                    if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                                });
                            }, function (error_reason) {
                                log.error('new video mixer failed. room_id:', room_id, 'reason:', error_reason);
                                makeRPC(
                                    amqper,
                                    terminals[audio_mixer].locality.node,
                                    'deinit',
                                    [audio_mixer]);
                                deleteTerminal(audio_mixer);
                                audio_mixer = undefined;
                                video_mixer = undefined;
                                if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                            });
                }, function (error_reason) {
                    log.error('init audio_mixer failed. room_id:', room_id, 'reason:', error_reason);
                    audio_mixer = undefined;
                    if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                });
            }, function(error_reason) {
                log.error('new audio_mixer failed. room_id:', room_id, 'reason:', error_reason);
                audio_mixer = undefined;
                if (typeof on_init_failed === 'function') on_init_failed(error_reason);
            });
        } else {
            if (typeof on_init_ok === 'function') on_init_ok();
        }
        return that;
    };

    var deinitialize = function () {
        log.debug('deinitialize');

        for (var terminal_id in terminals) {
            if (terminals[terminal_id].type === 'participant') {
                terminals[terminal_id].published.map(function (stream_id) {
                    unpublishStream(stream_id);
                });
            } else if (terminals[terminal_id].type === 'amixer' ||
                       terminals[terminal_id].type === 'vmixer' ||
                       terminals[terminal_id].type === 'axcoder' ||
                       terminals[terminal_id].type === 'vxcoder') {
                makeRPC(
                    amqper,
                    terminals[terminal_id].locality.node,
                    'deinit',
                    [terminal_id]);
            }
            deleteTerminal(terminal_id);
        }

        supported_audio_codecs = [];
        supported_video_codecs = [];
        supported_video_resolutions = [];

        audio_mixer = undefined;
        video_mixer = undefined;

        terminals = {};
        streams = {};
    };

    var newTerminal = function (terminal_id, terminal_type, owner, preAssignedNode, on_ok, on_error) {
        log.debug('newTerminal:', terminal_id, 'terminal_type:', terminal_type);
        if (terminals[terminal_id] === undefined) {
                var purpose = (terminal_type === 'vmixer' || terminal_type === 'vxcoder') ? 'video'
                              : ((terminal_type === 'amixer' || terminal_type === 'axcoder') ? 'audio' : 'unknown');

                var nodeLocality = (preAssignedNode ? Promise.resolve(preAssignedNode)
                                               : rpc_client.getMediaNode(cluster, purpose, {session: room_id, consumer: terminal_id}));

                return nodeLocality
                    .then(function(locality) {
                        terminals[terminal_id] = {owner: owner,
                                                  type: terminal_type,
                                                  locality: locality,
                                                  published: [],
                                                  subscribed: {}}
                        on_ok();
                    }, function(err) {
                        on_error(err.message);
                    });
        } else {
            on_ok();
        }
    };

    var deleteTerminal = function (terminal_id) {
        log.debug('deleteTerminal:', terminal_id);
        if (terminals[terminal_id]) {
            rpc_client.recycleMediaNode(terminals[terminal_id].locality.agent, terminals[terminal_id].locality.node, {session: room_id, consumer: terminal_id});
            delete terminals[terminal_id];
        }
    };

    var publisherCount = function () {
        var count = 0;
        for (var t in terminals) {
            if (terminals[t].type === 'participant' &&
                terminals[t].published.length > 0) {
                count = count + 1;
            }
        }
        return count;
    };

    var spreadStream = function (stream_id, target_node, target_node_type, on_ok, on_error) {
        log.debug('spreadStream, stream_id:', stream_id, 'target_node:', target_node, 'target_node_type:', target_node_type);

        if (!streams[stream_id] || !terminals[streams[stream_id].owner]) {
            return on_error('Cannot spread a non-existing stream');
        }

        var stream_owner = streams[stream_id].owner,
            original_node = terminals[stream_owner].locality.node,
            audio = ((streams[stream_id].audio && target_node_type !== 'vmixer' && target_node_type !== 'vxcoder') ? true : false),
            video = ((streams[stream_id].video && target_node_type !== 'amixer' && target_node_type !== 'axcoder') ? true : false),
            spread_id = stream_id + '@' + target_node;

        if (!audio && !video) {
            return on_error('Cannot spread stream without audio/video.');
        }

        if (original_node === target_node || hasStreamBeenSpread(stream_id, target_node)) {
            log.debug('spread already exists:', spread_id);
            return on_ok();
        }

        var on_spread_failed = function(reason, cancel_pub, cancel_sub) {
            if (cancel_sub) {
                makeRPC(
                    amqper,
                    original_node,
                    'unsubscribe',
                    [spread_id]);
            }

            if (cancel_pub) {
                makeRPC(
                    amqper,
                    target_node,
                    'unpublish',
                    [stream_id]);
            }

            on_error(reason);
        };

        makeRPC(
            amqper,
            target_node,
            'publish',
            [stream_id, 'internal', {owner: terminals[stream_owner].owner,
                                     audio: (audio ? {codec: streams[stream_id].audio.codec} : false),
                                     video: (video ? {codec: streams[stream_id].video.codec} : false),
                                     protocol: 'tcp'}],
            function (dest) {
                if (!streams[stream_id]) {
                    on_spread_failed('Late coming callback for spreading stream.', true, false);
                }
                log.debug('internally publish ok, dest:', dest.ip, dest.port);
                makeRPC(
                    amqper,
                    original_node,
                    'subscribe',
                    [spread_id, 'internal', {dest_ip: dest.ip, dest_port: dest.port, protocol: 'tcp'}],
                    function () {
                        log.debug('internally subscribe ok');
                        if (streams[stream_id]) {
                            makeRPC(
                                amqper,
                                original_node,
                                'linkup',
                                [spread_id, audio ? stream_id : undefined, video ? stream_id : undefined],
                                function () {
                                    log.debug('internally linkup ok');
                                    if (streams[stream_id]) {
                                        streams[stream_id].spread.push(target_node);
                                        on_ok();
                                    } else {
                                        on_spread_faild('Late coming callback for spreading stream.', true, true);
                                    }
                                },
                                function (error_reason) {
                                    on_spread_failed(error_reason, true, true);
                                });
                        } else {
                            on_spread_failed('Late coming callback for spreading stream.', true, true);
                        }
                    },
                    function (error_reason) {
                        on_spread_failed(error_reason, true, false);
                    });
            },
            on_error);
    };

    var shrinkStream = function (stream_id, target_node) {
        log.debug('shrinkStream, stream_id:', stream_id, 'target_node:', target_node);
        if (streams[stream_id] && terminals[streams[stream_id].owner]) {
            var stream_owner = streams[stream_id].owner,
                original_node = terminals[stream_owner].locality.node,
                spread_id = stream_id + '@' + target_node;

            if (original_node !== target_node && !isSpreadInUse(stream_id, target_node)) {
                var i = streams[stream_id].spread.indexOf(target_node);
                if (i !== -1) {
                    streams[stream_id].spread.splice(i, 1);
                }

                makeRPC(
                    amqper,
                    original_node,
                    'unsubscribe',
                    [spread_id]);

                makeRPC(
                    amqper,
                    target_node,
                    'unpublish',
                    [stream_id]);
            }
        }
    };

    var hasStreamBeenSpread = function (stream_id, node) {
        if (streams[stream_id] &&
            (streams[stream_id].spread.indexOf(node) !== -1)) {
            return true;
        }

        return false;
    };

    var isSpreadInUse = function (stream_id, node) {
        var audio_subscribers = (streams[stream_id] && streams[stream_id].audio && streams[stream_id].audio.subscribers) || [],
            video_subscribers = (streams[stream_id] && streams[stream_id].video && streams[stream_id].video.subscribers) || [],
            subscribers = audio_subscribers.concat(video_subscribers.filter(function (item) { return audio_subscribers.indexOf(item) < 0;}));

        for (var i in subscribers) {
            if (terminals[subscribers[i]] && terminals[subscribers[i]].locality.node === node) {
                return true;
            }
        }

        return false;
    };

    var mixAudio = function (stream_id, on_ok, on_error) {
        log.debug('to mix audio of stream:', stream_id);
        if (streams[stream_id] && audio_mixer && terminals[audio_mixer]) {
            var target_node = terminals[audio_mixer].locality.node,
                spread_id = stream_id + '@' + target_node;
            spreadStream(stream_id, target_node, 'amixer', function() {
                if (terminals[audio_mixer]) {
                    terminals[audio_mixer].subscribed[spread_id] = {audio: stream_id};
                    streams[stream_id].audio.subscribers.push(audio_mixer);
                    on_ok();
                } else {
                    shrinkStream(stream_id, target_node);
                    on_error('Audio mixer is early released.');
                }
            }, on_error);
        } else {
            log.error('Audio mixer is NOT ready.');
            on_error('Audio mixer is NOT ready.');
        }
    };

    var unmixAudio = function (stream_id) {
        log.debug('to unmix audio of stream:', stream_id);
        if (streams[stream_id] && streams[stream_id].audio && audio_mixer && terminals[audio_mixer]) {
            var target_node = terminals[audio_mixer].locality.node,
                spread_id = stream_id + '@' + target_node,
                i = streams[stream_id].audio.subscribers.indexOf(audio_mixer);
            delete terminals[audio_mixer].subscribed[spread_id];
            i > -1 && streams[stream_id].audio.subscribers.splice(i, 1);

            shrinkStream(stream_id, target_node);
        }
    };

    var mixVideo = function (stream_id, on_ok, on_error) {
        log.debug('to mix video of stream:', stream_id);
        if (streams[stream_id] && video_mixer && terminals[video_mixer]) {
            var target_node = terminals[video_mixer].locality.node,
                spread_id = stream_id + '@' + target_node;
            spreadStream(stream_id, target_node, 'vmixer', function() {
                if (terminals[video_mixer]) {
                    terminals[video_mixer].subscribed[spread_id] = {video: stream_id};
                    streams[stream_id].video.subscribers.push(video_mixer);
                    on_ok();
                } else {
                    shrink(stream_id, target_node);
                    on_error('Video mixer is early released.');
                }
            }, on_error);
        } else {
            log.error('Video mixer is NOT ready.');
            on_error('Video mixer is NOT ready.');
        }
    };

    var unmixVideo = function (stream_id) {
        log.debug('to unmix video of stream:', stream_id);
        if (streams[stream_id] && streams[stream_id].video && video_mixer && terminals[video_mixer]) {
            var target_node = terminals[video_mixer].locality.node,
                spread_id = stream_id + '@' + target_node,
                i = streams[stream_id].video.subscribers.indexOf(video_mixer);
            delete terminals[video_mixer].subscribed[spread_id];
            i > -1 && streams[stream_id].video.subscribers.splice(i, 1);

            shrinkStream(stream_id, target_node);
        }
    };

    var mixStream = function (stream_id, on_ok, on_error) {
        log.debug('to mix stream:', stream_id);
        if (streams[stream_id].audio) {
            mixAudio(stream_id, function () {
                if (streams[stream_id].video) {
                    mixVideo(stream_id, on_ok, function (error_reason) {
                        unmixAudio(stream_id);
                        on_error(error_reason);
                    });
                }
            }, on_error);
        } else if (streams[stream_id].video) {
            mixVideo(stream_id, on_ok, on_error);
        }
    };

    var unmixStream = function (stream_id) {
        log.debug('to unmix stream:', stream_id);
        if (streams[stream_id] && streams[stream_id].audio) {
            unmixAudio(stream_id);
        }

        if (streams[stream_id] && streams[stream_id].video) {
            unmixVideo(stream_id);
        }
    };

    var spawnMixedAudio = function (participant_id, audio_codec, on_ok, on_error) {
        if (audio_mixer && terminals[audio_mixer]) {
            var amixer_node = terminals[audio_mixer].locality.node;
            log.debug('spawnMixedAudio, for participant:', participant_id, 'audio_codec:', audio_codec);
            makeRPC(
                amqper,
                amixer_node,
                'generate',
                [participant_id, audio_codec],
                function (stream_id) {
                    log.debug('spawnMixedAudio ok, stream_id:', stream_id);
                    if (streams[stream_id] === undefined) {
                        streams[stream_id] = {owner: audio_mixer,
                                              audio: {codec: audio_codec,
                                                      subscribers: []},
                                              video: undefined,
                                              spread: []};
                        terminals[audio_mixer].published.push(stream_id);
                    }
                    on_ok(stream_id);
                }, on_error);
        } else {
            on_error('Audio mixer is not ready.');
        }
    };

    var spawnMixedVideo = function (video_codec, video_resolution, on_ok, on_error) {
        if (video_mixer && terminals[video_mixer]) {
            var vmixer_node = terminals[video_mixer].locality.node;
            makeRPC(
                amqper,
                vmixer_node,
                'generate',
                [video_codec, video_resolution],
                function (stream_id) {
                    if (streams[stream_id] === undefined) {
                        streams[stream_id] = {owner: video_mixer,
                                              audio: undefined,
                                              video: {codec: video_codec,
                                                      resolution: video_resolution,
                                                      subscribers: []},
                                              spread: []};
                        terminals[video_mixer].published.push(stream_id);
                    }
                    on_ok(stream_id);
                },
                on_error);
        } else {
            on_error('Video mixer is not ready.');
        }
    };

    var getMixedAudio = function (participant_id, audio_codec, on_ok, on_error) {
        spawnMixedAudio(participant_id,
                        audio_codec,
                        on_ok,
                        on_error);
    };

    var getMixedVideo = function (video_codec, video_resolution, on_ok, on_error) {
        var candidates = Object.keys(streams).filter(
            function (stream_id) { return streams[stream_id].owner === video_mixer &&
                                          streams[stream_id].video &&
                                          streams[stream_id].video.codec === video_codec &&
                                          (streams[stream_id].video.resolution === video_resolution);
            });
        if (candidates.length > 0) {
            on_ok(candidates[0]);
        } else {
            spawnMixedVideo(video_codec,
                            video_resolution,
                            on_ok,
                            on_error);
        }
    };

    var spawnTranscodedAudio = function (axcoder, audio_codec, on_ok, on_error) {
        var axcoder_node = terminals[axcoder].locality.node;
        log.debug('spawnTranscodedAudio, audio_codec:', audio_codec);
        makeRPC(
            amqper,
            axcoder_node,
            'generate',
            [audio_codec, audio_codec],
            function (stream_id) {
                log.debug('spawnTranscodedAudio ok, stream_id:', stream_id);
                if (streams[stream_id] === undefined) {
                    log.debug('add transcoded stream to streams:', stream_id);
                    streams[stream_id] = {owner: axcoder,
                                          audio: {codec: audio_codec,
                                                  subscribers: []},
                                          video: undefined,
                                          spread: []};
                    terminals[axcoder].published.push(stream_id);
                }
                on_ok(stream_id);
            }, on_error);
    };

    var findExistingTranscodedAudio = function (axcoder, audio_codec, on_ok, on_error) {
        var published = terminals[axcoder].published;
        for (var j in published) {
            if (streams[published[j]] && streams[published[j]].audio && streams[published[j]].audio.codec === audio_codec) {
                on_ok(published[j]);
                return;
            }
        }
        on_error();
    };

    var findExistingAudioTranscoder = function (stream_id, on_ok, on_error) {
        var subscribers = streams[stream_id].audio.subscribers;
        for (var i in subscribers) {
            if (terminals[subscribers[i]] && terminals[subscribers[i]].type === 'axcoder') {
                on_ok(subscribers[i]);
                return;
            }
        }
        on_error();
    };

    var getAudioTranscoder = function (stream_id, on_ok, on_error) {
        findExistingAudioTranscoder(stream_id, on_ok, function () {
            var axcoder = Math.random() * 1000000000000000000 + '';
            newTerminal(axcoder, 'axcoder', streams[stream_id].owner, false, function () {
                var on_failed = function (reason) {
                    makeRPC(
                        amqper,
                        terminals[axcoder].locality.node,
                        'deinit',
                        [axcoder]);
                    deleteTerminal(axcoder);
                    on_error(reason);
                };

                makeRPC(
                    amqper,
                    terminals[axcoder].locality.node,
                    'init',
                    ['transcoding', {}],
                    function (supported_audio) {
                        var target_node = terminals[axcoder].locality.node,
                            spread_id = stream_id + '@' + target_node;
                        spreadStream(stream_id, target_node, 'axcoder', function () {
                            if (terminals[axcoder]) {
                                terminals[axcoder].subscribed[spread_id] = {audio: stream_id};
                                streams[stream_id].audio.subscribers.push(axcoder);
                                on_ok(axcoder);
                            } else {
                                shrinkStream(stream_id, target_node);
                                on_failed('Audio transcoder is early released.');
                            }
                        }, on_failed);
                    }, on_error);
            }, on_error);
        });
    };

    var getTranscodedAudio = function (audio_codec, stream_id, on_ok, on_error) {
        getAudioTranscoder(stream_id, function (axcoder) {
            findExistingTranscodedAudio(axcoder, audio_codec, on_ok, function () {
                spawnTranscodedAudio(axcoder, audio_codec, on_ok, on_error);
            });
        }, on_error);
    };

    var spawnTranscodedVideo = function (vxcoder, video_codec, video_resolution, on_ok, on_error) {
        var vxcoder_node = terminals[vxcoder].locality.node;
        log.debug('spawnTranscodedVideo, video_codec:', video_codec, 'video_resolution:', video_resolution);
        makeRPC(
            amqper,
            vxcoder_node,
            'generate',
            [video_codec, video_resolution],
            function (stream_id) {
                log.debug('spawnTranscodedVideo ok, stream_id:', stream_id);
                if (streams[stream_id] === undefined) {
                    log.debug('add to streams.');
                    streams[stream_id] = {owner: vxcoder,
                                          audio: undefined,
                                          video: {codec: video_codec,
                                                  resolution: video_resolution,
                                                  subscribers: []},
                                          spread: []};
                    terminals[vxcoder].published.push(stream_id);
                }
                on_ok(stream_id);
            }, on_error);
    };

    var findExistingTranscodedVideo = function (vxcoder, video_codec, video_resolution, on_ok, on_error) {
        var published = terminals[vxcoder].published;
        for (var j in published) {
            if (streams[published[j]] && streams[published[j]].video && streams[published[j]].video.codec === video_codec && streams[published[j]].video.resolution === video_resolution) {
                on_ok(published[j]);
                return;
            }
        }
        on_error();
    };

    var findExistingVideoTranscoder = function (stream_id, on_ok, on_error) {
        var subscribers = streams[stream_id].video.subscribers;
        for (var i in subscribers) {
            if (terminals[subscribers[i]] && terminals[subscribers[i]].type === 'vxcoder') {
                on_ok(subscribers[i]);
                return;
            }
        }
        on_error();
    };

    var getVideoTranscoder = function (stream_id, on_ok, on_error) {
        findExistingVideoTranscoder(stream_id, on_ok, function () {
            var vxcoder = Math.random() * 1000000000000000000 + '';
            newTerminal(vxcoder, 'vxcoder', streams[stream_id].owner, false, function () {
                var on_failed = function (error_reason) {
                    makeRPC(
                        amqper,
                        terminals[vxcoder].locality.node,
                        'deinit',
                        [vxcoder]);
                    deleteTerminal(vxcoder);
                    on_error(error_reason);
                };

                makeRPC(
                    amqper,
                    terminals[vxcoder].locality.node,
                    'init',
                    ['transcoding', {resolution: streams[stream_id].video.resolution || 'hd1080p'/*FIXME: hard code*/}, room_id, undefined],
                    function (supported_video) {
                        var target_node = terminals[vxcoder].locality.node,
                            spread_id = stream_id + '@' + target_node;
                        spreadStream(stream_id, target_node, 'vxcoder', function () {
                            if (terminals[vxcoder]) {
                                terminals[vxcoder].subscribed[spread_id] = {video: stream_id};
                                streams[stream_id].video.subscribers.push(vxcoder);
                                on_ok(vxcoder);
                            } else {
                                shrinkStream(stream_id, target_node);
                                on_failed('Video transcoder is early released.');
                            }
                        }, on_failed);
                    }, on_error);
            }, on_error);
        });
    };

    var getTranscodedVideo = function (video_codec, video_resolution, stream_id, on_ok, on_error) {
        getVideoTranscoder(stream_id, function (vxcoder) {
            findExistingTranscodedVideo(vxcoder, video_codec, video_resolution, on_ok, function () {
                spawnTranscodedVideo(vxcoder, video_codec, video_resolution, on_ok, on_error);
            });
        }, on_error);
    };

    var terminateTemporaryStream = function (stream_id) {
        log.debug('terminaleTemporaryStream:', stream_id);
        var owner = streams[stream_id].owner;
        var node = terminals[owner].locality.node;
        makeRPC(
            amqper,
            node,
            'degenerate',
            [stream_id]);
        delete streams[stream_id];

        var i = terminals[owner].published.indexOf(stream_id);
        i > -1 && terminals[owner].published.splice(i ,1);

        if (terminals[owner].published.length === 0 && (terminals[owner].type === 'axcoder' || terminals[owner].type === 'vxcoder')) {
            for (var subscription_id in terminals[owner].subscribed) {
                unsubscribeStream(owner, subscription_id);
            }
            deleteTerminal(owner);
        }
    };

    var recycleTemporaryAudio = function (stream_id) {
        log.debug('trying recycleTemporaryAudio:', stream_id);
        if (streams[stream_id] &&
            streams[stream_id].audio &&
            streams[stream_id].audio.subscribers.length === 0) {

            if (terminals[streams[stream_id].owner] && (terminals[streams[stream_id].owner].type === 'amixer' || terminals[streams[stream_id].owner].type === 'axcoder')) {
                terminateTemporaryStream(stream_id);
            }
        }
    };

    var recycleTemporaryVideo = function (stream_id) {
        log.debug('trying recycleTemporaryVideo:', stream_id);
        if (streams[stream_id] &&
            streams[stream_id].video &&
            streams[stream_id].video.subscribers.length === 0) {

            if (terminals[streams[stream_id].owner] && (terminals[streams[stream_id].owner].type === 'vmixer' || terminals[streams[stream_id].owner].type === 'vxcoder')) {
                terminateTemporaryStream(stream_id);
            }
        }
    };

    var getMixedCodec = function (subMedia, supportedMixCodecs) {
        var codec = 'unavailable';
        if (subMedia.codecs instanceof Array && subMedia.codecs.length > 0) {
            for (var i = 0; i < subMedia.codecs.length; i++) {
                if (supportedMixCodecs.indexOf(subMedia.codecs[i]) !== -1) {
                    codec = subMedia.codecs[i];
                    break;
                }
            }
        } else {
            codec = supportedMixCodecs[0];
        }
        return codec;
    };

    var getForwardCodec = function (subMedia, originalCodec, transcodingEnabled) {
        var codec = 'unavailable';
        if (subMedia.codecs instanceof Array && subMedia.codecs.length > 0) {
            var i = 0;
            for (; i < subMedia.codecs.length; i++) {
                if (subMedia.codecs[i] === originalCodec) {
                    codec = subMedia.codecs[i];
                    break;
                }
            }
            if (i === subMedia.codecs.length && transcodingEnabled) {
                codec = subMedia.codecs[0];
            }
        } else {
            codec = originalCodec;
        }
        return codec;
    };

    var getMixedResolution = function (subMedia, supportedMixResolutions) {
        var resolution = 'unavailable';
        if (subMedia.resolution) {
            if (supportedMixResolutions.indexOf(subMedia.resolution) !== -1) {
                resolution = subMedia.resolution;
            }
        } else {
            resolution = supportedMixResolutions[0];
        }
        return resolution;
    };

    var getForwardResolution = function (subMedia, originalResolution, transcodingEnabled) {
        var resolution = 'unavailable';
        if (subMedia.resolution) {
            if (subMedia.resolution === originalResolution || transcodingEnabled) {
                resolution = subMedia.resolution;
            }
        } else {
            resolution = originalResolution;
        }
        return resolution;
    };

    var getAudioStream = function (participant_id, stream_id, audio_codec, on_ok, on_error) {
        log.debug('getAudioStream, participant_id:', participant_id, 'stream:', streams[stream_id], 'audio_codec:', audio_codec);
        if (stream_id === mixed_stream_id) {
            getMixedAudio(participant_id, audio_codec, function (streamID) {
                log.debug('Got mixed audio:', streamID);
                on_ok(streamID);
            }, on_error);
        } else if (streams[stream_id]) {
            if (streams[stream_id].audio) {
                if (streams[stream_id].audio.codec === audio_codec) {
                    on_ok(stream_id);
                } else {
                    getTranscodedAudio(audio_codec, stream_id, function (streamID) {
                        on_ok(streamID);
                    }, on_error);
                }
            } else {
                on_error('Stream:'+stream_id+' has no audio track.');
            }
        } else {
            on_error('No such an audio stream:'+stream_id);
        }
    };

    var getVideoStream = function (stream_id, video_codec, video_resolution, on_ok, on_error) {
        log.debug('getVideoStream, stream:', streams[stream_id], 'video_codec:', video_codec, 'video_resolution:', video_resolution);
        if (stream_id === mixed_stream_id) {
            getMixedVideo(video_codec, video_resolution, function (streamID) {
                log.debug('Got mixed video:', streamID);
                on_ok(streamID);
            }, on_error);
        } else if (streams[stream_id]) {
            if (streams[stream_id].video) {
                if (streams[stream_id].video.codec === video_codec && (!video_resolution || streams[stream_id].video.resolution === video_resolution)) {
                    on_ok(stream_id);
                } else {
                    getTranscodedVideo(video_codec, video_resolution, stream_id, function (streamID) {
                        on_ok(streamID);
                    }, on_error);
                }
            } else {
                on_error('Stream:'+stream_id+' has no video track.');
            }
        } else {
            on_error('No such a video stream:'+stream_id);
        }
    };

    var unpublishStream = function (stream_id) {
        if (streams[stream_id]) {
            log.debug('unpublishStream:', stream_id, 'stream.owner:', streams[stream_id].owner);
            var stream = streams[stream_id],
                terminal_id = stream.owner,
                node = terminals[terminal_id].locality.node;

            var i = terminals[terminal_id].published.indexOf(stream_id);
            if (i !== -1) {
                config.enableMixing && unmixStream(stream_id);
                removeSubscriptions(stream_id);
                terminals[terminal_id].published.splice(i, 1);
            }

            delete streams[stream_id];
        } else {
            log.info('try to unpublish an unexisting stream:', stream_id);
        }
    };

    var unsubscribeStream = function (subscriber, subscription_id) {
        if (terminals[subscriber]) {
            log.debug('unsubscribeStream, subscriber:', subscriber, 'subscription_id:', subscription_id);
            var node = terminals[subscriber].locality.node,
                subscription = terminals[subscriber].subscribed[subscription_id],
                audio_stream = subscription && subscription.audio,
                video_stream = subscription && subscription.video;

            if (terminals[subscriber].type === 'participant') {
                makeRPC(
                    amqper,
                    node,
                    'cutoff',
                    [subscription_id]);
            }

            if (audio_stream) {
                if (streams[audio_stream] && streams[audio_stream].audio) {
                    var i = streams[audio_stream].audio.subscribers.indexOf(subscriber);
                    i > -1 && streams[audio_stream].audio.subscribers.splice(i, 1);
                }
                terminals[streams[audio_stream].owner] && terminals[streams[audio_stream].owner].locality.node !== node && shrinkStream(audio_stream, node);
                terminals[streams[audio_stream].owner] && terminals[streams[audio_stream].owner].type !== 'participant' && recycleTemporaryAudio(audio_stream);
            }

            if (video_stream) {
                if (streams[video_stream] && streams[video_stream].video) {
                    var i = streams[video_stream].video.subscribers.indexOf(subscriber);
                    i > -1 && streams[video_stream].video.subscribers.splice(i, 1);
                }
                terminals[streams[video_stream].owner] && terminals[streams[video_stream].owner].locality.node !== node && shrinkStream(video_stream, node);
                terminals[streams[video_stream].owner] && terminals[streams[video_stream].owner].type !== 'participant' && recycleTemporaryVideo(video_stream);
            }

            delete terminals[subscriber].subscribed[subscription_id];
        } else {
            log.info('try to unsubscribe to an unexisting terminal:', subscriber);
        }
    };

    var removeSubscriptions = function (stream_id) {
        if (streams[stream_id]) {
            if (streams[stream_id].audio) {
                streams[stream_id].audio.subscribers.forEach(function(terminal_id) {
                    for (var subscription_id in terminals[terminal_id].subscribed) {
                        unsubscribeStream(terminal_id, subscription_id);
                        if (terminals[terminal_id].type === 'axcoder') {
                            for (var i in terminals[terminal_id].published) {
                                unpublishStream(terminals[terminal_id].published[i]);
                            }
                            deleteTerminal(terminal_id);
                        }
                    }
                });
                streams[stream_id].audio.subscribers = [];
            }

            if (streams[stream_id].video) {
                streams[stream_id].video.subscribers.forEach(function(terminal_id) {
                    for (var subscription_id in terminals[terminal_id].subscribed) {
                        unsubscribeStream(terminal_id, subscription_id);
                        if (terminals[terminal_id].type === 'vxcoder') {
                            for (var i in terminals[terminal_id].published) {
                                unpublishStream(terminals[terminal_id].published[i]);
                            }
                            deleteTerminal(terminal_id);
                        }
                    }
                });
                streams[stream_id].audio.subscribers = [];
            }
        }
    };

    // External interfaces.
    that.destroy = function () {
        deinitialize();
    };

    that.publish = function (participantId, streamId, accessNode, streamInfo, unmix, on_ok, on_error) {
        log.debug('publish, participantId: ', participantId, 'streamId:', streamId, 'accessNode:', accessNode.node, 'streamInfo:', JSON.stringify(streamInfo));
        if (streams[streamId] === undefined) {
            var currentPublisherCount = publisherCount();
            if (config.publishLimit < 0 || (config.publishLimit > currentPublisherCount)) {
               var terminal_id = participantId + '-pub-' + streamId;
                newTerminal(terminal_id, 'participant', participantId, accessNode, function () {
                    streams[streamId] = {owner: terminal_id,
                                         audio: streamInfo.audio ? {codec: streamInfo.audio.codec,
                                                                    subscribers: []} : undefined,
                                         video: streamInfo.video ? {codec: streamInfo.video.codec,
                                                                    resolution: streamInfo.video.resolution,
                                                                    framerate: streamInfo.video.framerate,
                                                                    subscribers: []} : undefined,
                                         spread: []
                                         };
                    terminals[terminal_id].published.push(streamId);

                    if (!unmix && config.enableMixing) {
                        mixStream(streamId, function () {
                            log.info('Mix stream['+streamId+'] successfully.');
                        }, function (error_reason) {
                            log.error(error_reason);
                        });
                    } else {
                        log.debug('will not mix stream:', streamId);
                    }
                    log.debug('publish ok.');
                    on_ok();
                }, function (error_reason) {
                    on_error(error_reason);
                });
            } else {
                on_error('Too many publishers.');
            }
        } else {
            on_error('Stream[' + streamId + '] already set for ' + participantId);
        }
    };

    that.unpublish = function (participantId, streamId) {
        log.debug('unpublish, stream_id:', streamId);
        var terminal_id = participantId + '-pub-' + streamId;

        if (streams[streamId] === undefined
            || streams[streamId].owner !== terminal_id
            || terminals[terminal_id] === undefined
            || terminals[terminal_id].published.indexOf(streamId) === -1) {
            log.info('unpublish a rogue stream');
        }

        if (streams[streamId]) {
            unpublishStream(streamId);
        }

        deleteTerminal(terminal_id);
    };

    that.subscribe = function(participantId, subscriptionId, accessNode, subInfo, on_ok, on_error) {
        log.debug('subscribe, participantId:', participantId, 'subscriptionId:', subscriptionId, 'accessNode:', accessNode.node, 'subInfo:', JSON.stringify(subInfo));
        if ((!subInfo.audio || (streams[subInfo.audio.fromStream] && streams[subInfo.audio.fromStream].audio) || subInfo.audio.fromStream === mixed_stream_id)
            && (!subInfo.video || (streams[subInfo.video.fromStream] && streams[subInfo.video.fromStream].video) || subInfo.video.fromStream === mixed_stream_id)) {

            var audio_codec = (subInfo.audio && (subInfo.audio.fromStream === mixed_stream_id) && getMixedCodec(subInfo.audio, supported_audio_codecs))
                              || (subInfo.audio && getForwardCodec(subInfo.audio, streams[subInfo.audio.fromStream].audio.codec, enable_audio_transcoding))
                              || undefined;

            if (audio_codec === 'unavailable') {
                log.error('No available audio codec');
                log.debug('subInfo.audio:', subInfo.audio, 'targetStream.video:', streams[subInfo.audio.fromStream].audio, 'enable_audio_transcoding:', enable_audio_transcoding);
                return on_error('No available audio codec');
            }

            var video_codec = (subInfo.video && (subInfo.video.fromStream === mixed_stream_id) && getMixedCodec(subInfo.video, supported_video_codecs))
                              || (subInfo.video && getForwardCodec(subInfo.video, streams[subInfo.video.fromStream].video.codec, enable_video_transcoding))
                              || undefined;

            if (video_codec === 'unavailable') {
                log.error('No available video codec');
                log.debug('subInfo.video:', subInfo.video, 'targetStream.video:', streams[subInfo.video.fromStream].video, 'enable_video_transcoding:', enable_video_transcoding);
                return on_error('No available video codec');
            }

            var video_resolution = (subInfo.video && (subInfo.video.fromStream === mixed_stream_id) && getMixedResolution(subInfo.video, supported_video_resolutions))
                              || (subInfo.video && getForwardResolution(subInfo.video, streams[subInfo.video.fromStream].video.resolution, enable_video_transcoding))
                              || undefined;

            if (video_resolution === 'unavailable') {
                log.error('No available video resolution');
                log.debug('subInfo.video:', subInfo.video, 'targetStream.video:', streams[subInfo.video.fromStream].video, 'enable_video_transcoding:', enable_video_transcoding);
                return on_error('No available video resolution');
            }

            if ((subInfo.audio && !audio_codec) || (subInfo.video && (!video_codec || !video_resolution))) {
                log.error('No proper audio/video codec or resolution');
                return on_error('No proper audio/video codec or resolution');
            }

            var terminal_id = participantId + '-sub-' + subscriptionId;

            var finaly_error = function (error_reason) {
                log.error('subscribe failed, reason:', error_reason);
                deleteTerminal(terminal_id);
                on_error(error_reason);
            };

            var finally_ok = function (audioStream, videoStream) {
                return function () {
                    if (terminals[terminal_id] &&
                        (!audioStream || streams[audioStream]) &&
                        (!videoStream || streams[videoStream])) {
                        log.debug('subscribe ok, audioStream:', audioStream, 'videoStream', videoStream);

                        terminals[terminal_id].subscribed[subscriptionId] = {};
                        if (audioStream) {
                            streams[audioStream].audio.subscribers = streams[audioStream].audio.subscribers || [];
                            streams[audioStream].audio.subscribers.push(terminal_id);
                            terminals[terminal_id].subscribed[subscriptionId].audio = audioStream;
                        }

                        if (videoStream) {
                            streams[videoStream].video.subscribers = streams[videoStream].video.subscribers || [];
                            streams[videoStream].video.subscribers.push(terminal_id);
                            terminals[terminal_id].subscribed[subscriptionId].video = videoStream;
                        }

                        on_ok('ok');
                    } else {
                        audioStream && recycleTemporaryAudio(audioStream);
                        videoStream && recycleTemporaryVideo(videoStream);
                        finaly_error('The subscribed stream has been broken. Canceling it.');
                    }
                }
            };

            var linkup = function (audioStream, videoStream) {
                log.debug('linkup, subscriber:', terminal_id, 'audioStream:', audioStream, 'videoStream:', videoStream);
                if (terminals[terminal_id] && (!audioStream || streams[audioStream]) && (!videoStream || streams[videoStream])) {
                    makeRPC(
                        amqper,
                        terminals[terminal_id].locality.node,
                        'linkup',
                        [subscriptionId, audioStream, videoStream],
                        finally_ok(audioStream, videoStream),
                        function (reason) {
                            audioStream && recycleTemporaryAudio(audioStream);
                            videoStream && recycleTemporaryVideo(videoStream);
                            finaly_error(reason);
                        });
                } else {
                    audioStream && recycleTemporaryAudio(audioStream);
                    videoStream && recycleTemporaryVideo(videoStream);
                    finaly_error('participant or streams early left');
                }
            };

            var spread2LocalNode = function (audioStream, videoStream, on_spread_ok, on_spread_error) {
                log.debug('spread2LocalNode, subscriber:', terminal_id, 'audioStream:', audioStream, 'videoStream:', videoStream);
                if (terminals[terminal_id] && (audioStream !== undefined || videoStream !== undefined)) {
                    var target_node = terminals[terminal_id].locality.node,
                        target_node_type = terminals[terminal_id].type;

                    if (audioStream === videoStream || audioStream === undefined || videoStream === undefined) {
                        var stream_id = (audioStream || videoStream);
                        spreadStream(stream_id, target_node, target_node_type, function () {
                            if (streams[stream_id] && terminals[terminal_id]) {
                                on_spread_ok();
                            } else {
                                shrinkStream(stream_id, target_node);
                                on_spread_error('terminal or stream early left.');
                            }
                        }, on_error);
                    } else {
                        log.debug('spread audio and video stream independently.');
                        spreadStream(audioStream, target_node, target_node_type, function () {
                            if (streams[audioStream] && streams[videoStream] && terminals[terminal_id]) {
                                log.debug('spread audioStream:', audioStream, ' ok.');
                                spreadStream(videoStream, target_node, target_node_type, function () {
                                    if (streams[audioStream] && streams[videoStream] && terminals[terminal_id]) {
                                        log.debug('spread videoStream:', videoStream, ' ok.');
                                        on_spread_ok();
                                    } else {
                                        streams[videoStream] && shrinkStream(videoStream, target_node);
                                        streams[audioStream] && shrinkStream(audioStream, target_node);
                                        on_spread_error('Uncomplished subscription.');
                                    }
                                }, on_spread_error);
                            } else {
                                streams[audioStream] && shrinkStream(audioStream, target_node);
                                on_spread_error('Uncomplished subscription.');
                            }
                        }, on_spread_error);
                    }
                } else {
                    on_spread_error('terminal or stream does not exist.');
                }
            };

            var doSubscribe = function () {
                var audio_stream, video_stream;
                if (subInfo.audio) {
                    log.debug('require audio track of stream:', subInfo.audio.fromStream);
                    var pt_id = (subInfo.isRecording ? participantId + '-recorder' : participantId);
                    getAudioStream(pt_id, subInfo.audio.fromStream, audio_codec, function (streamID) {
                        audio_stream = streamID;
                        log.debug('Got audio stream:', audio_stream);
                        if (subInfo.video) {
                            log.debug('require video track of stream:', subInfo.video.fromStream);
                            getVideoStream(subInfo.video.fromStream, video_codec, video_resolution, function (streamID) {
                                video_stream = streamID;
                                log.debug('Got video stream:', video_stream);
                                spread2LocalNode(audio_stream, video_stream, function () {
                                    linkup(audio_stream, video_stream);
                                }, function (error_reason) {
                                    recycleTemporaryVideo(video_stream);
                                    recycleTemporaryAudio(audio_stream);
                                    finaly_error(error_reason);
                                });
                            }, function (error_reason) {
                                recycleTemporaryAudio(audio_stream);
                                finaly_error(error_reason);
                            });
                        } else {
                            spread2LocalNode(audio_stream, undefined, function () {
                                linkup(audio_stream, undefined);
                            }, function (error_reason) {
                                recycleTemporaryAudio(audio_stream);
                                finaly_error(error_reason);
                            });
                        }
                    }, finaly_error);
                } else if (subInfo.video) {
                    log.debug('require video track of stream:', subInfo.video.fromStream);
                    getVideoStream(subInfo.video.fromStream, video_codec, video_resolution, function (streamID) {
                        video_stream = streamID;
                        spread2LocalNode(undefined, video_stream, function () {
                            linkup(undefined, video_stream);
                        }, function (error_reason) {
                            recycleTemporaryVideo(video_stream);
                            finaly_error(error_reason);
                        });
                    }, finaly_error);
                } else {
                    log.debug('No audio or video is required.');
                    finaly_error('No audio or video is required.');
                }
            };

            newTerminal(terminal_id, 'participant', participantId, accessNode, function () {
                doSubscribe();
            }, on_error);
        } else {
            on_error('streams do not exist');
        }
    };

    that.unsubscribe = function (participant_id, subscription_id) {
        log.debug('unsubscribe from participant:', participant_id, 'for subscription:', subscription_id);
        var terminal_id = participant_id + '-sub-' + subscription_id;
        if (terminals[terminal_id] && terminals[terminal_id].subscribed[subscription_id]) {
            unsubscribeStream(terminal_id, subscription_id);

            deleteTerminal(terminal_id);
        }
    };

    that.mix = function (stream_id, on_ok, on_error) {
        log.debug('mix, stream_id:', stream_id);
        if (streams[stream_id] && config.enableMixing) {
            mixStream(stream_id, on_ok, on_error);
        } else {
            on_error('Stream does not exist:'+stream_id);
        }
    };

    that.unmix = function (stream_id, on_ok, on_error) {
        log.debug('unmix, stream_id:', stream_id);
        if (streams[stream_id] && config.enableMixing) {
            unmixStream(stream_id);
            on_ok();
        } else {
            on_error('Stream does not exist:'+stream_id);
        }
    };

    that.getRegion = function (stream_id, on_ok, on_error) {
        log.debug('getRegion, stream_id:', stream_id);
        if (video_mixer) {
            makeRPC(
                amqper,
                terminals[video_mixer].locality.node,
                'getRegion',
                [stream_id],
                on_ok,
                on_error);
        }
    };

    that.setRegion = function (stream_id, region, on_ok, on_error) {
        log.debug('setRegion, stream_id:', stream_id, 'region:', region);
        if (video_mixer) {
            makeRPC(
                amqper,
                terminals[video_mixer].locality.node,
                'setRegion',
                [stream_id, region],
                function (data) {
                    on_ok(data);
                    resetVAD();
                }, on_error);
        }
    };

    that.setPrimary = function (participantId) {
        log.debug('setPrimary:', participantId);
        for (var terminal_id in terminals) {
            if (terminals[terminal_id].owner === participantId) {
                for (var i in terminals[terminal_id].published) {
                    var stream_id = terminals[terminal_id].published[i];
                    if (streams[stream_id] && streams[stream_id].video && (streams[stream_id].video.subscribers.indexOf(video_mixer) !== -1)) {
                        makeRPC(
                            amqper,
                            terminals[video_mixer].locality.node,
                            'setPrimary',
                            [stream_id]);
                        return;
                    }
                }
            }
        }
    };

    return initialize();
};
