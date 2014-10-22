/*global io, console*/
/*
 * Class Client represents a Licode Client. It will handle the connection, local stream publication and
 * remote stream subscription.
 * Typical Client initialization would be:
 * var client = Erizo.Client({
 *  username: 'foo',
 *  role: 'bar',
 *  host: '127.0.0.1:8080',
 *  type: 'oovoo'
 * });
 */

Erizo.Client = function (spec) {
    "use strict";

    var that = Erizo.EventDispatcher(spec),
        connectSocket,
        sendMessageSocket,
        sendSDPSocket,
        removeStream,
        DISCONNECTED = 0,
        CONNECTING = 1,
        CONNECTED = 2;

    that.remoteStreams = {};
    that.localStreams = {};
    that.socket = undefined;
    that.state = DISCONNECTED;
    that.ClientId = '';

    that.addEventListener("client-disconnected", function () {
        var index, stream, evt2;
        that.state = DISCONNECTED;
        that.ClientId = '';

        // Remove all streams
        for (index in that.remoteStreams) {
            if (that.remoteStreams.hasOwnProperty(index)) {
                stream = that.remoteStreams[index];
                removeStream(stream);
                delete that.remoteStreams[index];
                evt2 = Erizo.StreamEvent({type: 'stream-removed', stream: stream});
                that.dispatchEvent(evt2);
            }
        }
        that.remoteStreams = {};

        // Close Peer Connections
        for (index in that.localStreams) {
            if (that.localStreams.hasOwnProperty(index)) {
                stream = that.localStreams[index];
                if (stream.pc && typeof stream.pc.close === 'function') {
                    stream.pc.close();
                }
                delete that.localStreams[index];
            }
        }

        // Close socket
        try {
            that.socket.disconnect();
        } catch (error) {
            L.Logger.debug("Socket already disconnected");
        }
    });

    // Private functions

    // It removes the stream from HTML and close the PeerConnection associated
    removeStream = function (stream) {
        if (stream.stream !== undefined) {
            stream.onRemove();
            // Close PC stream
            stream.pc.close();
            if (stream.local) {
                stream.stream.stop();
            }
        }
    };

    function dispatchEventAfterSubscribed (event) {
        var remote_stream = event.stream;
        if (remote_stream.pc && typeof remote_stream.signalOnPlayAudio === 'function') {
            that.dispatchEvent(event);
        } else if (remote_stream.pc && remote_stream.pc.state !== 'closed') {
            setTimeout(function () {
                dispatchEventAfterSubscribed(event);
            }, 20);
        } else {
            L.Logger.warning('event missed:', event);
        }
    }

    // It connects to the server through socket.io
    connectSocket = function (token, callback, error) {
        // Once we have connected

        var isSecured = token.secure === true;
        var host = token.host;
        if (isSecured) {
            delete io.sockets['https://'+host];
        } else {
            delete io.sockets['http://'+host];
        }

        // This handles reconnection.
        // If we try to re-initialize `that.socket' each time when connecting,
        // reconnecting after disconnected from gateway could not result in success.
        // However, we could simply reuse previous created `that.socket' to re-establish
        // the connection between client and gateway, by calling `that.socket.socket.connect()'.
        // And we don't delete `that.socket' when `client-disconnect' event triggered.
        if (that.socket !== undefined) {
            that.socket.socket.connect();
        } else {
            that.socket = io.connect(host, {reconnect: false, secure: isSecured});

            // We receive an event with a new stream.
            // type can be "media" or "data"
            that.socket.on('onAddStream', function (arg) {
                var stream = Erizo.Stream({streamID: arg.id, local: false, audio: arg.audio, video: arg.video, screen: arg.screen, attributes: arg.attributes}),
                    evt;
                that.remoteStreams[arg.id] = stream;
                evt = Erizo.StreamEvent({type: 'stream-added', stream: stream});
                that.dispatchEvent(evt);
            });

            // We receive an event of a stream removed
            that.socket.on('onRemoveStream', function (arg) {
                var stream = that.remoteStreams[arg.id],
                    evt;
                delete that.remoteStreams[arg.id];
                removeStream(stream);
                evt = Erizo.StreamEvent({type: 'stream-removed', stream: stream});
                that.dispatchEvent(evt);
            });

            // We receive an event of local stream published
            that.socket.on('onPublishStream', function (arg) {
                L.Logger.info('Stream published');
                var stream = that.localStreams[arg.id];
                var evt = Erizo.StreamEvent({type: 'stream-published', stream: stream});
                that.dispatchEvent(evt);
            });

            // We receive an event of remote video stream paused
            that.socket.on('onVideoHold', function (arg) {
                var stream = that.remoteStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'video-hold', stream: stream});
                    dispatchEventAfterSubscribed(evt);
                } else {
                    L.Logger.debug('Stream does not exist:', arg.id);
                }
            });

            // We receive an event of remote video stream resumed
            that.socket.on('onVideoReady', function (arg) {
                var stream = that.remoteStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'video-ready', stream: stream});
                    dispatchEventAfterSubscribed(evt);
                }
            });

            // We receive an event of remote audio stream paused
            that.socket.on('onAudioHold', function (arg) {
                var stream = that.remoteStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'audio-hold', stream: stream});
                    dispatchEventAfterSubscribed(evt);
                } else {
                    L.Logger.debug('Stream does not exist:', arg.id);
                }
            });

            // We receive an event of remote audio stream resumed
            that.socket.on('onAudioReady', function (arg) {
                var stream = that.remoteStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'audio-ready', stream: stream});
                    dispatchEventAfterSubscribed(evt);
                }
            });

            // We receive an event of all the remote audio streams paused
            that.socket.on('onAllAudioHold', function () {
                for (var index in that.remoteStreams) {
                    if (that.remoteStreams.hasOwnProperty(index)) {
                        var stream = that.remoteStreams[index];
                        var evt = Erizo.StreamEvent({type: 'audio-hold', stream: stream});
                        dispatchEventAfterSubscribed(evt);
                    }
                }
            });

            // We receive an event of all the remote audio streams resumed
            that.socket.on('onAllAudioReady', function () {
                for (var index in that.remoteStreams) {
                    if (that.remoteStreams.hasOwnProperty(index)) {
                        var stream = that.remoteStreams[index];
                        var evt = Erizo.StreamEvent({type: 'audio-ready', stream: stream});
                        dispatchEventAfterSubscribed(evt);
                    }
                }
            });

            that.socket.on('onVideoOn', function (arg) {
                var stream = that.localStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'video-on', stream: stream});
                    that.dispatchEvent(evt);
                }
            });

            that.socket.on('onVideoOff', function (arg) {
                var stream = that.localStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'video-off', stream: stream});
                    that.dispatchEvent(evt);
                }
            });

            that.socket.on('onAudioOn', function (arg) {
                var stream = that.localStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'audio-on', stream: stream});
                    that.dispatchEvent(evt);
                }
            });

            that.socket.on('onAudioOff', function (arg) {
                var stream = that.localStreams[arg.id];
                if (stream) {
                    var evt = Erizo.StreamEvent({type: 'audio-off', stream: stream});
                    that.dispatchEvent(evt);
                }
            });

            // The socket has disconnected
            that.socket.on('disconnect', function () {
                L.Logger.info("Socket disconnected");
                if (that.state !== DISCONNECTED) {
                    var disconnectEvt = Erizo.ClientEvent({type: "client-disconnected"});
                    that.dispatchEvent(disconnectEvt);
                }
            });

            that.socket.on('onClientJoin', function (arg) {
                var evt = Erizo.ClientEvent({type: 'client-joined', attr: arg.attr, user: arg.user});
                that.dispatchEvent(evt);
            });

            that.socket.on('onClientLeave', function (arg) {
                var evt = Erizo.ClientEvent({type: 'client-left', attr: arg.attr, user: arg.user});
                that.dispatchEvent(evt);
            });

            that.socket.on('onClientId', function (arg) {
                that.ClientId = arg;
                var evt = Erizo.ClientEvent({type: 'client-id', attr: arg});
                that.dispatchEvent(evt);
            });

            that.socket.on('onCustomMessage', function (arg) {
                var evt = Erizo.ClientEvent({type: 'message-received', attr: arg.msg});
                that.dispatchEvent(evt);
            });

            that.socket.on('connect_failed', function(e) {
                error(e || 'connection_failed');
            });

            that.socket.on('error', function(e) {
                error(e || 'connection_error');
            });
        }

        // First message with the token
        sendMessageSocket('token', token, callback, error);
    };

    // Function to send a message to the server using socket.io
    sendMessageSocket = function (type, msg, callback, error) {
        if (that.socket === undefined) {
            safeCall(error, 'socket error');
            return;
        }
        try {
            that.socket.emit(type, msg, function (respType, msg) {
                if (respType === "success") {
                    if (typeof callback === 'function') {
                        callback(msg);
                    }
                } else {
                    if (typeof error === 'function') {
                        error(msg);
                    }
                }
            });
        } catch (err) {
            L.Logger.error("Error in sendMessageSocket():", err);
        }
    };

    // It sends a SDP message to the server using socket.io
    sendSDPSocket = function (type, options, sdp, callback) {
        if (that.socket === undefined) {
            L.Logger.error('Error in sendSDPSocket(): socket not ready');
            return;
        }
        try {
            that.socket.emit(type, options, sdp, function (response, respCallback) {
                if (callback !== undefined) {
                    callback(response, respCallback);
                }
            });
        } catch (err) {
            L.Logger.error("Error in sendSDPSocket():", err);
        }
    };

    function safeCall () {
        var callback = arguments[0];
        if (typeof callback === 'function') {
            var args = Array.prototype.slice.call(arguments, 1);
            callback.apply(null, args);
        }
    }

    function mkCtrlPayload(action, id) {
        return {
            "type": "control",
            "payload": {
                "action": action,
                "streamId": id
            }
        };
    }

    // Public functions

    that.connect = function (onFailure) {

        if (that.state !== DISCONNECTED) {
            L.Logger.error("Gateway already connecting/connected");
            safeCall(onFailure, that.state);
            return;
        }
        var token = spec;
        // 1- Connect to Erizo-Controller
        that.state = CONNECTING;
        connectSocket(token, function (response) {
            that.stunServerUrl = response.stunServerUrl;
            that.turnServer = response.turnServer;
            that.state = CONNECTED;
            spec.defaultVideoBW = response.defaultVideoBW;
            spec.maxVideoBW = response.maxVideoBW;
            L.Logger.info("Gateway connected.");

            var connectEvt = Erizo.ClientEvent({type: "client-connected"});
            that.dispatchEvent(connectEvt);
        }, function (error) {
            L.Logger.error("Not Connected! Error: " + error);
            safeCall(onFailure, error);
        });
    };

    // It disconnects from the gateway, dispatching a new ClientEvent("client-disconnected")
    that.disconnect = function () {
        var disconnectEvt = Erizo.ClientEvent({type: "client-disconnected"});
        that.dispatchEvent(disconnectEvt);
    };

    // It publishes the stream provided as argument. Once it is added it throws a
    // StreamEvent("stream-added").
    that.publish = function (stream, onFailure) {
        if (typeof stream !== 'object' || stream === null) {
            safeCall(onFailure, 'stream is invalid');
            return;
        }
        if ((typeof stream.stream !== 'object' || stream.stream === null) && stream.url === undefined) {
            safeCall(onFailure, 'stream access not ready');
            return;
        }
        var options = stream.getAttributes() || {};
        options.maxVideoBW = options.maxVideoBW || spec.defaultVideoBW;
        if (options.maxVideoBW > spec.maxVideoBW) {
            options.maxVideoBW = spec.maxVideoBW;
        }

        // 1- If the stream is not local we do nothing.
        if (stream.local && that.localStreams[stream.getId()] === undefined) {

            // 2- Publish Media Stream to Erizo-Controller
            if (stream.hasAudio() || stream.hasVideo() || stream.hasScreen()) {
                if (stream.url !== undefined) {
                    sendSDPSocket('publish', {state: 'url', audio: stream.hasAudio(), video: stream.hasVideo(), attributes: stream.getAttributes()}, stream.url, function (answer, id) {

                        if (answer === 'success') {
                            L.Logger.info('Stream published');
                            stream.getId = function () {
                                return id;
                            };
                            that.localStreams[id] = stream;
                            stream.onClose = function() {
                                that.unpublish(stream);
                            };
                        } else {
                            L.Logger.info('Error when publishing the stream', answer);
                            // Unauth -1052488119
                            // Network -5
                            safeCall(onFailure, answer);
                        }
                    });

                } else {
                    stream.pc = Erizo.Connection({callback: function (offer) {
                        sendSDPSocket('publish', {state: 'offer', audio: stream.hasAudio(), video: stream.hasVideo(), attributes: stream.getAttributes()}, offer, function (answer, info) {
                            if (answer === 'error') {
                                safeCall(onFailure, answer);
                                return;
                            }
                            stream.pc.onsignalingmessage = function (ok) {
                                stream.pc.onsignalingmessage = function () {};
                                sendSDPSocket('publish', {state: 'ok', audio: stream.hasAudio(), video: stream.hasVideo(), screen: stream.hasScreen(), attributes: stream.getAttributes()}, ok);
                                stream.getId = function () {
                                    return info.id;
                                };
                                that.localStreams[info.id] = stream;
                                stream.onClose = function() {
                                    that.unpublish(stream);
                                };
                                stream.signalOnPlayAudio = function (onSuccess, onFailure) {
                                    that.send(mkCtrlPayload('audio-out-on'), onSuccess, onFailure);
                                };
                                stream.signalOnPauseAudio = function (onSuccess, onFailure) {
                                    that.send(mkCtrlPayload('audio-out-off'), onSuccess, onFailure);
                                };
                                stream.signalOnPlayVideo = function (onSuccess, onFailure) {
                                    that.send(mkCtrlPayload('video-out-on'), onSuccess, onFailure);
                                };
                                stream.signalOnPauseVideo = function (onSuccess, onFailure) {
                                    that.send(mkCtrlPayload('video-out-off'), onSuccess, onFailure);
                                };
                            };

                            stream.pc.oniceconnectionstatechange = function (state) {
                                if (state === 'failed') {
                                    sendMessageSocket('unpublish', stream.getId(), function(){}, function(){});
                                    stream.pc.close();
                                    stream.pc = undefined;
                                    delete that.localStreams[stream.getId()];
                                    stream.getId = function () { return null; };
                                    stream.onClose = undefined;
                                    stream.signalOnPlayAudio = undefined;
                                    stream.signalOnPauseAudio = undefined;
                                    stream.signalOnPlayVideo = undefined;
                                    stream.signalOnPauseVideo = undefined;
                                    safeCall(onFailure, 'peer connection failed');
                                }
                            };

                            stream.pc.processSignalingMessage(answer);
                        });
                    }, iceServers: that.userSetIceServers, stunServerUrl: that.stunServerUrl, turnServer: that.turnServer, maxAudioBW: options.maxAudioBW, maxVideoBW: options.maxVideoBW});

                    stream.pc.addStream(stream.stream);
                }
            }
        }
    };

    // It unpublishes the local stream, dispatching a StreamEvent("stream-removed")
    that.unpublish = function (stream, onFailure) {
        if (typeof stream !== 'object' || stream === null) {
            safeCall(onFailure, 'stream is invalid');
            return;
        }
        // Unpublish stream from Erizo-Controller
        if (stream.local && that.localStreams[stream.getId()] !== undefined) {
            // Media stream
            sendMessageSocket('unpublish', stream.getId(), function () {
                if ((stream.hasAudio() || stream.hasVideo() || stream.hasScreen()) && stream.url === undefined) {
                    stream.pc.close();
                    stream.pc = undefined;
                }
                delete that.localStreams[stream.getId()];
                stream.getId = function () { return null; };
                stream.onClose = undefined;
                stream.signalOnPlayAudio = undefined;
                stream.signalOnPauseAudio = undefined;
                stream.signalOnPlayVideo = undefined;
                stream.signalOnPauseVideo = undefined;
            }, onFailure);
        } else {
            safeCall(onFailure, 'stream is not local');
        }
    };

    // It subscribe to a remote stream and draws it inside the HTML tag given by the ID='elementID'
    that.subscribe = function (stream, onFailure) {
        if (typeof stream !== 'object' || stream === null) {
            safeCall(onFailure, 'stream is invalid');
            return;
        }
        if (!stream.local) {
            if (stream.hasVideo() || stream.hasAudio() || stream.hasScreen()) { // TODO: screen
                stream.pc = Erizo.Connection({callback: function (offer) {
                    sendSDPSocket('subscribe', {streamId: stream.getId(), audio: stream.hasAudio(), video: stream.hasVideo()}, offer, function (answer) {
                        if (answer === 'error') {
                            safeCall(onFailure, answer);
                            return;
                        }
/*
                        // For compatibility with only audio in Firefox
                        if (answer.match(/a=ssrc:55543/)) {
                            answer = answer.replace(/a=sendrecv\\r\\na=mid:video/, 'a=recvonly\\r\\na=mid:video');
                            answer = answer.split('a=ssrc:55543')[0] + '"}';
                        }
*/                            
                        stream.pc.processSignalingMessage(answer);
                    });
                }, nop2p: true, audio: stream.hasAudio(), video: stream.hasVideo(), iceServers: that.userSetIceServers, stunServerUrl: that.stunServerUrl, turnServer: that.turnServer});

                stream.pc.onaddstream = function (evt) {
                    // Draw on html
                    L.Logger.info('Stream subscribed');
                    stream.stream = evt.stream;
                    var evt2 = Erizo.StreamEvent({type: 'stream-subscribed', stream: stream});
                    that.dispatchEvent(evt2);
                    stream.signalOnPlayAudio = function (onSuccess, onFailure) {
                        that.send(mkCtrlPayload('audio-in-on', stream.getId()), onSuccess, onFailure);
                    };
                    stream.signalOnPauseAudio = function (onSuccess, onFailure) {
                        that.send(mkCtrlPayload('audio-in-off', stream.getId()), onSuccess, onFailure);
                    };
                    stream.signalOnPlayVideo = function (onSuccess, onFailure) {
                        that.send(mkCtrlPayload('video-in-on', stream.getId()), onSuccess, onFailure);
                    };
                    stream.signalOnPauseVideo = function (onSuccess, onFailure) {
                        that.send(mkCtrlPayload('video-in-off', stream.getId()), onSuccess, onFailure);
                    };
                };

                stream.pc.oniceconnectionstatechange = function (state) {
                    if (state === 'failed') {
                        sendMessageSocket('unsubscribe', stream.getId(), function(){}, function(){});
                        removeStream(stream);
                        stream.signalOnPlayAudio = undefined;
                        stream.signalOnPauseAudio = undefined;
                        stream.signalOnPlayVideo = undefined;
                        stream.signalOnPauseVideo = undefined;
                        safeCall(onFailure, 'peer connection failed');
                    }
                };

                L.Logger.info("Subscribing to: " + stream.getId());
            } else {
                safeCall(onFailure, 'stream does not has video/audio/screen');
            }
        } else {
            safeCall(onFailure, 'stream is local');
        }
    };

    // It unsubscribes from the stream, removing the HTML element.
    that.unsubscribe = function (stream, onFailure) {
        if (typeof stream !== 'object' || stream === null) {
            safeCall(onFailure, 'stream is invalid');
            return;
        }
        // Unsubscribe from stream stream
        if (that.socket !== undefined) {
            if (!stream.local) {
                sendMessageSocket('unsubscribe', stream.getId(), function (answer) {
                    if (answer === 'error') {
                        safeCall(onFailure, answer);
                        return;
                    }
                    removeStream(stream);
                    stream.signalOnPlayAudio = undefined;
                    stream.signalOnPauseAudio = undefined;
                    stream.signalOnPlayVideo = undefined;
                    stream.signalOnPauseVideo = undefined;
                }, function (err) {
                    L.Logger.error("Error calling unsubscribe.");
                    safeCall(onFailure, err);
                });
            }
        } else {
            safeCall(onFailure, 'socket error');
        }
    };

    that.onMessage = function (callback) {
        if (typeof callback !== 'function') {
            L.Logger.error('onMessage:', 'callback is not a function');
            return;
        }
        that.on('message-received', callback);
    };

    that.send = function(msg, onSuccess, onFailure) {
        sendMessageSocket("customMessage", msg, onSuccess, onFailure);
    };

    // turn: {"username": username, "credential": password, "url": url}
    // stun: {"url": url}
    that.setIceServers = function() {
        that.userSetIceServers = [];
        var args = Array.prototype.slice.call(arguments, 0);
        args.map(function (arg) {
            if (arg instanceof Array) {
                arg.map(function (server) {
                    if (typeof server === 'object' && server !== null && typeof server.url === 'string' && server.url !== '') {
                        that.userSetIceServers.push(server);
                    } else if (typeof server === 'string' && server !== '') {
                        that.userSetIceServers.push({"url": server});
                    }
                });
            } else if (typeof arg === 'object' && arg !== null && typeof arg.url === 'string' && arg.url !== '') {
                that.userSetIceServers.push(arg);
            } else if (typeof arg === 'string' && arg !== '') {
                that.userSetIceServers.push({"url": arg});
            }
        });
        return that.userSetIceServers;
    };

    //It searchs the streams that have "name" attribute with "value" value
    that.getStreamsByAttribute = function (name, value) {

        var streams = [], index, stream;

        for (index in that.remoteStreams) {
            if (that.remoteStreams.hasOwnProperty(index)) {
                stream = that.remoteStreams[index];

                if (stream.getAttributes() !== undefined && stream.getAttributes()[name] !== undefined) {
                    if (stream.getAttributes()[name] === value) {
                        streams.push(stream);
                    }
                }
            }
        }

        return streams;
    };

    return that;
};
