/* global io, console */
Woogeen.Conference = (function () {
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

    this.join = function (token, callback) {
      var self = this;
      try {
        token = JSON.parse(L.Base64.decodeBase64(token));
      } catch (err) {
        return safeCall(callback, 'invalid token');
      }

      var isSecured = (token.secure === true);
      var host = token.host;
      if (typeof host !== 'string') {
        return safeCall(callback, 'invalid host');
      }
      // check connection>host< state
      if (self.state !== DISCONNECTED) {
        return safeCall(callback, 'connection state invalid');
      }

      self.on('client-disconnected', function () { // onConnectionClose handler
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
      if (isSecured) {
        delete io.sockets['https://'+host];
      } else {
        delete io.sockets['http://'+host];
      }
      if (self.socket !== undefined) { // whether reconnect
        self.socket.socket.connect();
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
            // log.warn('event missed:', event);
          }
        };

        self.socket = io.connect(host, {reconnect: false, secure: isSecured});

        self.socket.on('onAddStream', function (spec) {
          var stream = new Woogeen.RemoteStream({
            video: spec.video,
            audio: spec.audio,
            id: spec.id,
            attributes: spec.attributes
          });
          var evt;
          if (self.remoteStreams[spec.id] !== undefined) {
            evt = new Woogeen.StreamEvent({type: 'stream-updated', stream: stream});
          } else {
            evt = new Woogeen.StreamEvent({type: 'stream-added', stream: stream});
          }
          self.remoteStreams[spec.id] = stream;
          self.dispatchEvent(evt);
        });

        self.socket.on('onUpdateStream', function (spec) { // FIXME: mediaStream
          var stream = new Woogeen.RemoteStream({
            video: spec.video,
            audio: spec.audio,
            id: spec.id,
            attributes: spec.attributes
          });
          var evt = new Woogeen.StreamEvent({type: 'stream-updated', stream: stream});
          self.remoteStreams[spec.id] = stream;
          self.dispatchEvent(evt);
        });

        self.socket.on('onRemoveStream', function (spec) {
          var stream = self.remoteStreams[spec.id];
          stream.close(); // >removeStream<
          delete self.remoteStreams[spec.id];
          var evt = new Woogeen.StreamEvent({type: 'stream-removed', stream: stream});
          self.dispatchEvent(evt);
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
            var evt = new Woogeen.ClientEvent({type: 'client-disconnected'});
            self.dispatchEvent(evt);
          }
        });

        self.socket.on('onPeerJoin', function (spec) {
          var evt = new Woogeen.ClientEvent({type: 'peer-joined', attr: spec.attr, user: spec.user});
          self.dispatchEvent(evt);
        });

        self.socket.on('onPeerLeave', function (spec) {
          var evt = new Woogeen.ClientEvent({type: 'peer-left', attr: spec.attr, user: spec.user});
          self.dispatchEvent(evt);
        });

        self.socket.on('onCustomMessage', function (spec) {
          var evt = new Woogeen.ClientEvent({type: 'message-received', attr: spec.msg});
          self.dispatchEvent(evt);
        });

        self.socket.on('connect_failed', function (err) {
          safeCall(callback, err || 'connection_failed');
        });

        self.socket.on('error', function (err) {
          safeCall(callback, err || 'connection_error');
        });
      }

      sendMsg(self.socket, 'token', token, function (err, resp) {
        if (err) {
          return safeCall(callback, resp);
        }
        self.connSettings = {
          turn: resp.turnServer,
          stun: resp.stunServerUrl,
          defaultVideoBW: resp.defaultVideoBW,
          maxVideoBW: resp.maxVideoBW
        };
        self.myId = resp.clientId;
        self.conferenceId = resp.id;
        that.state = CONNECTED;
        var streams = resp.streams.map(function (st) {
          self.remoteStreams[st.id] = new Woogeen.RemoteStream(st);
          return self.remoteStreams[st.id];
        });
        safeCall(callback, null, {streams: streams});
      });
    };

  }

  function sendMsg(socket, type, message, callback) {
    if (!socket) {return callback('socket not ready');}
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
    if (!socket) {return callback('error', 'socket not ready');}
    try {
      socket.emit(type, option, sdp, function (status, resp) {
        callback(status, resp);
      });
    } catch (err) {
      callback('error', 'socket emit error');
    }
  }

  function mkCtrlPayload(action, id) {
    return {
      type: 'control',
      payload: {
        action: action,
        streamId: id
      }
    };
  }

  WoogeenConference.prototype = Woogeen.EventDispatcher({}); // make WoogeenConference a eventDispatcher

  WoogeenConference.prototype.quit = function () {
    var evt = new Woogeen.ClientEvent({type: 'client-disconnected'});
    this.dispatchEvent(evt);
  };

  WoogeenConference.prototype.send = function (data, receiver, callback) {
    if (data === undefined || data === null || typeof data === 'function') {
      return safeCall(callback, 'nothing to send');
    }
    if (typeof receiver === 'undefined') {
      receiver = 0; // 0 => ALL
    } else if (typeof receiver === 'number') {
      // supposed to be a valid receiverId.
      // pass.
    } else if (typeof receiver === 'function') {
      callback = receiver;
      receiver = 0;
    } else {
      return safeCall(callback, 'invalid receiver');
    }
    sendMsg(this.socket, 'customMessage', {
      data: data,
      receiver: receiver
    }, function (err, resp) {
      safeCall(callback, err, resp);
    });
  };

  WoogeenConference.prototype.publish = function (stream, options, callback) {
    var self = this;
    if (!(stream instanceof Woogeen.LocalStream) ||
      (typeof stream.mediaStream !== 'object' || stream.mediaStream === null)) {
      return safeCall(callback, 'invalid stream');
    }

    if (typeof options === 'function') {
      callback = options;
      options = stream.bitRate;
    } else if (typeof options !== 'object' || options === null) {
      options = stream.bitRate;
    }

    if (self.localStreams[stream.id()] === undefined) { // not pulished
      options.maxVideoBW = options.maxVideoBW || self.connSettings.defaultVideoBW;
      if (options.maxVideoBW > self.connSettings.maxVideoBW) {
        options.maxVideoBW = self.connSettings.maxVideoBW;
      }
      stream.channel = createChannel({
        callback: function (offer) {
          sendSdp(self.socket, 'publish', {
            state: 'offer',
            audio: stream.hasAudio(),
            video: stream.hasVideo(),
            attributes: stream.attributes()
          }, offer, function (answer, id) {
            if (answer === 'error') {
              return safeCall(callback, answer, id);
            }
            stream.channel.onsignalingmessage = function () {
              stream.channel.onsignalingmessage = function () {};
              stream.id = function () {
                return id;
              };
              self.localStreams[id] = stream;
              stream.signalOnPlayAudio = function (onSuccess, onFailure) {
                self.send(mkCtrlPayload('audio-out-on'), onSuccess, onFailure);
              };
              stream.signalOnPauseAudio = function (onSuccess, onFailure) {
                self.send(mkCtrlPayload('audio-out-off'), onSuccess, onFailure);
              };
              stream.signalOnPlayVideo = function (onSuccess, onFailure) {
                self.send(mkCtrlPayload('video-out-on'), onSuccess, onFailure);
              };
              stream.signalOnPauseVideo = function (onSuccess, onFailure) {
                self.send(mkCtrlPayload('video-out-off'), onSuccess, onFailure);
              };
              safeCall(callback, null, stream);
            };

            stream.channel.oniceconnectionstatechange = function (state) {
              if (state === 'failed') {
                sendMsg(self.socket, 'unpublish', stream.id(), function(){}, function(){});
                stream.channel.close();
                stream.channel = undefined;
                delete self.localStreams[stream.id()];
                stream.id = function () { return null; };
                stream.signalOnPlayAudio = undefined;
                stream.signalOnPauseAudio = undefined;
                stream.signalOnPlayVideo = undefined;
                stream.signalOnPauseVideo = undefined;
                delete stream.unpublish;
                safeCall(callback, 'peer connection failed');
              }
            };
            stream.channel.processSignalingMessage(answer);
          });
        },
        video: stream.hasVideo(),
        audio: stream.hasAudio(),
        iceServers: self.getIceServers(),
        stunServerUrl: self.connSettings.stun,
        turnServer: self.connSettings.turn,
        maxAudioBW: options.maxAudioBW,
        maxVideoBW: options.maxVideoBW
      });
      stream.channel.addStream(stream.mediaStream);
    } else {
      return safeCall(callback, 'already published');
    }
  };

  WoogeenConference.prototype.unpublish = function (stream, callback) {
    var self = this;
    if (!(stream instanceof Woogeen.LocalStream)) {
      return safeCall(callback, 'invalid stream');
    }
    sendMsg(self.socket, 'unpublish', stream.id(), function (err) {
      if (err) {return safeCall(callback, err);}
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
      safeCall(callback, null);
    });
  };

  WoogeenConference.prototype.subscribe = function (stream, options, callback) {
    var self = this;
    if (!(stream instanceof Woogeen.RemoteStream)) {
      return safeCall(callback, 'invalid stream');
    }
    if (typeof options === 'function') {
      callback = options;
      options = null;
    }
    stream.channel = createChannel({
      callback: function (offer) {
        sendSdp(self.socket, 'subscribe', {
          streamId: stream.id(),
          audio: stream.hasAudio(),
          video: stream.hasVideo()
        }, offer, function (answer) {
          if (answer === 'error') {
            return safeCall(callback, answer);
          }
          stream.channel.processSignalingMessage(answer);
        });
      },
      audio: stream.hasAudio(),
      video: stream.hasVideo(),
      iceServers: self.getIceServers(),
      stunServerUrl: self.connSettings.stun,
      turnServer: self.connSettings.turn
    });

    stream.channel.onaddstream = function (evt) {
      stream.mediaStream = evt.stream;
      safeCall(callback, null, stream);
      stream.signalOnPlayAudio = function (onSuccess, onFailure) {
        self.send(mkCtrlPayload('audio-in-on', stream.id()), onSuccess, onFailure);
      };
      stream.signalOnPauseAudio = function (onSuccess, onFailure) {
        self.send(mkCtrlPayload('audio-in-off', stream.id()), onSuccess, onFailure);
      };
      stream.signalOnPlayVideo = function (onSuccess, onFailure) {
        self.send(mkCtrlPayload('video-in-on', stream.id()), onSuccess, onFailure);
      };
      stream.signalOnPauseVideo = function (onSuccess, onFailure) {
        self.send(mkCtrlPayload('video-in-off', stream.id()), onSuccess, onFailure);
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
        safeCall(callback, 'peer connection failed');
      }
    };
  };

  WoogeenConference.prototype.unsubscribe = function (stream, callback) {
    var self = this;
    if (!(stream instanceof Woogeen.RemoteStream)) {
      return safeCall(callback, 'invalid stream');
    }
    sendMsg(self.socket, 'unsubscribe', stream.id(), function (err, resp) {
      if (err) {return safeCall(callback, err);}
      stream.close();
      stream.signalOnPlayAudio = undefined;
      stream.signalOnPauseAudio = undefined;
      stream.signalOnPlayVideo = undefined;
      stream.signalOnPauseVideo = undefined;
      safeCall(callback, null, resp);
    });
  };

  WoogeenConference.prototype.onMessage = function (callback) {
    if (typeof callback === 'function') {
      this.on('message-received', callback);
    }
  };

  WoogeenConference.prototype.shareScreen = function () {
    // this.stopShare = function () {};
  };

  WoogeenConference.create = function factory (spec) { // factory, not in prototype
    return new WoogeenConference(spec);
  };

  return WoogeenConference;
}());