/*global require, exports, , setInterval, clearInterval*/

var addon = require('./../../bindings/mcu/build/Release/addon');

var logger = require('./../common/logger').logger;
var rpc = require('./../common/rpc');
var cipher = require('../../common/cipher');

// Logger
var log = logger.getLogger("ErizoJSController");

exports.ErizoJSController = function (spec) {
    "use strict";

    var that = {},
        // {id: array of subscribers}
        subscribers = {},
        // {id: Gateway}
        publishers = {},
        // {id: MediaSourceConsumer}
        mixerProxies = {},

        // {id: ExternalOutput}
        externalOutputs = {},

        INTERVAL_TIME_SDP = 100,
        INTERVAL_TIME_FIR = 100,
        INTERVAL_TIME_KILL = 30*60*1000, // Timeout to kill itself after a timeout since the publisher leaves room.
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
                    "mixer": true,
                    "oop": oop,
                    "video": {
                        "hardware": hardwareAccelerated,
                        "avcoordinate": roomConfig.video.avCoordinated,
                        "maxinput": roomConfig.video.maxInput,
                        "bitrate": roomConfig.video.bitrate,
                        "resolution": roomConfig.video.resolution,
                        "backgroundcolor": roomConfig.video.bkColor,
                        "templates": roomConfig.video.layout
                    }
                };
                var mixer = new addon.Gateway(JSON.stringify(config));

                publishers[id] = mixer;
                subscribers[id] = [];
                callback('callback', 'success');
            };

            if (useHardware) {
                // Query the hardware capability only if we want to try it.
                require('child_process').exec('vainfo', function (err, stdout, stderr) {
                    var info = "";
                    if (err) {
                        info = err.toString();
                    } else if(stdout.length > 0) {
                        info = stdout.toString();
                    }
                    // Check whether hardware codec should be used for this room
                    useHardware = (info.indexOf('VA-API version 0.34.0') != -1)
                                   || (info.indexOf('VA-API version: 0.34') != -1);
                });
            }
            doInitMixer(useHardware);
        } else {
            log.info("Mixer already set for", id);
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
                log.info("Received RTCP stats: ", newStats);
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
          log.info("webrtc Addon status" + newStatus );

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
                publishers[id_pub].setMixer(mixer);
              } else {
                var mixerProxy = mixerProxies[id_pub];
                if (mixerProxy) {
                  publishers[id_pub].setMixer(mixerProxy);
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
            answererSessionId = "106",
            answer = ('\n{\n \"messageType\":\"ANSWER\",\n');

        sdp = sdp.replace(reg1, '\\r\\n');
        answer += ' \"sdp\":\"' + sdp + '\",\n';
        answer += ' ' + offererSessionId + '\n';
        answer += ' \"answererSessionId\":' + answererSessionId + ',\n \"seq\" : 1\n}\n';

        return answer;
    };

    that.addExternalInput = function (from, url, mixer, callback) {

        if (publishers[from] === undefined) {

            log.info("Adding external input peer_id ", from);

            var ei = new addon.ExternalInput(url);
            var answer = ei.init();
            if (answer < 0) {
                callback('callback', answer);
                return;
            }

            publishers[from] = ei;
            subscribers[from] = [];

            if (mixer.id) {
              var mixerObj = publishers[mixer.id];
              if (mixerObj) {
                mixerObj.addExternalSource(ei);
              }
            }

            callback('callback', 'success');
        } else {
            log.info("Publisher already set for", from);
        }
    };

    that.addExternalOutput = function (to, url) {
        if (publishers[to] !== undefined) {
            log.info("Adding ExternalOutput to " + to + " url " + url);
            var externalOutput = new addon.ExternalOutput(url);
            externalOutput.init();
            publishers[to].addExternalOutput(externalOutput, url);
            externalOutputs[url] = externalOutput;
        }
    };

    that.removeExternalOutput = function (to, url) {
      if (externalOutputs[url] !== undefined && publishers[to]!=undefined) {
        log.info("Stopping ExternalOutput: url " + url);
        publishers[to].removeSubscriber(url);
        delete externalOutputs[url];
      }
    };

    /*
     * Adds a publisher to the room. This creates a new Gateway
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the Gateway.
     */
    that.addPublisher = function (from, sdp, mixer, callback) {

        if (publishers[from] === undefined) {
            log.info("Adding publisher peer_id ", from);
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

                    if (mixer.id && mixer.oop)
                        mixerProxies[from] = new addon.MediaSourceConsumer();

                    initWebRtcConnection(wrtc, sdp, mixer.id, callback, from);
                    muxer.setPublisher(wrtc, from);
                } else {
                    log.warn('Failed to publish the stream:', err);
                }
            });
        } else {
            log.info("Publisher already set for", from);
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
            log.info("Adding subscriber from ", from, 'to ', to, 'audio', audio, 'video', video);
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
            publishers[from].close();
            log.info('Removing subscribers', from);
            delete subscribers[from];
            log.info('Removing publisher', from);
            delete publishers[from];
            if (mixerProxies[from] !== undefined) {
                log.info('Removing mixer proxy', from);
                mixerProxies[from].close();
                delete mixerProxies[from];
            }

            var count = 0;
            for (var k in publishers) {
                if (publishers.hasOwnProperty(k)) {
                   ++count;
                }
            }

            log.info("Publishers: ", count);
            if (count === 0) {
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

    that.startRecorder = function (mixer, url, interval, callback) {
        if (publishers[mixer] !== undefined) {
            publishers[mixer].setRecorder(url, interval);
            callback('callback', 'success');
        } else {
            callback('callback', 'error');
        }
    };

    that.stopRecorder = function (mixer, callback) {
        if (publishers[mixer] !== undefined) {
            publishers[mixer].unsetRecorder();
            callback('callback', 'success');
        } else {
            callback('callback', 'error');
        }
    };

    return that;
};
