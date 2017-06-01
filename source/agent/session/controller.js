/*global require, module*/
'use strict';

var assert = require('assert');
var logger = require('./logger').logger;
var makeRPC = require('./makeRPC').makeRPC;

// Logger
var log = logger.getLogger('Controller');

module.exports.create = function (spec, on_init_ok, on_init_failed) {

    var that = {};

    var cluster = spec.cluster,
        rpcReq = spec.rpcReq,
        rpcClient = spec.rpcClient,
        config = spec.config,
        room_id = spec.room,
        selfRpcId = spec.selfRpcId,
        enable_audio_transcoding = config.enableAudioTranscoding,
        enable_video_transcoding = config.enableVideoTranscoding,
        internal_conn_protocol = config.internalConnProtocol;

    /*
    mix_views = {
        view: {
            audio: {
                mixer: 'TerminalID',
                supported_codecs: ['opus', 'pcmu', 'pcm_raw', ...],
            },
            video: {
                mixer: 'TerminalID',
                supported_codecs: ['h264', 'vp8', 'h265', 'vp9', ...],
                supported_resolutions: ['hd1080p', 'vga', ...],
            },
        }
    }
    */
    var mix_views = {};

    /*
    terminals = {terminalID: {owner: ParticipantID | Room's mix stream Id(for amixer and vmixer),
                  type: 'participant' | 'amixer' | 'axcoder' | 'vmixer' | 'vxcoder',
                  locality: {agent: AgentRpcID, node: NodeRpcID},
                  published: [StreamID],
                  subscribed: {SubscriptionID: {audio: StreamID, video: StreamID}}
                 }
    }
    */
    var terminals = {};

    /*
    streams = {StreamID: {owner: terminalID,
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

    // Length 20 number ID generator
    var randomId = function() {
        var length = 20;
        return Math.random().toPrecision(length).toString().substr(2, length);
    };

    // Publish terminal ID generator
    var pubTermId = function(participantId, streamId) {
        return participantId + '-pub-' + streamId;
    };

    // Subscribe terminal ID generator
    var subTermId = function(participantId, subscriptionId) {
        return participantId + '-sub-' + subscriptionId;
    };

    // Given view label, return the mix stream ID
    var getMixStreamOfView = function (viewLabel) {
        if (!mix_views[viewLabel])
            return null;
        return room_id + '-' + viewLabel;
    };

    // Given mix stram ID, return the view
    var getViewOfMixStream = function(mixStreamId) {
        var prefix = room_id + '-';
        if (mixStreamId.indexOf(prefix) != 0) {
            return null;
        }
        var view = mixStreamId.substr(prefix.length);
        return mix_views[view]? view : null;
    };

    var getViewMixingConfig = function(view) {
        if (config.views && config.views[view])
            return config.views[view].mediaMixing;
        return {};
    };

    var enableAVCoordination = function (view) {
        log.debug('enableAVCoordination');
        if (!mix_views[view])
            return;

        var audio_mixer = mix_views[view].audio.mixer;
        var video_mixer = mix_views[view].video.mixer;
        if (config.enableMixing && audio_mixer && video_mixer) {
            makeRPC(
                rpcClient,
                terminals[audio_mixer].locality.node,
                'enableVAD',
                [1000]);
        }
    };

    var resetVAD = function (view) {
        log.debug('resetVAD', view);
        if (!mix_views[view] || !config.enableMixing)
            return;

        var audio_mixer = mix_views[view].audio.mixer;
        var video_mixer = mix_views[view].video.mixer;
        if (audio_mixer && video_mixer && config.views[view].mediaMixing.video.avCoordinated) {
            makeRPC(
                rpcClient,
                terminals[audio_mixer].locality.node,
                'resetVAD',
                []);
        }
    };

    var initMixView = function (view, mediaMixingConfig, onInitOk, onInitError) {
        if (!mix_views[view]) {
            onInitError('Mix view does not exist');
            return;
        }

        // Media mixer initializer
        var mixStreamId = getMixStreamOfView(view);
        var initMixer = (mixerId, type, mixConfig) => new Promise(function(resolve, reject) {
            newTerminal(mixerId, type, mixStreamId, false,
                function onTerminalReady() {
                    log.debug('new terminal ok. terminal_id', mixerId, 'type:', type, 'view:', view, 'mixConfig:', mixConfig);
                    initMediaProcessor(mixerId, ['mixing', mixConfig, room_id, selfRpcId, view])
                    .then(function(initMediaResult) {
                        resolve(initMediaResult);
                    }).catch(function(reason) {
                        log.error("Init media processor failed.:", reason);
                        deleteTerminal(mixerId);
                        reject(reason);
                    });
                },
                function onTerminalFail(reason) {
                    log.error('new mix terminal failed. room_id:', room_id, 'reason:', reason);
                    reject(reason);
                }
            );
        });

        // Initialize audio
        var audio_mixer = randomId();
        initMixer(audio_mixer, 'amixer', mediaMixingConfig.audio).then(
            function onAudioReady(supportedAudio) {
                // Save supported info
                mix_views[view].audio = {
                    mixer: audio_mixer,
                    supported_codecs: supportedAudio.codecs
                };

                // Initialize video
                var video_mixer = randomId();
                initMixer(video_mixer, 'vmixer', mediaMixingConfig.video).then(
                    function onVideoReady(supportedVideo) {
                        // Save supported info
                        mix_views[view].video = {
                            mixer: video_mixer,
                            supported_codecs: supportedVideo.codecs,
                            supported_resolutions: supportedVideo.resolutions
                        };

                        // Enable AV coordination if specified
                        if (mediaMixingConfig.video.avCoordinated) {
                            enableAVCoordination(view);
                        }
                        onInitOk();
                    },
                    function onVideoFail(reason) {
                        // Delete the audio mixer that init successfully
                        deleteTerminal(audio_mixer);
                        onInitError(reason);
                    }
                );
            },
            function onAudioFail(reason) {
                onInitError(reason);
            }
        );
    };

    var initialize = function (on_ok, on_error) {
        log.debug('initialize room', room_id);

        // Mix stream ID is room ID followed by view index
        if (config.enableMixing) {
            if (typeof config.views === 'object' && config.views !== null) {
                // Mutiple views configuration
                var viewProcessed = [];

                Object.keys(config.views).forEach(function(viewLabel, index) {
                    // Initialize mixer engine for each view
                    mix_views[viewLabel] = {};

                    // Save view init promises
                    viewProcessed.push(new Promise(function(resolve, reject) {
                        initMixView(viewLabel, config.views[viewLabel].mediaMixing,
                            function onOk() {
                                log.debug('init ok for view:', viewLabel);
                                resolve(viewLabel);
                            },
                            function onError(reason) {
                                log.error('init fail. view:', viewLabel, 'reason:', reason);
                                delete mix_views[viewLabel];
                                resolve(null);
                            });
                    }));
                });

                Promise.all(viewProcessed).then(function(results) {
                    // Result for callback
                    var viewCount = results.filter(function(re) { return re !== null; }).length;
                    if (viewCount < results.length) {
                        log.debug("Views incomplete initialization", viewCount);
                        on_error(that);
                    } else {
                        on_ok(that);
                    }
                }).catch(function(reason) {
                    log.error("Error initialize views:", reason);
                    on_error(reason);
                });
            } else {
                log.error('Bad room views config - no mediaMixing item when mixing enabled');
                on_error('Bad mixing config');
            }
        } else {
            log.debug('Room disable mixing init ok');
            on_ok(that);
        }
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
                    rpcClient,
                    terminals[terminal_id].locality.node,
                    'deinit',
                    [terminal_id]);
            }
            deleteTerminal(terminal_id);
        }

        mix_views = {};
        terminals = {};
        streams = {};
    };

    var newTerminal = function (terminal_id, terminal_type, owner, preAssignedNode, on_ok, on_error) {
        log.debug('newTerminal:', terminal_id, 'terminal_type:', terminal_type);
        if (terminals[terminal_id] === undefined) {
                var purpose = (terminal_type === 'vmixer' || terminal_type === 'vxcoder') ? 'video'
                              : ((terminal_type === 'amixer' || terminal_type === 'axcoder') ? 'audio' : 'unknown');

                var nodeLocality = (preAssignedNode ? Promise.resolve(preAssignedNode)
                                               : rpcReq.getMediaNode(cluster, purpose, {session: room_id, task: terminal_id}));

                return nodeLocality
                    .then(function(locality) {
                        terminals[terminal_id] = {
                            owner: owner,
                            type: terminal_type,
                            locality: locality,
                            published: [],
                            subscribed: {}};
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
            if (terminals[terminal_id].type === 'amixer'
                || terminals[terminal_id].type === 'axcoder'
                || terminals[terminal_id].type === 'vmixer'
                || terminals[terminal_id].type === 'vxcoder') {
                rpcReq.recycleMediaNode(terminals[terminal_id].locality.agent, terminals[terminal_id].locality.node, {session: room_id, task: terminal_id})
                .catch(function(reason) {
                    // Catch the error to avoid the UnhandledPromiseRejectionWarning in node v6,
                    // The current code can reach here due to recycle an already recycled node.
                    // There may be other UnhandledPromiseRejectionWarning somewhere, fix when they appear.
                    log.warn('MediaNode not recycled for:', terminal_id);
                });
            }
            delete terminals[terminal_id];
        }
    };

    var isTerminalFree = function (terminal_id) {
        return terminals[terminal_id] && terminals[terminal_id].published.length === 0 && (Object.keys(terminals[terminal_id].subscribed).length === 0) ? true : false;
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
        streams[stream_id].spread.push(target_node);

        var on_spread_failed = function(reason, cancel_pub, cancel_sub) {
            var i = (streams[stream_id] ? streams[stream_id].spread.indexOf(target_node) : -1);
            if (i > -1) {
                streams[stream_id] && streams[stream_id].spread.splice(i, 1);
            }

            if (cancel_sub) {
                makeRPC(
                    rpcClient,
                    original_node,
                    'unsubscribe',
                    [spread_id]);
            }

            if (cancel_pub) {
                makeRPC(
                    rpcClient,
                    target_node,
                    'unpublish',
                    [stream_id]);
            }

            on_error(reason);
        };

        // Transport protocol for creating internal connection
        var internalOpt = {
            protocol: internal_conn_protocol
        };

        // Prepare ports before internal connecting
        // For tcp/udp, the listening ports will also be get at preparation step
        Promise.all([
            new Promise(function(resolve, reject) {
                makeRPC(rpcClient, target_node, 'createInternalConnection', [stream_id, 'in', internalOpt], resolve, reject);
            }),
            new Promise(function(resolve, reject) {
                makeRPC(rpcClient, original_node, 'createInternalConnection', [spread_id, 'out', internalOpt], resolve, reject);
            })
        ]).then(function(prepared) {
            var targetAddr = prepared[0];
            var originAddr = prepared[1];

            var pubFail = false, subFail = false;
            // Publish/Subscribe internal connections
            Promise.all([
                new Promise(function(resolve, reject) {
                    makeRPC(
                        rpcClient,
                        target_node,
                        'publish',
                        [
                            stream_id,
                            'internal',
                            {
                                controller: selfRpcId,
                                owner: terminals[stream_owner].owner,
                                audio: (audio ? {codec: streams[stream_id].audio.codec} : false),
                                video: (video ? {codec: streams[stream_id].video.codec} : false),
                                ip: originAddr.ip,
                                port: originAddr.port,
                            }
                        ],
                        resolve,
                        reject
                    );
                }).catch(function(err) {
                    log.error('Publish internal failed:', err);
                    pubFail = true;
                }),
                new Promise(function(resolve, reject) {
                    makeRPC(
                        rpcClient,
                        original_node,
                        'subscribe',
                        [
                            spread_id,
                            'internal',
                            {controller: selfRpcId, ip: targetAddr.ip, port: targetAddr.port}
                        ],
                        resolve,
                        reject
                    );
                }).catch(function(err) {
                    log.error('Subscribe internal failed:', err);
                    subFail = true;
                })
            ]).then(function(ret) {
                if (pubFail || subFail) {
                    on_spread_failed("Error occurs during internal publish/subscribe.", pubFail, subFail);
                    return;
                }
                if (!streams[stream_id]) {
                    on_spread_failed('Late coming callback for spreading stream.', true, true);
                    return;
                }

                log.debug('internally publish/subscribe ok');

                // Linkup after publish/subscribe ready
                makeRPC(
                    rpcClient,
                    original_node,
                    'linkup',
                    [spread_id, audio ? stream_id : undefined, video ? stream_id : undefined],
                    function () {
                        log.debug('internally linkup ok');
                        if (streams[stream_id]) {
                            //streams[stream_id].spread.push(target_node);
                            on_ok();
                        } else {
                            on_spread_failed('Late coming callback for spreading stream.', true, true);
                        }
                    },
                    function (error_reason) {
                        on_spread_failed(error_reason, true, true);
                    });
            });

        }).catch(function(prepareError) {
            // The createInternalConnection never has error unless makeRPC fail
            log.error("Prepare internal connection error:", prepareError);
            var i = (streams[stream_id] ? streams[stream_id].spread.indexOf(target_node) : -1);
            if (i > -1) {
                streams[stream_id] && streams[stream_id].spread.splice(i, 1);
            }
            makeRPC(rpcClient, target_node, 'destroyInternalConnection', [stream_id, 'in']);
            makeRPC(rpcClient, original_node, 'destroyInternalConnection', [spread_id, 'out']);
            on_error(prepareError);
        });
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
                    rpcClient,
                    original_node,
                    'unsubscribe',
                    [spread_id]);

                makeRPC(
                    rpcClient,
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

    var getSubMediaMixer = function (view, subMedia) {
        if (mix_views[view] && mix_views[view][subMedia])
            return mix_views[view][subMedia].mixer;
        return null;
    };

    var mixAudio = function (stream_id, view, on_ok, on_error) {
        log.debug('to mix audio of stream:', stream_id, 'mixed view:', view);
        var audio_mixer = getSubMediaMixer(view, 'audio');
        if (streams[stream_id] && audio_mixer && terminals[audio_mixer]) {
            var target_node = terminals[audio_mixer].locality.node,
                spread_id = stream_id + '@' + target_node;
            spreadStream(stream_id, target_node, 'amixer', function() {
                if (terminals[audio_mixer]) {
                    terminals[audio_mixer].subscribed[spread_id] = {audio: stream_id};
                    (streams[stream_id].audio.subscribers.indexOf(audio_mixer) < 0) && streams[stream_id].audio.subscribers.push(audio_mixer);
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

    var unmixAudio = function (stream_id, view) {
        log.debug('to unmix audio of view:', view);
        var audio_mixer = getSubMediaMixer(view, 'audio');
        if (streams[stream_id] && streams[stream_id].audio && audio_mixer && terminals[audio_mixer]) {
            var target_node = terminals[audio_mixer].locality.node,
                spread_id = stream_id + '@' + target_node,
                i = streams[stream_id].audio.subscribers.indexOf(audio_mixer);
            delete terminals[audio_mixer].subscribed[spread_id];
            i > -1 && streams[stream_id].audio.subscribers.splice(i, 1);

            shrinkStream(stream_id, target_node);
        }
    };

    var mixVideo = function (stream_id, view, on_ok, on_error) {
        log.debug('to mix video of stream:', stream_id);
        var video_mixer = getSubMediaMixer(view, 'video');
        if (streams[stream_id] && video_mixer && terminals[video_mixer]) {
            var target_node = terminals[video_mixer].locality.node,
                spread_id = stream_id + '@' + target_node;
            spreadStream(stream_id, target_node, 'vmixer', function() {
                if (terminals[video_mixer]) {
                    terminals[video_mixer].subscribed[spread_id] = {video: stream_id};
                    (streams[stream_id].video.subscribers.indexOf(video_mixer) < 0) && streams[stream_id].video.subscribers.push(video_mixer);
                    on_ok();
                } else {
                    shrinkStream(stream_id, target_node);
                    on_error('Video mixer is early released.');
                }
            }, on_error);
        } else {
            log.error('Video mixer is NOT ready.');
            on_error('Video mixer is NOT ready.');
        }
    };

    var unmixVideo = function (stream_id, view) {
        log.debug('to unmix video of stream:', stream_id);
        var video_mixer = getSubMediaMixer(view, 'video');
        if (streams[stream_id] && streams[stream_id].video && video_mixer && terminals[video_mixer]) {
            var target_node = terminals[video_mixer].locality.node,
                spread_id = stream_id + '@' + target_node,
                i = streams[stream_id].video.subscribers.indexOf(video_mixer);
            delete terminals[video_mixer].subscribed[spread_id];
            i > -1 && streams[stream_id].video.subscribers.splice(i, 1);

            shrinkStream(stream_id, target_node);
        }
    };

    var mixStream = function (stream_id, view, on_ok, on_error) {
        log.debug('to mix stream:', stream_id, 'view:', view);
        if (streams[stream_id].audio) {
            mixAudio(stream_id, view, function () {
                if (streams[stream_id].video) {
                    mixVideo(stream_id, view, on_ok, function (error_reason) {
                        unmixAudio(stream_id, view);
                        on_error(error_reason);
                    });
                } else {
                    on_ok();
                }
            }, on_error);
        } else if (streams[stream_id].video) {
            mixVideo(stream_id, view, on_ok, on_error);
        }
    };

    var unmixStream = function (stream_id, view) {
        log.debug('to unmix stream:', stream_id);
        if (streams[stream_id] && streams[stream_id].audio) {
            unmixAudio(stream_id, view);
        }

        if (streams[stream_id] && streams[stream_id].video) {
            unmixVideo(stream_id, view);
        }
    };

    var spawnMixedAudio = function (view, participant_id, audio_codec, on_ok, on_error) {
        var audio_mixer = getSubMediaMixer(view, 'audio');
        if (audio_mixer && terminals[audio_mixer]) {
            var amixer_node = terminals[audio_mixer].locality.node;
            log.debug('spawnMixedAudio, for participant:', participant_id, 'audio_codec:', audio_codec);
            makeRPC(
                rpcClient,
                amixer_node,
                'generate',
                [participant_id, audio_codec],
                function (stream_id) {
                    log.debug('spawnMixedAudio ok, amixer_node:', amixer_node, 'stream_id:', stream_id);
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

    var spawnMixedVideo = function (view, video_codec, video_resolution, video_quality, on_ok, on_error) {
        var video_mixer = getSubMediaMixer(view, 'video');
        if (video_mixer && terminals[video_mixer]) {
            var vmixer_node = terminals[video_mixer].locality.node;
            log.debug('spawnMixedVideo, view:', view, 'video_codec:', video_codec, 'video_resolution:', video_resolution, 'video_quality:', video_quality);
            makeRPC(
                rpcClient,
                vmixer_node,
                'generate',
                [video_codec, video_resolution, video_quality],
                function (stream_id) {
                    log.debug('spawnMixedVideo ok, vmixer_node:', vmixer_node, 'stream_id:', stream_id);
                    if (streams[stream_id] === undefined) {
                        streams[stream_id] = {owner: video_mixer,
                                              audio: undefined,
                                              video: {codec: video_codec,
                                                      resolution: video_resolution,
                                                      quality_level: video_quality,
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

    var getMixedAudio = function (view, participant_id, audio_codec, on_ok, on_error) {
        spawnMixedAudio(view,
            participant_id,
            audio_codec,
            on_ok,
            on_error);
    };

    var getMixedVideo = function (view, video_codec, video_resolution, video_quality, on_ok, on_error) {
        var video_mixer = getSubMediaMixer(view, 'video');
        var candidates = Object.keys(streams).filter(
            function (stream_id) {
                return streams[stream_id].owner === video_mixer &&
                       streams[stream_id].video &&
                       streams[stream_id].video.codec === video_codec &&
                       streams[stream_id].video.resolution === video_resolution &&
                       streams[stream_id].video.quality_level === video_quality;
            });
        if (candidates.length > 0) {
            on_ok(candidates[0]);
        } else {
            spawnMixedVideo(view,
                video_codec,
                video_resolution,
                video_quality,
                on_ok,
                on_error);
        }
    };

    var spawnTranscodedAudio = function (axcoder, audio_codec, on_ok, on_error) {
        var axcoder_node = terminals[axcoder].locality.node;
        log.debug('spawnTranscodedAudio, audio_codec:', audio_codec);
        makeRPC(
            rpcClient,
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
            var axcoder = randomId();
            newTerminal(axcoder, 'axcoder', streams[stream_id].owner, false, function () {
                var on_failed = function (reason) {
                    makeRPC(
                        rpcClient,
                        terminals[axcoder].locality.node,
                        'deinit',
                        [axcoder]);
                    deleteTerminal(axcoder);
                    on_error(reason);
                };

                makeRPC(
                    rpcClient,
                    terminals[axcoder].locality.node,
                    'init',
                    ['transcoding', {}, stream_id, selfRpcId, 'transcoder'],
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

    var spawnTranscodedVideo = function (vxcoder, video_codec, video_resolution, video_quality, on_ok, on_error) {
        var vxcoder_node = terminals[vxcoder].locality.node;
        log.debug('spawnTranscodedVideo, video_codec:', video_codec, 'video_resolution:', video_resolution, 'video_quality:', video_quality);
        makeRPC(
            rpcClient,
            vxcoder_node,
            'generate',
            [video_codec],
            function (stream_id) {
                log.debug('spawnTranscodedVideo ok, stream_id:', stream_id);
                if (streams[stream_id] === undefined) {
                    log.debug('add to streams.');
                    streams[stream_id] = {owner: vxcoder,
                                          audio: undefined,
                                          video: {codec: video_codec,
                                                  resolution: video_resolution,
                                                  quality_level: video_quality,
                                                  subscribers: []},
                                          spread: []};
                    terminals[vxcoder].published.push(stream_id);
                }
                on_ok(stream_id);
            }, on_error);
    };

    var findExistingTranscodedVideo = function (vxcoder, video_codec, video_resolution, video_quality, on_ok, on_error) {
        var published = terminals[vxcoder].published;
        for (var j in published) {
            if (streams[published[j]] &&
                streams[published[j]].video &&
                streams[published[j]].video.codec === video_codec &&
                streams[published[j]].video.resolution === video_resolution &&
                streams[published[j]].video.quality_level === video_quality) {
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
            var vxcoder = randomId();
            newTerminal(vxcoder, 'vxcoder', streams[stream_id].owner, false, function () {
                var on_failed = function (error_reason) {
                    makeRPC(
                        rpcClient,
                        terminals[vxcoder].locality.node,
                        'deinit',
                        [vxcoder]);
                    deleteTerminal(vxcoder);
                    on_error(error_reason);
                };

                makeRPC(
                    rpcClient,
                    terminals[vxcoder].locality.node,
                    'init',
                    ['transcoding', {}, stream_id, selfRpcId, 'transcoder'],
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

    var getTranscodedVideo = function (video_codec, video_resolution, video_quality, stream_id, on_ok, on_error) {
        getVideoTranscoder(stream_id, function (vxcoder) {
            findExistingTranscodedVideo(vxcoder, video_codec, video_resolution, video_quality, on_ok, function () {
                spawnTranscodedVideo(vxcoder, video_codec, video_resolution, video_quality, on_ok, on_error);
            });
        }, on_error);
    };

    var terminateTemporaryStream = function (stream_id) {
        log.debug('terminateTemporaryStream:', stream_id);
        var owner = streams[stream_id].owner;
        var node = terminals[owner].locality.node;
        makeRPC(
            rpcClient,
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
        log.debug('getAudioStream, participant_id:', participant_id, 'stream:', stream_id, 'audio_codec:', audio_codec);
        var mixView = getViewOfMixStream(stream_id);
        if (mixView) {
            getMixedAudio(mixView, participant_id, audio_codec, function (streamID) {
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

    var getVideoStream = function (stream_id, video_codec, video_resolution, video_quality, on_ok, on_error) {
        log.debug('getVideoStream, stream:', stream_id, 'video_codec:', video_codec, 'video_resolution:', video_resolution, 'video_quality', video_quality);
        var mixView = getViewOfMixStream(stream_id);
        if (mixView) {
            getMixedVideo(mixView, video_codec, video_resolution, video_quality, function (streamID) {
                log.debug('Got mixed video:', streamID);
                on_ok(streamID);
            }, on_error);
        } else if (streams[stream_id]) {
            if (streams[stream_id].video) {
                // We do not check the quality_level for transcoding forward stream here
                if (streams[stream_id].video.codec === video_codec &&
                    (video_resolution === 'unspecified' || streams[stream_id].video.resolution === video_resolution)) {
                    on_ok(stream_id);
                } else {
                    getTranscodedVideo(video_codec, video_resolution, video_quality, stream_id, function (streamID) {
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
                if (config.enableMixing) {
                    // Unmix on every mix engine
                    for (var view in mix_views) {
                        unmixStream(stream_id, view);
                    }
                }
                removeSubscriptions(stream_id);
                terminals[terminal_id] && terminals[terminal_id].published.splice(i, 1);
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
                    rpcClient,
                    node,
                    'cutoff',
                    [subscription_id]);
            }

            if (audio_stream && streams[audio_stream]) {
                if (streams[audio_stream].audio) {
                    var i = streams[audio_stream].audio.subscribers.indexOf(subscriber);
                    i > -1 && streams[audio_stream].audio.subscribers.splice(i, 1);
                }
                terminals[streams[audio_stream].owner] && terminals[streams[audio_stream].owner].locality.node !== node && shrinkStream(audio_stream, node);
                terminals[streams[audio_stream].owner] && terminals[streams[audio_stream].owner].type !== 'participant' && recycleTemporaryAudio(audio_stream);
            }

            if (video_stream && streams[video_stream]) {
                if (streams[video_stream].video) {
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
                    if (terminals[terminal_id]) {
                        for (var subscription_id in terminals[terminal_id].subscribed) {
                            unsubscribeStream(terminal_id, subscription_id);
                            if (terminals[terminal_id].type === 'axcoder') {
                                for (var i in terminals[terminal_id].published) {
                                    unpublishStream(terminals[terminal_id].published[i]);
                                }
                            }
                        }
                        if (isTerminalFree(terminal_id)) {
                            deleteTerminal(terminal_id);
                        }
                    }
                });
                streams[stream_id] && (streams[stream_id].audio.subscribers = []);
            }

            if (streams[stream_id] && streams[stream_id].video) {
                streams[stream_id].video.subscribers.forEach(function(terminal_id) {
                    if (terminals[terminal_id]) {
                        for (var subscription_id in terminals[terminal_id].subscribed) {
                            unsubscribeStream(terminal_id, subscription_id);
                            if (terminals[terminal_id].type === 'vxcoder') {
                                for (var i in terminals[terminal_id].published) {
                                    unpublishStream(terminals[terminal_id].published[i]);
                                }
                            }
                        }
                        if (isTerminalFree(terminal_id)) {
                            deleteTerminal(terminal_id);
                        }
                    }
                });
                streams[stream_id] && (streams[stream_id].video.subscribers = []);
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
                var terminal_id = pubTermId(participantId, streamId);
                var terminal_owner = (streamInfo.type === 'webrtc' || streamInfo.type === 'sip') ? participantId : room_id + '-' + randomId();
                newTerminal(terminal_id, 'participant', terminal_owner, accessNode, function () {
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
                        // Try mix on "common" view
                        var mixId = getMixStreamOfView("common");
                        if (mix_views["common"]) {
                            mixStream(streamId, "common", function () {
                                log.debug('Mix stream[' + streamId + '] successfully.');
                                on_ok();
                            }, function (error_reason) {
                                log.error('Mix stream[' + streamId + '] failed, reason: ' + error_reason);
                                unpublishStream(streamId);
                                deleteTerminal(terminal_id);
                                on_error(error_reason);
                            });
                        } else {
                            log.warn('Mix stream[' + streamId + '] to no view.');
                            on_ok();
                        }
                    } else {
                        log.debug('will not mix stream:', streamId);
                        on_ok();
                    }
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
        var terminal_id = pubTermId(participantId, streamId);

        if (streams[streamId] === undefined
            || streams[streamId].owner !== terminal_id
            || terminals[terminal_id] === undefined
            || terminals[terminal_id].published.indexOf(streamId) === -1) {
            log.info('unpublish a rogue stream:', streamId);
        }

        if (streams[streamId]) {
            unpublishStream(streamId);
        }

        deleteTerminal(terminal_id);
    };

    that.subscribe = function(participantId, subscriptionId, accessNode, subInfo, on_ok, on_error) {
        log.debug('subscribe, participantId:', participantId, 'subscriptionId:', subscriptionId, 'accessNode:', accessNode.node, 'subInfo:', JSON.stringify(subInfo));
        if ((!subInfo.audio || (streams[subInfo.audio.fromStream] && streams[subInfo.audio.fromStream].audio) || getViewOfMixStream(subInfo.audio.fromStream))
            && (!subInfo.video || (streams[subInfo.video.fromStream] && streams[subInfo.video.fromStream].video) || getViewOfMixStream(subInfo.video.fromStream))) {

            var audio_codec = undefined;
            if (subInfo.audio) {
                var subAudioStream = subInfo.audio.fromStream;
                var subView = getViewOfMixStream(subAudioStream);
                var isMixStream = !!subView;
                audio_codec = isMixStream? getMixedCodec(subInfo.audio, mix_views[subView].audio.supported_codecs)
                    : getForwardCodec(subInfo.audio, streams[subAudioStream].audio.codec, enable_audio_transcoding);

                if (audio_codec === 'unavailable') {
                    log.error('No available audio codec');
                    log.debug('subInfo.audio:', subInfo.audio, 'targetStream.audio:', streams[subAudioStream] ? streams[subAudioStream].audio : 'mixed_stream', 'enable_audio_transcoding:', enable_audio_transcoding);
                    return on_error('No available audio codec');
                }
            }

            var video_codec = undefined;
            var video_resolution = undefined;
            var video_quality = 'unspecified';
            if (subInfo.video) {
                var subVideoStream = subInfo.video.fromStream;
                var subView = getViewOfMixStream(subVideoStream);
                var isMixStream = !!subView;

                if (isMixStream) {
                    // Is mix stream
                    video_codec = getMixedCodec(subInfo.video, mix_views[subView].video.supported_codecs);
                    video_resolution = getMixedResolution(subInfo.video, mix_views[subView].video.supported_resolutions);
                    var mixVideoConfig = config.views[subView].mediaMixing;
                    video_quality = (subInfo.video.quality_level || (mixVideoConfig && mixVideoConfig.quality_level) || 'unspecified');
                } else {
                    // Is forward stream
                    video_codec = getForwardCodec(subInfo.video, streams[subVideoStream].video.codec, enable_video_transcoding);
                    video_resolution = subInfo.video.resolution
                        ? getForwardResolution(subInfo.video, streams[subVideoStream].video.resolution, enable_video_transcoding)
                        : 'unspecified';
                    video_quality = 'standard'/*(subInfo.video.quality_level || 'unspecified')*/;
                }

                if (video_codec === 'unavailable') {
                    log.error('No available video codec');
                    log.debug('subInfo.video:', subInfo.video, 'targetStream.video:', streams[subVideoStream] ? streams[subVideoStream].video : 'mixed_stream', 'enable_video_transcoding:', enable_video_transcoding);
                    return on_error('No available video codec');
                }

                if (video_resolution === 'unavailable') {
                    log.error('No available video resolution');
                    var supported_video_resolutions = mix_views[subView].video.supported_resolutions;
                    log.debug('subInfo.video:', subInfo.video, 'targetStream.video:', streams[subVideoStream] ? streams[subVideoStream].video : 'mixed_stream, supported_resolutions: ' + supported_video_resolutions, 'enable_video_transcoding:', enable_video_transcoding);
                    return on_error('No available video resolution');
                }
            }

            if ((subInfo.audio && !audio_codec) || (subInfo.video && !video_codec)) {
                log.error('No proper audio/video codec');
                return on_error('No proper audio/video codec');
            }

            var terminal_id = subTermId(participantId, subscriptionId);

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
                };
            };

            var linkup = function (audioStream, videoStream) {
                log.debug('linkup, subscriber:', terminal_id, 'audioStream:', audioStream, 'videoStream:', videoStream);
                if (terminals[terminal_id] && (!audioStream || streams[audioStream]) && (!videoStream || streams[videoStream])) {
                    makeRPC(
                        rpcClient,
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
                    getAudioStream(terminals[terminal_id].owner, subInfo.audio.fromStream, audio_codec, function (streamID) {
                        audio_stream = streamID;
                        log.debug('Got audio stream:', audio_stream);
                        if (subInfo.video) {
                            log.debug('require video track of stream:', subInfo.video.fromStream);
                            getVideoStream(subInfo.video.fromStream, video_codec, video_resolution, video_quality, function (streamID) {
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
                    getVideoStream(subInfo.video.fromStream, video_codec, video_resolution, video_quality, function (streamID) {
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

            var terminal_owner = (subInfo.type === 'webrtc' || subInfo.type === 'sip') ? participantId : room_id + '-' + randomId();
            newTerminal(terminal_id, 'participant', terminal_owner, accessNode, function () {
                doSubscribe();
            }, on_error);
        } else {
            log.error('streams do not exist or are insufficient. subInfo:', subInfo);
            on_error('streams do not exist or are insufficient');
        }
    };

    that.unsubscribe = function (participant_id, subscription_id) {
        log.debug('unsubscribe from participant:', participant_id, 'for subscription:', subscription_id);
        var terminal_id = subTermId(participant_id, subscription_id);
        if (terminals[terminal_id] && terminals[terminal_id].subscribed[subscription_id]) {
            unsubscribeStream(terminal_id, subscription_id);

            deleteTerminal(terminal_id);
        }
    };

    that.updateStream = function (stream_id, track, status) {
        log.debug('updateStream, stream_id:', stream_id, 'track', track, 'status:', status);
        if ((track === 'video' || track === 'av') && (status === 'active' || status === 'inactive')) {
            if (streams[stream_id] && streams[stream_id].video && config.enableMixing) {
                for (var view in mix_views) {
                    var video_mixer = mix_views[view].video.mixer;
                    if (streams[stream_id].video && video_mixer && terminals[video_mixer]) {
                        if (streams[stream_id].video.subscribers.indexOf(video_mixer) >= 0) {
                            var target_node = terminals[video_mixer].locality.node;
                            var active = (status === 'active');
                            makeRPC(
                                rpcClient,
                                target_node,
                                'setInputActive',
                                [stream_id, active]);
                        }
                    }
                }
            }
        }
    };

    that.mix = function (stream_id, mixstream_list, on_ok, on_error) {
        log.debug('mix, stream_id:', stream_id, 'mixstream_list', mixstream_list);
        if (!Array.isArray(mixstream_list)) {
            if (!mix_views['common']) {
                on_error('No common view found when mix stream not specified');
                return;
            } else {
                mixstream_list = [getMixStreamOfView('common')];
            }
        }
        if (streams[stream_id] && config.enableMixing) {
            Promise.all(mixstream_list.map(function(mixStreamId) {
                return new Promise(function(resolve, reject) {
                    mixStream(stream_id, getViewOfMixStream(mixStreamId),
                        function() {
                            resolve(true);
                        },
                        function() {
                            resolve(false);
                        }
                    );
                });
            })).then(function(results) {
                on_ok();
            }).catch(function(reason) {
                on_error(reason);
            });
        } else {
            on_error('Stream does not exist:'+stream_id);
        }
    };

    that.unmix = function (stream_id, mixstream_list, on_ok, on_error) {
        log.debug('unmix, stream_id:', stream_id, 'mixstream_list', mixstream_list);
        if (!Array.isArray(mixstream_list)) {
            if (!mix_views['common']) {
                on_error('No common view found when mix stream not specified');
                return;
            } else {
                mixstream_list = [getMixStreamOfView('common')];
            }
        }
        if (streams[stream_id] && config.enableMixing) {
            mixstream_list.forEach(function(mixStreamId) {
                unmixStream(stream_id, getViewOfMixStream(mixStreamId));
            });
            on_ok();
        } else {
            on_error('Stream does not exist:'+stream_id);
        }
    };

    that.setMute = function (streamId, track, muted, onOk, onError) {
        log.debug('mute, streamId:', streamId, 'track:', track);
        let onOff = muted? 'off' : 'on';
        if (streams[streamId]) {
            let terminal = terminals[streams[streamId].owner];
            makeRPC(
                rpcClient,
                terminal.locality.node,
                'mediaOnOff',
                [streamId, track, 'in', onOff],
                onOk,
                onError);
        } else {
            onError('Invalid stream');
        }
    };

    that.getRegion = function (stream_id, mixstream_id, on_ok, on_error) {
        log.debug('getRegion, stream_id:', stream_id, 'mixstream_id', mixstream_id);
        if (!mixstream_id) {
            mixstream_id = getMixStreamOfView('common');
            if (!mixstream_id) {
                on_error('No common view found when mix stream not specified');
                return;
            }
        }
        var view = getViewOfMixStream(mixstream_id);
        var video_mixer = getSubMediaMixer(view, 'video');
        if (video_mixer) {
            makeRPC(
                rpcClient,
                terminals[video_mixer].locality.node,
                'getRegion',
                [stream_id],
                on_ok,
                on_error);
        } else {
            on_error('Invalid mix stream ID');
        }
    };

    that.setRegion = function (stream_id, region, mixstream_id, on_ok, on_error) {
        log.debug('setRegion, stream_id:', stream_id, 'mixstream_id:', mixstream_id, 'region:', region);
        if (!mixstream_id) {
            mixstream_id = getMixStreamOfView('common');
            if (!mixstream_id) {
                on_error('No common view found when mix stream not specified');
                return;
            }
        }
        var view = getViewOfMixStream(mixstream_id);
        var video_mixer = getSubMediaMixer(view, 'video');
        if (video_mixer) {
            makeRPC(
                rpcClient,
                terminals[video_mixer].locality.node,
                'setRegion',
                [stream_id, region],
                function (data) {
                    on_ok(data);
                    resetVAD(view);
                }, on_error);
        } else {
            on_error('Invalid mix stream ID');
        }
    };

    that.setPrimary = function (participantId, view) {
        log.debug('setPrimary:', participantId, view);
        var video_mixer = getSubMediaMixer(view, 'video');

        for (var terminal_id in terminals) {
            if (terminals[terminal_id].owner === participantId) {
                for (var i in terminals[terminal_id].published) {
                    var stream_id = terminals[terminal_id].published[i];
                    if (streams[stream_id] && streams[stream_id].video && (streams[stream_id].video.subscribers.indexOf(video_mixer) !== -1)) {
                        makeRPC(
                            rpcClient,
                            terminals[video_mixer].locality.node,
                            'setPrimary',
                            [stream_id]);
                        return;
                    }
                }
            }
        }
    };

    that.getMixedStreams = function() {
        if (!mix_views) {
            return [];
        }

        return Object.keys(mix_views).map(function(view) {
            log.debug('mix stream id:', getMixStreamOfView(view));
            return {
                streamId: getMixStreamOfView(view),
                view: view,
                resolutions: mix_views[view].video.supported_resolutions,
            };
        });
    };

    that.getMixedStream = function(view) {
        return getMixStreamOfView(view);
    };

    var isImpacted = function (locality, type, id) {
        return (type === 'worker' && locality.agent === id) || (type === 'node' && locality.node === id);
    };

    var allocateMediaProcessingNode  = function (forWhom, usage) {
        return  rpcReq.getMediaNode(cluster, purpose, {session: room_id, task: terminal_id});
    };

    var initMediaProcessor = function (terminal_id, parameters) {
        return new Promise(function (resolve, reject) {
            makeRPC(
                rpcClient,
                terminals[terminal_id].locality.node,
                'init',
                parameters,
                function (result) {
                    resolve(result);
                }, function (error_reason) {
                    reject(error_reason);
                });
        });
    };

    var initVideoMixer = function (vmixerId, view) {
        var videoMixingConfig = getViewMixingConfig(view).video;
        return initMediaProcessor(vmixerId, ['mixing', videoMixingConfig, room_id, selfRpcId, view])
            .then(function (supportedVideo) {
                log.debug('Init video mixer ok. room_id:', room_id, 'vmixer_id:', vmixerId, 'view:', view);
                // Save supported info
                if (mix_views[view]) {
                    mix_views[view].video = {
                        mixer: vmixerId,
                        supported_codecs: supportedVideo.codecs,
                        supported_resolutions: supportedVideo.resolutions
                    };
                }

                // Enable AV coordination if specified
                if (videoMixingConfig && videoMixingConfig.avCoordinated) {
                    enableAVCoordination(view);
                }
                return supportedVideo.resolutions;
            }, function (error_reason) {
                log.error('Init video_mixer failed. room_id:', room_id, 'reason:', error_reason);
                Promise.reject(error_reason);
            });
    };

    var rebuildVideoMixer = function (vmixerId) {
        var old_locality = terminals[vmixerId].locality;
        var inputs = [], outputs = [];
        var view = null;
        for (var vlabel in mix_views) {
            if (mix_views[vlabel].video.mixer === vmixerId) {
                view = vlabel;
                break;
            }
        }

        log.debug('rebuildVideoMixer, vmixerId:', vmixerId, 'view:', view);
        for (var sub_id in terminals[vmixerId].subscribed) {
            var vst_id = terminals[vmixerId].subscribed[sub_id].video;
            inputs.push(vst_id);
            log.debug('Abort stream mixing:', vst_id);
            unmixVideo(vst_id, view);
        }
        terminals[vmixerId].subscribed = {};

        terminals[vmixerId].published.forEach(function (st_id) {
            if (streams[st_id]) {
                var backup = JSON.parse(JSON.stringify(streams[st_id]));
                backup.old_stream_id = st_id;
                outputs.push(backup);
                streams[st_id].video.subscribers.forEach(function(t_id) {
                    log.debug('Aborting subscription to stream :', st_id, 'by subscriber:', t_id);
                    var i = streams[st_id].video.subscribers.indexOf(t_id);
                    i > -1 && streams[st_id].video.subscribers.splice(i, 1);
                    terminals[t_id] && shrinkStream(st_id, terminals[t_id].locality.node);
                });
                delete streams[st_id];
            }
        });
        terminals[vmixerId].published = [];

        return rpcReq.getMediaNode(cluster, 'video', {session: room_id, task: vmixerId})
            .then(function (locality) {
                log.debug('Got new video mixer node:', locality);
                terminals[vmixerId].locality = locality;
                return initVideoMixer(vmixerId, view);
            })
            .then(function () {
                return Promise.all(inputs.map(function (vst_id) {
                    log.debug('Resuming video mixer input:', vst_id);
                    return new Promise(function (resolve, reject) {
                        mixVideo(vst_id, view, resolve, reject);
                    });
                }));
            })
            .then(function () {
                return Promise.all(outputs.map(function (old_st) {
                    log.debug('Resuming video mixer output:', old_st);
                    return new Promise(function (resolve, reject) {
                        getMixedVideo(view, old_st.video.codec, old_st.video.resolution, old_st.video.quality_level, function(stream_id) {
                            log.debug('Got new stream:', stream_id);
                            return Promise.all(old_st.spread.map(function(target_node) {
                                return new Promise(function (res, rej) {
                                    spreadStream(stream_id, target_node, 'participant', function() {
                                        res('ok');
                                    }, function (reason) {
                                        log.warn('Failed in spreading video stream. reason:', reason);
                                        rej(reason);
                                    });
                                });
                            }))
                            .then(function () {
                                old_st.video.subscribers.forEach(function (t_id) {
                                    if (terminals[t_id]) {
                                        for (var sub_id in terminals[t_id].subscribed) {
                                            if (terminals[t_id].subscribed[sub_id].video === old_st.old_stream_id) {
                                                makeRPC(
                                                    rpcClient,
                                                    terminals[t_id].locality.node,
                                                    'linkup',
                                                    [sub_id, undefined, stream_id],
                                                    function () {
                                                        streams[stream_id].video.subscribers = streams[stream_id].video.subscribers || [];
                                                        streams[stream_id].video.subscribers.push(t_id);
                                                        terminals[t_id].subscribed[sub_id].video = stream_id;
                                                    }, function (reason) {
                                                        log.warn('Failed in resuming video subscription. reason:', reason);
                                                    });
                                            }
                                        }
                                    }
                                });
                            })
                            .then(function () {
                                log.debug('Resumed video mixer output ok.');
                                resolve('ok');
                            })
                            .catch(function (err) {
                                log.info('Resumed video mixer output failed. err:', err);
                                reject(err);
                            });
                        }, reject);
                    });
                }));
            })
            .catch(function (reason) {
                log.error('Rebuid video mixer failed, reason:', (typeof reason === 'string') ? reason : reason.message);
                setTimeout(function () {throw Error('Rebuild video mixer failed.');});
            });
    };

    var rebuildVideoTranscoder = function(vxcoderId) {
        var old_locality = terminals[vxcoderId].locality;
        var input, outputs = [];

        log.debug('rebuildVideoTranscoder, vxcoderId:', vxcoderId);
        for (var sub_id in terminals[vxcoderId].subscribed) {
            var vst_id = terminals[vxcoderId].subscribed[sub_id].video;
            input = vst_id;
            var i = streams[vst_id].video.subscribers.indexOf(vxcoderId);
            i > -1 && streams[vst_id].video.subscribers.splice(i, 1);
            shrinkStream(vst_id, old_locality.node);
        }
        terminals[vxcoderId].subscribed = {};

        terminals[vxcoderId].published.forEach(function (st_id) {
            if (streams[st_id]) {
                var backup = JSON.parse(JSON.stringify(streams[st_id]));
                backup.old_stream_id = st_id;
                outputs.push(backup);
                streams[st_id].video.subscribers.forEach(function(t_id) {
                    log.debug('Aborting subscription to stream :', st_id, 'by subscriber:', t_id);
                    var i = streams[st_id].video.subscribers.indexOf(t_id);
                    i > -1 && streams[st_id].video.subscribers.splice(i, 1);
                    terminals[t_id] && shrinkStream(st_id, terminals[t_id].locality.node);
                });
                delete streams[st_id];
            }
        });
        terminals[vxcoderId].published = [];

        return Promise.resolve('ok')
            .then(function () {
                return Promise.all(outputs.map(function (old_st) {
                    log.debug('Resuming video xcoder output:', old_st);
                    return new Promise(function (resolve, reject) {
                        getTranscodedVideo(old_st.video.codec, old_st.video.resolution, old_st.video.quality_level, input, function(stream_id) {
                            log.debug('Got new stream:', stream_id);
                            return Promise.all(old_st.spread.map(function(target_node) {
                                return new Promise(function (res, rej) {
                                    spreadStream(stream_id, target_node, 'participant', function() {
                                        res('ok');
                                    }, function (reason) {
                                        log.warn('Failed in spreading video stream. reason:', reason);
                                        rej(reason);
                                    });
                                });
                            }))
                            .then(function () {
                                old_st.video.subscribers.forEach(function (t_id) {
                                    if (terminals[t_id]) {
                                        for (var sub_id in terminals[t_id].subscribed) {
                                            if (terminals[t_id].subscribed[sub_id].video === old_st.old_stream_id) {
                                                makeRPC(
                                                    rpcClient,
                                                    terminals[t_id].locality.node,
                                                    'linkup',
                                                    [sub_id, undefined, stream_id],
                                                    function () {
                                                        streams[stream_id].video.subscribers = streams[stream_id].video.subscribers || [];
                                                        streams[stream_id].video.subscribers.push(t_id);
                                                        terminals[t_id].subscribed[sub_id].video = stream_id;
                                                    }, function (reason) {
                                                        log.warn('Failed in resuming video subscription. reason:', reason);
                                                    });
                                            }
                                        }
                                    }
                                });
                            })
                            .then(function () {
                                log.debug('Resumed video xcoder output ok.');
                                resolve('ok');
                            })
                            .catch(function (err) {
                                log.info('Resumed video xcoder output failed. err:', err);
                                reject(err);
                            });
                        }, reject);
                    });
                }));
            })
            .catch(function (reason) {
                log.error('Rebuid video transcoder failed, reason:', (typeof reason === 'string') ? reason : reason.message);
                setTimeout(function () {throw Error('Rebuild video transcoder failed.');});
            });
    };

    var initAudioMixer = function (amixerId, view) {
        var audioMixingConfig = getViewMixingConfig(view).audio;
        return initMediaProcessor(amixerId, ['mixing', audioMixingConfig, room_id, selfRpcId, view])
            .then(function (supportedAudio) {
                log.debug('Init audio mixer ok. room_id:', room_id, 'amixer_id:', amixerId, 'view:', view);
                // Save supported info
                if (mix_views[view]) {
                    mix_views[view].audio = {
                        mixer: amixerId,
                        supported_codecs: supportedAudio.codecs
                    };
                }

                // Enable AV coordination if specified
                var videoMixingConfig = getViewMixingConfig(view).video;
                if (videoMixingConfig && videoMixingConfig.avCoordinated) {
                    enableAVCoordination(view);
                }
                return 'ok';
            }, function (error_reason) {
                log.error('Init audio_mixer failed. room_id:', room_id, 'reason:', error_reason);
                Promise.reject(error_reason);
            });
    };

    var rebuildAudioMixer = function (amixerId) {
        var old_locality = terminals[amixerId].locality;
        var inputs = [], outputs = [];
        var view = null;
        for (var vlabel in mix_views) {
            if (mix_views[vlabel].audio.mixer === amixerId) {
                view = vlabel;
                break;
            }
        }

        for (var sub_id in terminals[amixerId].subscribed) {
            var ast_id = terminals[amixerId].subscribed[sub_id].audio;
            inputs.push(ast_id);
            log.debug('Aborting stream mixing:', ast_id);
            unmixAudio(ast_id, view);
        }
        terminals[amixerId].subscribed = {};

        terminals[amixerId].published.forEach(function (st_id) {
            if (streams[st_id]) {
                var backup = JSON.parse(JSON.stringify(streams[st_id]));
                backup.old_stream_id = st_id;
                streams[st_id].audio.subscribers.forEach(function(t_id) {
                    backup.for_whom = (terminals[t_id] && terminals[t_id].owner);
                    log.debug('Aborting subscription to stream:', st_id, 'by subscriber:', t_id);
                    var i = streams[st_id].audio.subscribers.indexOf(t_id);
                    i > -1 && streams[st_id].audio.subscribers.splice(i, 1);
                    terminals[t_id] && shrinkStream(st_id, terminals[t_id].locality.node);
                });
                outputs.push(backup);
                delete streams[st_id];
            }
        });
        terminals[amixerId].published = [];

        return rpcReq.getMediaNode(cluster, 'audio', {session: room_id, task: amixerId})
            .then(function (locality) {
                log.debug('Got new audio mixer node:', locality);
                terminals[amixerId].locality = locality;
                return initAudioMixer(amixerId, view);
            })
            .then(function () {
                return Promise.all(inputs.map(function (ast_id) {
                    log.debug('Resuming audio mixer input:', ast_id);
                    return new Promise(function (resolve, reject) {
                        mixAudio(ast_id, view, resolve, reject);
                    });
                }));
            })
            .then(function () {
                return Promise.all(outputs.map(function (old_st) {
                    log.debug('Resuming audio mixer output:', old_st, 'view:', view);
                    return new Promise(function (resolve, reject) {
                        getMixedAudio(view, old_st.for_whom, old_st.audio.codec, function(stream_id) {
                            log.debug('Got new stream:', stream_id);
                            return Promise.all(old_st.spread.map(function(target_node) {
                                    return new Promise(function (res, rej) {
                                        spreadStream(stream_id, target_node, 'participant', function() {
                                            res('ok');
                                        }, function (reason) {
                                            log.warn('Failed in spreading audio stream. reason:', reason);
                                            rej(reason);
                                        });
                                    });
                                }))
                                .then(function () {
                                    old_st.audio.subscribers.forEach(function (t_id) {
                                        if (terminals[t_id]) {
                                            for (var sub_id in terminals[t_id].subscribed) {
                                                if (terminals[t_id].subscribed[sub_id].audio === old_st.old_stream_id) {
                                                    makeRPC(
                                                        rpcClient,
                                                        terminals[t_id].locality.node,
                                                        'linkup',
                                                        [sub_id, stream_id, undefined],
                                                        function () {
                                                            streams[stream_id].audio.subscribers = streams[stream_id].audio.subscribers || [];
                                                            streams[stream_id].audio.subscribers.push(t_id);
                                                            terminals[t_id].subscribed[sub_id].audio = stream_id;
                                                        }, function (reason) {
                                                            log.warn('Failed in resuming video subscription. reason:', reason);
                                                        });
                                                }
                                            }
                                        }
                                    });
                                })
                                .then(function () {
                                    log.debug('Resumed audio mixer output ok.');
                                    resolve('ok');
                                })
                                .catch(function (err) {
                                    log.info('Resumed audio mixer output failed. err:', err);
                                    reject(err);
                                });
                        }, reject);
                    });
                }));
            })
            .catch(function (reason) {
                log.error('Rebuid audio mixer failed, reason:', (typeof reason === 'string') ? reason : reason.message);
                setTimeout(function () {throw Error('Rebuild audio mixer failed.');});
            });
    };

    var rebuildAudioTranscoder = function(axcoderId) {
        var old_locality = terminals[axcoderId].locality;
        var input, outputs = [];

        for (var sub_id in terminals[axcoderId].subscribed) {
            var vst_id = terminals[axcoderId].subscribed[sub_id].audio;
            input = vst_id;
            var i = streams[vst_id].audio.subscribers.indexOf(axcoderId);
            i > -1 && streams[vst_id].audio.subscribers.splice(i, 1);
            shrinkStream(vst_id, old_locality.node);
        }
        terminals[axcoderId].subscribed = {};

        terminals[axcoderId].published.forEach(function (st_id) {
            if (streams[st_id]) {
                var backup = JSON.parse(JSON.stringify(streams[st_id]));
                backup.old_stream_id = st_id;
                outputs.push(backup);
                streams[st_id].audio.subscribers.forEach(function(t_id) {
                    log.debug('Aborting subscription to stream :', st_id, 'by subscriber:', t_id);
                    var i = streams[st_id].audio.subscribers.indexOf(t_id);
                    i > -1 && streams[st_id].audio.subscribers.splice(i, 1);
                    terminals[t_id] && shrinkStream(st_id, terminals[t_id].locality.node);
                });
                delete streams[st_id];
            }
        });
        terminals[axcoderId].published = [];

        return Promise.resolve('ok')
            .then(function () {
                return Promise.all(outputs.map(function (old_st) {
                    log.debug('Resuming audio xcoder output:', old_st);
                    return new Promise(function (resolve, reject) {
                        getTranscodedAudio(old_st.audio.codec, input, function(stream_id) {
                            log.debug('Got new stream:', stream_id);
                            return Promise.all(old_st.spread.map(function(target_node) {
                                return new Promise(function (res, rej) {
                                    spreadStream(stream_id, target_node, 'participant', function() {
                                        res('ok');
                                    }, function (reason) {
                                        log.warn('Failed in spreading audio stream. reason:', reason);
                                        rej(reason);
                                    });
                                });
                            }))
                            .then(function () {
                                old_st.audio.subscribers.forEach(function (t_id) {
                                    if (terminals[t_id]) {
                                        for (var sub_id in terminals[t_id].subscribed) {
                                            if (terminals[t_id].subscribed[sub_id].audio === old_st.old_stream_id) {
                                                makeRPC(
                                                    rpcClient,
                                                    terminals[t_id].locality.node,
                                                    'linkup',
                                                    [sub_id, stream_id, undefined],
                                                    function () {
                                                        streams[stream_id].audio.subscribers = streams[stream_id].audio.subscribers || [];
                                                        streams[stream_id].audio.subscribers.push(t_id);
                                                        terminals[t_id].subscribed[sub_id].audio = stream_id;
                                                    }, function (reason) {
                                                        log.warn('Failed in resuming audio subscription. reason:', reason);
                                                    });
                                            }
                                        }
                                    }
                                });
                            })
                            .then(function () {
                                log.debug('Resumed audio xcoder output ok.');
                                resolve('ok');
                            })
                            .catch(function (err) {
                                log.info('Resumed audio xcoder output failed. err:', err);
                                reject(err);
                            });
                        }, reject);
                    });
                }));
            })
            .catch(function (reason) {
                log.error('Rebuid audio transcoder failed, reason:', (typeof reason === 'string') ? reason : reason.message);
                setTimeout(function () {throw Error('Rebuild audio transcoder failed.');});
            });
    };

    var onVideoFault = function (type, id) {
        for (var terminal_id in terminals) {
            if (isImpacted(terminals[terminal_id].locality, type, id)) {
                log.debug('Impacted terminal:', terminal_id, 'and its locality:', terminals[terminal_id].locality);
                if (terminals[terminal_id].type === 'vmixer') {
                    rebuildVideoMixer(terminal_id);
                } else if (terminals[terminal_id].type === 'vxcoder') {
                    rebuildVideoTranscoder(terminal_id);
                }
            }
        }
    };

    var onAudioFault = function (type, id) {
        for (var terminal_id in terminals) {
            if (isImpacted(terminals[terminal_id].locality, type, id)) {
                log.debug('Impacted terminal:', terminal_id, 'and its locality:', terminals[terminal_id].locality);
                if (terminals[terminal_id].type === 'amixer') {
                    rebuildAudioMixer(terminal_id);
                } else if (terminals[terminal_id].type === 'axcoder') {
                    rebuildAudioTranscoder(terminal_id);
                }
            }
        }
    };

    that.onFaultDetected = function (purpose, type, id) {
        log.debug('onFaultDetected, purpose:', purpose, 'type:', type, 'id:', id);
        if (purpose === 'video') {
            onVideoFault(type, id);
        } else if (purpose === 'audio') {
            onAudioFault(type, id);
        }
    };

    assert.equal(typeof on_init_ok, 'function');
    assert.equal(typeof on_init_failed, 'function');
    return initialize(on_init_ok, on_init_failed);
};
