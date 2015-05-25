/*
 * Class Stream represents a local or a remote Stream in the client. It will handle the WebRTC stream
 * and identify the stream and where it should be drawn.
 */
Erizo.Stream = function (spec) {
    "use strict";
    var that = Erizo.EventDispatcher(spec);

    that.stream = spec.stream;
    that.url = spec.url;
    that.onClose = undefined;
    that.local = false;
    that.video = spec.video;
    that.audio = spec.audio;
    that.screen = spec.screen;
    that.videoSize = spec.videoSize;

    if (that.videoSize !== undefined && (!(that.videoSize instanceof Array) || that.videoSize.length !== 4)) {
        throw Error("Invalid Video Size");
    }
    if (spec.local === undefined || spec.local === true) {
        that.local = true;
    }
    that.initialized = !that.local;
    function getReso(w, h) {
        return {
            mandatory: {
                minWidth: w,
                minHeight: h,
                maxWidth: w,
                maxHeight: h
            },
            optional: []
        };
    }
    function isLegacyChrome () {
        return window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./) !== null && window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] <= 35;
    }
    var supportedVideoList = {
        'true': true,
        'unspecified': true,
        'sif': getReso(320, 240),
        'vga': getReso(640, 480),
        'hd720p': getReso(1280, 720)
    };

    that.signalOnPlayAudio = undefined;
    that.signalOnPauseAudio = undefined;
    that.signalOnPlayVideo = undefined;
    that.signalOnPauseVideo = undefined;

    // Public functions

    that.getId = function () {
        return spec.streamID;
    };

    that.getAttributes = function () {
        return spec.attributes;
    };

    // Indicates if the stream has audio activated
    that.hasAudio = function () {
        return spec.audio;
    };

    // Indicates if the stream has video activated
    that.hasVideo = function () {
        return spec.video;
    };

    // Indicates if the stream has screen activated
    that.hasScreen = function () {
        return spec.screen;
    };

    that.setVideo = function (resolution, framerate) {
        resolution += '';
        if (supportedVideoList[resolution] !== undefined) {
            that.video = spec.video = supportedVideoList[resolution];
            spec.attributes = spec.attributes || {};
            spec.attributes.resolution = resolution;
            if (typeof framerate === 'object' && framerate instanceof Array && framerate.length > 1) {
                spec.attributes.minFrameRate = framerate[0];
                spec.attributes.maxFrameRate = framerate[1];
            }
            return true;
        }
        return false;
    };

    that.setAudio = function (value) {
        if (value === true || value === false) {
            that.audio = spec.audio = value;
            return true;
        }
        return false;
    };

    // Initializes the stream and tries to retrieve a stream from local video and audio
    // We need to call this method before we can publish it in the client.
    that.init = function (onFailure) {
        var init_retry = arguments[1];
        if (init_retry === undefined) {
            init_retry = 2;
        } // do not write: var init_retry = arguments[1] || 2;
        if (that.initialized === true) {
            if (typeof onFailure === 'function') {
                onFailure({
                    type: 'warning',
                    msg: 'STREAM_ALREADY_INITIALIZED'
                });
            }
            return;
        }
        if (that.local !== true) {
            if (typeof onFailure === 'function') {
                onFailure({
                    type: 'warning',
                    msg: 'STREAM_NOT_LOCAL'
                });
            }
            return;
        }
        try {
            if ((spec.audio || spec.video || spec.screen) && spec.url === undefined) {
                L.Logger.debug("Requested access to local media");
                var videoOpt = spec.video;
                if ((videoOpt === true || spec.screen) && that.videoSize !== undefined) {
                    videoOpt = {mandatory: {minWidth: that.videoSize[0], minHeight: that.videoSize[1], maxWidth: that.videoSize[2], maxHeight: that.videoSize[3]}};
                }
                var opt = {video: videoOpt, audio: spec.audio, fake: spec.fake};
                if (spec.screen) {
                    opt = {video: {mandatory: {chromeMediaSource: 'screen', maxWidth: screen.availWidth, maxHeight: screen.availHeight}}};
                }
                if (!isLegacyChrome() && spec.attributes.minFrameRate) {
                    if (opt.video === true) {
                        opt.video = {mandatory: {}};
                    }
                    opt.video.mandatory.maxFrameRate = spec.attributes.maxFrameRate;
                    opt.video.mandatory.minFrameRate = spec.attributes.minFrameRate;
                }
                if (opt.video.mandatory !== undefined && opt.video.mandatory.maxWidth !== undefined) {
                    switch (opt.video.mandatory.maxWidth) {
                    case 320:
                        spec.attributes.maxVideoBW = 512;
                        break;
                    case 640:
                        spec.attributes.maxVideoBW = 1024;
                        break;
                    case 1280:
                        spec.attributes.maxVideoBW = 2048;
                        break;
                    default:
                        spec.attributes.maxVideoBW = undefined;
                        break;
                    }
                } else {
                    spec.attributes.maxVideoBW = undefined;
                }
                L.Logger.debug(opt);
                Erizo.GetUserMedia(opt, function (stream) {
                    L.Logger.info("User has granted access to local media.");
                    that.stream = stream;
                    var mediaEvent = Erizo.MediaEvent({type: "access-accepted"});
                    that.dispatchEvent(mediaEvent);
                    that.initialized = true;
                    // report accurate video resolution demo in [sdk].ui.js::videoplayer.js, thru <1>.
                    // 1. from ui display (require ui): $('$streamid')[0].videoWidth, $('$streamid')[0].videoHeight
                    // 2. from webrtc_internals (stream should be published): stream.pc.peerConnection.getStats(function(stats){stats.result();});
                    // 2.1. new method? E.g., stream.getFrameResolution() when pc id ready.
                }, function (error) {
                    var err = {type: 'error', msg: error.name || error};
                    switch (err.msg) {
                    // below - internally handled
                    case 'Starting video failed':       // firefox: camera possessed by other process?
                    case 'TrackStartError':             // chrome: probably resolution not supported
                        that.setVideo('unspecified');
                        that.videoSize = undefined;
                        if (init_retry > 0) {
                            setTimeout(function () {
                                that.init(onFailure, init_retry-1);
                            }, 1);
                            return;
                        } else {
                            err.msg = 'MEDIA_OPTION_INVALID';
                        }
                        break;
                    // below - exposed
                    case 'DevicesNotFoundError':        // chrome
                        err.msg = 'DEVICES_NOT_FOUND';
                        break;
                    case 'NotSupportedError':           // chrome
                        err.msg = 'NOT_SUPPORTED';
                        break;
                    case 'PermissionDeniedError':       // chrome
                        err.msg = 'PERMISSION_DENIED';
                        break;
                    case 'PERMISSION_DENIED':           // firefox
                        break;
                    case 'ConstraintNotSatisfiedError': // chrome
                        err.msg = 'CONSTRAINT_NOT_SATISFIED';
                        break;
                    default:
                        if (!err.msg) {
                            err.msg = 'UNDEFINED';
                        }
                    }
                    L.Logger.error('Media access:', err.msg);
                    if (typeof onFailure === 'function') {
                        onFailure(err);
                    }
                });
            } else {
                if (typeof onFailure === 'function') {
                    onFailure({
                        type: 'warning',
                        msg: 'STREAM_HAS_NO_MEDIA_ATTRIBUTES'
                    });
                }
            }
        } catch (e) {
            L.Logger.error('Stream init:', e);
            if (typeof onFailure === 'function') {
                onFailure({
                    type: 'error',
                    msg: e.message || e
                });
            }
        }
    };

    that.onRemove = function (){};

    that.close = function () {
        if (that.initialized === true && that.local === true) {
            if (typeof that.onClose === 'function') {
                that.onClose();
            }
            if (that.stream !== undefined) {
                that.stream.stop();
            }
            that.stream = undefined;
            that.initialized = false;
        }
    };

    that.enableAudio = function () {
        if (that.hasAudio() && that.initialized && that.stream !== undefined && that.stream.getAudioTracks()[0].enabled !== true) {
            that.stream.getAudioTracks()[0].enabled = true;
            return true;
        }
        return false;
    };

    that.disableAudio = function () {
        if (that.hasAudio() && that.initialized && that.stream !== undefined && that.stream.getAudioTracks()[0].enabled) {
            that.stream.getAudioTracks()[0].enabled = false;
            return true;
        }
        return false;
    };

    that.enableVideo = function () {
        if (that.hasVideo() && that.initialized && that.stream !== undefined && that.stream.getVideoTracks()[0].enabled !== true) {
            that.stream.getVideoTracks()[0].enabled = true;
            return true;
        }
        return false;
    };

    that.disableVideo = function () {
        if (that.hasVideo() && that.initialized && that.stream !== undefined && that.stream.getVideoTracks()[0].enabled) {
            that.stream.getVideoTracks()[0].enabled = false;
            return true;
        }
        return false;
    };

    that.playAudio = function (onSuccess, onFailure) {
        if (that.enableAudio() && typeof that.signalOnPlayAudio === 'function') {
            return that.signalOnPlayAudio(onSuccess, onFailure);
        }
        if (typeof onFailure === 'function') {
            onFailure('unable to call playAudio');
        }
    };

    that.pauseAudio = function (onSuccess, onFailure) {
        if (that.disableAudio() && typeof that.signalOnPauseAudio === 'function') {
            return that.signalOnPauseAudio(onSuccess, onFailure);
        }
        if (typeof onFailure === 'function') {
            onFailure('unable to call pauseAudio');
        }
    };

    that.playVideo = function (onSuccess, onFailure) {
        if (that.enableVideo() && typeof that.signalOnPlayVideo === 'function') {
            return that.signalOnPlayVideo(onSuccess, onFailure);
        }
        if (typeof onFailure === 'function') {
            onFailure('unable to call playVideo');
        }
    };

    that.pauseVideo = function (onSuccess, onFailure) {
        if (that.disableVideo() && typeof that.signalOnPauseVideo === 'function') {
            return that.signalOnPauseVideo(onSuccess, onFailure);
        }
        if (typeof onFailure === 'function') {
            onFailure('unable to call pauseVideo');
        }
    };

    that.updateConfiguration = function (config, callback) {
        if (config === undefined) {
            return;
        }
        if (that.pc) {
            that.pc.updateSpec(config, callback);
        } else {
            return ("This stream has not been published, ignoring");
        }
    };

    return that;
};
