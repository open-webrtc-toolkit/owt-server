/*global require, exports, console*/

var addon = require('./../bindings/gateway/build/Release/addon');
var config = require('./../etc/gateway_config');
var logger = require('./util/logger').logger;

exports = module.exports = function () {
    "use strict";

    var that = {},
        customParam,
        subscribers = [],
        publisher,
        gateway,

        externalOutputs = {},

        INTERVAL_TIME_SDP = 100,
        initWebRtcConnection,
        getSdp,
        getRoap;


    that.init = function (customGatewayParam) {
        customParam = customGatewayParam;
        gateway = new addon.Gateway(customParam);
    };

    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP. 
     */
    initWebRtcConnection = function (wrtc, sdp, callback, onReady) {
        wrtc.init(function (newStatus) {
            var localSdp, answer;
            logger.info("webrtc Addon status: " + newStatus);
            if (newStatus === 102 && !sdpDelivered) {
                localSdp = wrtc.getLocalSdp();
                answer = getRoap(localSdp, roap);
                callback(answer);
                sdpDelivered = true;
            }
            if (newStatus === 103) {
                if (onReady != undefined) {
                    onReady();
                }
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
        var reg = new RegExp(/\\r/g);
        var string = roap.replace(reg, '');
        var jsonObj = JSON.parse(string);
        return jsonObj.sdp;
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

    that.clientJoin = function (uri) {
        logger.info('Connecting to', uri);

        if (gateway === undefined) {
            gateway = new addon.Gateway(customParam);
        }

        if (!gateway.clientJoin(uri)) {
            gateway.close();
            gateway = undefined;
        }
    }

    that.addExternalInput = function (from, url, callback) { // TODO: change gateway creation
        if (publisher === undefined) {
            logger.info("Adding external input peer_id ", from);

            if (gateway === undefined) {
                gateway = new addon.Gateway(customParam);
            }

            var ei = new addon.ExternalInput(url);
            if (gateway.setExternalPublisher(ei)) {
                var answer = ei.init();
                if (answer >= 0) {
                    callback('success');
                } else {
                    callback(answer);
                }
                publisher = ei;
            }
        } else {
            logger.info("Publisher already set");
        }
    };

    that.addExternalOutput = function (to, url) {
        if (gateway !== undefined) {
            logger.info("Adding ExternalOutput to " + to + " url " + url);
            var externalOutput = new addon.ExternalOutput(url);
            externalOutput.init();
            gateway.addExternalOutput(externalOutput, to);
            externalOutputs[url] = externalOutput;
        }

    };

    that.removeExternalOutput = function (to, url) {
        if (externalOutputs[url] !== undefined && gateway != undefined) {
            logger.info("Stopping ExternalOutput: url " + url);
            gateway.removeSubscriber(to);
            delete externalOutputs[url];
        }
    };

    /*
     * Adds a publisher to the Gateway. This creates a new WebRtcConnection.
     * This WebRtcConnection will be the WebRTC publisher of the Gateway.
     */
    that.addPublisher = function (from, sdp, callback, onReady, resolution) {
        if (publisher !== undefined) {
            logger.info("Publisher already set");
            return;
        }

        if (gateway === undefined) {
            logger.info("Gateway doesn't exist");
            return;
        }

        logger.info("Adding publisher peer_id ", from);
        var wrtc = new addon.WebRtcConnection(true, true, config.core.stunserver, config.core.stunport, config.core.minport, config.core.maxport, config.core.certificate.cert, config.core.certificate.key, config.core.certificate.passphrase, config.core.red, config.core.fec, config.core.nack_sender, config.core.nack_receiver);

        if (typeof resolution !== 'string') {
            resolution = '';
        }

        if (gateway.setPublisher(wrtc, resolution)) {
            publisher = wrtc;
            initWebRtcConnection(wrtc, sdp, callback, onReady);
        } else {
            logger.info("Failed to publish ", from);
            // TODO: We should notify the browser if there's anything wrong with its stream publishing
        }
    };

    /*
     * Adds a subscriber to the Gateway. This creates a new WebRtcConnection.
     * This WebRtcConnection will be added to the subscribers list of the Gateway.
     */
    that.addSubscriber = function (from, to, audio, video, sdp, callback) {
        if (gateway !== undefined && subscribers.indexOf(to) === -1 && sdp.match('OFFER') !== null) {
            logger.info("Adding subscriber from ", from, 'to ', to);

            if (audio === undefined) audio = true;
            if (video === undefined) video = true;

            var wrtc = new addon.WebRtcConnection(true, true, config.core.stunserver, config.core.stunport, config.core.minport, config.core.maxport, config.core.certificate.cert, config.core.certificate.key, config.core.certificate.passphrase, config.core.red, config.core.fec, config.core.nack_sender, config.core.nack_receiver);

            subscribers.push(to);
            gateway.addSubscriber(wrtc, to);

            var onReady = function () {
                // Automatically subscribe audio if any.
                gateway.subscribeStream(to, true);
                // Automatically subscribe video if any.
                gateway.subscribeStream(to, false);
            };

            initWebRtcConnection(wrtc, sdp, callback, onReady);
        }
    };

    /*
     * Removes the publisher from the associated Gateway.
     */
    that.removePublisher = function (from) {
        if (gateway !== undefined && publisher !== undefined) {
            logger.info('Removing publisher ', from);
            gateway.unsetPublisher();
        }
        publisher = undefined;
    };

    /*
     * Removes a subscriber from the associated Gateway.
     */
    that.removeSubscriber = function (from, to) {
        if (gateway !== undefined) {
            var index = subscribers.indexOf(to);
            if (index !== -1) {
                logger.info('Removing subscriber ', from, 'to ', to);
                gateway.removeSubscriber(to);
                subscribers.splice(index, 1);
            }
        }
    };

    /*
     * Removes a client from the session. This removes the publisher and all the subscribers related.
     */
    that.removeClient = function (from) {
        logger.info('Removing client ', from);
        if (gateway !== undefined) {
            logger.info('Removing gateway ', from);
            gateway.close();
            subscribers = [];
            publisher = undefined;
            gateway = undefined;
        }
    };

    that.addEventListener = function (event, callback) {
        if (typeof gateway === 'object' && gateway !== null) {
            logger.info('Add event listener for', event);
            gateway.addEventListener(event, callback);
        } else {
            logger.info('listener not add:', event);
        }
    };

    that.customMessage = function (message) {
        if (typeof gateway === 'object' && gateway !== null) {
            if (typeof message === 'object' && message !== null) {
                message = JSON.stringify(message);
            }
            if (typeof message === 'string') {
                gateway.customMessage(message);
            }
        }
    };

    that.retrieveGatewayStatistics = function () {
        if (gateway !== undefined) {
            var statsStr = gateway.retrieveGatewayStatistics();
            if (statsStr !== "") {
                var stats = JSON.parse(statsStr);
                logger.debug('Gateway statistics: packets received ', stats.packetsReceived, "packets lost ", stats.packetsLost, "average RTT ", stats.averageRTT);
                return stats;
            }
        }
        return undefined;
    };

    that['video-in-on'] = function (peerId) {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.subscribeStream(peerId, false);
        }
    };

    that['audio-in-on'] = function (peerId) {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.subscribeStream(peerId, true);
        }
    };

    that['video-in-off'] = function (peerId) {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unsubscribeStream(peerId, false);
        }
    };

    that['audio-in-off'] = function (peerId) {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unsubscribeStream(peerId, true);
        }
    };

    that['video-out-on'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.publishStream(false);
        }
    };

    that['audio-out-on'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.publishStream(true);
        }
    };

    that['video-out-off'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unpublishStream(false);
        }
    };

    that['audio-out-off'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unpublishStream(true);
        }
    };

    return that;
};
