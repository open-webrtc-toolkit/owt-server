// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var webrtcAddon = require('../webrtcLib/build/Release/webrtc');
var AudioFrameConstructor = webrtcAddon.AudioFrameConstructor;
var VideoFrameConstructor = webrtcAddon.VideoFrameConstructor;
var AudioFramePacketizer = webrtcAddon.AudioFramePacketizer;
var VideoFramePacketizer = webrtcAddon.VideoFramePacketizer;

var path = require('path');
var logger = require('../logger').logger;
var cipher = require('../cipher');
// Logger
var log = logger.getLogger('WrtcConnection');
var transform = require('sdp-transform');

const { Connection } = require('./connection');
const {
    processOffer,
    getAudioSsrc,
    getVideoSsrcList,
    getSimulcastInfo
} = require('./sdp');

var addon = require('../webrtcLib/build/Release/webrtc');//require('./erizo/build/Release/addon');

function createWrtc(id, threadPool, ioThreadPool, mediaConfiguration, ipAddresses) {
    var conn = new Connection(id, threadPool, ioThreadPool);

    return conn;
}

module.exports = function (spec, on_status, on_mediaUpdate) {
    var that = {},
        direction = spec.direction,
        wrtcId = spec.connectionId,
        threadPool = spec.threadPool,
        ioThreadPool = spec.ioThreadPool,
        // preferredAudioCodecs = spec.preferred_audio_codecs,
        // preferredVideoCodecs = spec.preferred_video_codecs,
        networkInterfaces = spec.network_interfaces,
        audio = !!spec.media.audio,
        video = !!spec.media.video,
        formatPreference = spec.formatPreference,
        audio_fmt,
        video_fmt,
        audioFrameConstructor,
        audioFramePacketizer,
        videoFrameConstructor,
        videoFramePacketizer,
        final_prf,
        stream,
        wrtc;

    var CONN_INITIAL = 101, CONN_STARTED = 102, CONN_GATHERED = 103, CONN_READY = 104, CONN_FINISHED = 105, CONN_CANDIDATE = 201, CONN_SDP = 202, CONN_FAILED = 500;
    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
     */
    var initWebRtcConnection = function (wrtc) {
        wrtc.on('status_event', (evt, status) => {
            if (evt.type === 'answer') {
                let message = evt.sdp;
                networkInterfaces.forEach((i) => {
                    if (i.ip_address && i.replaced_ip_address) {
                        message = message.replace(new RegExp(i.ip_address, 'g'), i.replaced_ip_address);
                    }
                });
                // audio && (message = determineAudioFmt(message));
                // video && (message = determineVideoFmt(message));
                log.debug('Answer SDP', message);
                on_status({type: 'answer', sdp: message});

            } else if (evt.type === 'candidate') {
                let message = evt.candidate;
                networkInterfaces.forEach((i) => {
                    if (i.ip_address && i.replaced_ip_address) {
                        message = message.replace(new RegExp(i.ip_address, 'g'), i.replaced_ip_address);
                    }
                });
                on_status({type: 'candidate', candidate: message});

            } else if (evt.type === 'failed') {
                log.warn('message: failed the ICE process, ' +
                         'code: ' + ', id: ' + wrtc.id);
                on_status({type: 'failed', reason: 'Ice procedure failed.'});

            } else if (evt.type === 'ready') {
                log.debug('message: connection ready, ' +
                          'id: ' + wrtc.wrtcId + ', ' +
                          'status: ' + status);
                // If I'm a subscriber and I'm bowser, I ask for a PLI
                on_status({
                    type: 'ready',
                    audio: audio_fmt ? audio_fmt : false,
                    video: video_fmt ? video_fmt : false
                });
            } else if (evt.type === 'rid') {
                log.error('discover id:', evt.rid, evt.ssrc);
            }
        });
        wrtc.init(wrtc.id);
    };

    var bindFrameConstructors = function () {
        if (audio) {
            audioFrameConstructor = new AudioFrameConstructor();
            //wrtc.setAudioReceiver(audioFrameConstructor);
            audioFrameConstructor.bindTransport(stream);
        }

        if (video) {
            videoFrameConstructor = new VideoFrameConstructor(on_mediaUpdate);
            //wrtc.setVideoReceiver(videoFrameConstructor);
            videoFrameConstructor.bindTransport(stream);
        }
    };

    var unbindFrameConstructors = function () {
        if (audio && audioFrameConstructor) {
            audioFrameConstructor.unbindTransport();
            audioFrameConstructor.close();
            audioFrameConstructor = undefined;
        }

        if (video && videoFrameConstructor) {
            videoFrameConstructor.unbindTransport();
            videoFrameConstructor.close();
            videoFrameConstructor = undefined;
        }
    };

    var bindFramePacketizers = function () {
        if (audio) {
            audioFramePacketizer = new AudioFramePacketizer();
            audioFramePacketizer.bindTransport(stream);
        }

        if (video) {
            // TODO: check remote-side's support on RED/ULPFEC instead of
            // hardcoding here
            videoFramePacketizer = new VideoFramePacketizer(true, true);
            videoFramePacketizer.bindTransport(stream);
            //video.resolution && wrtc.setSendResolution(video.resolution);
        }
    };

    var unbindFramePacketizers = function () {
        if (audio && audioFramePacketizer) {
            audioFramePacketizer.unbindTransport();
            audioFramePacketizer.close();
            audioFramePacketizer = undefined;
        }

        if (video && videoFramePacketizer) {
            videoFramePacketizer.unbindTransport();
            videoFramePacketizer.close();
            videoFramePacketizer = undefined;
        }
    };

    var checkOffer = function (sdp, on_ok, on_error) {
        var contains_audio = /(m=audio).*/g.test(sdp);
        var contains_video = /(m=video).*/g.test(sdp);

        //log.debug('checkOffer, sdp:', sdp, 'contains_audio:', contains_audio, 'contains_video:', contains_video);
        if (audio && !contains_audio) {
            return on_error('audio is required but not contained by offer sdp');
        } else if (!audio && contains_audio) {
            return on_error('audio is not required but contained by offer sdp');
        } else if (video && !contains_video) {
            return on_error('video is required but not contained by offer sdp');
        } else if (!video && contains_video) {
            return on_error('video is not required but contained by offer sdp');
        } else {
            on_ok();
        }
    };

    that.close = function () {
        if (wrtc) {
            wrtc.wrtc.stop();
            unbindFramePacketizers();
            unbindFrameConstructors();
            wrtc.close();
            wrtc = undefined;
        }
    };

    that.onSignalling = function (msg) {
        var processSignalling = function() {
            if (msg.type === 'offer') {
                log.debug('on offer:', msg.sdp);
                checkOffer(msg.sdp, function() {
                    var {sdp, audioFormat, videoFormat} = processOffer(msg.sdp, formatPreference, direction);
                    msg.sdp = sdp;
                    audio_fmt = audioFormat;
                    video_fmt = videoFormat;
                    log.debug('offer after processing:', msg.sdp);
                    if (direction === 'in') {
                        const aSsrc = getAudioSsrc(sdp);
                        const vSsrc = getVideoSsrcList(sdp);
                        log.debug('SDP ssrc:',aSsrc, vSsrc);
                        wrtc.setRemoteSsrc(aSsrc, vSsrc, '');
                        wrtc.setSimulcastInfo(getSimulcastInfo(sdp));
                    }
                    wrtc.setRemoteSdp(msg.sdp);
                }, function (reason) {
                    log.error('offer error:', reason);
                    on_status({type: 'failed', reason: reason});
                });
            } else if (msg.type === 'candidate') {
                wrtc.addRemoteCandidate(msg.candidate);
            } else if (msg.type === 'removed-candidates') {
                msg.candidates.forEach(function(val) {
                    wrtc.removeRemoteCandidate('', 0, val.candidate);
                });
                wrtc.removeRemoteCandidate('', -1, '');
            }
        };

        if (wrtc) {
            processSignalling();
        } else {
            // should not reach here
            log.error('wrtc is not ready');
        }
    };

    that.onTrackControl = function (track, dir, action, on_ok, on_error) {
        if (['audio', 'video', 'av'].indexOf(track) < 0) {
            on_error('Invalid track.');
            return;
        }

        var trackUpdate = false;
        if ((track === 'av' || track === 'audio') && audio) {
            if (dir === 'in' && audioFrameConstructor) {
                audioFrameConstructor.enable(action === 'on');
            } else if (dir === 'out' && audioFramePacketizer) {
                audioFramePacketizer.enable(action === 'on');
            } else {
                on_error('Ambiguous audio direction.');
                return;
            }
            trackUpdate = true;
        }
        if ((track === 'av' || track === 'video') && video) {
            if (dir === 'in' && videoFrameConstructor) {
                videoFrameConstructor.enable(action === 'on');
            } else if (dir === 'out' && videoFramePacketizer) {
                videoFramePacketizer.enable(action === 'on');
            } else {
                on_error('Ambiguous video direction.');
                return;
            }
            trackUpdate = true;
        }

        if (trackUpdate) {
            on_ok();
        } else {
            on_error('No track found');
        }
    };

    that.addDestination = function (track, dest) {
        if (direction === 'in') {
            if (audio && track === 'audio' && audioFrameConstructor) {
                audioFrameConstructor.addDestination(dest);
                return;
            } else if (video && track === 'video' && videoFrameConstructor) {
                videoFrameConstructor.addDestination(dest);
                return;
            }

            log.warn('Wrong track:'+track);
        }

        log.error('Direction error.');
    };

    that.removeDestination = function (track, dest) {
        if (direction === 'in') {
            if (audio && track === 'audio' && audioFrameConstructor) {
                audioFrameConstructor.removeDestination(dest);
                return;
            } else if (video && track === 'video' && videoFrameConstructor) {
                videoFrameConstructor.removeDestination(dest);
                return;
            }

            log.warn('Wrong track:'+track);
        }

        log.error('Direction error.');
    };

    that.receiver = function (track) {
        if (audio && track === 'audio') {
            return audioFramePacketizer;
        }

        if (video && track === 'video') {
            return videoFramePacketizer;
        }

        log.error('receiver error');
        return undefined;
    };

    that.setVideoBitrate = function (bitrateKBPS, on_ok, on_error) {
        if (video && videoFrameConstructor) {
            videoFrameConstructor.setBitrate(bitrateKBPS);
            return on_ok();
        }
        return on_error('no video track');
    };

    //FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
    that.requestKeyFrame = function () {
        if (direction === 'in' && video && videoFrameConstructor) {
            videoFrameConstructor.requestKeyFrame();
        }
    };

    // Libnice collects candidates on |ipAddresses| only.
    var ipAddresses=[];
    networkInterfaces.forEach((i) => {
      if (i.ip_address) {
        ipAddresses.push(i.ip_address);
      }
    });
    wrtc = createWrtc(wrtcId, threadPool, ioThreadPool, 'rtp_media_config', ipAddresses);
    wrtc.addMediaStream(wrtc.id, {label: ''}, direction === 'in');
    stream = wrtc.getMediaStream(wrtc.id);

    if (direction === 'in') {
        bindFrameConstructors();
    }

    if (direction === 'out') {
        bindFramePacketizers();
    }

    initWebRtcConnection(wrtc);

    return that;
};
