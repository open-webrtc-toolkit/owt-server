/*global require, exports, console*/

var addon = require('./../bindings/gateway/build/Release/addon');
var config = require('./../etc/gateway_config');
var logger = require('./util/logger').logger;

exports = module.exports = function () {
    "use strict";

    var that = {},
        customParam,
        subscribers = {},
        publisher,
        publisherId,
        gateway,

        externalOutputs = {},

        initWebRtcConnection;

    that.init = function (customGatewayParam) {
        customParam = customGatewayParam;
        gateway = new addon.Gateway(customParam);
    };

    var CONN_INITIAL = 101, CONN_STARTED = 102, CONN_GATHERED = 103, CONN_READY = 104, CONN_FINISHED = 105, CONN_CANDIDATE = 201, CONN_SDP = 202, CONN_FAILED = 500;

    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP. 
     */
    initWebRtcConnection = function (wrtc, callback, onReady) {
        var terminated = false;

        wrtc.init(function (newStatus, mess) {
            if (terminated) {
                return;
            }

            logger.info("webrtc Addon status: " + newStatus + mess);

            switch (newStatus) {
            case CONN_FINISHED:
                terminated = true;
                break;

            case CONN_INITIAL:
                callback({type: 'started'});
                break;

            case CONN_SDP:
            case CONN_GATHERED:
                logger.debug('Sending SDP', mess);
                mess = mess.replace(that.privateRegexp, that.publicIP);
                callback({type: 'answer', sdp: mess});
                break;

            case CONN_CANDIDATE:
                mess = mess.replace(that.privateRegexp, that.publicIP);
                callback({type: 'candidate', candidate: mess});
                break;

            case CONN_FAILED:
                callback({type: 'failed', sdp: mess});
                break;

            case CONN_READY:
                if (typeof onReady === 'function') onReady();
                break;

            default:
                break;
            }
        });

        callback({type: 'initializing'});
    };

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

    that.processSignaling = function (streamId, peerId, msg) {
        logger.info("Process Signaling message: ", streamId, peerId, msg);
        if (publisherId === streamId) {
            if (msg.type === 'offer') {
                publisher.setRemoteSdp(msg.sdp);
                publisher.start();
            } else if (msg.type === 'candidate') {
                publisher.addRemoteCandidate(msg.candidate.sdpMid, msg.candidate.sdpMLineIndex, msg.candidate.candidate);
            }
        } else if (subscribers[streamId]) {
            if (msg.type === 'offer') {
                subscribers[streamId].setRemoteSdp(msg.sdp);
                subscribers[streamId].start();
            } else if (msg.type === 'candidate') {
                subscribers[streamId].addRemoteCandidate(msg.candidate.sdpMid, msg.candidate.sdpMLineIndex, msg.candidate.candidate);
            }
        }
    };

    /*
     * Adds a publisher to the Gateway. This creates a new WebRtcConnection.
     * This WebRtcConnection will be the WebRTC publisher of the Gateway.
     */
    that.addPublisher = function (from, callback, resolution) {
        if (publisher !== undefined) {
            logger.info("Publisher already set");
            return;
        }

        if (gateway === undefined) {
            logger.info("Gateway doesn't exist");
            return;
        }

        logger.info("Adding publisher peer_id ", from);

        var wrtc = new addon.WebRtcConnection(true, true, false, config.core.stunserver, config.core.stunport, config.core.minport, config.core.maxport, config.core.certificate.cert, config.core.certificate.key, config.core.certificate.passphrase, config.core.red, config.core.fec, config.core.nack_sender, config.core.nack_receiver, false);

        if (typeof resolution !== 'string') {
            resolution = '';
        }

        if (gateway.addPublisher(wrtc, from, resolution)) {
            var overridenOnReady = function () {
                gateway.publishStream(from, true);
                gateway.publishStream(from, false);
                callback({type: 'ready'});
            };
            publisher = wrtc;
            publisherId = from;
            initWebRtcConnection(wrtc, callback, overridenOnReady);
        } else {
            logger.info("Failed to publish ", from);
            // TODO: We should notify the browser if there's anything wrong with its stream publishing
        }
    };

    /*
     * Adds a subscriber to the Gateway. This creates a new WebRtcConnection.
     * This WebRtcConnection will be added to the subscribers list of the Gateway.
     */
    that.addSubscriber = function (from, to, audio, video, callback) {
        if (gateway !== undefined && subscribers[to] === undefined) {
            logger.info("Adding subscriber from ", from, 'to ', to);

            var wrtc = new addon.WebRtcConnection(audio, video, false, config.core.stunserver, config.core.stunport, config.core.minport, config.core.maxport, config.core.certificate.cert, config.core.certificate.key, config.core.certificate.passphrase, config.core.red, config.core.fec, config.core.nack_sender, config.core.nack_receiver, false);

            subscribers[to] = wrtc;
            gateway.addSubscriber(wrtc, to);

            var onReady = function () {
                // Automatically subscribe audio if any.
                gateway.subscribeStream(to, true);
                // Automatically subscribe video if any.
                gateway.subscribeStream(to, false);
                callback({type: 'ready'});
            };

            initWebRtcConnection(wrtc, callback, onReady);
        }
    };

    /*
     * Removes the publisher from the associated Gateway.
     */
    that.removePublisher = function (from) {
        if (gateway !== undefined && publisher !== undefined) {
            logger.info('Removing publisher ', from);
            publisher.close();
            gateway.removePublisher(from);
        }
        publisher = undefined;
    };

    /*
     * Removes a subscriber from the associated Gateway.
     */
    that.removeSubscriber = function (from, to) {
        if (gateway !== undefined) {
            if (subscribers[to]) {
                logger.info('Removing subscriber ', from, 'to ', to);
                subscribers[to].close()
                gateway.removeSubscriber(to);
                delete subscribers[to];
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
            for (var key in subscribers) {
                if (subscribers.hasOwnProperty(key)){
                    logger.info("Iterating and closing ", key, subscribers, subscribers[key]);
                    subscribers[key].close();
                }
            }
            if (publisher) {
                publisher.close();
            }
            gateway.close();
            subscribers = {};
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
            var statsStr = gateway.retrieveStatistics();
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
            gateway.publishStream(publisherId, false);
        }
    };

    that['audio-out-on'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.publishStream(publisherId, true);
        }
    };

    that['video-out-off'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unpublishStream(publisherId, false);
        }
    };

    that['audio-out-off'] = function () {
        if (typeof gateway === 'object' && gateway !== null) {
            gateway.unpublishStream(publisherId, true);
        }
    };

    return that;
};
