/*global require, exports, , setInterval, clearInterval*/

var addon;
if (GLOBAL.config.erizo.mixer) {
    addon = require('./../../bindings/mcu/build/Release/addon');
} else {
    addon = require('./../../bindings/erizoAPI/build/Release/addon');
}

var logger = require('./../common/logger').logger;
var rpc = require('./../common/rpc');

// Logger
var log = logger.getLogger("ErizoJSController");

exports.ErizoJSController = function (spec) {
    "use strict";

    var that = {},
        muxer,
        // {id: array of subscribers}
        subscribers = {},
        // {id: OneToManyProcessor}
        publishers = {},

        // {id: ExternalOutput}
        externalOutputs = {},

        INTERVAL_TIME_SDP = 100,
        INTERVAL_TIME_FIR = 100,
        INTERVAL_TIME_KILL = 30*60*1000, // Timeout to kill itself after a timeout since the publisher leaves room.
        waitForFIR,
        initWebRtcConnection,
        getSdp,
        getRoap;

    that.init = function () {
        if (GLOBAL.config.erizo.mixer) {
            muxer = new addon.ManyToManyTranscoder();
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
    initWebRtcConnection = function (wrtc, sdp, callback, id_pub, id_sub) {

        if (GLOBAL.config.erizoController.sendStats) {
            wrtc.getStats(function (newStats){
                log.info("Received RTCP stats: ", newStats);
                var timeStamp = new Date();
                rpc.callRpc('stats_handler', 'stats', [{pub: id_pub, subs: id_sub, stats: JSON.parse(newStats), timestamp:timeStamp.getTime()}]);
            });
        }

        wrtc.init( function (newStatus){
          var localSdp, answer;
          log.info("webrtc Addon status" + newStatus );
          if (newStatus === 102 && !sdpDelivered) {
            localSdp = wrtc.getLocalSdp();
            answer = getRoap(localSdp, roap);
            callback('callback', answer);
            sdpDelivered = true;

          }
          if (newStatus === 103) {
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

        //var reg2 = new RegExp(/^.*offererSessionId\":(...).*$/);
        //var offererSessionId = offerRoap.match(reg2)[1];

        answer += ' \"sdp\":\"' + sdp + '\",\n';
        //answer += ' \"offererSessionId\":' + offererSessionId + ',\n';
        answer += ' ' + offererSessionId + '\n';
        answer += ' \"answererSessionId\":' + answererSessionId + ',\n \"seq\" : 1\n}\n';

        return answer;
    };

    that.addExternalInput = function (from, url, callback) {

        if (publishers[from] === undefined) {

            log.info("Adding external input peer_id ", from);

            var ei = new addon.ExternalInput(url);

            if (!GLOBAL.config.erizo.mixer) {
                muxer = new addon.OneToManyProcessor();
                publishers[from] = muxer;
            } else {
                publishers[from] = ei;
            }

            subscribers[from] = [];

            ei.setAudioReceiver(muxer);
            ei.setVideoReceiver(muxer);
            muxer.setExternalPublisher(ei);

            var answer = ei.init();

            if (answer >= 0) {
                callback('callback', 'success');
            } else {
                callback('callback', answer);
            }

        } else {
            log.info("Publisher already set for", from);
        }
    };

    that.addExternalOutput = function (to, url) {
        if (publishers[to] !== undefined) {
            log.info("Adding ExternalOutput to " + to + " url " + url);
            var externalOutput = new addon.ExternalOutput(url);
            externalOutput.init();
            if (!GLOBAL.config.erizo.mixer) {
                publishers[to].addExternalOutput(externalOutput, url);
            } else {
                muxer.addExternalOutput(externalOutput, url);
            }
            externalOutputs[url] = externalOutput;
        }
    };

    that.removeExternalOutput = function (to, url) {
      if (externalOutputs[url] !== undefined && publishers[to]!=undefined) {
        log.info("Stopping ExternalOutput: url " + url);
        if (!GLOBAL.config.erizo.mixer) {
            publishers[to].removeSubscriber(url);
        } else {
            muxer.removeSubscriber(url);
        }
        delete externalOutputs[url];
      }
    };

    /*
     * Adds a publisher to the room. This creates a new OneToManyProcessor
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the OneToManyProcessor.
     */
    that.addPublisher = function (from, sdp, callback) {

        if (publishers[from] === undefined) {

            log.info("Adding publisher peer_id ", from);

            var wrtc = new addon.WebRtcConnection(true, true, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, undefined, undefined, undefined, true, true, true, true);
            if (!GLOBAL.config.erizo.mixer) {
                muxer = new addon.OneToManyProcessor();
                muxer.setPublisher(wrtc);
                publishers[from] = muxer;
            } else {
                muxer.addPublisher(wrtc);
                publishers[from] = wrtc;
            }

            subscribers[from] = [];

            wrtc.setAudioReceiver(muxer);
            wrtc.setVideoReceiver(muxer);

            initWebRtcConnection(wrtc, sdp, callback, from);

            //log.info('Publishers: ', publishers);
            //log.info('Subscribers: ', subscribers);

        } else {
            log.info("Publisher already set for", from);
        }
    };

    /*
     * Adds a subscriber to the room. This creates a new WebRtcConnection.
     * This WebRtcConnection will be added to the subscribers list of the
     * OneToManyProcessor.
     */
    that.addSubscriber = function (from, to, audio, video, sdp, callback) {

        if (publishers[to] !== undefined && subscribers[to].indexOf(from) === -1 && sdp.match('OFFER') !== null) {

            log.info("Adding subscriber from ", from, 'to ', to, 'audio', audio, 'video', video);

            var wrtc = new addon.WebRtcConnection(audio, video, GLOBAL.config.erizo.stunserver, GLOBAL.config.erizo.stunport, GLOBAL.config.erizo.minport, GLOBAL.config.erizo.maxport, undefined, undefined, undefined, true, true, true, true);

            subscribers[to].push(from);
            if (!GLOBAL.config.erizo.mixer) {
                publishers[to].addSubscriber(wrtc, from);
            } else {
                muxer.addSubscriber(wrtc, from);
            }

            initWebRtcConnection(wrtc, sdp, callback, to, from);
//            waitForFIR(wrtc, to);

            //log.info('Publishers: ', publishers);
            //log.info('Subscribers: ', subscribers);
        }
    };

    /*
     * Removes a publisher from the room. This also deletes the associated OneToManyProcessor.
     */
    that.removePublisher = function (from) {

        if (subscribers[from] !== undefined && publishers[from] !== undefined) {
            if (!GLOBAL.config.erizo.mixer) {
                log.info('Removing muxer', from);
                publishers[from].close();
            } else {
                muxer.removePublisher(publishers[from]);
            }
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
            log.info("Publishers: ", count);
            if (count === 0)  {
                if (GLOBAL.config.erizo.mixer) {
                    log.info('Removing muxer');
                    muxer.close();
                }
                log.info('Removed all publishers. Killing process.');
                process.exit(0);
            }
        }
    };

    /*
     * Removes a subscriber from the room. This also removes it from the associated OneToManyProcessor.
     */
    that.removeSubscriber = function (from, to) {

        var index = subscribers[to].indexOf(from);
        if (index !== -1) {
            log.info('Removing subscriber ', from, 'to muxer ', to);
            if (!GLOBAL.config.erizo.mixer) {
                publishers[to].removeSubscriber(from);
            } else {
                muxer.removeSubscriber(from);
            }
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
                    if (!GLOBAL.config.erizo.mixer) {
                        publishers[key].removeSubscriber(from);
                    } else {
                        muxer.removeSubscriber(from);
                    }
                    subscribers[key].splice(index, 1);
                }
            }
        }
    };

    return that;
};
