/*global window, console, mozRTCSessionDescription, mozRTCPeerConnection*/

Erizo.FirefoxStack = function (spec) {
    "use strict";

    var that = {},
        WebkitRTCPeerConnection = mozRTCPeerConnection,
        RTCSessionDescription = mozRTCSessionDescription;

    var hasStream = false;

    that.pc_config = {
        "iceServers": []
    };

    // currently firefox does not support turn
    if (spec.iceServers instanceof Array) {
        spec.iceServers.map(function (server) {
            if (server.url.indexOf('stun:') === 0) {
                that.pc_config.iceServers.push({"url": server.url});
            }
        });
    } else {
        if (spec.stunServerUrl) {
            if (spec.stunServerUrl instanceof Array) {
                spec.stunServerUrl.map(function (url) {
                    if (typeof url === 'string' && url !== '') {
                        that.pc_config.iceServers.push({"url": url});
                    }
                });
            } else if (typeof spec.stunServerUrl === 'string' && spec.stunServerUrl !== '') {
                that.pc_config.iceServers.push({"url": spec.stunServerUrl});
            }
        }
    }

    if (spec.audio === undefined) {
        spec.audio = true;
    }

    if (spec.video === undefined) {
        spec.video = true;
    }

    that.mediaConstraints = {
            offerToReceiveAudio: spec.audio,
            offerToReceiveVideo: spec.video,
            mozDontOfferDataChannel: true
    };

    that.roapSessionId = 103;

    that.peerConnection = new WebkitRTCPeerConnection(that.pc_config);

    that.peerConnection.onicecandidate = function (event) {
        L.Logger.debug("PeerConnection: ", spec.session_id);
        if (!event.candidate) {
            // At the moment, we do not renegotiate when new candidates
            // show up after the more flag has been false once.
            L.Logger.debug("State: " + that.peerConnection.iceGatheringState);

            if (that.ices === undefined) {
                that.ices = 0;
            }
            that.ices = that.ices + 1;
            L.Logger.debug(that.ices);
            if (that.ices >= 1 && that.moreIceComing) {
                that.moreIceComing = false;
                that.markActionNeeded();
            }
        } else {
            that.iceCandidateCount += 1;
        }
    };

    L.Logger.debug("Created webkitRTCPeerConnnection with config \"" + JSON.stringify(that.pc_config) + "\".");

    /**
     * This function processes signalling messages from the other side.
     * @param {string} msgstring JSON-formatted string containing a ROAP message.
     */
    that.processSignalingMessage = function (msgstring) {
        // Offer: Check for glare and resolve.
        // Answer/OK: Remove retransmit for the msg this is an answer to.
        // Send back "OK" if this was an Answer.
        L.Logger.debug('Activity on conn ' + that.sessionId);
        var msg = JSON.parse(msgstring), sd;
        that.incomingMessage = msg;

        if (that.state === 'new') {
            if (msg.messageType === 'OFFER') {
                // Initial offer.
                sd = {
                    sdp: msg.sdp,
                    type: 'offer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug("setRemoteDescription succeeded");
                    }, function(error) {
                        L.Logger.info("setRemoteDescription failed: " + error.name);
                    });

                that.state = 'offer-received';
                // Allow other stuff to happen, then reply.
                that.markActionNeeded();
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }

        } else if (that.state === 'offer-sent') {
            if (msg.messageType === 'ANSWER') {

                msg.sdp = msg.sdp.replace(/ generation 0/g, "");
                msg.sdp = msg.sdp.replace(/ udp /g, " UDP ");

                sd = {
                    sdp: msg.sdp,
                    type: 'answer'
                };
                L.Logger.debug("Received ANSWER: ", sd.sdp);
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug("setRemoteDescription succeeded");
                    }, function(error) {
                        L.Logger.info("setRemoteDescription failed: " + error.name);
                    });
                that.sendOK();
                that.state = 'established';

            } else if (msg.messageType === 'pr-answer') {
                sd = {
                    sdp: msg.sdp,
                    type: 'pr-answer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug("setRemoteDescription succeeded");
                    }, function(error) {
                        L.Logger.info("setRemoteDescription failed: " + error.name);
                    });

                // No change to state, and no response.
            } else if (msg.messageType === 'offer') {
                // Glare processing.
                that.error('Not written yet');
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }

        } else if (that.state === 'established') {
            if (msg.messageType === 'OFFER') {
                // Subsequent offer.
                sd = {
                    sdp: msg.sdp,
                    type: 'offer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug("setRemoteDescription succeeded");
                    }, function(error) {
                        L.Logger.info("setRemoteDescription failed: " + error.name);
                    });

                that.state = 'offer-received';
                // Allow other stuff to happen, then reply.
                that.markActionNeeded();
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }
        }
    };

    /**
     * Adds a stream - this causes signalling to happen, if needed.
     * @param {MediaStream} stream The outgoing MediaStream to add.
     */
    that.addStream = function (stream) {
        hasStream = true;
        that.peerConnection.addStream(stream);
        that.markActionNeeded();
    };

    /**
     * Removes a stream.
     * @param {MediaStream} stream The MediaStream to remove.
     */
    that.removeStream = function () {
//        var i;
//        for (i = 0; i < that.peerConnection.localStreams.length; ++i) {
//            if (that.localStreams[i] === stream) {
//                that.localStreams[i] = null;
//            }
//        }
        that.markActionNeeded();
    };

    /**
     * Closes the connection.
     */
    that.close = function () {
        that.state = 'closed';
        that.peerConnection.close();
    };

    /**
     * Internal function: Mark that something happened.
     */
    that.markActionNeeded = function () {
        that.actionNeeded = true;
        that.doLater(function () {
            that.onstablestate();
        });
    };

    /**
     * Internal function: Do something later (not on this stack).
     * @param {function} what Callback to be executed later.
     */
    that.doLater = function (what) {
        // Post an event to myself so that I get called a while later.
        // (needs more JS/DOM info. Just call the processing function on a delay
        // for now.)
        window.setTimeout(what, 1);
    };

    /**
     * Internal function called when a stable state
     * is entered by the browser (to allow for multiple AddStream calls or
     * other interesting actions).
     * This function will generate an offer or answer, as needed, and send
     * to the remote party using our onsignalingmessage function.
     */
    that.onstablestate = function () {
        var mySDP;
        if (that.actionNeeded) {
            if (that.state === 'new' || that.state === 'established') {
                // See if the current offer is the same as what we already sent.
                // If not, no change is needed.   
                L.Logger.debug("Creating offer");
                var onSuccess = function() {
                    that.peerConnection.createOffer(function (sessionDescription) {

                        var newOffer = sessionDescription.sdp;

                        L.Logger.debug("Changed", sessionDescription.sdp);

                        if (newOffer !== that.prevOffer) {

                            that.peerConnection.setLocalDescription(sessionDescription);

                            that.state = 'preparing-offer';
                            that.markActionNeeded();
                            return;
                        } else {
                            L.Logger.debug('Not sending a new offer');
                        }

                    }, function(error) {
                        // Callback on error
                        L.Logger.debug("Ups! Something went wrong ", error);

                    }, that.mediaConstraints);
                };
                if (hasStream) {
                    that.mediaConstraints = undefined;

                }
                onSuccess();
                


            } else if (that.state === 'preparing-offer') {
                // Don't do anything until we have the ICE candidates.
                if (that.moreIceComing) {
                    return;
                }


                // Now able to send the offer we've already prepared.
                that.prevOffer = that.peerConnection.localDescription.sdp;
                L.Logger.debug("Sending OFFER: ", that.prevOffer);
                //L.Logger.debug('Sent SDP is ' + that.prevOffer);
                that.sendMessage('OFFER', that.prevOffer);
                // Not done: Retransmission on non-response.
                that.state = 'offer-sent';

            } else if (that.state === 'offer-received') {

                that.peerConnection.createAnswer(function (sessionDescription) {
                    that.peerConnection.setLocalDescription(sessionDescription);
                    that.state = 'offer-received-preparing-answer';

                    if (!that.iceStarted) {
                        var now = new Date();
                        L.Logger.debug(now.getTime() + ': Starting ICE in responder');
                        that.iceStarted = true;
                    } else {
                        that.markActionNeeded();
                        return;
                    }

                }, function() {
                    // Callback on error
                    L.Logger.debug("Ups! Something went wrong");

                });

            } else if (that.state === 'offer-received-preparing-answer') {
                if (that.moreIceComing) {
                    return;
                }

                mySDP = that.peerConnection.localDescription.sdp;

                that.sendMessage('ANSWER', mySDP);
                that.state = 'established';
            } else {
                that.error('Dazed and confused in state ' + that.state + ', stopping here');
            }
            that.actionNeeded = false;
        }
    };

    /**
     * Internal function to send an "OK" message.
     */
    that.sendOK = function () {
        that.sendMessage('OK');
    };

    /**
     * Internal function to send a signalling message.
     * @param {string} operation What operation to signal.
     * @param {string} sdp SDP message body.
     */
    that.sendMessage = function (operation, sdp) {
        var roapMessage = {};
        roapMessage.messageType = operation;
        roapMessage.sdp = sdp; // may be null or undefined
        if (operation === 'OFFER') {
            roapMessage.offererSessionId = that.sessionId;
            roapMessage.answererSessionId = that.otherSessionId; // may be null
            roapMessage.seq = (that.sequenceNumber += 1);
            // The tiebreaker needs to be neither 0 nor 429496725.
            roapMessage.tiebreaker = Math.floor(Math.random() * 429496723 + 1);
        } else {
            roapMessage.offererSessionId = that.incomingMessage.offererSessionId;
            roapMessage.answererSessionId = that.sessionId;
            roapMessage.seq = that.incomingMessage.seq;
        }
        that.onsignalingmessage(JSON.stringify(roapMessage));
    };

    /**
     * Internal something-bad-happened function.
     * @param {string} text What happened - suitable for logging.
     */
    that.error = function (text) {
        throw 'Error in RoapOnJsep: ' + text;
    };

    that.sessionId = (that.roapSessionId += 1);
    that.sequenceNumber = 0; // Number of last ROAP message sent. Starts at 1.
    that.actionNeeded = false;
    that.iceStarted = false;
    that.moreIceComing = true;
    that.iceCandidateCount = 0;
    that.onsignalingmessage = spec.callback;

    that.peerConnection.onopen = function () {
        if (that.onopen) {
            that.onopen();
        }
    };

    that.peerConnection.onaddstream = function (stream) {
        if (that.onaddstream) {
            that.onaddstream(stream);
        }
    };

    that.peerConnection.onremovestream = function (stream) {
        if (that.onremovestream) {
            that.onremovestream(stream);
        }
    };

    that.peerConnection.oniceconnectionstatechange = function (e) {
        if (that.oniceconnectionstatechange) {
            that.oniceconnectionstatechange(e.currentTarget.iceConnectionState);
        }
    };

    // Variables that are part of the public interface of PeerConnection
    // in the 28 January 2012 version of the webrtc specification.
    that.onaddstream = null;
    that.onremovestream = null;
    that.state = 'new';
    // Auto-fire next events.
    that.markActionNeeded();
    return that;
};
