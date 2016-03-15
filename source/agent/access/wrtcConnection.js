/*global require, exports, GLOBAL*/
'use strict';

var woogeenWebrtc = require('./webrtc/build/Release/webrtc');
var WebRtcConnection = woogeenWebrtc.WebRtcConnection;
var AudioFrameConstructor = woogeenWebrtc.AudioFrameConstructor;
var VideoFrameConstructor = woogeenWebrtc.VideoFrameConstructor;
var AudioFramePacketizer = woogeenWebrtc.AudioFramePacketizer;
var VideoFramePacketizer = woogeenWebrtc.VideoFramePacketizer;
var path = require('path');
var logger = require('./logger').logger;
var cipher = require('./cipher');
// Logger
var log = logger.getLogger('WrtcConnection');

exports.WrtcConnection = function (spec) {
    var that = {},
        direction = spec.direction,
        // preferredAudioCodecs = spec.preferred_audio_codecs,
        // preferredVideoCodecs = spec.preferred_video_codecs,
        privateRegexp = spec.private_ip_regexp,
        publicIP = spec.public_ip,
        audio = false,
        video = false,
        audioFrameConstructor,
        audioFramePacketizer,
        videoFrameConstructor,
        videoFramePacketizer,
        wrtc;

    var getAudioCodecList = function (sdp) {
        //TODO: find a better way to parse the top prior audio codec.
        var lines = sdp.toLowerCase().split('\n');
        var mline = lines.filter(function (line) {return line.startsWith('m=audio');});
        if (mline.length === 1) {
            var l = [];
            var fmtcodes = mline[0].split(' ').slice(3);
            for (var i in fmtcodes) {
                var fmtname = lines.filter(function (line) {
                    return line.startsWith('a=rtpmap:'+fmtcodes[i]);
                })[0].split(' ')[1].split('/'); // FIXME: making functions within a for-loop is potentially wrong.
                if (fmtname[1] === '8000') {
                    l.push(fmtname[0]);
                } else {
                    l.push(fmtname.join('_'));
                }
            }
            return l;
        }
        return [];
    };


    var getVideoCodecList = function (sdp) {
        //TODO: find a better way to parse the top prior video codec.
        var lines = sdp.toLowerCase().split('\n');
        var mline = lines.filter(function (line) {
            return line.startsWith('m=video');
        });
        if (mline.length === 1) {
            var l = [];
            var fmtcodes = mline[0].split(' ').slice(3);
            for (var i in fmtcodes) {
                var fmtname = lines.filter(function (line) {
                    return line.startsWith('a=rtpmap:'+fmtcodes[i]);
                })[0].split(' ')[1].split('/'); // FIXME: making functions within a for-loop is potentially wrong.
                if (fmtname[0] !== 'red' && fmtname[0] !== 'ulpfec') {
                    l.push(fmtname[0]);
                }
            }
            return l;
        }
        return [];
    };

   var CONN_INITIAL = 101, CONN_STARTED = 102, CONN_GATHERED = 103, CONN_READY = 104, CONN_FINISHED = 105, CONN_CANDIDATE = 201, CONN_SDP = 202, CONN_FAILED = 500;
    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
     */
    var initWebRtcConnection = function (wrtc, on_status) {
        var terminated = false;
        var audio_codec_list_in_answer = [],
            video_codec_list_in_answer = [];

        wrtc.init(function (newStatus, mess){

          if (terminated) {
            return;
          }

          log.info('webrtc Addon status' + newStatus + mess);

          switch (newStatus) {
            case CONN_FINISHED:
              terminated = true;
              break;

            case CONN_INITIAL:
              on_status({type: 'started'});
              break;

            case CONN_SDP:
            case CONN_GATHERED:
              log.debug('Sending SDP', mess);
              mess = mess.replace(privateRegexp, publicIP);
              audio_codec_list_in_answer = getAudioCodecList(mess);
              video_codec_list_in_answer = getVideoCodecList(mess);
              on_status({type: 'answer', sdp: mess});
              break;

            case CONN_CANDIDATE:
              mess = mess.replace(privateRegexp, publicIP);
              on_status({type: 'candidate', candidate: mess});
              break;

            case CONN_FAILED:
              on_status({type: 'failed', reason: 'Ice procedure failed.'});
              break;

            case CONN_READY: {
              // If I'm a subscriber and I'm bowser, I ask for a PLI
              //if (id_sub && browser === 'bowser') {
                //log.info('SENDING PLI from ', id_pub, ' to ', id_sub);
                //publishers[id_pub].muxer.sendPLI();
              //}
              on_status({type: 'ready', audio_codecs: audio_codec_list_in_answer, video_codecs: video_codec_list_in_answer});
              break;
            }

            default:
              break;
          }
        });

        on_status({type: 'initializing'});
    };

    that.init = function (audio_info, video_info, on_status) {
        audio = audio_info;
        video = video_info;
        var keystore = path.resolve(path.dirname(GLOBAL.config.erizo.keystorePath), '.woogeen.keystore');
        cipher.unlock(cipher.k, keystore, function cb (err, obj) {
            if (!err) {
                var erizoPassPhrase = obj.erizo;
                wrtc = new WebRtcConnection(!!audio, !!video, true/*FIXME: hash264:hard coded*/, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, GLOBAL.config.erizo.keystorePath, GLOBAL.config.erizo.keystorePath, erizoPassPhrase, true, true, true, true, false);

                if (direction === 'in') {
                    if (audio) {
                        audioFrameConstructor = new AudioFrameConstructor(wrtc);
                        wrtc.setAudioReceiver(audioFrameConstructor);
                    }

                    if (video) {
                        videoFrameConstructor = new VideoFrameConstructor(wrtc);
                        wrtc.setVideoReceiver(videoFrameConstructor);
                    }
                }

                if (direction === 'out') {
                    if (audio) {
                        audioFramePacketizer = new AudioFramePacketizer(wrtc);
                    }

                    if (video) {
                        videoFramePacketizer = new VideoFramePacketizer(wrtc);
                        //video.resolution && wrtc.setSendResolution(video.resolution);
                    }
                }

                initWebRtcConnection(wrtc, on_status);
            } else {
                log.error('init error:', err);
            }
        });
    };

    that.close = function () {
        audio && audioFramePacketizer && audioFramePacketizer.close();
        video && videoFramePacketizer && videoFramePacketizer.close();
        wrtc && wrtc.close();
        audio && audioFrameConstructor && audioFrameConstructor.close();
        video && videoFrameConstructor && videoFrameConstructor.close();
    };

    that.onSignalling = function (msg) {
        if (msg.type === 'offer') {
            wrtc.setRemoteSdp(msg.sdp);
            wrtc.start();
        } else if (msg.type === 'candidate') {
            wrtc.addRemoteCandidate(msg.candidate.sdpMid, msg.candidate.sdpMLineIndex, msg.candidate.candidate);
        }
    };

    that.onTrackControl = function (track, dir, action, on_ok, on_error) {
        if (track === 'audio' && audio) {
            if ((dir === 'in' && direction === 'out')
                || (dir === 'out' && direction === 'in')) {
                wrtc.enableAudio(action === 'on');
                on_ok();
            } else {
                on_error('Ambiguous direction.');
            }
        } else if (track === 'video' && video) {
            if ((dir === 'in' && direction === 'out')
                || (dir === 'out' && direction === 'in')) {
                wrtc.enableVideo(action === 'on');
                on_ok();
            } else {
                on_error('Ambiguous direction.');
            }
        } else {
            on_error('Invalid track.');
        }
    };

    that.addDestination = function (track, dest) {
        if (direction === 'in') {
            if (audio && track === 'audio') {
                audioFrameConstructor.addDestination(dest);
                return;
            } else if (video && track === 'video') {
                videoFrameConstructor.addDestination(dest);
                return;
            }

            log.warn('Wrong track:'+track);
        }

        log.error('Direction error.');
    };

    that.removeDestination = function (track, dest) {
        if (direction === 'in') {
            if (audio && track === 'audio') {
                audioFrameConstructor.removeDestination(dest);
                return;
            } else if (video && track === 'video') {
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

    that.setVideoBitrate = function (bitrateKBPS) {
        if (video) {
            videoFrameConstructor.setBitrate(bitrateKBPS);
        }
    };

    //FIXME: Temporarily add this interface to workround the hardware mode's absence of feedback mechanism.
    that.requestKeyFrame = function () {
        if (direction === 'in' && video) {
            videoFrameConstructor.requestKeyFrame();
        }
    };

    return that;
};
