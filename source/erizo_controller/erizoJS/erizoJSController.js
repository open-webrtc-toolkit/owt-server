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
        externalOutputs = {},
        mixers = {},
        mixerProxy,
        roomId,
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
                        simulcast: roomConfig.video.simulcast===true,
                        templates: roomConfig.video.layout
                    }
                };
                var mixer = new addon.Mixer(JSON.stringify(config));
                // setup event listeners
                mixer.addEventListener('UpdateStream', function (evt) {
                    if (erizoControllerId !== undefined) {
                        rpc.callRpc(erizoControllerId, 'eventReport', ['updateStream', roomId, {event: evt, id: id, data: null}]);
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
                    useHardware = (info.indexOf('VA-API version 0.35.0') != -1) ||
                                    (info.indexOf('VA-API version: 0.35') != -1);
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
                if (wrtc.getCurrentState() >= 103 && publishers[to].getPublisherState() >= 103) {
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
    initWebRtcConnection = function (wrtc, sdp, id_mixer, unmix, callback, id_pub, id_sub) {
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

          if (newStatus === 104) { // Connection Finished
            terminated = true;
          }
          if (newStatus === 102 && !sdpDelivered) { // Connection Started
            localSdp = wrtc.getLocalSdp();
            answer = getRoap(localSdp, roap);
            callback('callback', answer);
            sdpDelivered = true;

          }
          if (newStatus === 103) { // Connection Ready
            // Perform the additional work for publishers.
            if (id_mixer && (unmix !== true)) {
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
    /*options: {
        url: 'xxx',
        transport: 'tcp'/'udp',
        buffer_size: 1024*1024*2
    }*/
    that.addExternalInput = function (from, options, mixer, callback) {

        if (publishers[from] === undefined) {

            log.info('Adding external input peer_id', from);

            var ei = new addon.ExternalInput(JSON.stringify(options));
            var muxer = new addon.Gateway(JSON.stringify({externalInput: true}));

            publishers[from] = muxer;
            subscribers[from] = [];

            var initialized = false;
            ei.init(function (message) {
                if (publishers[from] === undefined) {
                    log.info('External input', from, 'removed before initialized');
                    return;
                }

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

                    initialized = true;
                    callback('onReady');
                } else if (!initialized) {
                    // Reclaim the native resource.
                    // TODO: In normal (successful) case the native resource of
                    // the ExternalInput is managed implicitly by the muxer. We
                    // may need to think about whether it would be better to
                    // have it also managed explicitly in the JS layer.
                    // Similar cases apply to the normal WebRTC publishers/subscribers.
                    ei.close();
                    publishers[from].close();
                    delete subscribers[from];
                    delete publishers[from];
                    log.error('External input', from, 'initialization failed');
                    callback('callback', message);
                } else {
                    // Some errors happen after the external input has been initialized.
                    log.warn('External input', from, 'encounters following error after initialized:', message);
                    if (erizoControllerId !== undefined) {
                        rpc.callRpc(erizoControllerId, 'eventReport', ['unpublish', roomId, {event: '', id: from, data: null}]);
                    }
                }
            });
            callback('callback', 'success'); // avoid rpc timeout
        } else {
            log.info('Publisher already set for', from);
            callback('callback', 'external input already existed');
        }
    };

    that.addExternalOutput = function (video_publisher_id, audio_publisher_id, output_id, url, interval, callback) {
        if (publishers[video_publisher_id] !== undefined && publishers[audio_publisher_id] !== undefined) {
            var config = {
                id: output_id,
                url: url,
                interval: interval
            };

            if (output_id === '' && url === '' && interval === 0) { // rtsp out
                // FIXME: Currently, ONLY mixed stream can be used for RTSP output
                // which means video_publisher_id equals to audio_publisher_id
                if (GLOBAL.config.erizo.rtsp.enabled !== true) {
                    callback('callback', 'not enabled');
                    return;
                }
                config.url = [rtsp_prefix, 'room_', video_publisher_id, '.sdp'].join('');
                config.id = config.url;
            }

            log.info('Adding ExternalOutput to ' + video_publisher_id + " and " + audio_publisher_id + " with url: " + url);

            // FIXME: Callback to the native layer is needed as well as the timeout timer
            if (externalOutputs[output_id] === undefined) {
                externalOutputs[output_id] = new addon.ExternalOutput(JSON.stringify(config));
            }

            // FIXME: Callback to the native layer is needed as well as the timeout timer
            externalOutputs[output_id].setMediaSource(publishers[video_publisher_id], publishers[audio_publisher_id]);

            callback('callback', 'success');
            return;
        }

        log.error('Failed adding ExternalOutput to ' + video_publisher_id + " and " + audio_publisher_id + ' with url ' + url);
        callback('callback', 'error');
    };

    that.removeExternalOutput = function (output_id, close, callback) {
        log.info('Stopping ExternalOutput:' + output_id);

        if (externalOutputs[output_id].unsetMediaSource(output_id, close)) {
            delete externalOutputs[output_id];
            return callback('callback', 'success');
        }

        callback('callback', 'error');
    };

    /*
     * Adds a publisher to the room. This creates a new Gateway
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the Gateway.
     */
    that.addPublisher = function (from, sdp, mixer, unmix, callback) {

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

                    initWebRtcConnection(wrtc, sdp, mixer.id, unmix, callback, from);
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
                if (!err && publishers[to] !== undefined && subscribers[to].indexOf(from) === -1) {
                    var erizoPassPhrase = obj.erizo;

                    var hasH264 = true;
                    if (!useHardware && !openh264Enabled) {
                        hasH264 = false;
                    }
                    var wrtc = new addon.WebRtcConnection(audio, (!!video), hasH264, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, GLOBAL.config.erizo.keystorePath, GLOBAL.config.erizo.keystorePath, erizoPassPhrase, true, true, true, true);

                    initWebRtcConnection(wrtc, sdp, undefined, undefined, callback, to, from);

                    subscribers[to].push(from);
                    publishers[to].addSubscriber(wrtc, from, JSON.stringify(video.resolution));
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
        if (subscribers[to] === undefined) {
            return;
        }

        var index = subscribers[to].indexOf(from);
        if (index !== -1) {
            log.info('Removing subscriber ', from, 'to muxer ', to);
            publishers[to].removeSubscriber(from);
            subscribers[to].splice(index, 1);
        }
    };

    that.subscribeStream = function subscribeStream (publisher_id, subscriber_id, isAudio, callback) {
        if (subscribers[publisher_id] !== undefined && subscribers[publisher_id].indexOf(subscriber_id) !== -1) {
            log.info('Enabling subscriber', subscriber_id, 'in', publisher_id, isAudio ? '[audio]' : '[video]');
            publishers[publisher_id].subscribeStream(subscriber_id, isAudio);
            return callback('callback');
        }
        callback('callback', 'error');
    };

    that.unsubscribeStream = function unsubscribeStream (publisher_id, subscriber_id, isAudio, callback) {
        if (subscribers[publisher_id] !== undefined && subscribers[publisher_id].indexOf(subscriber_id) !== -1) {
            log.info('Disabling subscriber', subscriber_id, 'in', publisher_id, isAudio ? '[audio]' : '[video]');
            publishers[publisher_id].unsubscribeStream(subscriber_id, isAudio);
            return callback('callback');
        }
        callback('callback', 'error');
    };

    that.publishStream = function publishStream (publisher_id, isAudio, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Enabling publisher', publisher_id, isAudio ? '[audio]' : '[video]');
            publishers[publisher_id].publishStream(publisher_id, isAudio);
            return callback('callback');
        }
        callback('callback', 'error');
    };

    that.unpublishStream = function unpublishStream (publisher_id, isAudio, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Disabling publisher', publisher_id, isAudio ? '[audio]' : '[video]');
            publishers[publisher_id].unpublishStream(publisher_id, isAudio);
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

    that.setControllerId = function (id, room_id) {
        erizoControllerId = id;
        roomId = room_id;
    };

    return that;
};
