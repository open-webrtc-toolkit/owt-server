/*global window, console, RTCSessionDescription, RTCIceCandidate, webkitRTCPeerConnection*/

Erizo.ChromeJSEPStack = function (spec) {
    "use strict";

    var that = {},
        WebkitRTCPeerConnection = webkitRTCPeerConnection;

    that.pc_config = {
        "iceServers": []
    };

    that.con = {'optional': [{'DtlsSrtpKeyAgreement': true}]};

    if (spec.stunServerUrl !== undefined) {
        that.pc_config.iceServers.push({"url": spec.stunServerUrl});
    } 

    if ((spec.turnServer || {}).url) {
        that.pc_config.iceServers.push({"username": spec.turnServer.username, "credential": spec.turnServer.password, "url": spec.turnServer.url});
    }

    if (spec.audio === undefined || spec.nop2p) {
        spec.audio = true;
    }

    if (spec.video === undefined || spec.nop2p) {
        spec.video = true;
    }

    that.mediaConstraints = {
        'mandatory': {
            'OfferToReceiveVideo': spec.video,
            'OfferToReceiveAudio': spec.audio
        }
    };

    that.peerConnection = new WebkitRTCPeerConnection(that.pc_config, that.con);
    
    var setMaxBW = function (sdp) {
        var a, r;
        if (spec.maxVideoBW) {
            a = sdp.match(/m=video.*\r\n/);
            r = a[0] + "b=AS:" + spec.maxVideoBW + "\r\n";
            sdp = sdp.replace(a[0], r);
        }

        if (spec.maxAudioBW) {
            a = sdp.match(/m=audio.*\r\n/);
            r = a[0] + "b=AS:" + spec.maxAudioBW + "\r\n";
            sdp = sdp.replace(a[0], r);
        }

        return sdp;
    };

    /**
     * Closes the connection.
     */
    that.close = function () {
        that.state = 'closed';
        that.peerConnection.close();
    };

    that.peerConnection.onicecandidate =  function (event) {
        if (event.candidate) {
            event.candidate.candidate ="a="+event.candidate.candidate;

            spec.callback({type:'candidate', candidate: event.candidate});

            // sendMessage({type: 'candidate',
            //        label: event.candidate.sdpMLineIndex,
            //        id: event.candidate.sdpMid,
            //        candidate: event.candidate.candidate});


        } else {
            console.log("End of candidates.");
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

    var localDesc;

    var setLocalDesc = function (sessionDescription) {
        sessionDescription.sdp = setMaxBW(sessionDescription.sdp);
        sessionDescription.sdp = sessionDescription.sdp.replace(/a=ice-options:google-ice\r\n/g, "");
        spec.callback(sessionDescription);
        localDesc = sessionDescription;
        //that.peerConnection.setLocalDescription(sessionDescription);
    };

    that.createOffer = function () {
        that.peerConnection.createOffer(setLocalDesc, null, that.mediaConstraints);
    };

    that.addStream = function (stream) {
        that.peerConnection.addStream(stream);
    };
    spec.candidates = [];
    spec.remoteDescriptionSet = false;

    that.processSignalingMessage = function (msg) {
        console.log("Process Signaling Message", msg);

        // if (msg.type === 'offer') {
        //     msg.sdp = setMaxBW(msg.sdp);
        //     that.peerConnection.setRemoteDescription(new RTCSessionDescription(msg));
        //     that.peerConnection.createAnswer(setLocalAndSendMessage, null, sdpConstraints);
        // } else 

        if (msg.type === 'answer') {


            // // For compatibility with only audio in Firefox Revisar
            // if (answer.match(/a=ssrc:55543/)) {
            //     answer = answer.replace(/a=sendrecv\\r\\na=mid:video/, 'a=recvonly\\r\\na=mid:video');
            //     answer = answer.split('a=ssrc:55543')[0] + '"}';
            // }

            console.log("Set remote and local description", msg.sdp);

            //msg.sdp = setMaxBW(msg.sdp);

            that.peerConnection.setLocalDescription(localDesc);
            that.peerConnection.setRemoteDescription(new RTCSessionDescription(msg));
            spec.remoteDescriptionSet = true;
            console.log("Candidates to be added: ", spec.candidates.length, spec.candidates);
            while (spec.candidates.length > 0) {
                that.peerConnection.addIceCandidate(spec.candidates.pop());
            }

        } else if (msg.type === 'candidate') {
            try {
                var obj = JSON.parse(msg.candidate);
                obj.candidate = obj.candidate.replace(/a=/g, "");
                obj.sdpMLineIndex = parseInt(obj.sdpMLineIndex, 10);
                var candidate = new RTCIceCandidate(obj);
                if (spec.remoteDescriptionSet) {
                    that.peerConnection.addIceCandidate(candidate);
                } else {
                    spec.candidates.push(candidate);
                    console.log("Candidates stored: ", spec.candidates.length, spec.candidates);
                }
            } catch(e) {
                L.Logger.error("Error parsing candidate", msg.candidate);
            }
        }
    };

    return that;
};
