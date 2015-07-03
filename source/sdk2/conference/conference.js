/* global io, console */
Woogeen.ConferenceClient = (function () {
  'use strict';

  function safeCall () {
    var callback = arguments[0];
    if (typeof callback === 'function') {
      var args = Array.prototype.slice.call(arguments, 1);
      callback.apply(null, args);
    }
  }

  Woogeen.sessionId = 103;

  function createChannel (spec) {
    spec.session_id = (Woogeen.sessionId += 1);
    var that;
    if (window.navigator.userAgent.match('Firefox') !== null) {
      // Firefox
      that = Erizo.FirefoxStack(spec);
      that.browser = 'mozilla';
    } else if (window.navigator.appVersion.indexOf("Trident") > -1) {
      that = Erizo.IEStableStack(spec);
      that.browser = 'internet-explorer';
    } else if (window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] >= 26) {
      // Google Chrome Stable.
      that = Erizo.ChromeStableStack(spec);
      that.browser = 'chrome-stable';
    } else {
      // None.
      throw 'WebRTC stack not available';
    }
    return that;
  }

  var DISCONNECTED = 0, CONNECTING = 1, CONNECTED = 2;
  var internalDispatcher = Woogeen.EventDispatcher({});

  function WoogeenConference (spec) {
    var that = spec || {};
    this.remoteStreams = {};
    this.localStreams = {};
    that.state = DISCONNECTED;

    this.setIceServers = function () {
      that.userSetIceServers = [];
      Array.prototype.slice.call(arguments, 0).map(function (arg) {
        if (arg instanceof Array) {
          arg.map(function (server) {
            if (typeof server === 'object' && server !== null && typeof server.url === 'string' && server.url !== '') {
              that.userSetIceServers.push(server);
            } else if (typeof server === 'string' && server !== '') {
              that.userSetIceServers.push({url: server});
            }
          });
        } else if (typeof arg === 'object' && arg !== null && typeof arg.url === 'string' && arg.url !== '') {
          that.userSetIceServers.push(arg);
        } else if (typeof arg === 'string' && arg !== '') {
          that.userSetIceServers.push({url: arg});
        }
      });
      return that.userSetIceServers;
    };

    this.getIceServers = function () {
      return that.userSetIceServers;
    };

    Object.defineProperties(this, {
      state: {
        get: function () {
          return that.state;
        }
      }
    });

    this.join = function (token, onSuccess, onFailure) {
      var self = this;
      try {
        token = JSON.parse(L.Base64.decodeBase64(token));
      } catch (err) {
        return safeCall(onFailure, 'invalid token');
      }

      var isSecured = (token.secure === true);
      var host = token.host;
      if (typeof host !== 'string') {
        return safeCall(onFailure, 'invalid host');
      }
      // check connection>host< state
      if (self.state !== DISCONNECTED) {
        return safeCall(onFailure, 'connection state invalid');
      }

      self.on('server-disconnected', function () { // onConnectionClose handler
        that.state = DISCONNECTED;
        self.myId = null;
        var i, stream;
        // remove all remote streams
        for (i in self.remoteStreams) {
          if (self.remoteStreams.hasOwnProperty(i)) {
            stream = self.remoteStreams[i];
            stream.close();
            delete self.remoteStreams[i];
            var evt = new Woogeen.StreamEvent({type: 'stream-removed', stream: stream});
            self.dispatchEvent(evt);
          }
        }

        // close all channel
        for (i in self.localStreams) {
          if (self.localStreams.hasOwnProperty(i)) {
            stream = self.localStreams[i];
            if (stream.channel && typeof stream.channel.close === 'function') {
              stream.channel.close();
            }
            delete self.localStreams[i];
          }
        }

        // close socket.io
        try {
          self.socket.disconnect();
        } catch (err) {}
      });

      that.state = CONNECTING;

      if (self.socket !== undefined) { // whether reconnect
        self.socket.connect();
      } else {
        var dispatchEventAfterSubscribed = function (event) {
          var remoteStream = event.stream;
          if (remoteStream.channel && typeof remoteStream.signalOnPlayAudio === 'function') {
            self.dispatchEvent(event);
          } else if (remoteStream.channel && remoteStream.channel.state !== 'closed') {
            setTimeout(function () {
              dispatchEventAfterSubscribed(event);
            }, 20);
          } else {
            L.Logger.warning('event missed:', event.type);
          }
        };

        self.socket = io.connect(host, {
          reconnect: false,
          secure: isSecured,
          'force new connection': true
        });

        self.socket.on('onAddStream', function (spec) {
          var stream = new Woogeen.RemoteStream({
            video: spec.video,
            audio: spec.audio,
            id: spec.id,
            from: spec.from,
            attributes: spec.attributes
          });
          var evt;
          if (self.remoteStreams[spec.id] !== undefined) {
            stream.mediaStream = self.remoteStreams[spec.id].mediaStream;
            evt = new Woogeen.StreamEvent({type: 'stream-updated', stream: stream});
          } else {
            evt = new Woogeen.StreamEvent({type: 'stream-added', stream: stream});
          }
          self.remoteStreams[spec.id] = stream;
          self.dispatchEvent(evt);
        });

        self.socket.on('onUpdateStream', function (spec) {
          var stream = new Woogeen.RemoteStream({
            video: spec.video,
            audio: spec.audio,
            id: spec.id,
            from: spec.from,
            attributes: spec.attributes,
            mediaStream: (self.remoteStreams[spec.id] || {}).mediaStream
          });
          var evt = new Woogeen.StreamEvent({type: 'stream-updated', stream: stream});
          self.remoteStreams[spec.id] = stream;
          self.dispatchEvent(evt);
        });

        self.socket.on('onRemoveStream', function (spec) {
          var stream = self.remoteStreams[spec.id];
          if (stream) {
            stream.close(); // >removeStream<
            delete self.remoteStreams[spec.id];
            var evt = new Woogeen.StreamEvent({type: 'stream-removed', stream: stream});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onSubscribeP2P', function (spec) { // p2p conference call
          var myStream = self.localStreams[spec.streamId];
          if (myStream.channel === undefined) {
            myStream.channel = {};
          }

          myStream.channel[spec.subsSocket] = createChannel({
            callback: function (offer) {
              sendSdp(self.socket, 'publish', {
                state: 'p2pSignaling',
                streamId: spec.streamId,
                subsSocket: spec.subsSocket
              }, offer, function (answer) {
                if (answer === 'error' || answer === 'timeout') {
                  L.Logger.warning('invalid answer');
                  return;
                }
                myStream.channel[spec.subsSocket].onsignalingmessage = function () {
                  myStream.channel[spec.subsSocket].onsignalingmessage = function () {};
                };
                myStream.channel[spec.subsSocket].processSignalingMessage(answer);
              });
            },
            audio: myStream.hasAudio(),
            video: myStream.hasVideo(),
            stunServerUrl: self.connSettings.stun,
            turnServer: self.connSettings.turn
          });

          myStream.channel[spec.subsSocket].addStream(myStream.mediaStream);
          myStream.channel[spec.subsSocket].oniceconnectionstatechange = function (state) {
            if (state === 'disconnected') {
              myStream.channel[spec.subsSocket].close();
              delete myStream.channel[spec.subsSocket];
            }
          };
        });

        self.socket.on('onPublishP2P', function (spec, callback) {
          var myStream = self.remoteStreams[spec.streamId];

          myStream.channel = createChannel({
            callback: function () {},
            stunServerUrl: self.connSettings.stun,
            turnServer: self.connSettings.turn,
            maxAudioBW: self.connSettings.maxAudioBW,
            maxVideoBW: self.connSettings.maxVideoBW
          });

          myStream.channel.onsignalingmessage = function (answer) {
            myStream.channel.onsignalingmessage = function () {};
            safeCall(callback, answer);
          };

          myStream.channel.processSignalingMessage(spec.sdp);

          myStream.channel.onaddstream = function (evt) {
            myStream.mediaStream = evt.stream;
            internalDispatcher.dispatchEvent(new Woogeen.StreamEvent({type: 'p2p-stream-subscribed', stream: myStream}));
          };
        });

        // We receive an event of remote video stream paused
        self.socket.on('onVideoHold', function (spec) {
          var stream = self.remoteStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'video-hold', stream: stream});
            dispatchEventAfterSubscribed(evt);
          }
        });

        // We receive an event of remote video stream resumed
        self.socket.on('onVideoReady', function (spec) {
          var stream = self.remoteStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'video-ready', stream: stream});
            dispatchEventAfterSubscribed(evt);
          }
        });

        // We receive an event of remote audio stream paused
        self.socket.on('onAudioHold', function (spec) {
          var stream = self.remoteStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'audio-hold', stream: stream});
            dispatchEventAfterSubscribed(evt);
          }
        });

        // We receive an event of remote audio stream resumed
        self.socket.on('onAudioReady', function (spec) {
          var stream = self.remoteStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'audio-ready', stream: stream});
            dispatchEventAfterSubscribed(evt);
          }
        });

        // We receive an event of all the remote audio streams paused
        self.socket.on('onAllAudioHold', function () {
          for (var index in self.remoteStreams) {
            if (self.remoteStreams.hasOwnProperty(index)) {
              var stream = self.remoteStreams[index];
              var evt = new Woogeen.StreamEvent({type: 'audio-hold', stream: stream});
              dispatchEventAfterSubscribed(evt);
            }
          }
        });

        // We receive an event of all the remote audio streams resumed
        self.socket.on('onAllAudioReady', function () {
          for (var index in self.remoteStreams) {
            if (self.remoteStreams.hasOwnProperty(index)) {
              var stream = self.remoteStreams[index];
              var evt = new Woogeen.StreamEvent({type: 'audio-ready', stream: stream});
              dispatchEventAfterSubscribed(evt);
            }
          }
        });

        self.socket.on('onVideoOn', function (spec) {
          var stream = self.localStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'video-on', stream: stream});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onVideoOff', function (spec) {
          var stream = self.localStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'video-off', stream: stream});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onAudioOn', function (spec) {
          var stream = self.localStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'audio-on', stream: stream});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onAudioOff', function (spec) {
          var stream = self.localStreams[spec.id];
          if (stream) {
            var evt = new Woogeen.StreamEvent({type: 'audio-off', stream: stream});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('disconnect', function () {
          if (that.state !== DISCONNECTED) {
            var evt = new Woogeen.ClientEvent({type: 'server-disconnected'});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onUserJoin', function (spec) {
          var evt = new Woogeen.ClientEvent({type: 'user-joined', user: spec.user});
          self.dispatchEvent(evt);
        });

        self.socket.on('onUserLeave', function (spec) {
          var evt = new Woogeen.ClientEvent({type: 'user-left', user: spec.user});
          self.dispatchEvent(evt);
        });

        self.socket.on('onCustomMessage', function (spec) {
          var evt = new Woogeen.MessageEvent({type: 'message-received', msg: spec});
          self.dispatchEvent(evt);
        });

        self.socket.on('connect_failed', function (err) {
          safeCall(onFailure, err || 'connection_failed');
        });

        self.socket.on('error', function (err) {
          safeCall(onFailure, err || 'connection_error');
        });
      }

      try {
        self.socket.emit('token', token, function (status, resp) {
          if (status === 'success') {
            self.connSettings = {
              turn: resp.turnServer,
              stun: resp.stunServerUrl,
              defaultVideoBW: resp.defaultVideoBW,
              maxVideoBW: resp.maxVideoBW
            };
            self.myId = resp.clientId;
            self.conferenceId = resp.id;
            self.p2p = resp.p2p;
            that.state = CONNECTED;
            var streams = resp.streams.map(function (st) {
              self.remoteStreams[st.id] = new Woogeen.RemoteStream(st);
              return self.remoteStreams[st.id];
            });
            return safeCall(onSuccess, {streams: streams, users: resp.users});
          }
          return safeCall(onFailure, resp||'response error');
        });
      } catch (e) {
        safeCall(onFailure, 'socket emit error');
      }
    };

  }

  function sendMsg(socket, type, message, callback) {
    if (!socket || !socket.connected) {
      return callback('socket not ready');
    }
    try {
      socket.emit(type, message, function (resp, mesg) {
        if (resp === 'success') {
          return callback(null, mesg);
        }
        return callback(mesg||'response error');
      });
    } catch (err) {
      callback('socket emit error');
    }
  }

  function sendSdp(socket, type, option, sdp, callback) {
    if (!socket || !socket.connected) {
      return callback('error', 'socket not ready');
    }
    try {
      socket.emit(type, option, sdp, function (status, resp) {
        callback(status, resp);
      });
    } catch (err) {
      callback('error', 'socket emit error');
    }
  }

  function sendCtrlPayload(socket, action, streamId, onSuccess, onFailure) {
    var payload = {
      type: 'control',
      payload: {
        action: action,
        streamId: streamId
      }
    };
    sendMsg(socket, 'customMessage', payload, function(err, resp) {
      if (err) {
        return safeCall(onFailure, err);
      }
      safeCall(onSuccess, resp);
    });
  }

  WoogeenConference.prototype = Woogeen.EventDispatcher({}); // make WoogeenConference a eventDispatcher

  WoogeenConference.prototype.leave = function () {
    var evt = new Woogeen.ClientEvent({type: 'server-disconnected'});
    this.dispatchEvent(evt);
  };

  WoogeenConference.prototype.send = function (data, receiver, onSuccess, onFailure) {
    if (data === undefined || data === null || typeof data === 'function') {
      return safeCall(onFailure, 'nothing to send');
    }
    if (typeof receiver === 'undefined') {
      receiver = 'all';
    } else if (typeof receiver === 'string') {
      // supposed to be a valid receiverId.
      // pass.
    } else if (typeof receiver === 'function') {
      onFailure = onSuccess;
      onSuccess = receiver;
      receiver = 'all';
    } else {
      return safeCall(onFailure, 'invalid receiver');
    }
    sendMsg(this.socket, 'customMessage', {
      type: 'data',
      data: data,
      receiver: receiver
    }, function (err, resp) {
      if (err) {
        return safeCall(onFailure, err);
      }
      safeCall(onSuccess, resp);
    });
  };

  /*
  options: {
    audio: true/false,
    video: true/false,
    maxVideoBW: xxx,
    maxAudioBW: xxx,
    videoCodec: 'h264' // not for p2p room
  }
  */
  WoogeenConference.prototype.publish = function (stream, options, onSuccess, onFailure) {
    var self = this;
    stream = stream || {};
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = stream.bitRate;
    } else if (typeof options !== 'object' || options === null) {
      options = stream.bitRate;
    }
    if (!(stream instanceof Woogeen.LocalStream) ||
          ((typeof stream.mediaStream !== 'object' || stream.mediaStream === null) &&
             stream.url() === undefined)) {
      return safeCall(onFailure, 'invalid stream');
    }

    if (self.localStreams[stream.id()] === undefined) { // not pulished
      var opt = stream.toJson();
      if (stream.url() !== undefined) {
        opt.state = 'url';
        sendSdp(self.socket, 'publish', opt, stream.url(), function (answer, id) {
            if (answer !== 'success') {
              return safeCall(onFailure, answer);
            }
            stream.id = function () {
              return id;
            };
            stream.unpublish = function (onSuccess, onFailure) {
              self.unpublish(stream, onSuccess, onFailure);
            };
            self.localStreams[id] = stream;
            safeCall(onSuccess, stream);
        });
        return;
      } else if (self.p2p) {
        self.connSettings.maxVideoBW = options.maxVideoBW;
        self.connSettings.maxAudioBW = options.maxAudioBW;
        opt.state = 'p2p';
        sendSdp(self.socket, 'publish', opt, null, function (answer, id) {
            if (answer === 'error') {
              return safeCall(onFailure, answer);
            }
            stream.id = function () {
              return id;
            };
            stream.unpublish = function (onSuccess, onFailure) {
              self.unpublish(stream, onSuccess, onFailure);
            };
            self.localStreams[id] = stream;
            safeCall(onSuccess, stream);
        });
        return;
      }
      options.maxVideoBW = options.maxVideoBW || self.connSettings.defaultVideoBW;
      if (options.maxVideoBW > self.connSettings.maxVideoBW) {
        options.maxVideoBW = self.connSettings.maxVideoBW;
      }
      stream.channel = createChannel({
        callback: function (offer) {
          opt.state = 'offer';
          sendSdp(self.socket, 'publish', opt, offer, function (answer, id) {
            if (answer === 'error') {
              return safeCall(onFailure, id);
            }
            if (answer === 'timeout') {
              return safeCall(onFailure, answer);
            }
            stream.channel.onsignalingmessage = function () {
              stream.channel.onsignalingmessage = function () {};
              stream.id = function () {
                return id;
              };
              self.localStreams[id] = stream;
              stream.signalOnPlayAudio = function (onSuccess, onFailure) {
                sendCtrlPayload(self.socket, 'audio-out-on', id, onSuccess, onFailure);
              };
              stream.signalOnPauseAudio = function (onSuccess, onFailure) {
                sendCtrlPayload(self.socket, 'audio-out-off', id, onSuccess, onFailure);
              };
              stream.signalOnPlayVideo = function (onSuccess, onFailure) {
                sendCtrlPayload(self.socket, 'video-out-on', id, onSuccess, onFailure);
              };
              stream.signalOnPauseVideo = function (onSuccess, onFailure) {
                sendCtrlPayload(self.socket, 'video-out-off', id, onSuccess, onFailure);
              };
              stream.unpublish = function (onSuccess, onFailure) {
                self.unpublish(stream, onSuccess, onFailure);
              };
              safeCall(onSuccess, stream);
            };

            stream.channel.oniceconnectionstatechange = function (state) {
              if (state === 'failed') {
                sendMsg(self.socket, 'unpublish', stream.id(), function () {}, function () {});
                stream.channel.close();
                stream.channel = undefined;
                delete self.localStreams[stream.id()];
                stream.id = function () { return null; };
                stream.signalOnPlayAudio = undefined;
                stream.signalOnPauseAudio = undefined;
                stream.signalOnPlayVideo = undefined;
                stream.signalOnPauseVideo = undefined;
                delete stream.unpublish;
                safeCall(onFailure, 'peer connection failed');
              }
            };
            stream.channel.processSignalingMessage(answer);
          });
        },
        video: stream.hasVideo() && (options.video !== false),
        audio: stream.hasAudio() && (options.audio !== false),
        iceServers: self.getIceServers(),
        stunServerUrl: self.connSettings.stun,
        turnServer: self.connSettings.turn,
        maxAudioBW: options.maxAudioBW,
        maxVideoBW: options.maxVideoBW,
        videoCodec: options.videoCodec
      });
      stream.channel.addStream(stream.mediaStream);
    } else {
      return safeCall(onFailure, 'already published');
    }
  };

  WoogeenConference.prototype.mix = function(stream, onSuccess, onFailure) {
    if (!(stream instanceof Woogeen.LocalStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    sendMsg(this.socket, 'addToMixer', stream.id(), function (err) {
      if (err) { return safeCall(onFailure, err); }
      safeCall(onSuccess, null);
    });
  };

  WoogeenConference.prototype.unmix = function(stream, onSuccess, onFailure) {
    if (!(stream instanceof Woogeen.LocalStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    sendMsg(this.socket, 'removeFromMixer', stream.id(), function (err) {
      if (err) { return safeCall(onFailure, err); }
      safeCall(onSuccess, null);
    });
  };

  WoogeenConference.prototype.unpublish = function (stream, onSuccess, onFailure) {
    var self = this;
    if (!(stream instanceof Woogeen.LocalStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    sendMsg(self.socket, 'unpublish', stream.id(), function (err) {
      if (err) {return safeCall(onFailure, err);}
      if (stream.channel && typeof stream.channel.close === 'function') {
        stream.channel.close();
        stream.channel = null;
      }
      delete self.localStreams[stream.id()];
      stream.id = function () {return null;};
      stream.signalOnPlayAudio = undefined;
      stream.signalOnPauseAudio = undefined;
      stream.signalOnPlayVideo = undefined;
      stream.signalOnPauseVideo = undefined;
      delete stream.unpublish;
      safeCall(onSuccess, null);
    });
  };

  /*
  options: {
    audio: true/false,
    video: true/false,
    videoCodec: 'h264' // not for p2p room
  }
  */
  WoogeenConference.prototype.subscribe = function (stream, options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }
    if (!(stream instanceof Woogeen.RemoteStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    if (self.p2p) {
      internalDispatcher.on('p2p-stream-subscribed', function p2pStreamHandler (evt) {
        internalDispatcher.removeEventListener('p2p-stream-subscribed', p2pStreamHandler);
        safeCall(onSuccess, evt.stream);
      });
      sendSdp(self.socket, 'subscribe', {streamId: stream.id()}, null, function () {});
      return;
    }

    stream.channel = createChannel({
      callback: function (offer) {
        if (JSON.parse(offer).messageType !== 'OFFER') {return;} // filter out 'sendOK'
        sendSdp(self.socket, 'subscribe', {
          streamId: stream.id(),
          audio: stream.hasAudio(),
          video: stream.hasVideo()
        }, offer, function (answer, errText) {
          if (answer === 'error' || answer === 'timeout') {
            return safeCall(onFailure, errText || answer);
          }
          stream.channel.processSignalingMessage(answer);
        });
      },
      audio: stream.hasAudio() && (options.audio !== false),
      video: stream.hasVideo() && (options.video !== false),
      iceServers: self.getIceServers(),
      stunServerUrl: self.connSettings.stun,
      turnServer: self.connSettings.turn,
      videoCodec: options.videoCodec
    });

    stream.channel.onaddstream = function (evt) {
      stream.mediaStream = evt.stream;
      if(navigator.appVersion.indexOf("Trident") > -1){
        stream["pcid"] = evt.pcid;
      }
      safeCall(onSuccess, stream);
      stream.signalOnPlayAudio = function (onSuccess, onFailure) {
        sendCtrlPayload(self.socket, 'audio-in-on', stream.id(), onSuccess, onFailure);
      };
      stream.signalOnPauseAudio = function (onSuccess, onFailure) {
        sendCtrlPayload(self.socket, 'audio-in-off', stream.id(), onSuccess, onFailure);
      };
      stream.signalOnPlayVideo = function (onSuccess, onFailure) {
        sendCtrlPayload(self.socket, 'video-in-on', stream.id(), onSuccess, onFailure);
      };
      stream.signalOnPauseVideo = function (onSuccess, onFailure) {
        sendCtrlPayload(self.socket, 'video-in-off', stream.id(), onSuccess, onFailure);
      };
    };

    stream.channel.oniceconnectionstatechange = function (state) {
      if (state === 'failed') {
        sendMsg(self.socket, 'unsubscribe', stream.id(), function () {}, function () {});
        stream.close();
        stream.signalOnPlayAudio = undefined;
        stream.signalOnPauseAudio = undefined;
        stream.signalOnPlayVideo = undefined;
        stream.signalOnPauseVideo = undefined;
        safeCall(onFailure, 'peer connection failed');
      }
    };
  };

  WoogeenConference.prototype.unsubscribe = function (stream, onSuccess, onFailure) {
    var self = this;
    if (!(stream instanceof Woogeen.RemoteStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    sendMsg(self.socket, 'unsubscribe', stream.id(), function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      stream.close();
      stream.signalOnPlayAudio = undefined;
      stream.signalOnPauseAudio = undefined;
      stream.signalOnPlayVideo = undefined;
      stream.signalOnPauseVideo = undefined;
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.prototype.onMessage = function (callback) {
    if (typeof callback === 'function') {
      this.on('message-received', callback);
    }
  };

  WoogeenConference.prototype.shareScreen = function (option, onSuccess, onFailure) {
    var self = this;
    if (typeof option === 'function') {
      onFailure = onSuccess;
      onSuccess = option;
      option = {};
    }
    option = option || {};
    Woogeen.LocalStream.create({
      video: {
        device: 'screen',
        resolution: option.resolution,
        frameRate: option.frameRate
      },
      audio: false
    }, function (err, stream) {
      if (err) {
        return safeCall(onFailure, err);
      }
      self.publish(stream, function (st) {
        safeCall(onSuccess, st);
      }, function (err) {
        safeCall(onFailure, err);
      });
    });
  };

  /*
  options: {
    streamId: xxxxxx,
    recorderId: yyyyyy
  }
  */
  WoogeenConference.prototype.startRecorder = function (options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }

    sendMsg(self.socket, 'startRecorder', options, function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      safeCall(onSuccess, resp);
    });
  };

  /*
  options: {
    recorderId: xxxxxx
  }
  */
  WoogeenConference.prototype.stopRecorder = function (options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }

    sendMsg(self.socket, 'stopRecorder', options, function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.prototype.getRegion = function (options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }

    sendMsg(self.socket, 'getRegion', options, function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.prototype.setRegion = function (options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }

    sendMsg(self.socket, 'setRegion', options, function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.prototype.setVideoBitrate = function (options, onSuccess, onFailure) {
    var self = this;
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
      options = {};
    } else if (typeof options !== 'object' || options === null) {
      options = {};
    }

    sendMsg(self.socket, 'setVideoBitrate', options, function (err, resp) {
      if (err) {return safeCall(onFailure, err);}
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.create = function factory (spec) { // factory, not in prototype
    return new WoogeenConference(spec);
  };

  return WoogeenConference;
}());
