/*global require, exports, setInterval, GLOBAL*/
'use strict';

var logger = require('./../common/logger').logger;
var makeRPC = require('./../common/makeRPC').makeRPC;
var ErizoManager = require('./erizoManager').ErizoManager;

// Logger
var log = logger.getLogger("RoomController");

exports.RoomController = function (spec, on_init_ok, on_init_failed) {

    var that = {};

    var amqper = spec.amqper,
        config = spec.config,
        room_id = spec.room,
        observer = spec.observer,
        mixed_stream_id = spec.room,
        supported_audio_codecs = [],
        supported_video_codecs = [],
        supported_video_resolutions = [],

        audio_mixer = undefined,
        video_mixer = undefined;

    /*{ErizoID: {addr: Address}}*/
    var erizos = {};

    /* {terminalID: {type: 'webrtc' | 'rtsp' | 'file' | 'amixer' | 'axcoder' | 'vmixer' | 'vxcoder',
                     erizo: ErizoID,
                     published:[StreamID],
                     subscribed:{SubscriptionID: {audio: StreamID | undefined, video: StreamID | undefined}}
                    }
       }*/
    var terminals = {};

    /* {StreamID: {owner: terminalID,
                   audio: {codec: 'pcmu' | 'pcma' | 'isac_16000' | 'isac_32000' | 'opus_48000_2' |...,
                           subscribers: [terminalID]
                          } | undefined,
                   video: {codec: 'h264' | 'vp8' |...,
                           resolution: 'cif' | 'vga' | 'svga' | 'xga' | 'hd720p' | 'sif' | 'hvga' | 'r640x360' | 'r480x360' | 'qcif' | 'r192x144' | 'hd1080p' | 'uhd_4k' |...,
                           framerate: Number(1~120) | undefined,
                           subscribers: [terminalID]
                          } | undefined,
                   spread: [ErizoID]
                  }
       }*/
    var streams = {};

    var erizoManager = ErizoManager({amqper: amqper, room_id: room_id, on_broken: function (erizo_id, erizo_purpose) {
        if (erizos[erizo_id]) {
            if (erizo_purpose === 'audio'
                || erizo_purpose === 'video') {
                rebuildErizo(erizo_id);
            } else {
                cleanErizo(erizo_id);
            }
        }
    }});

    var enableAVCoordination = function () {
        log.debug("enableAVCoordination");
        if (config.enableMixing && audio_mixer && video_mixer) {
            log.debug("enableAVCoordination 1");
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[audio_mixer].erizo,
                "enableVAD",
                [1000, room_id, observer]);
        }
    };

    var initialize = function () {
        log.debug("initialize...");
        if (config.enableMixing) {
            audio_mixer = Math.random() * 1000000000000000000 + '';
            newTerminal(audio_mixer, 'amixer', function () {
                log.debug("new audio mixer ok. audio_mixer:", audio_mixer);
                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[audio_mixer].erizo,
                    "init",
                    ['mixing', config.mediaMixing.audio],
                    function (supported_audio) {
                        log.debug("init audio mixer ok.");
                        video_mixer = Math.random() * 1000000000000000000 + '';
                        newTerminal(video_mixer, 'vmixer', function () {
                            log.debug("new video mixer ok. video_mixer:", video_mixer);
                            makeRPC(
                                amqper,
                                'ErizoJS_' + terminals[video_mixer].erizo,
                                "init",
                                ['mixing', config.mediaMixing.video],
                                function (supported_video) {
                                    log.debug("init video mixer ok.");
                                    supported_audio_codecs = supported_audio.codecs;
                                    supported_video_codecs = supported_video.codecs;
                                    supported_video_resolutions = supported_video.resolutions;

                                    if (typeof on_init_ok === 'function') on_init_ok(supported_video_resolutions);
                                    if (config.mediaMixing.video.avCoordinated) {
                                        enableAVCoordination();
                                    }
                                }, function (error_reason) {
                                    deleteTerminal(video_mixer);
                                    makeRPC(
                                        amqper,
                                        'ErizoJS_' + terminals[audio_mixer].erizo,
                                        "deinit",
                                        [audio_mixer]);
                                    deleteTerminal(audio_mixer);
                                    audio_mixer = undefined;
                                    video_mixer = undefined;
                                    if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                                });
                            }, function (error_reason) {
                                makeRPC(
                                    amqper,
                                    'ErizoJS_' + terminals[audio_mixer].erizo,
                                    "deinit",
                                    [audio_mixer]);
                                deleteTerminal(audio_mixer);
                                audio_mixer = undefined;
                                video_mixer = undefined;
                                if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                            });
                }, function (error_reason) {
                    audio_mixer = undefined;
                    if (typeof on_init_failed === 'function') on_init_failed(error_reason);
                });
            }, function(error_reason) {
                audio_mixer = undefined;
                if (typeof on_init_failed === 'function') on_init_failed(error_reason);
            });
        } else {
            if (typeof on_init_ok === 'function') on_init_ok();
        }
        return that;
    };

    var deinitialize = function () {
        log.debug("deinitialize");

        for (var terminal_id in terminals) {
            if (terminals[terminal_id].type === 'webrtc'
                || terminals[terminal_id].type === 'rtsp'
                || terminals[terminal_id].type === 'file') {
                terminals[terminal_id].published.map(function (stream_id) {
                    unpublishStream(stream_id);
                });
            } else if (terminals[terminal_id].type === 'amixer'
                       || terminals[terminal_id].type === 'vmixer'
                       || terminals[terminal_id].type === 'axcoder'
                       || terminals[terminal_id].type === 'vxcoder') {
                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[terminal_id].erizo,
                    "deinit",
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

    var rebuildErizo = function (erizo_id) {
        //TODO: clean the old erizo and rebuild a new one to resume the biz.
    };

    var cleanErizo = function (erizo_id) {
        //TODO: clean up the connections related to the erizo.
    };

    var isErizoFree = function (erizo_id) {
        for (var terminal_id in terminals) {
            if (terminals[terminal_id].erizo === erizo_id) {
                return false;
            }
        }
        return true;
    };

    var newTerminal = function (terminal_id, terminal_type, on_ok, on_error) {
        log.debug("newTerminal:", terminal_id, "terminal_type:", terminal_type);
        if (terminals[terminal_id] === undefined) {
            var purpose = (terminal_type === 'vmixer' || terminal_type === 'vxcoder') ? 'video'
                          : ((terminal_type === 'amixer' || terminal_type === 'axcoder') ? 'audio' : terminal_type);
            erizoManager.allocateErizo(
                purpose,
                {room: room_id, user: 'TODO: fill user_id'},
                function (erizo) {
                    if (terminals[terminal_id] === undefined) {
                        if (erizos[erizo.id] === undefined) {
                            erizos[erizo.id] = {addr: erizo.addr};
                        }
                        terminals[terminal_id] = {type: terminal_type,
                                                  erizo: erizo.id,
                                                  published: [],
                                                  subscribed: {}};
                    } else {
                        log.debug("A previous erizo has been allocated for terminal:", terminal_id);
                        erizoManager.deallocateErizo(erizo);
                    }
                    on_ok();
                },
                on_error);
        } else {
            on_ok();
        }
    };

    var deleteTerminal = function (terminal_id) {
        log.debug("deleteTerminal:", terminal_id);
        var erizo = terminals[terminal_id] ? terminals[terminal_id].erizo : undefined;
        delete terminals[terminal_id];

        if (erizo && isErizoFree(erizo)) {
            erizoManager.deallocateErizo(erizo);
            delete erizos[erizo];
        }
    };

    var isTerminalFree = function (terminal_id) {
        return  terminals[terminal_id]
                && (terminals[terminal_id].published.length === 0
                    && Object.keys(terminals[terminal_id].subscribed).length === 0)
                && (terminals[terminal_id].type !== 'amixer'
                    && terminals[terminal_id].type !== 'vmixer');
    };

    var spreadStream = function (stream_id, target_erizo_id, audio, video, on_ok, on_error) {
        log.debug("spreadStream, stream_id:", stream_id, "target_erizo_id:", target_erizo_id, "audio:", audio, "video:", video);
        log.debug("original stream(streams[stream_id]):", streams[stream_id]);
        var stream_owner = streams[stream_id].owner,
            original_erizo = terminals[stream_owner].erizo,
            hasAudio = audio && !!streams[stream_id].audio,
            hasVideo = video && !!streams[stream_id].video,
            audio_codec = hasAudio ? streams[stream_id].audio.codec : undefined,
            video_codec = hasVideo ? streams[stream_id].video.codec : undefined;
        if (!hasAudio && !hasVideo) {
            on_error("Can't spread stream without audio/video.");
            return;
        }

        if (hasStreamBeenSpread(stream_id, target_erizo_id)) {
            on_ok();
            return;
        }
        log.debug("spreadStream:", stream_id, "audio:", audio, "audio_codec:", audio_codec, "video:", video, "video_codec:", video_codec);
        makeRPC(
            amqper,
            'ErizoJS_' + target_erizo_id,
            "publish",
            [stream_id, 'internal', {owner: stream_owner, has_audio: hasAudio, audio_codec: audio_codec, has_video: hasVideo, video_codec: video_codec, protocol: 'tcp'}],
            function (dest_port) {
                if (!erizos[target_erizo_id] || !erizos[original_erizo]) {
                    makeRPC(
                            amqper,
                            'ErizoJS_' + target_erizo_id,
                            "unpublish",
                            [stream_id]);
                    on_error("Late coming callback for spreading stream.");
                }
                log.debug("internally publish ok, dest_port:", dest_port);
                var dest_ip = erizos[target_erizo_id].addr;
                //FIXME: Hard coded for VCA
                if (/\b(172).(31).(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).(1)\b/g.test(erizos[original_erizo].addr)) {
                    dest_ip = erizos[original_erizo].addr.replace(/.1$/g, ".254");
                }
                makeRPC(
                    amqper,
                    'ErizoJS_' + original_erizo,
                    "subscribe",
                    [stream_id+'@'+target_erizo_id, 'internal', hasAudio ? stream_id : undefined, hasVideo ? stream_id : undefined, {require_audio: hasAudio, require_video: hasVideo, dest_ip: dest_ip, dest_port: dest_port, protocol: 'tcp'}],
                    function () {
                        log.debug("internally subscribe ok");
                        if (streams[stream_id] && erizos[target_erizo_id] && erizos[original_erizo]) {
                            streams[stream_id].spread.push(target_erizo_id);
                            on_ok();
                        } else {
                            makeRPC(
                                amqper,
                                'ErizoJS_' + original_erizo,
                                "unsubscribe",
                                [stream_id+'@'+target_erizo_id]);

                            makeRPC(
                                amqper,
                                'ErizoJS_' + target_erizo_id,
                                "unpublish",
                                [stream_id]);
                            on_error("Late coming callback for spreading stream.");
                        }
                    },
                    function (error_reason) {
                        makeRPC(
                            amqper,
                            'ErizoJS_' + target_erizo_id,
                            "unpublish",
                            [stream_id]);
                        on_error(error_reason);
                    });
            },
            on_error);
    };

    var shrinkStream = function (stream_id, target_erizo_id) {
        var i = streams[stream_id] ? streams[stream_id].spread.indexOf(target_erizo_id) : -1;
        if (i !== -1) {
            log.debug("shrinkStream:", stream_id, "target_erizo_id:", target_erizo_id);
            var stream_owner = streams[stream_id].owner,
                original_erizo = terminals[stream_owner].erizo;

            makeRPC(
                amqper,
                'ErizoJS_' + original_erizo,
                "unsubscribe",
                [stream_id+'@'+target_erizo_id]);

            makeRPC(
                amqper,
                'ErizoJS_' + target_erizo_id,
                "unpublish",
                [stream_id]);

            streams[stream_id].spread.splice(i, 1);
        }
    };

    var hasStreamBeenSpread = function (stream_id, erizo_id) {
        if (streams[stream_id]
            && ((terminals[streams[stream_id].owner] && terminals[streams[stream_id].owner].erizo === erizo_id)
                || (streams[stream_id].spread.indexOf(erizo_id) !== -1))) {
            return true;
        }

        return false;
    };

    var isSpreadNeeded = function (stream_id, erizo_id) {
        var audio_subscribers = (streams[stream_id] && streams[stream_id].audio && streams[stream_id].audio.subscribers) || [],
            video_subscribers = (streams[stream_id] && streams[stream_id].video && streams[stream_id].video.subscribers) || [],
            subscribers = audio_subscribers.concat(video_subscribers.filter(function (item) { return audio_subscribers.indexOf(item) < 0;}));

        for (var i in subscribers) {
            if (terminals[subscribers[i]] && terminals[subscribers[i]].erizo === erizo_id) {
                return true;
            }
        }

        return false;
    };

    var mixAudio = function (stream_id, on_ok, on_error) {
        log.debug("to mix audio of stream:", stream_id);
        spreadStream(stream_id, terminals[audio_mixer].erizo, true, false, function () {
            streams[stream_id].audio.subscribers.push(audio_mixer);
            terminals[audio_mixer].subscribed[stream_id] = {audio: stream_id, video: undefined};
            on_ok();
        }, on_error);
    };

    var unmixAudio = function (stream_id) {
        shrinkStream(stream_id, terminals[audio_mixer].erizo);
        var i = streams[stream_id] && streams[stream_id].audio && streams[stream_id].audio.subscribers.indexOf(audio_mixer);
        if (i !== -1) {
            streams[stream_id].audio.subscribers.splice(i, 1);
        }
        delete terminals[audio_mixer].subscribed[stream_id];
    };

    var mixVideo = function (stream_id, on_ok, on_error) {
        log.debug("to mix video of stream:", stream_id);
        spreadStream(stream_id, terminals[video_mixer].erizo, false, true, function () {
            streams[stream_id].video.subscribers.push(video_mixer);
            terminals[video_mixer].subscribed[stream_id] = {audio: undefined, video: stream_id};
            on_ok();
        }, on_error);
    };

    var unmixVideo = function (stream_id) {
        shrinkStream(stream_id, terminals[video_mixer].erizo);
        var i = streams[stream_id] && streams[stream_id].video && streams[stream_id].video.subscribers.indexOf(audio_mixer);
        if (i !== -1) {
            streams[stream_id].video.subscribers.splice(i, 1);
        }
        delete terminals[video_mixer].subscribed[stream_id];
    };

    var mixStream = function (stream_id, on_ok, on_error) {
        log.debug("to mix stream:", stream_id);
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
        if (streams[stream_id].audio) {
            unmixAudio(stream_id);
        }

        if (streams[stream_id].video) {
            unmixVideo(stream_id);
        }
    };

    var spawnMixedAudio = function (terminal_id, audio_codec, on_ok, on_error) {
        var amixer_erizo = terminals[audio_mixer].erizo;
        log.debug("spawnMixedAudio, for terminal:", terminal_id, "audio_codec:", audio_codec);
        makeRPC(
            amqper,
            'ErizoJS_' + amixer_erizo,
            "generate",
            [terminal_id, audio_codec],
            function (stream_id) {
                log.debug("spawnMixedAudio ok, stream_id:", stream_id);
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
    };

    var spawnMixedVideo = function (video_codec, video_resolution, on_ok, on_error) {
        var vmixer_erizo = terminals[video_mixer].erizo;
        makeRPC(
            amqper,
            'ErizoJS_' + vmixer_erizo,
            "generate",
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
    };

    var getMixedAudio = function (terminal_id, audio_codec, on_ok, on_error) {
        spawnMixedAudio(terminal_id,
                        audio_codec,
                        on_ok,
                        on_error);
    };

    var getMixedVideo = function (video_codec, video_resolution, on_ok, on_error) {
        var candidates = Object.keys(streams).filter(
            function (stream_id) { return streams[stream_id].owner === video_mixer
                                          && streams[stream_id].video
                                          && streams[stream_id].video.codec === video_codec
                                          && (streams[stream_id].video.resolution === video_resolution);
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
        var axcoder_erizo = terminals[axcoder].erizo;
        log.debug("spawnTranscodedAudio, audio_codec:", audio_codec);
        makeRPC(
            amqper,
            'ErizoJS_' + axcoder_erizo,
            "generate",
            [audio_codec, audio_codec],
            function (stream_id) {
                log.debug("spawnTranscodedAudio ok, stream_id:", stream_id);
                if (streams[stream_id] === undefined) {
                    log.debug("add to streams.");
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
        var subscribers = streams[stream_id].audio.subscribers
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
            newTerminal(axcoder, 'axcoder', function () {
                var on_failed = function (reason) {
                    makeRPC(
                        amqper,
                        'ErizoJS_' + terminals[axcoder].erizo,
                        "deinit",
                        [axcoder]);
                    deleteTerminal(axcoder);
                    on_error(error_reason);
                };

                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[axcoder].erizo,
                    "init",
                    ['transcoding', {}],
                    function (supported_audio) {
                        spreadStream(stream_id, terminals[axcoder].erizo, true, false, function () {
                            streams[stream_id].audio.subscribers.push(axcoder);
                            terminals[axcoder].subscribed[stream_id] = {audio: stream_id, video: undefined};
                            on_ok(axcoder);
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
        var vxcoder_erizo = terminals[vxcoder].erizo;
        log.debug("spawnTranscodedVideo, video_codec:", video_codec, "video_resolution:", video_resolution);
        makeRPC(
            amqper,
            'ErizoJS_' + vxcoder_erizo,
            "generate",
            [video_codec, video_resolution],
            function (stream_id) {
                log.debug("spawnTranscodedVideo ok, stream_id:", stream_id);
                if (streams[stream_id] === undefined) {
                    log.debug("add to streams.");
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
        var subscribers = streams[stream_id].video.subscribers
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
            newTerminal(vxcoder, 'vxcoder', function () {
                var on_failed = function (error_reason) {
                    makeRPC(
                        amqper,
                        'ErizoJS_' + terminals[vxcoder].erizo,
                        "deinit",
                        [vxcoder]);
                    deleteTerminal(vxcoder);
                    on_error(error_reason);
                };

                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[vxcoder].erizo,
                    "init",
                    ['transcoding', {resolution: streams[stream_id].video.resolution || 'hd1080p'/*FIXME: hard code*/}],
                    function (supported_video) {
                        spreadStream(stream_id, terminals[vxcoder].erizo, false, true, function () {
                            streams[stream_id].video.subscribers.push(vxcoder);
                            terminals[vxcoder].subscribed[stream_id] = {audio: undefined, video: stream_id};
                            on_ok(vxcoder);
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
        var owner = streams[stream_id].owner;
        var erizo = terminals[owner].erizo;
        makeRPC(
            amqper,
            'ErizoJS_' + erizo,
            "degenerate",
            [stream_id]);
        delete streams[stream_id];

        var i = terminals[owner].published.indexOf(stream_id);
        terminals[owner].published.splice(i ,1);

        if (terminals[owner].published.length === 0 && (terminals[owner].type === 'axcoder' || terminals[owner].type === 'axcoder')) {
            for (var subscription_id in terminals[owner].subscribed) {
                unsubscribeStream(owner, subscription_id);
            }

            deleteTerminal(owner);
        }
    };

    var recycleTemporaryAudio = function (stream_id) {
        if (streams[stream_id]
            && streams[stream_id].audio
            && streams[stream_id].audio.subscribers.length === 0
            && streams[stream_id].spread.length === 0) {

            var terminal_type = terminals[streams[stream_id].owner].type;
            if (terminal_type === 'amixer' || terminal_type === 'axcoder') {
                terminateTemporaryStream(stream_id);
            }
        }
    };

    var recycleTemporaryVideo = function (stream_id) {
        if (streams[stream_id]
            && streams[stream_id].video
            && streams[stream_id].video.subscribers.length === 0
            && streams[stream_id].spread.length === 0) {

            var terminal_type = terminals[streams[stream_id].owner].type;
            if (terminal_type === 'vmixer' || terminal_type === 'vxcoder') {
                terminateTemporaryStream(stream_id);
            }
        }
    };

    var getAudioStream = function (terminal_id, stream_id, audio_codec, on_ok, on_error) {
        log.debug("getAudioStream, terminal_id:", terminal_id, "stream:", streams[stream_id], "audio_codec:", audio_codec);
        if (stream_id === mixed_stream_id) {
            getMixedAudio(terminal_id, audio_codec, function (streamID) {
                log.debug("Got mixed audio:", streamID);
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
        log.debug("getVideoStream, stream:", streams[stream_id], "video_codec:", video_codec, "video_resolution:", video_resolution);
        if (stream_id === mixed_stream_id) {
            getMixedVideo(video_codec, video_resolution, function (streamID) {
                log.debug("Got mixed video:", streamID);
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

    var publishStream = function (terminal_id, stream_id, stream_type, stream_info, onResponse, on_error) {
        makeRPC(
            amqper,
            'ErizoJS_' + terminals[terminal_id].erizo,
            "publish",
            [stream_id, stream_type, stream_info],
            onResponse, on_error);
    };

    var unpublishStream = function (stream_id) {
        log.debug("unpublishStream:", stream_id, "stream.owner:", streams[stream_id].owner);
        var stream = streams[stream_id],
            terminal_id = stream.owner,
            erizo = terminals[terminal_id].erizo;

        var i = terminals[terminal_id].published.indexOf(stream_id);
        if (i !== -1) {
            makeRPC(
                amqper,
                'ErizoJS_' + erizo,
                "unpublish",
                [stream_id]);

            unmixStream(stream_id);
            removeSubscriptions(stream_id);
            terminals[terminal_id].published.splice(i, 1);
        }

        delete streams[stream_id];

        if (isTerminalFree(terminal_id)) {
            deleteTerminal(terminal_id);
        }
    };

    var subscribeStream = function (subscriber, subscription_id, audio_stream, video_stream, options, on_ok, on_error) {
        log.debug("subscribeStream, subscriber:", subscriber, "subscription_id:", subscription_id, "audio_stream:", audio_stream, "video_stream:", video_stream);
        if (audio_stream === video_stream || audio_stream === undefined || video_stream === undefined) {
            var stream_id = audio_stream || video_stream;
            spreadStream(stream_id, terminals[subscriber].erizo, true, true, function () {
                var audioStream = options.require_audio ? stream_id : undefined,
                    videoStream = options.require_video ? stream_id : undefined;

                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[subscriber].erizo,
                    "subscribe",
                    [subscription_id, terminals[subscriber].type, audioStream, videoStream, options],
                    on_ok,
                    on_error);
            }, on_error);
        } else {
            log.debug("spread audio and video stream independently.");
            spreadStream(audio_stream, terminals[subscriber].erizo, true, false, function () {
                log.debug("spread audio_stream:", audio_stream, " ok.");
                spreadStream(video_stream, terminals[subscriber].erizo, false, true, function () {
                    log.debug("spread video_stream:", video_stream, " ok.");
                    makeRPC(
                        amqper,
                        'ErizoJS_' + terminals[subscriber].erizo,
                        "subscribe",
                        [subscription_id, terminals[subscriber].type, audio_stream, video_stream, options],
                        on_ok,
                        on_error);
                }, on_error);
            }, on_error);
        }
    };

    var unsubscribeStream = function (subscriber, subscription_id) {
        var erizo_id = terminals[subscriber].erizo,
            subscription = terminals[subscriber].subscribed[subscription_id],
            audio_stream = subscription && subscription.audio,
            video_stream = subscription && subscription.video;

        makeRPC(
            amqper,
            'ErizoJS_' + erizo_id,
            "unsubscribe",
            [subscription_id]);

        if (audio_stream) {
            if (streams[audio_stream] && streams[audio_stream].audio) {
                var i = streams[audio_stream].audio.subscribers.indexOf(subscriber);
                streams[audio_stream].audio.subscribers.splice(i, 1);
                subscription.audio = undefined
            }
            if (!isSpreadNeeded(audio_stream, erizo_id)) {
                shrinkStream(audio_stream, erizo_id);
            }
        }

        if (video_stream) {
            if (streams[video_stream] && streams[video_stream].video) {
                var i = streams[video_stream].video.subscribers.indexOf(subscriber);
                streams[video_stream].video.subscribers.splice(i, 1);
                subscription.video = undefined;
            }

            if ((!audio_stream || video_stream !== audio_stream)
                && !isSpreadNeeded(video_stream, erizo_id)) {
                shrinkStream(video_stream, erizo_id);
            }
        }

        if (subscription_id === subscriber+'-'+mixed_stream_id) {
            if (audio_stream) {
                recycleTemporaryAudio(audio_stream);
            }

            if (video_stream) {
                recycleTemporaryVideo(video_stream);
            }
        }

        delete terminals[subscriber].subscribed[subscription_id];

        if (isTerminalFree(subscriber)) {
            deleteTerminal(subscriber);
        }
    };

    var removeSubscriptions = function (stream_id) {
        for (var terminal_id in terminals) {
            for (var subscription_id in terminals[terminal_id].subscribed) {
                if (terminals[terminal_id].subscribed[subscription_id].audio === stream_id || terminals[terminal_id].subscribed[subscription_id].video === stream_id) {
                    unsubscribeStream(terminal_id, subscription_id);
                }
            }
        }
    };

    // External interfaces.
    that.destroy = function () {
        deinitialize();
    };

    that.publish = function (terminal_id, stream_id, stream_type, stream_info, enable_mixing, onResponse) {
        log.debug("publish, terminal_id:", terminal_id, "stream_id:", stream_id, "stream_type:", stream_type, "enable_mixing:", enable_mixing, "stream_info:", stream_info);
        var doPublish = function () {
            var erizo_id = terminals[terminal_id].erizo,
                audio_codec = undefined,
                video_codec = undefined;

            publishStream(
                terminal_id,
                stream_id,
                stream_type,
                stream_info,
                function (response) {
                    log.debug("publish response:", response);
                    if (response.type === 'ready') {
                        audio_codec = response.audio_codecs && response.audio_codecs.length > 0 && response.audio_codecs[0];
                        video_codec = response.video_codecs && response.video_codecs.length > 0 && response.video_codecs[0];
                        streams[stream_id] = {owner: terminal_id,
                                              audio: (stream_info.has_audio && audio_codec) ? {codec: audio_codec,
                                                                                               subscribers: []} : undefined,
                                              video: (stream_info.has_video && video_codec) ? {codec: video_codec,
                                                                                               resolution: undefined, //FIXME: the publication should carry resolution info.
                                                                                               framerate: undefined,  //FIXME: the publication should carry frame-rate info.
                                                                                               subscribers: []} : undefined,
                                              spread: []
                                              };

                        terminals[terminal_id].published.push(stream_id);

                        if (enable_mixing && config.enableMixing) {
                            mixStream(stream_id, function () {
                                log.info("Mix stream["+stream_id+"] successfully.");
                            }, function (error_reason) {
                                log.error(error_reason);
                            });
                        } else {
                            log.debug("publish ok, will not mix in.");
                        }
                        onResponse(response);
                    } else {
                        onResponse(response);
                    }
                }, function (error_reason) {
                    log.debug("publish failed, reason:", error_reason);
                    onResponse({type: 'failed', reason: error_reason});
                });
        };

        if (streams[stream_id] === undefined) {
            newTerminal(terminal_id, stream_type, function () {
                doPublish();
            }, function (error_reason) {onResponse({type: 'failed', reason: error_reason});});
        } else {
            onResponse({type: 'failed', reason: "Stream[" + stream_id + "] already set for " + terminal_id});
        }
    };

    that.unpublish = function (stream_id) {
        log.debug("unpublish, stream_id:", stream_id);
        if (streams[stream_id]) {
            unpublishStream(stream_id);
        }
    };

    that.subscribeSelectively = function (terminal_id, subscription_id, subscription_type, audio_stream_id, video_stream_id, options, onResponse) {
        log.debug("subscribe, terminal_id:", terminal_id, ", subscription_id:", subscription_id, ", subscription_type:", subscription_type, ", audio_stream_id:", audio_stream_id, ", video_stream_id:", video_stream_id, ", options:", options);
        var audio_codec = options.audio_codec
                       || (audio_stream_id === mixed_stream_id && supported_audio_codecs[0])
                       || (streams[audio_stream_id] && streams[audio_stream_id].audio && streams[audio_stream_id].audio.codec)
                       || undefined,

            video_codec = options.video_codec
                       || (video_stream_id === mixed_stream_id && supported_video_codecs[0])
                       || (streams[video_stream_id] && streams[video_stream_id].video && streams[video_stream_id].video.codec)
                       || undefined,

            video_resolution = options.video_resolution
                       || (video_stream_id === mixed_stream_id && supported_video_resolutions[0])
                       || (streams[video_stream_id] && streams[video_stream_id].video && streams[video_stream_id].video.resolution)
                       || undefined;

        if ((options.require_audio && !audio_codec) || (options.require_video && !video_codec)) {
            onResponse({type: 'failed', reason: 'No proper audio/video codec.'});
            return;
        }

        if ((options.require_audio && !audio_stream_id) || (options.require_video && !video_stream_id)) {
            onResponse({type: 'failed', reason: 'Invalid subscribe request.'});
            return;
        }

        var doSubscribe = function () {
            var audio_stream = undefined, video_stream = undefined;

            var on_ok = function (audioStream, videoStream) {
                return function () {
                    log.debug("subscribe ok, audioStream:", audioStream, "videoStream", videoStream);
                    streams[audio_stream] && streams[audio_stream].audio.subscribers.push(terminal_id);
                    streams[video_stream] && streams[video_stream].video.subscribers.push(terminal_id);
                    terminals[terminal_id].subscribed[subscription_id] = {audio: audioStream, video: videoStream};
                    onResponse({type: 'ready'});
                }};

            var on_error = function (error_reason) {
                log.debug("subscribe failed, reason:", error_reason);
                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[terminal_id].erizo,
                    "disconnect",
                    [subscription_id]);
                onResponse({type: 'failed', reason: error_reason});
            };

            if (options.require_audio && audio_stream_id) {
                log.debug("require audio track of stream:", audio_stream_id);
                getAudioStream(terminal_id, audio_stream_id, audio_codec, function (streamID) {
                    audio_stream = streamID;
                    log.debug("Got audio stream:", audio_stream);
                    if (options.require_video && video_stream_id) {
                        log.debug("require video track of stream:", video_stream_id);
                        getVideoStream(video_stream_id, video_codec, video_resolution, function (streamID) {
                            video_stream = streamID;
                            log.debug("Got video stream:", video_stream);
                            subscribeStream(terminal_id, subscription_id, audio_stream, video_stream, options, on_ok(audio_stream, video_stream), function (error_reason) {
                                recycleTemporaryVideo(video_stream);
                                recycleTemporaryAudio(audio_stream);
                                on_error(error_reason);
                            });
                        }, function (error_reason) {
                            recycleTemporaryAudio(audio_stream);
                            on_error(error_reason);
                        })
                    } else {
                        subscribeStream(terminal_id, subscription_id, audio_stream, undefined, options, on_ok(audio_stream, undefined), function (error_reason) {
                            recycleTemporaryAudio(audio_stream);
                            on_error(error_reason);
                        });
                    }
                }, on_error);
            } else if (options.require_video && video_stream_id) {
                log.debug("require video track of stream:", video_stream_id);
                getVideoStream(video_stream_id, video_codec, video_resolution, function (streamID) {
                    video_stream = streamID;
                    subscribeStream(terminal_id, subscription_id, undefined, video_stream, options, on_ok(undefined, video_stream), function (error_reason) {
                        recycleTemporaryVideo(video_stream);
                        on_error(error_reason);
                    });
                }, on_error);
            } else {
                log.debug("No audio or video is required.");
                on_error("No audio or video is required.");
            }
        };

        var doConnect = function () {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[terminal_id].erizo,
                "connect",
                [subscription_id, subscription_type, options],
                function (response) {
                    if (response.type === 'ready') {
                        audio_codec = response.audio_codecs && response.audio_codecs.length > 0 && response.audio_codecs[0];
                        video_codec = response.video_codecs && response.video_codecs.length > 0 && response.video_codecs[0];
                        log.debug("subscriber connected ok.");
                        doSubscribe();
                    } else {
                        onResponse(response);
                    }
                },
                onResponse);
        };

        newTerminal(terminal_id, subscription_type, function () {
            doConnect();
        }, function (error_reason) {onResponse({type: 'failed', reason: error_reason});});
    };

    that.subscribe = function (terminal_id, subscription_id, subscription_type, stream_id, options, onResponse) {
        if (streams[stream_id] || stream_id === mixed_stream_id) {
            that.subscribeSelectively(
                terminal_id,
                subscription_id,
                subscription_type,
                options.require_audio ? stream_id : undefined,
                options.require_video ? stream_id : undefined,
                options,
                onResponse);
        }
    };

    that.unsubscribe = function (terminal_id, subscription_id) {
        log.debug("unsubscribe from terminal:", terminal_id, "for subscription:", subscription_id);
        if (terminals[terminal_id]) {
            unsubscribeStream(terminal_id, subscription_id);
        }
    };

    that.unsubscribeAll = function (terminal_id) {
        log.debug("unsubscribeAll from terminal:", terminal_id);
        if (terminals[terminal_id]) {
            Object.keys(terminals[terminal_id].subscribed).map(function (subscription_id) {
                unsubscribeStream(terminal_id, subscription_id);
            });
        }
    };

    that.mix = function (stream_id, on_ok, on_error) {
        if (streams[stream_id]) {
            mixStream(stream_id, on_ok, on_error);
        } else {
            on_error("Stream does not exist:"+stream_id);
        }
    };

    that.unmix = function (stream_id, on_ok, on_error) {
        if (streams[stream_id]) {
            unmixStream(stream_id);
            on_ok();
        } else {
            on_error("Stream does not exist:"+stream_id);
        }
    };

    that.onConnectionSignalling = function (terminal_id, connection_id, msg) {
        log.debug("onConnectionSignalling, terminal_id:", terminal_id, "connection_id:", connection_id/*, "msg:", msg*/);
        if (terminals[terminal_id]) {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[terminal_id].erizo,
                "onConnectionSignalling",
                [connection_id, msg]);
        }
    };

    that.onTrackControl = function (terminal_id, connection_id, track, direction, action, on_ok, on_error) {
        if (terminals[terminal_id]) {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[terminal_id].erizo,
                "onTrackControl",
                [connection_id, track, direction, action],
                on_ok,
                on_error);
        } else {
            on_error("No such a terminal:"+terminal_id);
        }
    };

    that.setVideoBitrate = function (stream_id, bitrate, on_ok, on_error) {
        if (streams[stream_id] && terminals[streams[stream_id].owner]) {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[streams[stream_id].owner].erizo,
                "setVideoBitrate",
                [stream_id, bitrate]);
            on_ok();
        } else {
            on_error("No such a stream:"+stream_id);
        }
    };

    that.getRegion = function (stream_id, on_ok, on_error) {
        if (video_mixer) {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[video_mixer].erizo,
                "getRegion",
                [stream_id],
                on_ok,
                on_error);
        }
    };

    that.setRegion = function (stream_id, region, on_ok, on_error) {
        if (video_mixer) {
            makeRPC(
                amqper,
                'ErizoJS_' + terminals[video_mixer].erizo,
                "setRegion",
                [stream_id, region],
                on_ok,
                on_error);
        }
    };

    that.setPrimary = function (terminal_id) {
        log.debug("setPrimary:", terminal_id);
        for (var i in terminals[terminal_id].published) {
            var stream_id = terminals[terminal_id].published[i];
            if (streams[stream_id] && streams[stream_id].video && (streams[stream_id].video.subscribers.indexOf(video_mixer) !== -1)) {
                makeRPC(
                    amqper,
                    'ErizoJS_' + terminals[video_mixer].erizo,
                    "setPrimary",
                    [stream_id]);
                return;
            }
        }
    };

    return initialize();
};
