/*global require, module, global*/
'use strict';
require = require('module')._load('./AgentLoader');
var woogeenWebrtc = require('./webrtcLib/build/Release/webrtc');
// var WebRtcConnection = woogeenWebrtc.WebRtcConnection;
var AudioFrameConstructor = woogeenWebrtc.AudioFrameConstructor;
var VideoFrameConstructor = woogeenWebrtc.VideoFrameConstructor;
var AudioFramePacketizer = woogeenWebrtc.AudioFramePacketizer;
var VideoFramePacketizer = woogeenWebrtc.VideoFramePacketizer;
var path = require('path');
var logger = require('./logger').logger;
var cipher = require('./cipher');
// Logger
var log = logger.getLogger('WrtcConnection');

var addon = require('./webrtcLib/build/Release/webrtc');//require('./erizo/build/Release/addon');

function createWrtc(id, threadPool, ioThreadPool, mediaConfiguration, ipAddresses) {
    var wrtc = new addon.WebRtcConnection(
        threadPool, ioThreadPool, id,
        global.config.webrtc.stunserver,
        global.config.webrtc.stunport,
        global.config.webrtc.minport,
        global.config.webrtc.maxport,
        false, // should trickle
        '', // rtp_media_config
        false, // useNicer
        '', // turnserver,
        '', // turnport,
        '', //turnusername,
        '', //turnpass,
        '', //networkinterface
        ipAddresses //ipAddresses
    );

    return wrtc;
}

module.exports = function (spec, on_status) {
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
        wrtc;

    var isReserve = function (line, reserved) {
        var re = new RegExp('^a=((rtpmap)|(fmtp)|(rtcp-fb))', 'i'),
            re1 = new RegExp('^a=((rtpmap)|(fmtp)|(rtcp-fb)):(' + reserved.map(function(c){return '('+c+')';}).join('|') + ')\\s+', 'i');

        return !re.test(line) || re1.test(line);
    };

    var isAudioFmtEqual = function (fmt1, fmt2) {
      return (fmt1.codec === fmt2.codec)
        && (((fmt1.sampleRate === fmt2.sampleRate) && ((fmt1.channelNum === fmt2.channelNum) || (fmt1.channelNum === 1 && fmt2.channelNum === undefined) || (fmt1.channelNum === undefined && fmt2.channelNum === 1)))
            || (fmt1.codec === 'pcmu') || (fmt1.codec === 'pcma'));
    };

    var determineAudioFmt = function (sdp) {
        var lines = sdp.split('\n');
        var a_begin = lines.findIndex(function (line) {return line.startsWith('m=audio');});
        if ((a_begin >= 0) && formatPreference.audio) {
            var a_end = lines.slice(a_begin + 1).findIndex(function(line) {return line.startsWith('m=');});
                a_end = (a_end > 0 ? a_begin + a_end + 1 : lines.length);

            var a_lines = lines.slice(a_begin, a_end);

            var fmtcode = '';
            var fmts = [];
            var fmtcodes = a_lines[0].split(' ').slice(3);
            for (var i in fmtcodes) {
                var fmtname = a_lines.filter(function (line) {
                    return line.startsWith('a=rtpmap:'+fmtcodes[i]);
                })[0].split(' ')[1].split('/');

                var fmt = {codec: fmtname[0].toLowerCase()};
                (fmt.codec !== 'pcmu' && fmt.codec !== 'pcma') && fmtname[1] && (fmt.sampleRate = Number(fmtname[1]));
                (fmt.codec !== 'pcmu' && fmt.codec !== 'pcma') && fmtname[2] && (fmt.channelNum = Number(fmtname[2]));
                (fmt.code === 'g722') && (fmt.sampleRate = 16000);

                fmts.push({code: fmtcodes[i], fmt: fmt});

                if ((fmtcode === '') && formatPreference.audio.preferred && isAudioFmtEqual(formatPreference.audio.preferred, fmt)) {
                    audio_fmt = fmt;
                    fmtcode = fmtcodes[i];
                }
            }

            if ((fmtcode === '') && formatPreference.audio.optional) {
                for (var j in fmts) {
                    if (formatPreference.audio.optional.findIndex(function (o) {return isAudioFmtEqual(o, fmts[j].fmt);}) !== -1) {
                        audio_fmt = fmts[j].fmt;
                        fmtcode = fmts[j].code;
                        break;
                    }
                }
            }

            if (fmtcode) {
                var reserved_codes = [fmtcode];
                fmts.forEach(function (f) {
                    if ((f.fmt.codec === 'telephone-event')
                        || ((f.fmt.codec === 'cn') && f.fmt.sampleRate && (f.fmt.sampleRate === audio_fmt.sampleRate))) {
                        reserved_codes.push(f.code);
                    }
                });

                a_lines[0] = lines[a_begin].split(' ').slice(0, 3).concat(reserved_codes).join(' ');
                a_lines = a_lines.filter(function(line) {
                    return isReserve(line, reserved_codes);
                }).join('\n');
                return lines.slice(0, a_begin).concat(a_lines).concat(lines.slice(a_end)).join('\n');
            } else {
                log.info('No proper audio format');
                return sdp;
            }
        }
        return sdp;
    };

    var isVideoFmtEqual = function (fmt1, fmt2) {
      return (fmt1.codec === fmt2.codec);
    };

    var determineVideoFmt = function (sdp) {
        var lines = sdp.split('\n');
        var v_begin = lines.findIndex(function (line) {return line.startsWith('m=video');});
        if ((v_begin >= 0) && formatPreference.video) {
            var v_end = lines.slice(v_begin + 1).findIndex(function(line) {return line.startsWith('m=');});
                v_end = (v_end > 0 ? v_begin + v_end + 1 : lines.length);

            var v_lines = lines.slice(v_begin, v_end);

            var fmtcode = '';
            var fmts = [];
            var fmtcodes = v_lines[0].split(' ').slice(3);
            for (var i in fmtcodes) {
                var fmtname = v_lines.filter(function (line) {
                    return line.startsWith('a=rtpmap:'+fmtcodes[i]);
                })[0].split(' ')[1].split('/');

                var fmt = {codec: fmtname[0].toLowerCase()};
                if (fmtname[0] === 'rtx') {
                    var m_code = v_lines.filter(function(line) {return line.startsWith('a=fmtp:' + fmtcodes[i] + ' apt=');
                    })[0].split(' ')[1].split('=')[1];
                   fmts.push({code: fmtcodes[i], fmt: fmt, main_code: m_code});
                } else {
                   fmts.push({code: fmtcodes[i], fmt: fmt});
                }

                if ((fmtcode === '') && formatPreference.video.preferred && isVideoFmtEqual(formatPreference.video.preferred, fmt)) {
                    video_fmt = fmt;
                    fmtcode = fmtcodes[i];
                }
            }

            if ((fmtcode === '') && formatPreference.video.optional) {
                for (var j in fmts) {
                    if (formatPreference.video.optional.findIndex(function (o) {return isVideoFmtEqual(o, fmts[j].fmt);}) !== -1) {
                        video_fmt = fmts[j].fmt;
                        fmtcode = fmts[j].code;
                        break;
                    }
                }
            }

            if (fmtcode) {
                var reserved_codes = [fmtcode];
                fmts.forEach(function (f) {
                    if ((f.fmt.codec === 'red')
                        || (f.fmt.codec === 'ulpfec')
                        || ((f.fmt.codec === 'rtx') && (f.main_code === fmtcode))) {
                        reserved_codes.push(f.code);
                    }
                });

                v_lines[0] = v_lines[0].split(' ').slice(0, 3).concat(reserved_codes).join(' ');
                v_lines = v_lines.filter(function(line) {
                    return isReserve(line, reserved_codes);
                }).join('\n');
                return lines.slice(0, v_begin).concat(v_lines).concat(lines.slice(v_end)).join('\n');
            } else {
                log.info('No proper video format');
                return sdp;
            }
        }
        return sdp;
    };

    var CONN_INITIAL = 101, CONN_STARTED = 102, CONN_GATHERED = 103, CONN_READY = 104, CONN_FINISHED = 105, CONN_CANDIDATE = 201, CONN_SDP = 202, CONN_FAILED = 500;
    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
     */
    var initWebRtcConnection = function (wrtc) {

        wrtc.init(function (newStatus, mess) {
            log.info('message: WebRtcConnection status update, ' +
                     'id: ' + wrtc.wrtcId + ', status: ' + newStatus + ', mess:' + mess);

            var message = mess;

            switch(newStatus) {
                case CONN_INITIAL:
                    //callback('callback', {type: 'started'});
                    break;

                case CONN_SDP:
                case CONN_GATHERED:
                    networkInterfaces.forEach((i) => {
                        if (i.private_ip_match_pattern && i.replaced_ip_address) {
                            message = message.replace(i.private_ip_match_pattern, i.replaced_ip_address);
                        }
                    });
                    audio && (message = determineAudioFmt(message));
                    video && (message = determineVideoFmt(message));
                    log.debug('Answer SDP', message);
                    on_status({type: 'answer', sdp: message});
                    break;

                case CONN_CANDIDATE:
                    networkInterfaces.forEach((i) => {
                        if (i.private_ip_match_pattern && i.replaced_ip_address) {
                          message = message.replace(i.private_ip_match_pattern, i.replaced_ip_address);
                        }
                      });
                    on_status({type: 'candidate', candidate: message});

                case CONN_FAILED:
                    log.warn('message: failed the ICE process, ' +
                             'code: ' + ', id: ' + wrtc.wrtcId);
                    on_status({type: 'failed', reason: 'Ice procedure failed.'});
                    break;

                case CONN_READY:
                    log.debug('message: connection ready, ' +
                              'id: ' + wrtc.wrtcId + ', ' +
                              'status: ' + newStatus);
                    // If I'm a subscriber and I'm bowser, I ask for a PLI
                    on_status({
                        type: 'ready',
                        audio: audio_fmt ? audio_fmt : false,
                        video: video_fmt ? video_fmt : false
                    });
                    break;
                default:
                    log.warn('Status not proccess:', newStatus);
                    break;
            }
        });
    };

    var bindFrameConstructors = function () {
        if (audio) {
            audioFrameConstructor = new AudioFrameConstructor();
            //wrtc.setAudioReceiver(audioFrameConstructor);
            audioFrameConstructor.bindTransport(wrtc);
        }

        if (video) {
            videoFrameConstructor = new VideoFrameConstructor();
            //wrtc.setVideoReceiver(videoFrameConstructor);
            videoFrameConstructor.bindTransport(wrtc);
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
            audioFramePacketizer.bindTransport(wrtc);
        }

        if (video) {
            // TODO: check remote-side's support on RED/ULPFEC instead of
            // hardcoding here
            videoFramePacketizer = new VideoFramePacketizer(true, true);
            videoFramePacketizer.bindTransport(wrtc);
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
        unbindFramePacketizers();
        unbindFrameConstructors();
        wrtc && wrtc.close();
        wrtc = undefined;
    };

    that.onSignalling = function (msg) {
        var processSignalling = function() {
            if (msg.type === 'offer') {
                log.debug('on offer:', msg.sdp);
                checkOffer(msg.sdp, function() {
                    wrtc.setRemoteSdp(msg.sdp);
                }, function (reason) {
                    log.error('offer error:', reason);
                    on_status({type: 'failed', reason: reason});
                });
            } else if (msg.type === 'candidate') {
                wrtc.addRemoteCandidate(msg.candidate.sdpMid, msg.candidate.sdpMLineIndex, msg.candidate.candidate);
            }
        };

        if (wrtc) {
            processSignalling();
        } else {
            var count = 0,
                interval = setInterval(function() {
                    if (wrtc) {
                        processSignalling();
                        clearInterval(interval);
                    } else {
                        if (count > 200) {
                            log.info('wrtc has not got ready in 10s, will drop a ignalling, type:', msg.type);
                            clearInterval(interval);
                        } else {
                            count = count + 1;
                        }
                    }
                }, 50);
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

    var keystore = path.resolve(path.dirname(global.config.webrtc.keystorePath), '.woogeen.keystore');
    cipher.unlock(cipher.k, keystore, function cb (err, passphrase) {
        if (!err) {
            // Libnice collects candidates on |ipAddresses| only.
            var ipAddresses=[];
            networkInterfaces.forEach((i) => {
              if (i.ip_address) {
                ipAddresses.push(i.ip_address);
              }
            });
            wrtc = createWrtc(wrtcId, threadPool, ioThreadPool, 'rtp_media_config', ipAddresses);

            if (direction === 'in') {
                bindFrameConstructors();
            }

            if (direction === 'out') {
                bindFramePacketizers();
            }

            initWebRtcConnection(wrtc);
        } else {
            log.error('init error:', err);
        }
    });

    return that;
};
