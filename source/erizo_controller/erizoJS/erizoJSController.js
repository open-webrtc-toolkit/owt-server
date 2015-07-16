/*global require, exports, setInterval, clearInterval, GLOBAL, process*/
'use strict';
var addon = require('./../../bindings/mcu/build/Release/addon');

var logger = require('./../common/logger').logger;
var rpc = require('./../common/rpc');
var cipher = require('../../common/cipher');

// Logger
var log = logger.getLogger('ErizoJSController');

var rtsp_prefix = 'rtsp://';
if (GLOBAL.config.erizo.rtsp) {
    if (GLOBAL.config.erizo.rtsp.username && GLOBAL.config.erizo.rtsp.passwd) {
        rtsp_prefix += [GLOBAL.config.erizo.rtsp.username, ':', GLOBAL.config.erizo.rtsp.passwd, '@'].join('');
    }
    rtsp_prefix += [GLOBAL.config.erizo.rtsp.addr, ':', GLOBAL.config.erizo.rtsp.port, GLOBAL.config.erizo.rtsp.application].join('');
} else {
    rtsp_prefix += 'localhost:1935/live/';
}

exports.ErizoJSController = function () {
    var that = {},
        subscribers = {},
        publishers = {},
        mixers = {},
        mixerProxy,
        erizoControllerId,
        // INTERVAL_TIME_SDP = 100,
        INTERVAL_TIME_FIR = 100,
        // INTERVAL_TIME_KILL = 30*60*1000, // Timeout to kill itself after a timeout since the publisher leaves room.
        waitForFIR,
        initWebRtcConnection,
        getSdp,
        getRoap,
        useHardware = GLOBAL.config.erizo.hardwareAccelerated,
        openh264Enabled = GLOBAL.config.erizo.openh264Enabled;

    that.initMixer = function (id, oop, roomConfig, callback) {
        if (publishers[id] === undefined) {
            // Place holder to avoid duplicated mixer creations when the initMixer request
            // comes again before current mixer with the same id is created.
            // The mixer creation may take a while partly because we need to query the
            // hardware capability in some case.
            publishers[id] = true;

            var doInitMixer = function(hardwareAccelerated) {
                var config = {
                    mixer: true,
                    oop: oop,
                    proxy: false,
                    video: {
                        hardware: hardwareAccelerated,
                        avcoordinate: roomConfig.video.avCoordinated,
                        maxinput: roomConfig.video.maxInput,
                        bitrate: roomConfig.video.bitrate,
                        resolution: roomConfig.video.resolution,
                        backgroundcolor: roomConfig.video.bkColor,
                        templates: roomConfig.video.layout
                    }
                };
                var mixer = new addon.Mixer(JSON.stringify(config));
                // setup event listeners
                mixer.addEventListener('UpdateStream', function (evt) {
                    if (erizoControllerId !== undefined) {
                        rpc.callRpc(erizoControllerId, 'eventReport', ['updateStream', evt, id]);
                    }
                });
                publishers[id] = mixer;
                subscribers[id] = [];
                callback('callback', 'success');
            };

            if (useHardware) {
                // Query the hardware capability only if we want to try it.
                require('child_process').exec('vainfo', function (err, stdout) {
                    var info = '';
                    if (err) {
                        info = err.toString();
                    } else if(stdout.length > 0) {
                        info = stdout.toString();
                    }
                    // Check whether hardware codec should be used for this room
                    useHardware = (info.indexOf('VA-API version 0.34.0') != -1) ||
                                    (info.indexOf('VA-API version: 0.34') != -1);
                });
            }
            doInitMixer(useHardware);
        } else {
            log.info('Mixer already set for', id);
        }
    };

    /*
     * Given a WebRtcConnection waits for the state READY for ask it to send a FIR packet to its publisher.
     */
    waitForFIR = function (wrtc, to) {

        if (publishers[to] !== undefined) {
            var intervarId = setInterval(function () {
              if (publishers[to]!==undefined){
                if (wrtc.getCurrentState() >= 103 && publishers[to].getPublisherState() >=103) {
                    publishers[to].sendFIR();
                    clearInterval(intervarId);
                }
              }

            }, INTERVAL_TIME_FIR);
        }
    };

    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
     */
    initWebRtcConnection = function (wrtc, sdp, id_mixer, callback, id_pub, id_sub) {
        if(typeof sdp != 'string') sdp = JSON.stringify(sdp); // fixes some issues with sending json object as json, and not as string

        if (GLOBAL.config.erizoController.sendStats) {
            wrtc.getStats(function (newStats){
                log.info('Received RTCP stats:', newStats);
                var timeStamp = new Date();
                rpc.callRpc('stats_handler', 'stats', [{pub: id_pub, subs: id_sub, stats: JSON.parse(newStats), timestamp:timeStamp.getTime()}]);
            });
        }

        var terminated = false;

        wrtc.init(function (newStatus){

          if (terminated) {
            return;
          }

          var localSdp, answer;
          log.info('webrtc Addon status', newStatus);

          if (GLOBAL.config.erizoController.sendStats) {
            var timeStamp = new Date();
            rpc.callRpc('stats_handler', 'event', [{pub: id_pub, subs: id_sub, type: 'connection_status', status: newStatus, timestamp:timeStamp.getTime()}]);
          }

          if (newStatus === 104) {
            terminated = true;
          }
          if (newStatus === 102 && !sdpDelivered) {
            localSdp = wrtc.getLocalSdp();
            answer = getRoap(localSdp, roap);
            callback('callback', answer);
            sdpDelivered = true;

          }
          if (newStatus === 103) {
            // Perform the additional work for publishers.
            if (id_mixer) {
              var mixer = publishers[id_mixer];
              if (mixer) {
                mixer.addPublisher(publishers[id_pub], id_pub);
                mixers[id_pub] = mixer;
              } else {
                if (mixerProxy) {
                  mixerProxy.addPublisher(publishers[id_pub], id_pub);
                  mixers[id_pub] = mixerProxy;
                }
              }
            }

            callback('onReady');
          }
        });

        var roap = sdp,
            remoteSdp = getSdp(roap);
        wrtc.setRemoteSdp(remoteSdp);

        var sdpDelivered = false;
    };

    /*
     * Gets SDP from roap message.
     */
    getSdp = function (roap) {

        var reg1 = new RegExp(/^.*sdp\":\"(.*)\",.*$/),
            sdp = roap.match(reg1)[1],
            reg2 = new RegExp(/\\r\\n/g);

        sdp = sdp.replace(reg2, '\n');

        return sdp;

    };

    /*
     * Gets roap message from SDP.
     */
    getRoap = function (sdp, offerRoap) {

        var reg1 = new RegExp(/\n/g),
            offererSessionId = offerRoap.match(/("offererSessionId":)(.+?)(,)/)[0],
            answererSessionId = '106',
            answer = ('\n{\n \"messageType\":\"ANSWER\",\n');

        sdp = sdp.replace(reg1, '\\r\\n');
        answer += ' \"sdp\":\"' + sdp + '\",\n';
        answer += ' ' + offererSessionId + '\n';
        answer += ' \"answererSessionId\":' + answererSessionId + ',\n \"seq\" : 1\n}\n';

        return answer;
    };

    that.addExternalInput = function (from, url, mixer, callback) {

        if (publishers[from] === undefined) {

            log.info('Adding external input peer_id', from);

            var ei = new addon.ExternalInput(url);
            var muxer = new addon.Gateway(JSON.stringify({externalInput: true}));

            publishers[from] = muxer;
            subscribers[from] = [];

            var answer = ei.init(function (message) {
                if (message === 'success') {
                    log.info('External input', from, 'initialization succeed');

                    if (mixer.id && mixer.oop && mixerProxy === undefined) {
                        var config = {
                            mixer: true,
                            oop: true,
                            proxy: true,
                        };
                        mixerProxy = new addon.Mixer(JSON.stringify(config));
                    }

                    muxer.addExternalPublisher(ei, from);

                    if (mixer.id) {
                        var mixerObj = publishers[mixer.id];
                        if (mixerObj) {
                            mixerObj.addPublisher(muxer, from);
                            mixers[from] = mixerObj;
                        } else {
                            if (mixerProxy) {
                                mixerProxy.addPublisher(muxer, from);
                                mixers[from] = mixerProxy;
                            }
                        }
                    }

                    callback('onReady');
                } else {
                    log.error('External input', from, 'initialization failed');
                    publishers[from].close();
                    delete publishers[from];
                }
            });

            if (answer < 0) {
                callback('callback', 'input url initialization error');
            } else {
                callback('callback', 'success');
            }
        } else {
            log.info('Publisher already set for', from);
            callback('callback', 'external input already existed');
        }
    };

    that.addExternalOutput = function (to, from, url, interval, callback) {
        if (publishers[to] !== undefined) {
            var config = {
                id: from,
                url: url,
                interval: interval
            };
            if (from === '' && url === '' && interval === 0) { // rtsp out
                if (GLOBAL.config.erizo.rtsp.enabled !== true) {
                    callback('callback', 'not enabled');
                    return;
                }
                config.url = [rtsp_prefix, 'room_', to, '.sdp'].join('');
                config.id = config.url;
                publishers[to].addExternalOutput(JSON.stringify(config), function (resp) {
                    callback('callback', resp);
                });
                return;
            }
            log.info('Adding ExternalOutput to ' + to + ' url ' + url);

            // recordingId here is used as the peerId
            publishers[to].addExternalOutput(JSON.stringify(config), function (resp) {
                callback('callback', resp);
            });
            return;
        }

        log.error('Failed adding ExternalOutput to ' + to + ' with url ' + url);
        callback('callback', 'error');
    };

    that.removeExternalOutput = function (to, from, close, callback) {
        if (publishers[to] !== undefined) {
            log.info('Stopping ExternalOutput:' + from);
            if (publishers[to].removeExternalOutput(from, close)) {
                return callback('callback', 'success');
            }
        }

        callback('callback', 'error');
    };

    /*
     * Adds a publisher to the room. This creates a new Gateway
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the Gateway.
     */
    that.addPublisher = function (from, sdp, mixer, callback) {

        if (publishers[from] === undefined) {
            log.info('Adding publisher peer_id', from);
            cipher.unlock(cipher.k, '../../cert/.woogeen.keystore', function cb (err, obj) {
                if (!err) {
                    var erizoPassPhrase = obj.erizo;

                    var hasAudio = false;
                    var hasVideo = false;
                    if (sdp.indexOf('m=audio') !== -1) {
                        hasAudio = true;
                    }
                    if (sdp.indexOf('m=video') !== -1) {
                        hasVideo = true;
                    }

                    var hasH264 = true;
                    if (!useHardware && !openh264Enabled) {
                        hasH264 = false;
                    }

                    var wrtc = new addon.WebRtcConnection(hasAudio, hasVideo, hasH264, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, GLOBAL.config.erizo.keystorePath, GLOBAL.config.erizo.keystorePath, erizoPassPhrase, true, true, true, true);
                    var muxer = new addon.Gateway();
                    publishers[from] = muxer;
                    subscribers[from] = [];

                    if (mixer.id && mixer.oop && mixerProxy === undefined) {
                        var config = {
                            mixer: true,
                            oop: true,
                            proxy: true,
                        };
                        mixerProxy = new addon.Mixer(JSON.stringify(config));
                    }

                    initWebRtcConnection(wrtc, sdp, mixer.id, callback, from);
                    muxer.addPublisher(wrtc, from);
                } else {
                    log.warn('Failed to publish the stream:', err);
                }
            });
        } else {
            log.info('Publisher already set for', from);
        }
    };

    /*
     * Adds a subscriber to the room. This creates a new WebRtcConnection.
     * This WebRtcConnection will be added to the subscribers list of the
     * Gateway.
     */
    that.addSubscriber = function (from, to, audio, video, sdp, callback) {
        if (typeof sdp != 'string') sdp = JSON.stringify(sdp);

        if (publishers[to] !== undefined && subscribers[to].indexOf(from) === -1 && sdp.match('OFFER') !== null) {
            log.info('Adding subscriber from', from, 'to', to, 'audio', audio, 'video', video);
            cipher.unlock(cipher.k, '../../cert/.woogeen.keystore', function cb (err, obj) {
                if (!err) {
                    var erizoPassPhrase = obj.erizo;

                    var hasH264 = true;
                    if (!useHardware && !openh264Enabled) {
                        hasH264 = false;
                    }
                    var wrtc = new addon.WebRtcConnection(audio, video, hasH264, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, GLOBAL.config.erizo.keystorePath, GLOBAL.config.erizo.keystorePath, erizoPassPhrase, true, true, true, true);

                    initWebRtcConnection(wrtc, sdp, undefined, callback, to, from);

                    subscribers[to].push(from);
                    publishers[to].addSubscriber(wrtc, from);
                } else {
                    log.warn('Failed to subscribe the stream:', err);
                }
            });
        }
    };

    /*
     * Removes a publisher from the room. This also deletes the associated Gateway.
     */
    that.removePublisher = function (from) {

        if (subscribers[from] !== undefined && publishers[from] !== undefined) {
            log.info('Removing muxer', from);
            if (mixers[from]) {
                mixers[from].removePublisher(from);
                delete mixers[from];
            }
            publishers[from].close();
            log.info('Removing subscribers', from);
            delete subscribers[from];
            log.info('Removing publisher', from);
            delete publishers[from];

            var count = 0;
            for (var k in publishers) {
                if (publishers.hasOwnProperty(k)) {
                   ++count;
                }
            }

            log.info('Publishers:', count);
            if (count === 0) {
                if (mixerProxy !== undefined) {
                    log.info('Removing mixer proxy', from);
                    mixerProxy.close();
                }
                log.info('Removed all publishers. Killing process.');
                process.exit(0);
            }
        }
    };

    /*
     * Removes a subscriber from the room. This also removes it from the associated Gateway.
     */
    that.removeSubscriber = function (from, to) {

        var index = subscribers[to].indexOf(from);
        if (index !== -1) {
            log.info('Removing subscriber ', from, 'to muxer ', to);
            publishers[to].removeSubscriber(from);
            subscribers[to].splice(index, 1);
        }
    };

    that.subscribeStream = function subscribeStream (publisher_id, subscriber_id, isAudio, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Enabling subscriber', subscriber_id, 'in', publisher_id);
            publishers[publisher_id].subscribeStream(subscriber_id, isAudio);
            return callback('callback');
        }
        callback('callback', 'error');
    };

    that.unsubscribeStream = function unsubscribeStream (publisher_id, subscriber_id, isAudio, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Disabling subscriber', subscriber_id, 'in', publisher_id);
            publishers[publisher_id].unsubscribeStream(subscriber_id, isAudio);
            return callback('callback');
        }
        callback('callback', 'error');
    };

    /*
     * Removes all the subscribers related with a client.
     */
    that.removeSubscriptions = function (from) {

        var key, index;

        log.info('Removing subscriptions of ', from);
        for (key in subscribers) {
            if (subscribers.hasOwnProperty(key)) {
                index = subscribers[key].indexOf(from);
                if (index !== -1) {
                    log.info('Removing subscriber ', from, 'to muxer ', key);
                    publishers[key].removeSubscriber(from);
                    subscribers[key].splice(index, 1);
                }
            }
        }
    };

    that.addToMixer = function addToMixer (publisher_id, mixer_id, callback) {
        if (mixers[publisher_id]) {
            return callback('callback', 'already added to mix');
        }
        var mixer = publishers[mixer_id];
        if (mixer) {
            mixer.addPublisher(publishers[publisher_id], publisher_id);
            mixers[publisher_id] = mixer;
        } else {
            if (mixerProxy) {
                mixerProxy.addPublisher(publishers[publisher_id], publisher_id);
                mixers[publisher_id] = mixerProxy;
            }
        }
        callback('callback');
    };

    that.removeFromMixer = function removeFromMixer (publisher_id, mixer_id, callback) {
        if (!mixers[publisher_id]) {
            return callback('callback', 'already removed from mix');
        }
        mixers[publisher_id].removePublisher(publisher_id);
        delete mixers[publisher_id];
        callback('callback');
    };

    that.getRegion = function (mixer, publisher, callback) {
        if (publishers[mixer] !== undefined) {
            log.info('get the Region of ' + publisher + ' in mixer ' + mixer);
            var ret = publishers[mixer].getRegion(publisher);
            return callback('callback', ret);
        }

        callback('callback');
    };

    that.setRegion = function (mixer, publisher, region, callback) {
        if (publishers[mixer] !== undefined) {
            log.info('set the Region of ' + publisher + ' to ' + region  + ' in mixer ' + mixer);
            if (publishers[mixer].setRegion(publisher, region)) {
                return callback('callback');
            }
        }

        callback('callback', 'failed');
    };

    that.setVideoBitrate = function (mixer, publisher, bitrate, callback) {
        if (publishers[mixer] !== undefined) {
            log.info('set the bitrate of ' + publisher + ' to ' + bitrate  + ' in mixer ' + mixer);
            if (publishers[mixer].setVideoBitrate(publisher, bitrate)) {
                return callback('callback');
            }
        }

        callback('callback', 'failed');
    };

    that.setControllerId = function (id) {
        erizoControllerId = id;
    };

    return that;
};
