/*
 * Intel WebRTC SDK version 2.0.0
 * Copyright (c) 2014 Intel <http://webrtc.intel.com>
 * Homepage: http://webrtc.intel.com
 */


(function(window) {


var Woogeen = (function() {
  'use strict';

  var Woogeen = {};

  Object.defineProperties(Woogeen, {
    version: {
      get: function() { return '2.0.0'; }
    },
    name: {
      get: function() { return 'Intel WebRTC SDK'; }
    }
  });

  return Woogeen;
})();

var L = {};
var Erizo = {};



/*global unescape*/
L.Base64 = (function () {
    "use strict";
    var END_OF_INPUT, base64Chars, reverseBase64Chars, base64Str, base64Count, i, setBase64Str, readBase64, encodeBase64, readReverseBase64, ntos, decodeBase64;

    END_OF_INPUT = -1;

    base64Chars = [
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '/'
    ];

    reverseBase64Chars = [];

    for (i = 0; i < base64Chars.length; i = i + 1) {
        reverseBase64Chars[base64Chars[i]] = i;
    }

    setBase64Str = function (str) {
        base64Str = str;
        base64Count = 0;
    };

    readBase64 = function () {
        var c;
        if (!base64Str) {
            return END_OF_INPUT;
        }
        if (base64Count >= base64Str.length) {
            return END_OF_INPUT;
        }
        c = base64Str.charCodeAt(base64Count) & 0xff;
        base64Count = base64Count + 1;
        return c;
    };

    encodeBase64 = function (str) {
        var result, inBuffer, done;
        setBase64Str(str);
        result = '';
        inBuffer = new Array(3);
        done = false;
        while (!done && (inBuffer[0] = readBase64()) !== END_OF_INPUT) {
            inBuffer[1] = readBase64();
            inBuffer[2] = readBase64();
            result = result + (base64Chars[inBuffer[0] >> 2]);
            if (inBuffer[1] !== END_OF_INPUT) {
                result = result + (base64Chars [((inBuffer[0] << 4) & 0x30) | (inBuffer[1] >> 4)]);
                if (inBuffer[2] !== END_OF_INPUT) {
                    result = result + (base64Chars [((inBuffer[1] << 2) & 0x3c) | (inBuffer[2] >> 6)]);
                    result = result + (base64Chars[inBuffer[2] & 0x3F]);
                } else {
                    result = result + (base64Chars[((inBuffer[1] << 2) & 0x3c)]);
                    result = result + ('=');
                    done = true;
                }
            } else {
                result = result + (base64Chars[((inBuffer[0] << 4) & 0x30)]);
                result = result + ('=');
                result = result + ('=');
                done = true;
            }
        }
        return result;
    };

    readReverseBase64 = function () {
        if (!base64Str) {
            return END_OF_INPUT;
        }
        while (true) {
            if (base64Count >= base64Str.length) {
                return END_OF_INPUT;
            }
            var nextCharacter = base64Str.charAt(base64Count);
            base64Count = base64Count + 1;
            if (reverseBase64Chars[nextCharacter]) {
                return reverseBase64Chars[nextCharacter];
            }
            if (nextCharacter === 'A') {
                return 0;
            }
        }
    };

    ntos = function (n) {
        n = n.toString(16);
        if (n.length === 1) {
            n = "0" + n;
        }
        n = "%" + n;
        return unescape(n);
    };

    decodeBase64 = function (str) {
        var result, inBuffer, done;
        setBase64Str(str);
        result = "";
        inBuffer = new Array(4);
        done = false;
        while (!done && (inBuffer[0] = readReverseBase64()) !== END_OF_INPUT && (inBuffer[1] = readReverseBase64()) !== END_OF_INPUT) {
            inBuffer[2] = readReverseBase64();
            inBuffer[3] = readReverseBase64();
            result = result + ntos((((inBuffer[0] << 2) & 0xff)| inBuffer[1] >> 4));
            if (inBuffer[2] !== END_OF_INPUT) {
                result +=  ntos((((inBuffer[1] << 4) & 0xff) | inBuffer[2] >> 2));
                if (inBuffer[3] !== END_OF_INPUT) {
                    result = result +  ntos((((inBuffer[2] << 6)  & 0xff) | inBuffer[3]));
                } else {
                    done = true;
                }
            } else {
                done = true;
            }
        }
        return result;
    };

    return {
        encodeBase64: encodeBase64,
        decodeBase64: decodeBase64
    };
}());


/*global console*/

/*
 * API to write logs based on traditional logging mechanisms: debug, trace, info, warning, error
 */
L.Logger = (function () {
  "use strict";
  var DEBUG = 0, TRACE = 1, INFO = 2, WARNING = 3, ERROR = 4, NONE = 5, logLevel = DEBUG, setLogLevel, log, debug, trace, info, warning, error;

  // It sets the new log level. We can set it to NONE if we do not want to print logs
  setLogLevel = function (level) {
    if (level > NONE) {
      level = NONE;
    } else if (level < DEBUG) {
      level = DEBUG;
    }
    logLevel = level;
  };

  // Generic function to print logs for a given level: [DEBUG, TRACE, INFO, WARNING, ERROR]
  log = function () {
    var level = arguments[0];
    var args = arguments;
    if (level < logLevel) {
      return;
    }
    switch (level) {
    case DEBUG:
      args[0] = 'DEBUG:';
      break;
    case TRACE:
      args[0] = 'TRACE:';
      break;
    case INFO:
      args[0] = 'INFO:';
      break;
    case WARNING:
      args[0] = 'WARNING:';
      break;
    case ERROR:
      args[0] = 'ERROR:';
      break;
    default:
      return;
    }
    console.log.apply(console, args);
  };

  // It prints debug logs
  debug = function () {
    var args = [DEBUG];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints trace logs
  trace = function () {
    var args = [TRACE];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints info logs
  info = function () {
    var args = [INFO];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints warning logs
  warning = function () {
    var args = [WARNING];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints error logs
  error = function () {
    var args = [ERROR];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  return {
    DEBUG: DEBUG,
    TRACE: TRACE,
    INFO: INFO,
    WARNING: WARNING,
    ERROR: ERROR,
    NONE: NONE,
    setLogLevel: setLogLevel,
    log: log,
    debug: debug,
    trace: trace,
    info: info,
    warning: warning,
    error: error
  };
}());


/* global webkitURL, chrome */
(function () {
  'use strict';

  function WoogeenStream (spec) {
    // if (!(this instanceof WoogeenStream)) {
    //   return new WoogeenStream();
    // }
    this.mediaStream = spec.mediaStream;
    spec.attributes = spec.attributes || {};
    this.hasVideo = function () {
      return !!spec.video;
    };
    this.hasAudio = function () {
      return !!spec.audio;
    };
    this.attributes = function () {
      return spec.attributes;
    };
    this.attr = function (key, value) {
      if (arguments.length > 1) {
        spec.attributes[key] = value;
      }
      return spec.attributes[key];
    };
    this.id = function () {
      return spec.id;
    };
    this.isScreen = function () {
      return (!!spec.video) && (spec.video.device === 'screen'); // device: 'camera', 'screen'
    };
    this.bitRate = {
      maxVideoBW: undefined,
      maxAudioBW: undefined
    }; // mutable;
  }

  WoogeenStream.prototype.close = function() {
    if (typeof this.hide === 'function') {this.hide();}
    if (this.mediaStream && typeof this.mediaStream.stop === 'function') {
      this.mediaStream.stop();
    }
    this.mediaStream = null;
    // close peer connection if necessary
    if (this.channel && typeof this.channel.close === 'function') {
      this.channel.close();
    }
  };

  WoogeenStream.prototype.createObjectURL = function() {
    if (!this.mediaStream) {return '';}
    return (window.URL || webkitURL).createObjectURL(this.mediaStream);
  };

  WoogeenStream.prototype.disableAudio = function(tracknum) {
    var self = this;
    if (self.audio() && self.mediaStream) {
      if (arguments.length > 0) {
        var tracks = self.mediaStream.getAudioTracks();
        if (tracks && tracks[tracknum]) {
          tracks[tracknum].enable = true;
        }
        return;
      }
      self.mediaStream.getAudioTracks().map(function (track) {
        track.enable = true;
      });
    }
  };

  WoogeenStream.prototype.enableAudio = function(tracknum) {
    var self = this;
    if (self.audio() && self.mediaStream) {
      if (arguments.length > 0) {
        var tracks = self.mediaStream.getAudioTracks();
        if (tracks && tracks[tracknum]) {
          tracks[tracknum].enable = false;
        }
        return;
      }
      self.mediaStream.getAudioTracks().map(function (track) {
        track.enable = false;
      });
    }
  };

  WoogeenStream.prototype.disableVideo = function(tracknum) {
    var self = this;
    if (self.audio() && self.mediaStream) {
      if (arguments.length > 0) {
        var tracks = self.mediaStream.getVideoTracks();
        if (tracks && tracks[tracknum]) {
          tracks[tracknum].enable = false;
        }
        return;
      }
      self.mediaStream.getVideoTracks().map(function (track) {
        track.enable = false;
      });
    }
  };

  WoogeenStream.prototype.enableVideo = function(tracknum) {
    var self = this;
    if (self.audio() && self.mediaStream) {
      if (arguments.length > 0) {
        var tracks = self.mediaStream.getVideoTracks();
        if (tracks && tracks[tracknum]) {
          tracks[tracknum].enable = true;
        }
        return;
      }
      self.mediaStream.getVideoTracks().map(function (track) {
        track.enable = true;
      });
    }
  };

  function WoogeenLocalStream (spec) {
    WoogeenStream.call(this, spec);
  }

  function WoogeenRemoteStream (spec) {
    WoogeenStream.call(this, spec);
    this.isMixed = function () {
      return (!!spec.video) && (spec.video.category === 'mix'); // category: 'mix', 'single'
    };
  }

  WoogeenLocalStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenRemoteStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenLocalStream.prototype.constructor = WoogeenLocalStream;
  WoogeenRemoteStream.prototype.constructor = WoogeenRemoteStream;

  function isLegacyChrome () {
    return window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./) !== null &&
      window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] <= 35;
  }

  function getReso(w, h) {
    return {
      mandatory: {
        minWidth: w,
        minHeight: h,
        maxWidth: w,
        maxHeight: h
      },
      optional: []
    };
  }

  var supportedVideoList = {
    'true': {mandatory: {}},
    'unspecified': {mandatory: {}},
    'sif': getReso(320, 240),
    'vga': getReso(640, 480),
    'hd720p': getReso(1280, 720),
    'hd1080p': getReso(1920, 1080)
  };

  var getMedia = (navigator.getUserMedia || navigator.webkitGetUserMedia ||
                     navigator.mozGetUserMedia || navigator.msGetUserMedia);


  /*
  createLocalStream({
    video: {
      device: 'camera',
      resolution: '720p',
      frameRate: [200, 500]
    },
    audio: true,
    attribtues: null
  }, function () {});
  */
  function createLocalStream (option, callback) {
    if (typeof getMedia !== 'function') {
      if (typeof callback === 'function') {
        callback({
          code: 4100,
          msg: 'webrtc support not available'
        });
      }
      return;
    }
    var init_retry = arguments[3];
    if (init_retry === undefined) {
      init_retry = 2;
    }
    var mediaOption = {};

    if (typeof option === 'object' && option !== null) {
      if (option.video) {
        if (option.video.device === 'screen') {
          if (window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] < 34) {
            if (typeof callback === 'function') {
              callback({
                code: 4101,
                msg: 'browser screen sharing not supported'
              });
              return;
            }
          }
        }
        mediaOption.video = supportedVideoList[option.video.resolution] || supportedVideoList.unspecified;

        if (!isLegacyChrome() && option.frameRate instanceof Array && option.frameRate.length >= 2) {
          mediaOption.video.mandatory.maxFrameRate = option.frameRate[0];
          mediaOption.video.mandatory.minFrameRate = option.frameRate[1];
        }
      }
      if (option.audio) {
        mediaOption.audio = true;
      }
    } else {
      if (typeof callback === 'function') {
        callback({
          code: 4109,
          msg: 'invalid media option'
        });
        return;
      }
    }


    var onSuccess = function (mediaStream) {
      option.mediaStream = mediaStream;
      var localStream = new Woogeen.LocalStream(option);

      // set default bit rate
      switch (mediaOption.video.mandatory.maxWidth) {
      case 320:
        localStream.bitRate.maxVideoBW = 512;
        break;
      case 640:
        localStream.bitRate.maxVideoBW = 1024;
        break;
      case 1280:
        localStream.bitRate.maxVideoBW = 2048;
        break;
      default:
        // localStream.bitRate.maxVideoBW = undefined;
        break;
      }
      if (typeof callback === 'function') {
        callback(null, localStream);
      }
    };

    var onFailure = function (error) {
      var err = {type: 'error', msg: error.name || error};
      switch (err.msg) {
      // below - internally handled
      case 'Starting video failed':       // firefox: camera possessed by other process?
      case 'TrackStartError':             // chrome: probably resolution not supported
        // that.setVideo('unspecified');
        option.video = true;
        if (init_retry > 0) {
          setTimeout(function () {
            createLocalStream(option, callback, init_retry-1);
          }, 1);
          return;
        } else {
          err.msg = 'MEDIA_OPTION_INVALID';
        }
        break;
      // below - exposed
      case 'DevicesNotFoundError':        // chrome
        err.msg = 'DEVICES_NOT_FOUND';
        break;
      case 'NotSupportedError':           // chrome
        err.msg = 'NOT_SUPPORTED';
        break;
      case 'PermissionDeniedError':       // chrome
        err.msg = 'PERMISSION_DENIED';
        break;
      case 'PERMISSION_DENIED':           // firefox
        break;
      case 'ConstraintNotSatisfiedError': // chrome
        err.msg = 'CONSTRAINT_NOT_SATISFIED';
        break;
      default:
        if (!err.msg) {
          err.msg = 'UNDEFINED';
        }
      }
      if (typeof callback === 'function') {
        callback(err);
      }
    };

    if (option.video && option.video.device === 'screen') {
      var extensionId = Woogeen.chromeExtensionId || 'pndohhifhheefbpeljcmnhnkphepimhe';
      try {
        chrome.runtime.sendMessage(extensionId, {getStream: true}, function (response) {
          mediaOption.video.mandatory.chromeMediaSource = 'desktop';
          mediaOption.video.mandatory.chromeMediaSourceId = response.streamId;
          getMedia.apply(navigator, [mediaOption, onSuccess, onFailure]);
        });
      } catch (err) {
        if (typeof callback === 'function') {
          callback({
            code: 4102,
            msg: 'screen sharing plugin inaccessible',
            err: err
          });
        }
      }
      return;
    }
    getMedia.apply(navigator, [mediaOption, onSuccess, onFailure]);
  }

  WoogeenLocalStream.create = function() {
    createLocalStream.apply(this, arguments);
  };

  Woogeen.Stream = WoogeenStream;
  Woogeen.LocalStream = WoogeenLocalStream;
  Woogeen.RemoteStream = WoogeenRemoteStream;

  // return {
  //   Stream: WoogeenStream,
  //   LocalStream: WoogeenLocalStream,
  //   RemoteStream: WoogeenRemoteStream
  // };
}());


/*
 * Class EventDispatcher provides event handling to sub-classes.
 * It is inherited from Publisher, Room, etc.
 */
Woogeen.EventDispatcher = function (spec) {
  'use strict';
  var that = {};
  // Private vars
  spec.dispatcher = {};
  spec.dispatcher.eventListeners = {};

  // Public functions

  // It adds an event listener attached to an event type.
  that.addEventListener = function (eventType, listener) {
    if (spec.dispatcher.eventListeners[eventType] === undefined) {
      spec.dispatcher.eventListeners[eventType] = [];
    }
    spec.dispatcher.eventListeners[eventType].push(listener);
  };

  that.on = that.addEventListener;

  // It removes an available event listener.
  that.removeEventListener = function (eventType, listener) {
    if (!spec.dispatcher.eventListeners[eventType]) {return;}
    var index = spec.dispatcher.eventListeners[eventType].indexOf(listener);
    if (index !== -1) {
      spec.dispatcher.eventListeners[eventType].splice(index, 1);
    }
  };

  // It removes all event listeners for one type.
  that.clearEventListener = function (eventType) {
    spec.dispatcher.eventListeners[eventType] = [];
  };

  // It dispatch a new event to the event listeners, based on the type
  // of event. All events are intended to be LicodeEvents.
  that.dispatchEvent = function (event) {
    if (!spec.dispatcher.eventListeners[event.type]) {return;}
    spec.dispatcher.eventListeners[event.type].map(function (listener) {
      listener(event);
    });
  };

  return that;
};

// **** EVENTS ****

function WoogeenEvent (spec) { // base event class
  'use strict';
  this.type = spec.type;
  this.attributes = spec.attributes;
  this.label = spec.label;
}

/*
 * Class StreamEvent represents an event related to a stream.
 * It is usually initialized this way:
 * var streamEvent = StreamEvent({type:'stream-added', stream:stream1});
 * Event types:
 * 'stream-added' - indicates that there is a new stream available in the room.
 * 'stream-removed' - shows that a previous available stream has been removed from the room.
 */
Woogeen.StreamEvent = function WoogeenStreamEvent (spec) {
  'use strict';
  WoogeenEvent.call(this, spec);
  this.stream = spec.stream;
  this.msg = spec.msg;
};

/*
 * Class ClientEvent represents an event related to a client.
 * It is usually initialized this way:
 * var clientEvent = ClientEvent({type:'peer-left', user: user1, attr: attributes});
 * Event types:
 * 'client-disconnected' - shows that the user has been already disconnected.
 * 'peer-joined' - indicates that there is a new peer joined.
 * 'peer-left' - indicates that a peer has left.
 */
Woogeen.ClientEvent = function WoogeenClientEvent (spec) {
  'use strict';
  WoogeenEvent.call(this, spec);
  this.user = spec.user;
};

// inheritance
Woogeen.StreamEvent.prototype = Object.create(WoogeenEvent.prototype);
Woogeen.StreamEvent.prototype.constructor = Woogeen.StreamEvent;
Woogeen.ClientEvent.prototype = Object.create(WoogeenEvent.prototype);
Woogeen.ClientEvent.prototype.constructor = Woogeen.ClientEvent;


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
          if (stream) {
            stream.close(); // >removeStream<
            delete self.remoteStreams[spec.id];
            var evt = new Woogeen.StreamEvent({type: 'stream-removed', stream: stream});
            self.dispatchEvent(evt);
          }
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
          safeCall(onFailure, err || 'connection_failed');
        });

        self.socket.on('error', function (err) {
          safeCall(onFailure, err || 'connection_error');
        });
      }

      sendMsg(self.socket, 'token', token, function (err, resp) {
        if (err) {
          return safeCall(onFailure, err);
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
        safeCall(onSuccess, {streams: streams});
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

  WoogeenConference.prototype.send = function (data, receiver, onSuccess, onFailure) {
    if (data === undefined || data === null || typeof data === 'function') {
      return safeCall(onFailure, 'nothing to send');
    }
    if (typeof receiver === 'undefined') {
      receiver = 0; // 0 => ALL
    } else if (typeof receiver === 'number') {
      // supposed to be a valid receiverId.
      // pass.
    } else if (typeof receiver === 'function') {
      onFailure = onSuccess;
      onSuccess = receiver;
      receiver = 0;
    } else {
      return safeCall(onFailure, 'invalid receiver');
    }
    sendMsg(this.socket, 'customMessage', {
      data: data,
      receiver: receiver
    }, function (err, resp) {
      if (err) {
        return safeCall(onFailure, err);
      }
      safeCall(onSuccess, resp);
    });
  };

  WoogeenConference.prototype.publish = function (stream, options, onSuccess, onFailure) {
    var self = this;
    if (!(stream instanceof Woogeen.LocalStream) ||
      (typeof stream.mediaStream !== 'object' || stream.mediaStream === null)) {
      return safeCall(onFailure, 'invalid stream');
    }

    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
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
              return safeCall(onFailure, id);
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
              safeCall(onSuccess, stream);
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
                safeCall(onFailure, 'peer connection failed');
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
      return safeCall(onFailure, 'already published');
    }
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
      safeCall(onSuccess, null);
    });
  };

  WoogeenConference.prototype.subscribe = function (stream, options, onSuccess, onFailure) {
    var self = this;
    if (!(stream instanceof Woogeen.RemoteStream)) {
      return safeCall(onFailure, 'invalid stream');
    }
    if (typeof options === 'function') {
      onFailure = onSuccess;
      onSuccess = options;
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
            return safeCall(onFailure, answer);
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
      safeCall(onSuccess, stream);
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

  WoogeenConference.prototype.shareScreen = function () {
    // this.stopShare = function () {};
  };

  WoogeenConference.create = function factory (spec) { // factory, not in prototype
    return new WoogeenConference(spec);
  };

  return WoogeenConference;
}());


/* global window, RTCSessionDescription, webkitRTCPeerConnection */

Erizo.ChromeCanaryStack = function (spec) {
    'use strict';

    var that = {},
        WebkitRTCPeerConnection = webkitRTCPeerConnection;

    that.pc_config = {
        iceServers: []
    };

    that.con = {'optional': [{'DtlsSrtpKeyAgreement': true}]};

    if (spec.iceServers instanceof Array) {
        that.pc_config.iceServers = spec.iceServers;
    } else {
        if (spec.stunServerUrl) {
            if (spec.stunServerUrl instanceof Array) {
                spec.stunServerUrl.map(function (url) {
                    if (typeof url === 'string' && url !== '') {
                        that.pc_config.iceServers.push({url: url});
                    }
                });
            } else if (typeof spec.stunServerUrl === 'string' && spec.stunServerUrl !== '') {
                that.pc_config.iceServers.push({url: spec.stunServerUrl});
            }
        }

        if (spec.turnServer) {
            if (spec.turnServer instanceof Array) {
                spec.turnServer.map(function (turn) {
                    if (typeof turn.url === 'string' && turn.url !== '') {
                        that.pc_config.iceServers.push({
                            username: turn.username,
                            credential: turn.password,
                            url: turn.url
                        });
                    }
                });
            } else if (typeof spec.turnServer.url === 'string' && spec.turnServer.url !== '') {
                that.pc_config.iceServers.push({
                    username: spec.turnServer.username,
                    credential: spec.turnServer.password,
                    url: spec.turnServer.url
                });
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
        'mandatory': {
            'OfferToReceiveVideo': spec.video,
            'OfferToReceiveAudio': spec.audio
        }
    };

    that.roapSessionId = 103;

    that.peerConnection = new WebkitRTCPeerConnection(that.pc_config, that.con);

    that.peerConnection.onicecandidate = function (event) {
        // L.Logger.debug("PeerConnection: ", spec.session_id);
        if (!event.candidate) {
            // At the moment, we do not renegotiate when new candidates
            // show up after the more flag has been false once.
            // L.Logger.debug("State: " + that.peerConnection.iceGatheringState);

            if (that.ices === undefined) {
                that.ices = 0;
            }
            that.ices = that.ices + 1;
            if (that.ices >= 1 && that.moreIceComing) {
                that.moreIceComing = false;
                that.markActionNeeded();
            }
        } else {
            that.iceCandidateCount += 1;
        }
    };

    //L.Logger.debug("Created webkitRTCPeerConnnection with config \"" + JSON.stringify(that.pc_config) + "\".");

    var setMaxBW = function (sdp) {
        var a, r;
        if (spec.maxVideoBW) {
            a = sdp.match(/m=video.*\r\n/);
            r = a[0] + 'b=AS:' + spec.maxVideoBW + '\r\n';
            sdp = sdp.replace(a[0], r);
        }

        if (spec.maxAudioBW) {
            a = sdp.match(/m=audio.*\r\n/);
            r = a[0] + 'b=AS:' + spec.maxAudioBW + '\r\n';
            sdp = sdp.replace(a[0], r);
        }

        return sdp;
    };

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
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

                that.state = 'offer-received';
                // Allow other stuff to happen, then reply.
                that.markActionNeeded();
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }

        } else if (that.state === 'offer-sent') {
            if (msg.messageType === 'ANSWER') {

                sd = {
                    sdp: msg.sdp,
                    type: 'answer'
                };
                L.Logger.debug('Received ANSWER: ', sd.sdp);

                sd.sdp = setMaxBW(sd.sdp);

                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));
                that.sendOK();
                that.state = 'established';

            } else if (msg.messageType === 'pr-answer') {
                sd = {
                    sdp: msg.sdp,
                    type: 'pr-answer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

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
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

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

                that.peerConnection.createOffer(function (sessionDescription) {

                    //sessionDescription.sdp = newOffer.replace(/a=ice-options:google-ice\r\n/g, "");
                    //sessionDescription.sdp = newOffer.replace(/a=crypto:0 AES_CM_128_HMAC_SHA1_80 inline:.*\r\n/g, "a=crypto:0 AES_CM_128_HMAC_SHA1_80 inline:eUMxlV2Ib6U8qeZot/wEKHw9iMzfKUYpOPJrNnu3\r\n");
                    //sessionDescription.sdp = newOffer.replace(/a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:.*\r\n/g, "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:eUMxlV2Ib6U8qeZot/wEKHw9iMzfKUYpOPJrNnu3\r\n");

                    sessionDescription.sdp = setMaxBW(sessionDescription.sdp);
                    L.Logger.debug('Changed', sessionDescription.sdp);

                    var newOffer = sessionDescription.sdp;

                    if (newOffer !== that.prevOffer) {

                        that.peerConnection.setLocalDescription(sessionDescription);

                        that.state = 'preparing-offer';
                        that.markActionNeeded();
                        return;
                    } else {
                        L.Logger.debug('Not sending a new offer');
                    }

                }, null, that.mediaConstraints);


            } else if (that.state === 'preparing-offer') {
                // Don't do anything until we have the ICE candidates.
                if (that.moreIceComing) {
                    return;
                }


                // Now able to send the offer we've already prepared.
                that.prevOffer = that.peerConnection.localDescription.sdp;
                L.Logger.debug('Sending OFFER: ' + that.prevOffer);
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

                }, null, that.mediaConstraints);

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



/* global window, RTCSessionDescription, webkitRTCPeerConnection */

Erizo.ChromeStableStack = function (spec) {
    'use strict';

    var that = {},
        WebkitRTCPeerConnection = webkitRTCPeerConnection;

    that.pc_config = {
        iceServers: []
    };

    that.con = {'optional': [{'DtlsSrtpKeyAgreement': true}]};

    if (spec.iceServers instanceof Array) {
        that.pc_config.iceServers = spec.iceServers;
    } else {
        if (spec.stunServerUrl) {
            if (spec.stunServerUrl instanceof Array) {
                spec.stunServerUrl.map(function (url) {
                    if (typeof url === 'string' && url !== '') {
                        that.pc_config.iceServers.push({url: url});
                    }
                });
            } else if (typeof spec.stunServerUrl === 'string' && spec.stunServerUrl !== '') {
                that.pc_config.iceServers.push({url: spec.stunServerUrl});
            }
        }

        if (spec.turnServer) {
            if (spec.turnServer instanceof Array) {
                spec.turnServer.map(function (turn) {
                    if (typeof turn.url === 'string' && turn.url !== '') {
                        that.pc_config.iceServers.push({
                            username: turn.username,
                            credential: turn.password,
                            url: turn.url
                        });
                    }
                });
            } else if (typeof spec.turnServer.url === 'string' && spec.turnServer.url !== '') {
                that.pc_config.iceServers.push({
                    username: spec.turnServer.username,
                    credential: spec.turnServer.password,
                    url: spec.turnServer.url
                });
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
        'mandatory': {
            'OfferToReceiveVideo': spec.video,
            'OfferToReceiveAudio': spec.audio
        }
    };

    that.roapSessionId = 103;

    that.peerConnection = new WebkitRTCPeerConnection(that.pc_config, that.con);

    that.peerConnection.onicecandidate = function (event) {
        L.Logger.debug('PeerConnection: ', spec.session_id);
        if (!event.candidate) {
            // At the moment, we do not renegotiate when new candidates
            // show up after the more flag has been false once.
            L.Logger.debug('State: ' + that.peerConnection.iceGatheringState);

            if (that.ices === undefined) {
                that.ices = 0;
            }
            that.ices = that.ices + 1;
            if (that.ices >= 1 && that.moreIceComing) {
                that.moreIceComing = false;
                that.markActionNeeded();
            }
        } else {
            that.iceCandidateCount += 1;
        }
    };

    //L.Logger.debug("Created webkitRTCPeerConnnection with config \"" + JSON.stringify(that.pc_config) + "\".");

    var setMaxBW = function (sdp) {
        var a, r;
        if (spec.maxVideoBW) {
            a = sdp.match(/m=video.*\r\n/);
            r = a[0] + 'b=AS:' + spec.maxVideoBW + '\r\n';
            sdp = sdp.replace(a[0], r);
        }

        if (spec.maxAudioBW) {
            a = sdp.match(/m=audio.*\r\n/);
            r = a[0] + 'b=AS:' + spec.maxAudioBW + '\r\n';
            sdp = sdp.replace(a[0], r);
        }

        return sdp;
    };

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
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

                that.state = 'offer-received';
                // Allow other stuff to happen, then reply.
                that.markActionNeeded();
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }

        } else if (that.state === 'offer-sent') {
            if (msg.messageType === 'ANSWER') {

                sd = {
                    sdp: msg.sdp,
                    type: 'answer'
                };
                L.Logger.debug('Received ANSWER: ', sd.sdp);

                sd.sdp = setMaxBW(sd.sdp);

                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));
                that.sendOK();
                that.state = 'established';

            } else if (msg.messageType === 'pr-answer') {
                sd = {
                    sdp: msg.sdp,
                    type: 'pr-answer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

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
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd));

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

                that.peerConnection.createOffer(function (sessionDescription) {

                    //sessionDescription.sdp = newOffer.replace(/a=ice-options:google-ice\r\n/g, "");
                    //sessionDescription.sdp = newOffer.replace(/a=crypto:0 AES_CM_128_HMAC_SHA1_80 inline:.*\r\n/g, "a=crypto:0 AES_CM_128_HMAC_SHA1_80 inline:eUMxlV2Ib6U8qeZot/wEKHw9iMzfKUYpOPJrNnu3\r\n");
                    //sessionDescription.sdp = newOffer.replace(/a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:.*\r\n/g, "a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:eUMxlV2Ib6U8qeZot/wEKHw9iMzfKUYpOPJrNnu3\r\n");

                    sessionDescription.sdp = setMaxBW(sessionDescription.sdp);
                    L.Logger.debug('Changed', sessionDescription.sdp);

                    var newOffer = sessionDescription.sdp;

                    if (newOffer !== that.prevOffer) {

                        that.peerConnection.setLocalDescription(sessionDescription);

                        that.state = 'preparing-offer';
                        that.markActionNeeded();
                        return;
                    } else {
                        L.Logger.debug('Not sending a new offer');
                    }

                }, null, that.mediaConstraints);


            } else if (that.state === 'preparing-offer') {
                // Don't do anything until we have the ICE candidates.
                if (that.moreIceComing) {
                    return;
                }


                // Now able to send the offer we've already prepared.
                that.prevOffer = that.peerConnection.localDescription.sdp;
                L.Logger.debug('Sending OFFER: ' + that.prevOffer);
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

                }, null, that.mediaConstraints);

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



/* global window, mozRTCSessionDescription, mozRTCPeerConnection*/

Erizo.FirefoxStack = function (spec) {
    'use strict';

    var that = {},
        WebkitRTCPeerConnection = mozRTCPeerConnection,
        RTCSessionDescription = mozRTCSessionDescription;

    var hasStream = false;

    that.pc_config = {
        iceServers: []
    };

    // currently firefox does not support turn
    if (spec.iceServers instanceof Array) {
        spec.iceServers.map(function (server) {
            if (server.url.indexOf('stun:') === 0) {
                that.pc_config.iceServers.push({url: server.url});
            }
        });
    } else {
        if (spec.stunServerUrl) {
            if (spec.stunServerUrl instanceof Array) {
                spec.stunServerUrl.map(function (url) {
                    if (typeof url === 'string' && url !== '') {
                        that.pc_config.iceServers.push({url: url});
                    }
                });
            } else if (typeof spec.stunServerUrl === 'string' && spec.stunServerUrl !== '') {
                that.pc_config.iceServers.push({url: spec.stunServerUrl});
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
        optional: [],
        mandatory: {
            OfferToReceiveAudio: spec.audio,
            OfferToReceiveVideo: spec.video,
            MozDontOfferDataChannel: true
        }
    };

    that.roapSessionId = 103;

    that.peerConnection = new WebkitRTCPeerConnection(that.pc_config);

    that.peerConnection.onicecandidate = function (event) {
        L.Logger.debug('PeerConnection: ', spec.session_id);
        if (!event.candidate) {
            // At the moment, we do not renegotiate when new candidates
            // show up after the more flag has been false once.
            L.Logger.debug('State: ' + that.peerConnection.iceGatheringState);

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

    // L.Logger.debug("Created webkitRTCPeerConnnection with config \"" + JSON.stringify(that.pc_config) + "\".");

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
                        L.Logger.debug('setRemoteDescription succeeded');
                    }, function(error) {
                        L.Logger.info('setRemoteDescription failed: ' + error.name);
                    });

                that.state = 'offer-received';
                // Allow other stuff to happen, then reply.
                that.markActionNeeded();
            } else {
                that.error('Illegal message for this state: ' + msg.messageType + ' in state ' + that.state);
            }

        } else if (that.state === 'offer-sent') {
            if (msg.messageType === 'ANSWER') {

                msg.sdp = msg.sdp.replace(/ generation 0/g, '');
                msg.sdp = msg.sdp.replace(/ udp /g, ' UDP ');

                sd = {
                    sdp: msg.sdp,
                    type: 'answer'
                };
                L.Logger.debug('Received ANSWER: ', sd.sdp);
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug('setRemoteDescription succeeded');
                    }, function(error) {
                        L.Logger.info('setRemoteDescription failed: ' + error.name);
                    });
                that.sendOK();
                that.state = 'established';

            } else if (msg.messageType === 'pr-answer') {
                sd = {
                    sdp: msg.sdp,
                    type: 'pr-answer'
                };
                that.peerConnection.setRemoteDescription(new RTCSessionDescription(sd), function() {
                        L.Logger.debug('setRemoteDescription succeeded');
                    }, function(error) {
                        L.Logger.info('setRemoteDescription failed: ' + error.name);
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
                        L.Logger.debug('setRemoteDescription succeeded');
                    }, function(error) {
                        L.Logger.info('setRemoteDescription failed: ' + error.name);
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
                L.Logger.debug('Creating offer');
                var onSuccess = function() {
                    that.peerConnection.createOffer(function (sessionDescription) {

                        var newOffer = sessionDescription.sdp;

                        L.Logger.debug('Changed', sessionDescription.sdp);

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
                        L.Logger.debug('Ups! Something went wrong ', error);

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
                L.Logger.debug('Sending OFFER: ', that.prevOffer);
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
                    L.Logger.debug('Ups! Something went wrong');

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



window.Erizo = Erizo;
window.Woogeen = Woogeen;
window.L = L;
}(window));



/*! Socket.IO.js build:0.9.6, development. Copyright(c) 2011 LearnBoost <dev@learnboost.com> MIT Licensed */

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, global) {

  /**
   * IO namespace.
   *
   * @namespace
   */

  var io = exports;

  /**
   * Socket.IO version
   *
   * @api public
   */

  io.version = '0.9.6';

  /**
   * Protocol implemented.
   *
   * @api public
   */

  io.protocol = 1;

  /**
   * Available transports, these will be populated with the available transports
   *
   * @api public
   */

  io.transports = [];

  /**
   * Keep track of jsonp callbacks.
   *
   * @api private
   */

  io.j = [];

  /**
   * Keep track of our io.Sockets
   *
   * @api private
   */
  io.sockets = {};


  /**
   * Manages connections to hosts.
   *
   * @param {String} uri
   * @Param {Boolean} force creation of new socket (defaults to false)
   * @api public
   */

  io.connect = function (host, details) {
    var uri = io.util.parseUri(host)
      , uuri
      , socket;

    if (global && global.location) {
      uri.protocol = uri.protocol || global.location.protocol.slice(0, -1);
      uri.host = uri.host || (global.document
        ? global.document.domain : global.location.hostname);
      uri.port = uri.port || global.location.port;
    }

    uuri = io.util.uniqueUri(uri);

    var options = {
        host: uri.host
      , secure: 'https' == uri.protocol
      , port: uri.port || ('https' == uri.protocol ? 443 : 80)
      , query: uri.query || ''
    };

    io.util.merge(options, details);

    if (options['force new connection'] || !io.sockets[uuri]) {
      socket = new io.Socket(options);
    }

    if (!options['force new connection'] && socket) {
      io.sockets[uuri] = socket;
    }

    socket = socket || io.sockets[uuri];

    // if path is different from '' or /
    return socket.of(uri.path.length > 1 ? uri.path : '');
  };

})('object' === typeof module ? module.exports : (this.io = {}), this);
/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, global) {

  /**
   * Utilities namespace.
   *
   * @namespace
   */

  var util = exports.util = {};

  /**
   * Parses an URI
   *
   * @author Steven Levithan <stevenlevithan.com> (MIT license)
   * @api public
   */

  var re = /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/;

  var parts = ['source', 'protocol', 'authority', 'userInfo', 'user', 'password',
               'host', 'port', 'relative', 'path', 'directory', 'file', 'query',
               'anchor'];

  util.parseUri = function (str) {
    var m = re.exec(str || '')
      , uri = {}
      , i = 14;

    while (i--) {
      uri[parts[i]] = m[i] || '';
    }

    return uri;
  };

  /**
   * Produces a unique url that identifies a Socket.IO connection.
   *
   * @param {Object} uri
   * @api public
   */

  util.uniqueUri = function (uri) {
    var protocol = uri.protocol
      , host = uri.host
      , port = uri.port;

    if ('document' in global) {
      host = host || document.domain;
      port = port || (protocol == 'https'
        && document.location.protocol !== 'https:' ? 443 : document.location.port);
    } else {
      host = host || 'localhost';

      if (!port && protocol == 'https') {
        port = 443;
      }
    }

    return (protocol || 'http') + '://' + host + ':' + (port || 80);
  };

  /**
   * Mergest 2 query strings in to once unique query string
   *
   * @param {String} base
   * @param {String} addition
   * @api public
   */

  util.query = function (base, addition) {
    var query = util.chunkQuery(base || '')
      , components = [];

    util.merge(query, util.chunkQuery(addition || ''));
    for (var part in query) {
      if (query.hasOwnProperty(part)) {
        components.push(part + '=' + query[part]);
      }
    }

    return components.length ? '?' + components.join('&') : '';
  };

  /**
   * Transforms a querystring in to an object
   *
   * @param {String} qs
   * @api public
   */

  util.chunkQuery = function (qs) {
    var query = {}
      , params = qs.split('&')
      , i = 0
      , l = params.length
      , kv;

    for (; i < l; ++i) {
      kv = params[i].split('=');
      if (kv[0]) {
        query[kv[0]] = kv[1];
      }
    }

    return query;
  };

  /**
   * Executes the given function when the page is loaded.
   *
   *     io.util.load(function () { console.log('page loaded'); });
   *
   * @param {Function} fn
   * @api public
   */

  var pageLoaded = false;

  util.load = function (fn) {
    if ('document' in global && document.readyState === 'complete' || pageLoaded) {
      return fn();
    }

    util.on(global, 'load', fn, false);
  };

  /**
   * Adds an event.
   *
   * @api private
   */

  util.on = function (element, event, fn, capture) {
    if (element.attachEvent) {
      element.attachEvent('on' + event, fn);
    } else if (element.addEventListener) {
      element.addEventListener(event, fn, capture);
    }
  };

  /**
   * Generates the correct `XMLHttpRequest` for regular and cross domain requests.
   *
   * @param {Boolean} [xdomain] Create a request that can be used cross domain.
   * @returns {XMLHttpRequest|false} If we can create a XMLHttpRequest.
   * @api private
   */

  util.request = function (xdomain) {

    if (xdomain && 'undefined' != typeof XDomainRequest) {
      return new XDomainRequest();
    }

    if ('undefined' != typeof XMLHttpRequest && (!xdomain || util.ua.hasCORS)) {
      return new XMLHttpRequest();
    }

    if (!xdomain) {
      try {
        return new window[(['Active'].concat('Object').join('X'))]('Microsoft.XMLHTTP');
      } catch(e) { }
    }

    return null;
  };

  /**
   * XHR based transport constructor.
   *
   * @constructor
   * @api public
   */

  /**
   * Change the internal pageLoaded value.
   */

  if ('undefined' != typeof window) {
    util.load(function () {
      pageLoaded = true;
    });
  }

  /**
   * Defers a function to ensure a spinner is not displayed by the browser
   *
   * @param {Function} fn
   * @api public
   */

  util.defer = function (fn) {
    if (!util.ua.webkit || 'undefined' != typeof importScripts) {
      return fn();
    }

    util.load(function () {
      setTimeout(fn, 100);
    });
  };

  /**
   * Merges two objects.
   *
   * @api public
   */
  
  util.merge = function merge (target, additional, deep, lastseen) {
    var seen = lastseen || []
      , depth = typeof deep == 'undefined' ? 2 : deep
      , prop;

    for (prop in additional) {
      if (additional.hasOwnProperty(prop) && util.indexOf(seen, prop) < 0) {
        if (typeof target[prop] !== 'object' || !depth) {
          target[prop] = additional[prop];
          seen.push(additional[prop]);
        } else {
          util.merge(target[prop], additional[prop], depth - 1, seen);
        }
      }
    }

    return target;
  };

  /**
   * Merges prototypes from objects
   *
   * @api public
   */
  
  util.mixin = function (ctor, ctor2) {
    util.merge(ctor.prototype, ctor2.prototype);
  };

  /**
   * Shortcut for prototypical and static inheritance.
   *
   * @api private
   */

  util.inherit = function (ctor, ctor2) {
    function f() {};
    f.prototype = ctor2.prototype;
    ctor.prototype = new f;
  };

  /**
   * Checks if the given object is an Array.
   *
   *     io.util.isArray([]); // true
   *     io.util.isArray({}); // false
   *
   * @param Object obj
   * @api public
   */

  util.isArray = Array.isArray || function (obj) {
    return Object.prototype.toString.call(obj) === '[object Array]';
  };

  /**
   * Intersects values of two arrays into a third
   *
   * @api public
   */

  util.intersect = function (arr, arr2) {
    var ret = []
      , longest = arr.length > arr2.length ? arr : arr2
      , shortest = arr.length > arr2.length ? arr2 : arr;

    for (var i = 0, l = shortest.length; i < l; i++) {
      if (~util.indexOf(longest, shortest[i]))
        ret.push(shortest[i]);
    }

    return ret;
  }

  /**
   * Array indexOf compatibility.
   *
   * @see bit.ly/a5Dxa2
   * @api public
   */

  util.indexOf = function (arr, o, i) {
    
    for (var j = arr.length, i = i < 0 ? i + j < 0 ? 0 : i + j : i || 0; 
         i < j && arr[i] !== o; i++) {}

    return j <= i ? -1 : i;
  };

  /**
   * Converts enumerables to array.
   *
   * @api public
   */

  util.toArray = function (enu) {
    var arr = [];

    for (var i = 0, l = enu.length; i < l; i++)
      arr.push(enu[i]);

    return arr;
  };

  /**
   * UA / engines detection namespace.
   *
   * @namespace
   */

  util.ua = {};

  /**
   * Whether the UA supports CORS for XHR.
   *
   * @api public
   */

  util.ua.hasCORS = 'undefined' != typeof XMLHttpRequest && (function () {
    try {
      var a = new XMLHttpRequest();
    } catch (e) {
      return false;
    }

    return a.withCredentials != undefined;
  })();

  /**
   * Detect webkit.
   *
   * @api public
   */

  util.ua.webkit = 'undefined' != typeof navigator
    && /webkit/i.test(navigator.userAgent);

})('undefined' != typeof io ? io : module.exports, this);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Expose constructor.
   */

  exports.EventEmitter = EventEmitter;

  /**
   * Event emitter constructor.
   *
   * @api public.
   */

  function EventEmitter () {};

  /**
   * Adds a listener
   *
   * @api public
   */

  EventEmitter.prototype.on = function (name, fn) {
    if (!this.$events) {
      this.$events = {};
    }

    if (!this.$events[name]) {
      this.$events[name] = fn;
    } else if (io.util.isArray(this.$events[name])) {
      this.$events[name].push(fn);
    } else {
      this.$events[name] = [this.$events[name], fn];
    }

    return this;
  };

  EventEmitter.prototype.addListener = EventEmitter.prototype.on;

  /**
   * Adds a volatile listener.
   *
   * @api public
   */

  EventEmitter.prototype.once = function (name, fn) {
    var self = this;

    function on () {
      self.removeListener(name, on);
      fn.apply(this, arguments);
    };

    on.listener = fn;
    this.on(name, on);

    return this;
  };

  /**
   * Removes a listener.
   *
   * @api public
   */

  EventEmitter.prototype.removeListener = function (name, fn) {
    if (this.$events && this.$events[name]) {
      var list = this.$events[name];

      if (io.util.isArray(list)) {
        var pos = -1;

        for (var i = 0, l = list.length; i < l; i++) {
          if (list[i] === fn || (list[i].listener && list[i].listener === fn)) {
            pos = i;
            break;
          }
        }

        if (pos < 0) {
          return this;
        }

        list.splice(pos, 1);

        if (!list.length) {
          delete this.$events[name];
        }
      } else if (list === fn || (list.listener && list.listener === fn)) {
        delete this.$events[name];
      }
    }

    return this;
  };

  /**
   * Removes all listeners for an event.
   *
   * @api public
   */

  EventEmitter.prototype.removeAllListeners = function (name) {
    // TODO: enable this when node 0.5 is stable
    //if (name === undefined) {
      //this.$events = {};
      //return this;
    //}

    if (this.$events && this.$events[name]) {
      this.$events[name] = null;
    }

    return this;
  };

  /**
   * Gets all listeners for a certain event.
   *
   * @api publci
   */

  EventEmitter.prototype.listeners = function (name) {
    if (!this.$events) {
      this.$events = {};
    }

    if (!this.$events[name]) {
      this.$events[name] = [];
    }

    if (!io.util.isArray(this.$events[name])) {
      this.$events[name] = [this.$events[name]];
    }

    return this.$events[name];
  };

  /**
   * Emits an event.
   *
   * @api public
   */

  EventEmitter.prototype.emit = function (name) {
    if (!this.$events) {
      return false;
    }

    var handler = this.$events[name];

    if (!handler) {
      return false;
    }

    var args = Array.prototype.slice.call(arguments, 1);

    if ('function' == typeof handler) {
      handler.apply(this, args);
    } else if (io.util.isArray(handler)) {
      var listeners = handler.slice();

      for (var i = 0, l = listeners.length; i < l; i++) {
        listeners[i].apply(this, args);
      }
    } else {
      return false;
    }

    return true;
  };

})(
    'undefined' != typeof io ? io : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

/**
 * Based on JSON2 (http://www.JSON.org/js.html).
 */

(function (exports, nativeJSON) {
  "use strict";

  // use native JSON if it's available
  if (nativeJSON && nativeJSON.parse){
    return exports.JSON = {
      parse: nativeJSON.parse
    , stringify: nativeJSON.stringify
    }
  }

  var JSON = exports.JSON = {};

  function f(n) {
      // Format integers to have at least two digits.
      return n < 10 ? '0' + n : n;
  }

  function date(d, key) {
    return isFinite(d.valueOf()) ?
        d.getUTCFullYear()     + '-' +
        f(d.getUTCMonth() + 1) + '-' +
        f(d.getUTCDate())      + 'T' +
        f(d.getUTCHours())     + ':' +
        f(d.getUTCMinutes())   + ':' +
        f(d.getUTCSeconds())   + 'Z' : null;
  };

  var cx = /[\u0000\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,
      escapable = /[\\\"\x00-\x1f\x7f-\x9f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g,
      gap,
      indent,
      meta = {    // table of character substitutions
          '\b': '\\b',
          '\t': '\\t',
          '\n': '\\n',
          '\f': '\\f',
          '\r': '\\r',
          '"' : '\\"',
          '\\': '\\\\'
      },
      rep;


  function quote(string) {

// If the string contains no control characters, no quote characters, and no
// backslash characters, then we can safely slap some quotes around it.
// Otherwise we must also replace the offending characters with safe escape
// sequences.

      escapable.lastIndex = 0;
      return escapable.test(string) ? '"' + string.replace(escapable, function (a) {
          var c = meta[a];
          return typeof c === 'string' ? c :
              '\\u' + ('0000' + a.charCodeAt(0).toString(16)).slice(-4);
      }) + '"' : '"' + string + '"';
  }


  function str(key, holder) {

// Produce a string from holder[key].

      var i,          // The loop counter.
          k,          // The member key.
          v,          // The member value.
          length,
          mind = gap,
          partial,
          value = holder[key];

// If the value has a toJSON method, call it to obtain a replacement value.

      if (value instanceof Date) {
          value = date(key);
      }

// If we were called with a replacer function, then call the replacer to
// obtain a replacement value.

      if (typeof rep === 'function') {
          value = rep.call(holder, key, value);
      }

// What happens next depends on the value's type.

      switch (typeof value) {
      case 'string':
          return quote(value);

      case 'number':

// JSON numbers must be finite. Encode non-finite numbers as null.

          return isFinite(value) ? String(value) : 'null';

      case 'boolean':
      case 'null':

// If the value is a boolean or null, convert it to a string. Note:
// typeof null does not produce 'null'. The case is included here in
// the remote chance that this gets fixed someday.

          return String(value);

// If the type is 'object', we might be dealing with an object or an array or
// null.

      case 'object':

// Due to a specification blunder in ECMAScript, typeof null is 'object',
// so watch out for that case.

          if (!value) {
              return 'null';
          }

// Make an array to hold the partial results of stringifying this object value.

          gap += indent;
          partial = [];

// Is the value an array?

          if (Object.prototype.toString.apply(value) === '[object Array]') {

// The value is an array. Stringify every element. Use null as a placeholder
// for non-JSON values.

              length = value.length;
              for (i = 0; i < length; i += 1) {
                  partial[i] = str(i, value) || 'null';
              }

// Join all of the elements together, separated with commas, and wrap them in
// brackets.

              v = partial.length === 0 ? '[]' : gap ?
                  '[\n' + gap + partial.join(',\n' + gap) + '\n' + mind + ']' :
                  '[' + partial.join(',') + ']';
              gap = mind;
              return v;
          }

// If the replacer is an array, use it to select the members to be stringified.

          if (rep && typeof rep === 'object') {
              length = rep.length;
              for (i = 0; i < length; i += 1) {
                  if (typeof rep[i] === 'string') {
                      k = rep[i];
                      v = str(k, value);
                      if (v) {
                          partial.push(quote(k) + (gap ? ': ' : ':') + v);
                      }
                  }
              }
          } else {

// Otherwise, iterate through all of the keys in the object.

              for (k in value) {
                  if (Object.prototype.hasOwnProperty.call(value, k)) {
                      v = str(k, value);
                      if (v) {
                          partial.push(quote(k) + (gap ? ': ' : ':') + v);
                      }
                  }
              }
          }

// Join all of the member texts together, separated with commas,
// and wrap them in braces.

          v = partial.length === 0 ? '{}' : gap ?
              '{\n' + gap + partial.join(',\n' + gap) + '\n' + mind + '}' :
              '{' + partial.join(',') + '}';
          gap = mind;
          return v;
      }
  }

// If the JSON object does not yet have a stringify method, give it one.

  JSON.stringify = function (value, replacer, space) {

// The stringify method takes a value and an optional replacer, and an optional
// space parameter, and returns a JSON text. The replacer can be a function
// that can replace values, or an array of strings that will select the keys.
// A default replacer method can be provided. Use of the space parameter can
// produce text that is more easily readable.

      var i;
      gap = '';
      indent = '';

// If the space parameter is a number, make an indent string containing that
// many spaces.

      if (typeof space === 'number') {
          for (i = 0; i < space; i += 1) {
              indent += ' ';
          }

// If the space parameter is a string, it will be used as the indent string.

      } else if (typeof space === 'string') {
          indent = space;
      }

// If there is a replacer, it must be a function or an array.
// Otherwise, throw an error.

      rep = replacer;
      if (replacer && typeof replacer !== 'function' &&
              (typeof replacer !== 'object' ||
              typeof replacer.length !== 'number')) {
          throw new Error('JSON.stringify');
      }

// Make a fake root object containing our value under the key of ''.
// Return the result of stringifying the value.

      return str('', {'': value});
  };

// If the JSON object does not yet have a parse method, give it one.

  JSON.parse = function (text, reviver) {
  // The parse method takes a text and an optional reviver function, and returns
  // a JavaScript value if the text is a valid JSON text.

      var j;

      function walk(holder, key) {

  // The walk method is used to recursively walk the resulting structure so
  // that modifications can be made.

          var k, v, value = holder[key];
          if (value && typeof value === 'object') {
              for (k in value) {
                  if (Object.prototype.hasOwnProperty.call(value, k)) {
                      v = walk(value, k);
                      if (v !== undefined) {
                          value[k] = v;
                      } else {
                          delete value[k];
                      }
                  }
              }
          }
          return reviver.call(holder, key, value);
      }


  // Parsing happens in four stages. In the first stage, we replace certain
  // Unicode characters with escape sequences. JavaScript handles many characters
  // incorrectly, either silently deleting them, or treating them as line endings.

      text = String(text);
      cx.lastIndex = 0;
      if (cx.test(text)) {
          text = text.replace(cx, function (a) {
              return '\\u' +
                  ('0000' + a.charCodeAt(0).toString(16)).slice(-4);
          });
      }

  // In the second stage, we run the text against regular expressions that look
  // for non-JSON patterns. We are especially concerned with '()' and 'new'
  // because they can cause invocation, and '=' because it can cause mutation.
  // But just to be safe, we want to reject all unexpected forms.

  // We split the second stage into 4 regexp operations in order to work around
  // crippling inefficiencies in IE's and Safari's regexp engines. First we
  // replace the JSON backslash pairs with '@' (a non-JSON character). Second, we
  // replace all simple value tokens with ']' characters. Third, we delete all
  // open brackets that follow a colon or comma or that begin the text. Finally,
  // we look to see that the remaining characters are only whitespace or ']' or
  // ',' or ':' or '{' or '}'. If that is so, then the text is safe for eval.

      if (/^[\],:{}\s]*$/
              .test(text.replace(/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g, '@')
                  .replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g, ']')
                  .replace(/(?:^|:|,)(?:\s*\[)+/g, ''))) {

  // In the third stage we use the eval function to compile the text into a
  // JavaScript structure. The '{' operator is subject to a syntactic ambiguity
  // in JavaScript: it can begin a block or an object literal. We wrap the text
  // in parens to eliminate the ambiguity.

          j = eval('(' + text + ')');

  // In the optional fourth stage, we recursively walk the new structure, passing
  // each name/value pair to a reviver function for possible transformation.

          return typeof reviver === 'function' ?
              walk({'': j}, '') : j;
      }

  // If the text is not JSON parseable, then a SyntaxError is thrown.

      throw new SyntaxError('JSON.parse');
  };

})(
    'undefined' != typeof io ? io : module.exports
  , typeof JSON !== 'undefined' ? JSON : undefined
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Parser namespace.
   *
   * @namespace
   */

  var parser = exports.parser = {};

  /**
   * Packet types.
   */

  var packets = parser.packets = [
      'disconnect'
    , 'connect'
    , 'heartbeat'
    , 'message'
    , 'json'
    , 'event'
    , 'ack'
    , 'error'
    , 'noop'
  ];

  /**
   * Errors reasons.
   */

  var reasons = parser.reasons = [
      'transport not supported'
    , 'client not handshaken'
    , 'unauthorized'
  ];

  /**
   * Errors advice.
   */

  var advice = parser.advice = [
      'reconnect'
  ];

  /**
   * Shortcuts.
   */

  var JSON = io.JSON
    , indexOf = io.util.indexOf;

  /**
   * Encodes a packet.
   *
   * @api private
   */

  parser.encodePacket = function (packet) {
    var type = indexOf(packets, packet.type)
      , id = packet.id || ''
      , endpoint = packet.endpoint || ''
      , ack = packet.ack
      , data = null;

    switch (packet.type) {
      case 'error':
        var reason = packet.reason ? indexOf(reasons, packet.reason) : ''
          , adv = packet.advice ? indexOf(advice, packet.advice) : '';

        if (reason !== '' || adv !== '')
          data = reason + (adv !== '' ? ('+' + adv) : '');

        break;

      case 'message':
        if (packet.data !== '')
          data = packet.data;
        break;

      case 'event':
        var ev = { name: packet.name };

        if (packet.args && packet.args.length) {
          ev.args = packet.args;
        }

        data = JSON.stringify(ev);
        break;

      case 'json':
        data = JSON.stringify(packet.data);
        break;

      case 'connect':
        if (packet.qs)
          data = packet.qs;
        break;

      case 'ack':
        data = packet.ackId
          + (packet.args && packet.args.length
              ? '+' + JSON.stringify(packet.args) : '');
        break;
    }

    // construct packet with required fragments
    var encoded = [
        type
      , id + (ack == 'data' ? '+' : '')
      , endpoint
    ];

    // data fragment is optional
    if (data !== null && data !== undefined)
      encoded.push(data);

    return encoded.join(':');
  };

  /**
   * Encodes multiple messages (payload).
   *
   * @param {Array} messages
   * @api private
   */

  parser.encodePayload = function (packets) {
    var decoded = '';

    if (packets.length == 1)
      return packets[0];

    for (var i = 0, l = packets.length; i < l; i++) {
      var packet = packets[i];
      decoded += '\ufffd' + packet.length + '\ufffd' + packets[i];
    }

    return decoded;
  };

  /**
   * Decodes a packet
   *
   * @api private
   */

  var regexp = /([^:]+):([0-9]+)?(\+)?:([^:]+)?:?([\s\S]*)?/;

  parser.decodePacket = function (data) {
    var pieces = data.match(regexp);

    if (!pieces) return {};

    var id = pieces[2] || ''
      , data = pieces[5] || ''
      , packet = {
            type: packets[pieces[1]]
          , endpoint: pieces[4] || ''
        };

    // whether we need to acknowledge the packet
    if (id) {
      packet.id = id;
      if (pieces[3])
        packet.ack = 'data';
      else
        packet.ack = true;
    }

    // handle different packet types
    switch (packet.type) {
      case 'error':
        var pieces = data.split('+');
        packet.reason = reasons[pieces[0]] || '';
        packet.advice = advice[pieces[1]] || '';
        break;

      case 'message':
        packet.data = data || '';
        break;

      case 'event':
        try {
          var opts = JSON.parse(data);
          packet.name = opts.name;
          packet.args = opts.args;
        } catch (e) { }

        packet.args = packet.args || [];
        break;

      case 'json':
        try {
          packet.data = JSON.parse(data);
        } catch (e) { }
        break;

      case 'connect':
        packet.qs = data || '';
        break;

      case 'ack':
        var pieces = data.match(/^([0-9]+)(\+)?(.*)/);
        if (pieces) {
          packet.ackId = pieces[1];
          packet.args = [];

          if (pieces[3]) {
            try {
              packet.args = pieces[3] ? JSON.parse(pieces[3]) : [];
            } catch (e) { }
          }
        }
        break;

      case 'disconnect':
      case 'heartbeat':
        break;
    };

    return packet;
  };

  /**
   * Decodes data payload. Detects multiple messages
   *
   * @return {Array} messages
   * @api public
   */

  parser.decodePayload = function (data) {
    // IE doesn't like data[i] for unicode chars, charAt works fine
    if (data.charAt(0) == '\ufffd') {
      var ret = [];

      for (var i = 1, length = ''; i < data.length; i++) {
        if (data.charAt(i) == '\ufffd') {
          ret.push(parser.decodePacket(data.substr(i + 1).substr(0, length)));
          i += Number(length) + 1;
          length = '';
        } else {
          length += data.charAt(i);
        }
      }

      return ret;
    } else {
      return [parser.decodePacket(data)];
    }
  };

})(
    'undefined' != typeof io ? io : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);
/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Expose constructor.
   */

  exports.Transport = Transport;

  /**
   * This is the transport template for all supported transport methods.
   *
   * @constructor
   * @api public
   */

  function Transport (socket, sessid) {
    this.socket = socket;
    this.sessid = sessid;
  };

  /**
   * Apply EventEmitter mixin.
   */

  io.util.mixin(Transport, io.EventEmitter);

  /**
   * Handles the response from the server. When a new response is received
   * it will automatically update the timeout, decode the message and
   * forwards the response to the onMessage function for further processing.
   *
   * @param {String} data Response from the server.
   * @api private
   */

  Transport.prototype.onData = function (data) {
    this.clearCloseTimeout();
    
    // If the connection in currently open (or in a reopening state) reset the close 
    // timeout since we have just received data. This check is necessary so
    // that we don't reset the timeout on an explicitly disconnected connection.
    if (this.socket.connected || this.socket.connecting || this.socket.reconnecting) {
      this.setCloseTimeout();
    }

    if (data !== '') {
      // todo: we should only do decodePayload for xhr transports
      var msgs = io.parser.decodePayload(data);

      if (msgs && msgs.length) {
        for (var i = 0, l = msgs.length; i < l; i++) {
          this.onPacket(msgs[i]);
        }
      }
    }

    return this;
  };

  /**
   * Handles packets.
   *
   * @api private
   */

  Transport.prototype.onPacket = function (packet) {
    this.socket.setHeartbeatTimeout();

    if (packet.type == 'heartbeat') {
      return this.onHeartbeat();
    }

    if (packet.type == 'connect' && packet.endpoint == '') {
      this.onConnect();
    }

    if (packet.type == 'error' && packet.advice == 'reconnect') {
      this.open = false;
    }

    this.socket.onPacket(packet);

    return this;
  };

  /**
   * Sets close timeout
   *
   * @api private
   */
  
  Transport.prototype.setCloseTimeout = function () {
    if (!this.closeTimeout) {
      var self = this;

      this.closeTimeout = setTimeout(function () {
        self.onDisconnect();
      }, this.socket.closeTimeout);
    }
  };

  /**
   * Called when transport disconnects.
   *
   * @api private
   */

  Transport.prototype.onDisconnect = function () {
    if (this.close && this.open) this.close();
    this.clearTimeouts();
    this.socket.onDisconnect();
    return this;
  };

  /**
   * Called when transport connects
   *
   * @api private
   */

  Transport.prototype.onConnect = function () {
    this.socket.onConnect();
    return this;
  }

  /**
   * Clears close timeout
   *
   * @api private
   */

  Transport.prototype.clearCloseTimeout = function () {
    if (this.closeTimeout) {
      clearTimeout(this.closeTimeout);
      this.closeTimeout = null;
    }
  };

  /**
   * Clear timeouts
   *
   * @api private
   */

  Transport.prototype.clearTimeouts = function () {
    this.clearCloseTimeout();

    if (this.reopenTimeout) {
      clearTimeout(this.reopenTimeout);
    }
  };

  /**
   * Sends a packet
   *
   * @param {Object} packet object.
   * @api private
   */

  Transport.prototype.packet = function (packet) {
    this.send(io.parser.encodePacket(packet));
  };

  /**
   * Send the received heartbeat message back to server. So the server
   * knows we are still connected.
   *
   * @param {String} heartbeat Heartbeat response from the server.
   * @api private
   */

  Transport.prototype.onHeartbeat = function (heartbeat) {
    this.packet({ type: 'heartbeat' });
  };
 
  /**
   * Called when the transport opens.
   *
   * @api private
   */

  Transport.prototype.onOpen = function () {
    this.open = true;
    this.clearCloseTimeout();
    this.socket.onOpen();
  };

  /**
   * Notifies the base when the connection with the Socket.IO server
   * has been disconnected.
   *
   * @api private
   */

  Transport.prototype.onClose = function () {
    var self = this;

    /* FIXME: reopen delay causing a infinit loop
    this.reopenTimeout = setTimeout(function () {
      self.open();
    }, this.socket.options['reopen delay']);*/

    this.open = false;
    this.socket.onClose();
    this.onDisconnect();
  };

  /**
   * Generates a connection url based on the Socket.IO URL Protocol.
   * See <https://github.com/learnboost/socket.io-node/> for more details.
   *
   * @returns {String} Connection url
   * @api private
   */

  Transport.prototype.prepareUrl = function () {
    var options = this.socket.options;

    return this.scheme() + '://'
      + options.host + ':' + options.port + '/'
      + options.resource + '/' + io.protocol
      + '/' + this.name + '/' + this.sessid;
  };

  /**
   * Checks if the transport is ready to start a connection.
   *
   * @param {Socket} socket The socket instance that needs a transport
   * @param {Function} fn The callback
   * @api private
   */

  Transport.prototype.ready = function (socket, fn) {
    fn.call(this);
  };
})(
    'undefined' != typeof io ? io : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);
/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io, global) {

  /**
   * Expose constructor.
   */

  exports.Socket = Socket;

  /**
   * Create a new `Socket.IO client` which can establish a persistent
   * connection with a Socket.IO enabled server.
   *
   * @api public
   */

  function Socket (options) {
    this.options = {
        port: 80
      , secure: false
      , document: 'document' in global ? document : false
      , resource: 'socket.io'
      , transports: io.transports
      , 'connect timeout': 10000
      , 'try multiple transports': true
      , 'reconnect': true
      , 'reconnection delay': 500
      , 'reconnection limit': Infinity
      , 'reopen delay': 3000
      , 'max reconnection attempts': 10
      , 'sync disconnect on unload': true
      , 'auto connect': true
      , 'flash policy port': 10843
    };

    io.util.merge(this.options, options);

    this.connected = false;
    this.open = false;
    this.connecting = false;
    this.reconnecting = false;
    this.namespaces = {};
    this.buffer = [];
    this.doBuffer = false;

    if (this.options['sync disconnect on unload'] &&
        (!this.isXDomain() || io.util.ua.hasCORS)) {
      var self = this;

      io.util.on(global, 'unload', function () {
        self.disconnectSync();
      }, false);
    }

    if (this.options['auto connect']) {
      this.connect();
    }
};

  /**
   * Apply EventEmitter mixin.
   */

  io.util.mixin(Socket, io.EventEmitter);

  /**
   * Returns a namespace listener/emitter for this socket
   *
   * @api public
   */

  Socket.prototype.of = function (name) {
    if (!this.namespaces[name]) {
      this.namespaces[name] = new io.SocketNamespace(this, name);

      if (name !== '') {
        this.namespaces[name].packet({ type: 'connect' });
      }
    }

    return this.namespaces[name];
  };

  /**
   * Emits the given event to the Socket and all namespaces
   *
   * @api private
   */

  Socket.prototype.publish = function () {
    this.emit.apply(this, arguments);

    var nsp;

    for (var i in this.namespaces) {
      if (this.namespaces.hasOwnProperty(i)) {
        nsp = this.of(i);
        nsp.$emit.apply(nsp, arguments);
      }
    }
  };

  /**
   * Performs the handshake
   *
   * @api private
   */

  function empty () { };

  Socket.prototype.handshake = function (fn) {
    var self = this
      , options = this.options;

    function complete (data) {
      if (data instanceof Error) {
        self.onError(data.message);
      } else {
        fn.apply(null, data.split(':'));
      }
    };

    var url = [
          'http' + (options.secure ? 's' : '') + ':/'
        , options.host + ':' + options.port
        , options.resource
        , io.protocol
        , io.util.query(this.options.query, 't=' + +new Date)
      ].join('/');

    if (this.isXDomain() && !io.util.ua.hasCORS) {
      var insertAt = document.getElementsByTagName('script')[0]
        , script = document.createElement('script');

      script.src = url + '&jsonp=' + io.j.length;
      insertAt.parentNode.insertBefore(script, insertAt);

      io.j.push(function (data) {
        complete(data);
        script.parentNode.removeChild(script);
      });
    } else {
      var xhr = io.util.request();

      xhr.open('GET', url, true);
      xhr.withCredentials = true;
      xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
          xhr.onreadystatechange = empty;

          if (xhr.status == 200) {
            complete(xhr.responseText);
          } else {
            !self.reconnecting && self.onError(xhr.responseText);
          }
        }
      };
      xhr.send(null);
    }
  };

  /**
   * Find an available transport based on the options supplied in the constructor.
   *
   * @api private
   */

  Socket.prototype.getTransport = function (override) {
    var transports = override || this.transports, match;

    for (var i = 0, transport; transport = transports[i]; i++) {
      if (io.Transport[transport]
        && io.Transport[transport].check(this)
        && (!this.isXDomain() || io.Transport[transport].xdomainCheck())) {
        return new io.Transport[transport](this, this.sessionid);
      }
    }

    return null;
  };

  /**
   * Connects to the server.
   *
   * @param {Function} [fn] Callback.
   * @returns {io.Socket}
   * @api public
   */

  Socket.prototype.connect = function (fn) {
    if (this.connecting) {
      return this;
    }

    var self = this;

    this.handshake(function (sid, heartbeat, close, transports) {
      self.sessionid = sid;
      self.closeTimeout = close * 1000;
      self.heartbeatTimeout = heartbeat * 1000;
      self.transports = transports ? io.util.intersect(
          transports.split(',')
        , self.options.transports
      ) : self.options.transports;

      self.setHeartbeatTimeout();

      function connect (transports){
        if (self.transport) self.transport.clearTimeouts();

        self.transport = self.getTransport(transports);
        if (!self.transport) return self.publish('connect_failed');

        // once the transport is ready
        self.transport.ready(self, function () {
          self.connecting = true;
          self.publish('connecting', self.transport.name);
          self.transport.open();

          if (self.options['connect timeout']) {
            self.connectTimeoutTimer = setTimeout(function () {
              if (!self.connected) {
                self.connecting = false;

                if (self.options['try multiple transports']) {
                  if (!self.remainingTransports) {
                    self.remainingTransports = self.transports.slice(0);
                  }

                  var remaining = self.remainingTransports;

                  while (remaining.length > 0 && remaining.splice(0,1)[0] !=
                         self.transport.name) {}

                    if (remaining.length){
                      connect(remaining);
                    } else {
                      self.publish('connect_failed');
                    }
                }
              }
            }, self.options['connect timeout']);
          }
        });
      }

      connect(self.transports);

      self.once('connect', function (){
        clearTimeout(self.connectTimeoutTimer);

        fn && typeof fn == 'function' && fn();
      });
    });

    return this;
  };

  /**
   * Clears and sets a new heartbeat timeout using the value given by the
   * server during the handshake.
   *
   * @api private
   */

  Socket.prototype.setHeartbeatTimeout = function () {
    clearTimeout(this.heartbeatTimeoutTimer);

    var self = this;
    this.heartbeatTimeoutTimer = setTimeout(function () {
      self.transport.onClose();
    }, this.heartbeatTimeout);
  };

  /**
   * Sends a message.
   *
   * @param {Object} data packet.
   * @returns {io.Socket}
   * @api public
   */

  Socket.prototype.packet = function (data) {
    if (this.connected && !this.doBuffer) {
      this.transport.packet(data);
    } else {
      this.buffer.push(data);
    }

    return this;
  };

  /**
   * Sets buffer state
   *
   * @api private
   */

  Socket.prototype.setBuffer = function (v) {
    this.doBuffer = v;

    if (!v && this.connected && this.buffer.length) {
      this.transport.payload(this.buffer);
      this.buffer = [];
    }
  };

  /**
   * Disconnect the established connect.
   *
   * @returns {io.Socket}
   * @api public
   */

  Socket.prototype.disconnect = function () {
    if (this.connected || this.connecting) {
      if (this.open) {
        this.of('').packet({ type: 'disconnect' });
      }

      // handle disconnection immediately
      this.onDisconnect('booted');
    }

    return this;
  };

  /**
   * Disconnects the socket with a sync XHR.
   *
   * @api private
   */

  Socket.prototype.disconnectSync = function () {
    // ensure disconnection
    var xhr = io.util.request()
      , uri = this.resource + '/' + io.protocol + '/' + this.sessionid;

    xhr.open('GET', uri, true);

    // handle disconnection immediately
    this.onDisconnect('booted');
  };

  /**
   * Check if we need to use cross domain enabled transports. Cross domain would
   * be a different port or different domain name.
   *
   * @returns {Boolean}
   * @api private
   */

  Socket.prototype.isXDomain = function () {

    var port = global.location.port ||
      ('https:' == global.location.protocol ? 443 : 80);

    return this.options.host !== global.location.hostname 
      || this.options.port != port;
  };

  /**
   * Called upon handshake.
   *
   * @api private
   */

  Socket.prototype.onConnect = function () {
    if (!this.connected) {
      this.connected = true;
      this.connecting = false;
      if (!this.doBuffer) {
        // make sure to flush the buffer
        this.setBuffer(false);
      }
      this.emit('connect');
    }
  };

  /**
   * Called when the transport opens
   *
   * @api private
   */

  Socket.prototype.onOpen = function () {
    this.open = true;
  };

  /**
   * Called when the transport closes.
   *
   * @api private
   */

  Socket.prototype.onClose = function () {
    this.open = false;
    clearTimeout(this.heartbeatTimeoutTimer);
  };

  /**
   * Called when the transport first opens a connection
   *
   * @param text
   */

  Socket.prototype.onPacket = function (packet) {
    this.of(packet.endpoint).onPacket(packet);
  };

  /**
   * Handles an error.
   *
   * @api private
   */

  Socket.prototype.onError = function (err) {
    if (err && err.advice) {
      if (err.advice === 'reconnect' && (this.connected || this.connecting)) {
        this.disconnect();
        if (this.options.reconnect) {
          this.reconnect();
        }
      }
    }

    this.publish('error', err && err.reason ? err.reason : err);
  };

  /**
   * Called when the transport disconnects.
   *
   * @api private
   */

  Socket.prototype.onDisconnect = function (reason) {
    var wasConnected = this.connected
      , wasConnecting = this.connecting;

    this.connected = false;
    this.connecting = false;
    this.open = false;

    if (wasConnected || wasConnecting) {
      this.transport.close();
      this.transport.clearTimeouts();
      if (wasConnected) {
        this.publish('disconnect', reason);

        if ('booted' != reason && this.options.reconnect && !this.reconnecting) {
          this.reconnect();
        }
      }
    }
  };

  /**
   * Called upon reconnection.
   *
   * @api private
   */

  Socket.prototype.reconnect = function () {
    this.reconnecting = true;
    this.reconnectionAttempts = 0;
    this.reconnectionDelay = this.options['reconnection delay'];

    var self = this
      , maxAttempts = this.options['max reconnection attempts']
      , tryMultiple = this.options['try multiple transports']
      , limit = this.options['reconnection limit'];

    function reset () {
      if (self.connected) {
        for (var i in self.namespaces) {
          if (self.namespaces.hasOwnProperty(i) && '' !== i) {
              self.namespaces[i].packet({ type: 'connect' });
          }
        }
        self.publish('reconnect', self.transport.name, self.reconnectionAttempts);
      }

      clearTimeout(self.reconnectionTimer);

      self.removeListener('connect_failed', maybeReconnect);
      self.removeListener('connect', maybeReconnect);

      self.reconnecting = false;

      delete self.reconnectionAttempts;
      delete self.reconnectionDelay;
      delete self.reconnectionTimer;
      delete self.redoTransports;

      self.options['try multiple transports'] = tryMultiple;
    };

    function maybeReconnect () {
      if (!self.reconnecting) {
        return;
      }

      if (self.connected) {
        return reset();
      };

      if (self.connecting && self.reconnecting) {
        return self.reconnectionTimer = setTimeout(maybeReconnect, 1000);
      }

      if (self.reconnectionAttempts++ >= maxAttempts) {
        if (!self.redoTransports) {
          self.on('connect_failed', maybeReconnect);
          self.options['try multiple transports'] = true;
          self.transport = self.getTransport();
          self.redoTransports = true;
          self.connect();
        } else {
          self.publish('reconnect_failed');
          reset();
        }
      } else {
        if (self.reconnectionDelay < limit) {
          self.reconnectionDelay *= 2; // exponential back off
        }

        self.connect();
        self.publish('reconnecting', self.reconnectionDelay, self.reconnectionAttempts);
        self.reconnectionTimer = setTimeout(maybeReconnect, self.reconnectionDelay);
      }
    };

    this.options['try multiple transports'] = false;
    this.reconnectionTimer = setTimeout(maybeReconnect, this.reconnectionDelay);

    this.on('connect', maybeReconnect);
  };

})(
    'undefined' != typeof io ? io : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
  , this
);
/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Expose constructor.
   */

  exports.SocketNamespace = SocketNamespace;

  /**
   * Socket namespace constructor.
   *
   * @constructor
   * @api public
   */

  function SocketNamespace (socket, name) {
    this.socket = socket;
    this.name = name || '';
    this.flags = {};
    this.json = new Flag(this, 'json');
    this.ackPackets = 0;
    this.acks = {};
  };

  /**
   * Apply EventEmitter mixin.
   */

  io.util.mixin(SocketNamespace, io.EventEmitter);

  /**
   * Copies emit since we override it
   *
   * @api private
   */

  SocketNamespace.prototype.$emit = io.EventEmitter.prototype.emit;

  /**
   * Creates a new namespace, by proxying the request to the socket. This
   * allows us to use the synax as we do on the server.
   *
   * @api public
   */

  SocketNamespace.prototype.of = function () {
    return this.socket.of.apply(this.socket, arguments);
  };

  /**
   * Sends a packet.
   *
   * @api private
   */

  SocketNamespace.prototype.packet = function (packet) {
    packet.endpoint = this.name;
    this.socket.packet(packet);
    this.flags = {};
    return this;
  };

  /**
   * Sends a message
   *
   * @api public
   */

  SocketNamespace.prototype.send = function (data, fn) {
    var packet = {
        type: this.flags.json ? 'json' : 'message'
      , data: data
    };

    if ('function' == typeof fn) {
      packet.id = ++this.ackPackets;
      packet.ack = true;
      this.acks[packet.id] = fn;
    }

    return this.packet(packet);
  };

  /**
   * Emits an event
   *
   * @api public
   */
  
  SocketNamespace.prototype.emit = function (name) {
    var args = Array.prototype.slice.call(arguments, 1)
      , lastArg = args[args.length - 1]
      , packet = {
            type: 'event'
          , name: name
        };

    if ('function' == typeof lastArg) {
      packet.id = ++this.ackPackets;
      packet.ack = 'data';
      this.acks[packet.id] = lastArg;
      args = args.slice(0, args.length - 1);
    }

    packet.args = args;

    return this.packet(packet);
  };

  /**
   * Disconnects the namespace
   *
   * @api private
   */

  SocketNamespace.prototype.disconnect = function () {
    if (this.name === '') {
      this.socket.disconnect();
    } else {
      this.packet({ type: 'disconnect' });
      this.$emit('disconnect');
    }

    return this;
  };

  /**
   * Handles a packet
   *
   * @api private
   */

  SocketNamespace.prototype.onPacket = function (packet) {
    var self = this;

    function ack () {
      self.packet({
          type: 'ack'
        , args: io.util.toArray(arguments)
        , ackId: packet.id
      });
    };

    switch (packet.type) {
      case 'connect':
        this.$emit('connect');
        break;

      case 'disconnect':
        if (this.name === '') {
          this.socket.onDisconnect(packet.reason || 'booted');
        } else {
          this.$emit('disconnect', packet.reason);
        }
        break;

      case 'message':
      case 'json':
        var params = ['message', packet.data];

        if (packet.ack == 'data') {
          params.push(ack);
        } else if (packet.ack) {
          this.packet({ type: 'ack', ackId: packet.id });
        }

        this.$emit.apply(this, params);
        break;

      case 'event':
        var params = [packet.name].concat(packet.args);

        if (packet.ack == 'data')
          params.push(ack);

        this.$emit.apply(this, params);
        break;

      case 'ack':
        if (this.acks[packet.ackId]) {
          this.acks[packet.ackId].apply(this, packet.args);
          delete this.acks[packet.ackId];
        }
        break;

      case 'error':
        if (packet.advice){
          this.socket.onError(packet);
        } else {
          if (packet.reason == 'unauthorized') {
            this.$emit('connect_failed', packet.reason);
          } else {
            this.$emit('error', packet.reason);
          }
        }
        break;
    }
  };

  /**
   * Flag interface.
   *
   * @api private
   */

  function Flag (nsp, name) {
    this.namespace = nsp;
    this.name = name;
  };

  /**
   * Send a message
   *
   * @api public
   */

  Flag.prototype.send = function () {
    this.namespace.flags[this.name] = true;
    this.namespace.send.apply(this.namespace, arguments);
  };

  /**
   * Emit an event
   *
   * @api public
   */

  Flag.prototype.emit = function () {
    this.namespace.flags[this.name] = true;
    this.namespace.emit.apply(this.namespace, arguments);
  };

})(
    'undefined' != typeof io ? io : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io, global) {

  /**
   * Expose constructor.
   */

  exports.websocket = WS;

  /**
   * The WebSocket transport uses the HTML5 WebSocket API to establish an
   * persistent connection with the Socket.IO server. This transport will also
   * be inherited by the FlashSocket fallback as it provides a API compatible
   * polyfill for the WebSockets.
   *
   * @constructor
   * @extends {io.Transport}
   * @api public
   */

  function WS (socket) {
    io.Transport.apply(this, arguments);
  };

  /**
   * Inherits from Transport.
   */

  io.util.inherit(WS, io.Transport);

  /**
   * Transport name
   *
   * @api public
   */

  WS.prototype.name = 'websocket';

  /**
   * Initializes a new `WebSocket` connection with the Socket.IO server. We attach
   * all the appropriate listeners to handle the responses from the server.
   *
   * @returns {Transport}
   * @api public
   */

  WS.prototype.open = function () {
    var query = io.util.query(this.socket.options.query)
      , self = this
      , Socket


    if (!Socket) {
      Socket = global.MozWebSocket || global.WebSocket;
    }

    this.websocket = new Socket(this.prepareUrl() + query);

    this.websocket.onopen = function () {
      self.onOpen();
      self.socket.setBuffer(false);
    };
    this.websocket.onmessage = function (ev) {
      self.onData(ev.data);
    };
    this.websocket.onclose = function () {
      self.onClose();
      self.socket.setBuffer(true);
    };
    this.websocket.onerror = function (e) {
      self.onError(e);
    };

    return this;
  };

  /**
   * Send a message to the Socket.IO server. The message will automatically be
   * encoded in the correct message format.
   *
   * @returns {Transport}
   * @api public
   */

  WS.prototype.send = function (data) {
    this.websocket.send(data);
    return this;
  };

  /**
   * Payload
   *
   * @api private
   */

  WS.prototype.payload = function (arr) {
    for (var i = 0, l = arr.length; i < l; i++) {
      this.packet(arr[i]);
    }
    return this;
  };

  /**
   * Disconnect the established `WebSocket` connection.
   *
   * @returns {Transport}
   * @api public
   */

  WS.prototype.close = function () {
    this.websocket.close();
    return this;
  };

  /**
   * Handle the errors that `WebSocket` might be giving when we
   * are attempting to connect or send messages.
   *
   * @param {Error} e The error.
   * @api private
   */

  WS.prototype.onError = function (e) {
    this.socket.onError(e);
  };

  /**
   * Returns the appropriate scheme for the URI generation.
   *
   * @api private
   */
  WS.prototype.scheme = function () {
    return this.socket.options.secure ? 'wss' : 'ws';
  };

  /**
   * Checks if the browser has support for native `WebSockets` and that
   * it's not the polyfill created for the FlashSocket transport.
   *
   * @return {Boolean}
   * @api public
   */

  WS.check = function () {
    return ('WebSocket' in global && !('__addTask' in WebSocket))
          || 'MozWebSocket' in global;
  };

  /**
   * Check if the `WebSocket` transport support cross domain communications.
   *
   * @returns {Boolean}
   * @api public
   */

  WS.xdomainCheck = function () {
    return true;
  };

  /**
   * Add the transport to your public io.transports array.
   *
   * @api private
   */

  io.transports.push('websocket');

})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
  , this
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Expose constructor.
   */

  exports.flashsocket = Flashsocket;

  /**
   * The FlashSocket transport. This is a API wrapper for the HTML5 WebSocket
   * specification. It uses a .swf file to communicate with the server. If you want
   * to serve the .swf file from a other server than where the Socket.IO script is
   * coming from you need to use the insecure version of the .swf. More information
   * about this can be found on the github page.
   *
   * @constructor
   * @extends {io.Transport.websocket}
   * @api public
   */

  function Flashsocket () {
    io.Transport.websocket.apply(this, arguments);
  };

  /**
   * Inherits from Transport.
   */

  io.util.inherit(Flashsocket, io.Transport.websocket);

  /**
   * Transport name
   *
   * @api public
   */

  Flashsocket.prototype.name = 'flashsocket';

  /**
   * Disconnect the established `FlashSocket` connection. This is done by adding a 
   * new task to the FlashSocket. The rest will be handled off by the `WebSocket` 
   * transport.
   *
   * @returns {Transport}
   * @api public
   */

  Flashsocket.prototype.open = function () {
    var self = this
      , args = arguments;

    WebSocket.__addTask(function () {
      io.Transport.websocket.prototype.open.apply(self, args);
    });
    return this;
  };
  
  /**
   * Sends a message to the Socket.IO server. This is done by adding a new
   * task to the FlashSocket. The rest will be handled off by the `WebSocket` 
   * transport.
   *
   * @returns {Transport}
   * @api public
   */

  Flashsocket.prototype.send = function () {
    var self = this, args = arguments;
    WebSocket.__addTask(function () {
      io.Transport.websocket.prototype.send.apply(self, args);
    });
    return this;
  };

  /**
   * Disconnects the established `FlashSocket` connection.
   *
   * @returns {Transport}
   * @api public
   */

  Flashsocket.prototype.close = function () {
    WebSocket.__tasks.length = 0;
    io.Transport.websocket.prototype.close.call(this);
    return this;
  };

  /**
   * The WebSocket fall back needs to append the flash container to the body
   * element, so we need to make sure we have access to it. Or defer the call
   * until we are sure there is a body element.
   *
   * @param {Socket} socket The socket instance that needs a transport
   * @param {Function} fn The callback
   * @api private
   */

  Flashsocket.prototype.ready = function (socket, fn) {
    function init () {
      var options = socket.options
        , port = options['flash policy port']
        , path = [
              'http' + (options.secure ? 's' : '') + ':/'
            , options.host + ':' + options.port
            , options.resource
            , 'static/flashsocket'
            , 'WebSocketMain' + (socket.isXDomain() ? 'Insecure' : '') + '.swf'
          ];

      // Only start downloading the swf file when the checked that this browser
      // actually supports it
      if (!Flashsocket.loaded) {
        if (typeof WEB_SOCKET_SWF_LOCATION === 'undefined') {
          // Set the correct file based on the XDomain settings
          WEB_SOCKET_SWF_LOCATION = path.join('/');
        }

        if (port !== 843) {
          WebSocket.loadFlashPolicyFile('xmlsocket://' + options.host + ':' + port);
        }

        WebSocket.__initialize();
        Flashsocket.loaded = true;
      }

      fn.call(self);
    }

    var self = this;
    if (document.body) return init();

    io.util.load(init);
  };

  /**
   * Check if the FlashSocket transport is supported as it requires that the Adobe
   * Flash Player plug-in version `10.0.0` or greater is installed. And also check if
   * the polyfill is correctly loaded.
   *
   * @returns {Boolean}
   * @api public
   */

  Flashsocket.check = function () {
    if (
        typeof WebSocket == 'undefined'
      || !('__initialize' in WebSocket) || !swfobject
    ) return false;

    return swfobject.getFlashPlayerVersion().major >= 10;
  };

  /**
   * Check if the FlashSocket transport can be used as cross domain / cross origin 
   * transport. Because we can't see which type (secure or insecure) of .swf is used
   * we will just return true.
   *
   * @returns {Boolean}
   * @api public
   */

  Flashsocket.xdomainCheck = function () {
    return true;
  };

  /**
   * Disable AUTO_INITIALIZATION
   */

  if (typeof window != 'undefined') {
    WEB_SOCKET_DISABLE_AUTO_INITIALIZATION = true;
  }

  /**
   * Add the transport to your public io.transports array.
   *
   * @api private
   */

  io.transports.push('flashsocket');
})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);
/*	SWFObject v2.2 <http://code.google.com/p/swfobject/> 
	is released under the MIT License <http://www.opensource.org/licenses/mit-license.php> 
*/
if ('undefined' != typeof window) {
var swfobject=function(){var D="undefined",r="object",S="Shockwave Flash",W="ShockwaveFlash.ShockwaveFlash",q="application/x-shockwave-flash",R="SWFObjectExprInst",x="onreadystatechange",O=window,j=document,t=navigator,T=false,U=[h],o=[],N=[],I=[],l,Q,E,B,J=false,a=false,n,G,m=true,M=function(){var aa=typeof j.getElementById!=D&&typeof j.getElementsByTagName!=D&&typeof j.createElement!=D,ah=t.userAgent.toLowerCase(),Y=t.platform.toLowerCase(),ae=Y?/win/.test(Y):/win/.test(ah),ac=Y?/mac/.test(Y):/mac/.test(ah),af=/webkit/.test(ah)?parseFloat(ah.replace(/^.*webkit\/(\d+(\.\d+)?).*$/,"$1")):false,X=!+"\v1",ag=[0,0,0],ab=null;if(typeof t.plugins!=D&&typeof t.plugins[S]==r){ab=t.plugins[S].description;if(ab&&!(typeof t.mimeTypes!=D&&t.mimeTypes[q]&&!t.mimeTypes[q].enabledPlugin)){T=true;X=false;ab=ab.replace(/^.*\s+(\S+\s+\S+$)/,"$1");ag[0]=parseInt(ab.replace(/^(.*)\..*$/,"$1"),10);ag[1]=parseInt(ab.replace(/^.*\.(.*)\s.*$/,"$1"),10);ag[2]=/[a-zA-Z]/.test(ab)?parseInt(ab.replace(/^.*[a-zA-Z]+(.*)$/,"$1"),10):0}}else{if(typeof O[(['Active'].concat('Object').join('X'))]!=D){try{var ad=new window[(['Active'].concat('Object').join('X'))](W);if(ad){ab=ad.GetVariable("$version");if(ab){X=true;ab=ab.split(" ")[1].split(",");ag=[parseInt(ab[0],10),parseInt(ab[1],10),parseInt(ab[2],10)]}}}catch(Z){}}}return{w3:aa,pv:ag,wk:af,ie:X,win:ae,mac:ac}}(),k=function(){if(!M.w3){return}if((typeof j.readyState!=D&&j.readyState=="complete")||(typeof j.readyState==D&&(j.getElementsByTagName("body")[0]||j.body))){f()}if(!J){if(typeof j.addEventListener!=D){j.addEventListener("DOMContentLoaded",f,false)}if(M.ie&&M.win){j.attachEvent(x,function(){if(j.readyState=="complete"){j.detachEvent(x,arguments.callee);f()}});if(O==top){(function(){if(J){return}try{j.documentElement.doScroll("left")}catch(X){setTimeout(arguments.callee,0);return}f()})()}}if(M.wk){(function(){if(J){return}if(!/loaded|complete/.test(j.readyState)){setTimeout(arguments.callee,0);return}f()})()}s(f)}}();function f(){if(J){return}try{var Z=j.getElementsByTagName("body")[0].appendChild(C("span"));Z.parentNode.removeChild(Z)}catch(aa){return}J=true;var X=U.length;for(var Y=0;Y<X;Y++){U[Y]()}}function K(X){if(J){X()}else{U[U.length]=X}}function s(Y){if(typeof O.addEventListener!=D){O.addEventListener("load",Y,false)}else{if(typeof j.addEventListener!=D){j.addEventListener("load",Y,false)}else{if(typeof O.attachEvent!=D){i(O,"onload",Y)}else{if(typeof O.onload=="function"){var X=O.onload;O.onload=function(){X();Y()}}else{O.onload=Y}}}}}function h(){if(T){V()}else{H()}}function V(){var X=j.getElementsByTagName("body")[0];var aa=C(r);aa.setAttribute("type",q);var Z=X.appendChild(aa);if(Z){var Y=0;(function(){if(typeof Z.GetVariable!=D){var ab=Z.GetVariable("$version");if(ab){ab=ab.split(" ")[1].split(",");M.pv=[parseInt(ab[0],10),parseInt(ab[1],10),parseInt(ab[2],10)]}}else{if(Y<10){Y++;setTimeout(arguments.callee,10);return}}X.removeChild(aa);Z=null;H()})()}else{H()}}function H(){var ag=o.length;if(ag>0){for(var af=0;af<ag;af++){var Y=o[af].id;var ab=o[af].callbackFn;var aa={success:false,id:Y};if(M.pv[0]>0){var ae=c(Y);if(ae){if(F(o[af].swfVersion)&&!(M.wk&&M.wk<312)){w(Y,true);if(ab){aa.success=true;aa.ref=z(Y);ab(aa)}}else{if(o[af].expressInstall&&A()){var ai={};ai.data=o[af].expressInstall;ai.width=ae.getAttribute("width")||"0";ai.height=ae.getAttribute("height")||"0";if(ae.getAttribute("class")){ai.styleclass=ae.getAttribute("class")}if(ae.getAttribute("align")){ai.align=ae.getAttribute("align")}var ah={};var X=ae.getElementsByTagName("param");var ac=X.length;for(var ad=0;ad<ac;ad++){if(X[ad].getAttribute("name").toLowerCase()!="movie"){ah[X[ad].getAttribute("name")]=X[ad].getAttribute("value")}}P(ai,ah,Y,ab)}else{p(ae);if(ab){ab(aa)}}}}}else{w(Y,true);if(ab){var Z=z(Y);if(Z&&typeof Z.SetVariable!=D){aa.success=true;aa.ref=Z}ab(aa)}}}}}function z(aa){var X=null;var Y=c(aa);if(Y&&Y.nodeName=="OBJECT"){if(typeof Y.SetVariable!=D){X=Y}else{var Z=Y.getElementsByTagName(r)[0];if(Z){X=Z}}}return X}function A(){return !a&&F("6.0.65")&&(M.win||M.mac)&&!(M.wk&&M.wk<312)}function P(aa,ab,X,Z){a=true;E=Z||null;B={success:false,id:X};var ae=c(X);if(ae){if(ae.nodeName=="OBJECT"){l=g(ae);Q=null}else{l=ae;Q=X}aa.id=R;if(typeof aa.width==D||(!/%$/.test(aa.width)&&parseInt(aa.width,10)<310)){aa.width="310"}if(typeof aa.height==D||(!/%$/.test(aa.height)&&parseInt(aa.height,10)<137)){aa.height="137"}j.title=j.title.slice(0,47)+" - Flash Player Installation";var ad=M.ie&&M.win?(['Active'].concat('').join('X')):"PlugIn",ac="MMredirectURL="+O.location.toString().replace(/&/g,"%26")+"&MMplayerType="+ad+"&MMdoctitle="+j.title;if(typeof ab.flashvars!=D){ab.flashvars+="&"+ac}else{ab.flashvars=ac}if(M.ie&&M.win&&ae.readyState!=4){var Y=C("div");X+="SWFObjectNew";Y.setAttribute("id",X);ae.parentNode.insertBefore(Y,ae);ae.style.display="none";(function(){if(ae.readyState==4){ae.parentNode.removeChild(ae)}else{setTimeout(arguments.callee,10)}})()}u(aa,ab,X)}}function p(Y){if(M.ie&&M.win&&Y.readyState!=4){var X=C("div");Y.parentNode.insertBefore(X,Y);X.parentNode.replaceChild(g(Y),X);Y.style.display="none";(function(){if(Y.readyState==4){Y.parentNode.removeChild(Y)}else{setTimeout(arguments.callee,10)}})()}else{Y.parentNode.replaceChild(g(Y),Y)}}function g(ab){var aa=C("div");if(M.win&&M.ie){aa.innerHTML=ab.innerHTML}else{var Y=ab.getElementsByTagName(r)[0];if(Y){var ad=Y.childNodes;if(ad){var X=ad.length;for(var Z=0;Z<X;Z++){if(!(ad[Z].nodeType==1&&ad[Z].nodeName=="PARAM")&&!(ad[Z].nodeType==8)){aa.appendChild(ad[Z].cloneNode(true))}}}}}return aa}function u(ai,ag,Y){var X,aa=c(Y);if(M.wk&&M.wk<312){return X}if(aa){if(typeof ai.id==D){ai.id=Y}if(M.ie&&M.win){var ah="";for(var ae in ai){if(ai[ae]!=Object.prototype[ae]){if(ae.toLowerCase()=="data"){ag.movie=ai[ae]}else{if(ae.toLowerCase()=="styleclass"){ah+=' class="'+ai[ae]+'"'}else{if(ae.toLowerCase()!="classid"){ah+=" "+ae+'="'+ai[ae]+'"'}}}}}var af="";for(var ad in ag){if(ag[ad]!=Object.prototype[ad]){af+='<param name="'+ad+'" value="'+ag[ad]+'" />'}}aa.outerHTML='<object classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000"'+ah+">"+af+"</object>";N[N.length]=ai.id;X=c(ai.id)}else{var Z=C(r);Z.setAttribute("type",q);for(var ac in ai){if(ai[ac]!=Object.prototype[ac]){if(ac.toLowerCase()=="styleclass"){Z.setAttribute("class",ai[ac])}else{if(ac.toLowerCase()!="classid"){Z.setAttribute(ac,ai[ac])}}}}for(var ab in ag){if(ag[ab]!=Object.prototype[ab]&&ab.toLowerCase()!="movie"){e(Z,ab,ag[ab])}}aa.parentNode.replaceChild(Z,aa);X=Z}}return X}function e(Z,X,Y){var aa=C("param");aa.setAttribute("name",X);aa.setAttribute("value",Y);Z.appendChild(aa)}function y(Y){var X=c(Y);if(X&&X.nodeName=="OBJECT"){if(M.ie&&M.win){X.style.display="none";(function(){if(X.readyState==4){b(Y)}else{setTimeout(arguments.callee,10)}})()}else{X.parentNode.removeChild(X)}}}function b(Z){var Y=c(Z);if(Y){for(var X in Y){if(typeof Y[X]=="function"){Y[X]=null}}Y.parentNode.removeChild(Y)}}function c(Z){var X=null;try{X=j.getElementById(Z)}catch(Y){}return X}function C(X){return j.createElement(X)}function i(Z,X,Y){Z.attachEvent(X,Y);I[I.length]=[Z,X,Y]}function F(Z){var Y=M.pv,X=Z.split(".");X[0]=parseInt(X[0],10);X[1]=parseInt(X[1],10)||0;X[2]=parseInt(X[2],10)||0;return(Y[0]>X[0]||(Y[0]==X[0]&&Y[1]>X[1])||(Y[0]==X[0]&&Y[1]==X[1]&&Y[2]>=X[2]))?true:false}function v(ac,Y,ad,ab){if(M.ie&&M.mac){return}var aa=j.getElementsByTagName("head")[0];if(!aa){return}var X=(ad&&typeof ad=="string")?ad:"screen";if(ab){n=null;G=null}if(!n||G!=X){var Z=C("style");Z.setAttribute("type","text/css");Z.setAttribute("media",X);n=aa.appendChild(Z);if(M.ie&&M.win&&typeof j.styleSheets!=D&&j.styleSheets.length>0){n=j.styleSheets[j.styleSheets.length-1]}G=X}if(M.ie&&M.win){if(n&&typeof n.addRule==r){n.addRule(ac,Y)}}else{if(n&&typeof j.createTextNode!=D){n.appendChild(j.createTextNode(ac+" {"+Y+"}"))}}}function w(Z,X){if(!m){return}var Y=X?"visible":"hidden";if(J&&c(Z)){c(Z).style.visibility=Y}else{v("#"+Z,"visibility:"+Y)}}function L(Y){var Z=/[\\\"<>\.;]/;var X=Z.exec(Y)!=null;return X&&typeof encodeURIComponent!=D?encodeURIComponent(Y):Y}var d=function(){if(M.ie&&M.win){window.attachEvent("onunload",function(){var ac=I.length;for(var ab=0;ab<ac;ab++){I[ab][0].detachEvent(I[ab][1],I[ab][2])}var Z=N.length;for(var aa=0;aa<Z;aa++){y(N[aa])}for(var Y in M){M[Y]=null}M=null;for(var X in swfobject){swfobject[X]=null}swfobject=null})}}();return{registerObject:function(ab,X,aa,Z){if(M.w3&&ab&&X){var Y={};Y.id=ab;Y.swfVersion=X;Y.expressInstall=aa;Y.callbackFn=Z;o[o.length]=Y;w(ab,false)}else{if(Z){Z({success:false,id:ab})}}},getObjectById:function(X){if(M.w3){return z(X)}},embedSWF:function(ab,ah,ae,ag,Y,aa,Z,ad,af,ac){var X={success:false,id:ah};if(M.w3&&!(M.wk&&M.wk<312)&&ab&&ah&&ae&&ag&&Y){w(ah,false);K(function(){ae+="";ag+="";var aj={};if(af&&typeof af===r){for(var al in af){aj[al]=af[al]}}aj.data=ab;aj.width=ae;aj.height=ag;var am={};if(ad&&typeof ad===r){for(var ak in ad){am[ak]=ad[ak]}}if(Z&&typeof Z===r){for(var ai in Z){if(typeof am.flashvars!=D){am.flashvars+="&"+ai+"="+Z[ai]}else{am.flashvars=ai+"="+Z[ai]}}}if(F(Y)){var an=u(aj,am,ah);if(aj.id==ah){w(ah,true)}X.success=true;X.ref=an}else{if(aa&&A()){aj.data=aa;P(aj,am,ah,ac);return}else{w(ah,true)}}if(ac){ac(X)}})}else{if(ac){ac(X)}}},switchOffAutoHideShow:function(){m=false},ua:M,getFlashPlayerVersion:function(){return{major:M.pv[0],minor:M.pv[1],release:M.pv[2]}},hasFlashPlayerVersion:F,createSWF:function(Z,Y,X){if(M.w3){return u(Z,Y,X)}else{return undefined}},showExpressInstall:function(Z,aa,X,Y){if(M.w3&&A()){P(Z,aa,X,Y)}},removeSWF:function(X){if(M.w3){y(X)}},createCSS:function(aa,Z,Y,X){if(M.w3){v(aa,Z,Y,X)}},addDomLoadEvent:K,addLoadEvent:s,getQueryParamValue:function(aa){var Z=j.location.search||j.location.hash;if(Z){if(/\?/.test(Z)){Z=Z.split("?")[1]}if(aa==null){return L(Z)}var Y=Z.split("&");for(var X=0;X<Y.length;X++){if(Y[X].substring(0,Y[X].indexOf("="))==aa){return L(Y[X].substring((Y[X].indexOf("=")+1)))}}}return""},expressInstallCallback:function(){if(a){var X=c(R);if(X&&l){X.parentNode.replaceChild(l,X);if(Q){w(Q,true);if(M.ie&&M.win){l.style.display="block"}}if(E){E(B)}}a=false}}}}();
}
// Copyright: Hiroshi Ichikawa <http://gimite.net/en/>
// License: New BSD License
// Reference: http://dev.w3.org/html5/websockets/
// Reference: http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol

(function() {
  
  if ('undefined' == typeof window || window.WebSocket) return;

  var console = window.console;
  if (!console || !console.log || !console.error) {
    console = {log: function(){ }, error: function(){ }};
  }
  
  if (!swfobject.hasFlashPlayerVersion("10.0.0")) {
    console.error("Flash Player >= 10.0.0 is required.");
    return;
  }
  if (location.protocol == "file:") {
    console.error(
      "WARNING: web-socket-js doesn't work in file:///... URL " +
      "unless you set Flash Security Settings properly. " +
      "Open the page via Web server i.e. http://...");
  }

  /**
   * This class represents a faux web socket.
   * @param {string} url
   * @param {array or string} protocols
   * @param {string} proxyHost
   * @param {int} proxyPort
   * @param {string} headers
   */
  WebSocket = function(url, protocols, proxyHost, proxyPort, headers) {
    var self = this;
    self.__id = WebSocket.__nextId++;
    WebSocket.__instances[self.__id] = self;
    self.readyState = WebSocket.CONNECTING;
    self.bufferedAmount = 0;
    self.__events = {};
    if (!protocols) {
      protocols = [];
    } else if (typeof protocols == "string") {
      protocols = [protocols];
    }
    // Uses setTimeout() to make sure __createFlash() runs after the caller sets ws.onopen etc.
    // Otherwise, when onopen fires immediately, onopen is called before it is set.
    setTimeout(function() {
      WebSocket.__addTask(function() {
        WebSocket.__flash.create(
            self.__id, url, protocols, proxyHost || null, proxyPort || 0, headers || null);
      });
    }, 0);
  };

  /**
   * Send data to the web socket.
   * @param {string} data  The data to send to the socket.
   * @return {boolean}  True for success, false for failure.
   */
  WebSocket.prototype.send = function(data) {
    if (this.readyState == WebSocket.CONNECTING) {
      throw "INVALID_STATE_ERR: Web Socket connection has not been established";
    }
    // We use encodeURIComponent() here, because FABridge doesn't work if
    // the argument includes some characters. We don't use escape() here
    // because of this:
    // https://developer.mozilla.org/en/Core_JavaScript_1.5_Guide/Functions#escape_and_unescape_Functions
    // But it looks decodeURIComponent(encodeURIComponent(s)) doesn't
    // preserve all Unicode characters either e.g. "\uffff" in Firefox.
    // Note by wtritch: Hopefully this will not be necessary using ExternalInterface.  Will require
    // additional testing.
    var result = WebSocket.__flash.send(this.__id, encodeURIComponent(data));
    if (result < 0) { // success
      return true;
    } else {
      this.bufferedAmount += result;
      return false;
    }
  };

  /**
   * Close this web socket gracefully.
   */
  WebSocket.prototype.close = function() {
    if (this.readyState == WebSocket.CLOSED || this.readyState == WebSocket.CLOSING) {
      return;
    }
    this.readyState = WebSocket.CLOSING;
    WebSocket.__flash.close(this.__id);
  };

  /**
   * Implementation of {@link <a href="http://www.w3.org/TR/DOM-Level-2-Events/events.html#Events-registration">DOM 2 EventTarget Interface</a>}
   *
   * @param {string} type
   * @param {function} listener
   * @param {boolean} useCapture
   * @return void
   */
  WebSocket.prototype.addEventListener = function(type, listener, useCapture) {
    if (!(type in this.__events)) {
      this.__events[type] = [];
    }
    this.__events[type].push(listener);
  };

  /**
   * Implementation of {@link <a href="http://www.w3.org/TR/DOM-Level-2-Events/events.html#Events-registration">DOM 2 EventTarget Interface</a>}
   *
   * @param {string} type
   * @param {function} listener
   * @param {boolean} useCapture
   * @return void
   */
  WebSocket.prototype.removeEventListener = function(type, listener, useCapture) {
    if (!(type in this.__events)) return;
    var events = this.__events[type];
    for (var i = events.length - 1; i >= 0; --i) {
      if (events[i] === listener) {
        events.splice(i, 1);
        break;
      }
    }
  };

  /**
   * Implementation of {@link <a href="http://www.w3.org/TR/DOM-Level-2-Events/events.html#Events-registration">DOM 2 EventTarget Interface</a>}
   *
   * @param {Event} event
   * @return void
   */
  WebSocket.prototype.dispatchEvent = function(event) {
    var events = this.__events[event.type] || [];
    for (var i = 0; i < events.length; ++i) {
      events[i](event);
    }
    var handler = this["on" + event.type];
    if (handler) handler(event);
  };

  /**
   * Handles an event from Flash.
   * @param {Object} flashEvent
   */
  WebSocket.prototype.__handleEvent = function(flashEvent) {
    if ("readyState" in flashEvent) {
      this.readyState = flashEvent.readyState;
    }
    if ("protocol" in flashEvent) {
      this.protocol = flashEvent.protocol;
    }
    
    var jsEvent;
    if (flashEvent.type == "open" || flashEvent.type == "error") {
      jsEvent = this.__createSimpleEvent(flashEvent.type);
    } else if (flashEvent.type == "close") {
      // TODO implement jsEvent.wasClean
      jsEvent = this.__createSimpleEvent("close");
    } else if (flashEvent.type == "message") {
      var data = decodeURIComponent(flashEvent.message);
      jsEvent = this.__createMessageEvent("message", data);
    } else {
      throw "unknown event type: " + flashEvent.type;
    }
    
    this.dispatchEvent(jsEvent);
  };
  
  WebSocket.prototype.__createSimpleEvent = function(type) {
    if (document.createEvent && window.Event) {
      var event = document.createEvent("Event");
      event.initEvent(type, false, false);
      return event;
    } else {
      return {type: type, bubbles: false, cancelable: false};
    }
  };
  
  WebSocket.prototype.__createMessageEvent = function(type, data) {
    if (document.createEvent && window.MessageEvent && !window.opera) {
      var event = document.createEvent("MessageEvent");
      event.initMessageEvent("message", false, false, data, null, null, window, null);
      return event;
    } else {
      // IE and Opera, the latter one truncates the data parameter after any 0x00 bytes.
      return {type: type, data: data, bubbles: false, cancelable: false};
    }
  };
  
  /**
   * Define the WebSocket readyState enumeration.
   */
  WebSocket.CONNECTING = 0;
  WebSocket.OPEN = 1;
  WebSocket.CLOSING = 2;
  WebSocket.CLOSED = 3;

  WebSocket.__flash = null;
  WebSocket.__instances = {};
  WebSocket.__tasks = [];
  WebSocket.__nextId = 0;
  
  /**
   * Load a new flash security policy file.
   * @param {string} url
   */
  WebSocket.loadFlashPolicyFile = function(url){
    WebSocket.__addTask(function() {
      WebSocket.__flash.loadManualPolicyFile(url);
    });
  };

  /**
   * Loads WebSocketMain.swf and creates WebSocketMain object in Flash.
   */
  WebSocket.__initialize = function() {
    if (WebSocket.__flash) return;
    
    if (WebSocket.__swfLocation) {
      // For backword compatibility.
      window.WEB_SOCKET_SWF_LOCATION = WebSocket.__swfLocation;
    }
    if (!window.WEB_SOCKET_SWF_LOCATION) {
      console.error("[WebSocket] set WEB_SOCKET_SWF_LOCATION to location of WebSocketMain.swf");
      return;
    }
    var container = document.createElement("div");
    container.id = "webSocketContainer";
    // Hides Flash box. We cannot use display: none or visibility: hidden because it prevents
    // Flash from loading at least in IE. So we move it out of the screen at (-100, -100).
    // But this even doesn't work with Flash Lite (e.g. in Droid Incredible). So with Flash
    // Lite, we put it at (0, 0). This shows 1x1 box visible at left-top corner but this is
    // the best we can do as far as we know now.
    container.style.position = "absolute";
    if (WebSocket.__isFlashLite()) {
      container.style.left = "0px";
      container.style.top = "0px";
    } else {
      container.style.left = "-100px";
      container.style.top = "-100px";
    }
    var holder = document.createElement("div");
    holder.id = "webSocketFlash";
    container.appendChild(holder);
    document.body.appendChild(container);
    // See this article for hasPriority:
    // http://help.adobe.com/en_US/as3/mobile/WS4bebcd66a74275c36cfb8137124318eebc6-7ffd.html
    swfobject.embedSWF(
      WEB_SOCKET_SWF_LOCATION,
      "webSocketFlash",
      "1" /* width */,
      "1" /* height */,
      "10.0.0" /* SWF version */,
      null,
      null,
      {hasPriority: true, swliveconnect : true, allowScriptAccess: "always"},
      null,
      function(e) {
        if (!e.success) {
          console.error("[WebSocket] swfobject.embedSWF failed");
        }
      });
  };
  
  /**
   * Called by Flash to notify JS that it's fully loaded and ready
   * for communication.
   */
  WebSocket.__onFlashInitialized = function() {
    // We need to set a timeout here to avoid round-trip calls
    // to flash during the initialization process.
    setTimeout(function() {
      WebSocket.__flash = document.getElementById("webSocketFlash");
      WebSocket.__flash.setCallerUrl(location.href);
      WebSocket.__flash.setDebug(!!window.WEB_SOCKET_DEBUG);
      for (var i = 0; i < WebSocket.__tasks.length; ++i) {
        WebSocket.__tasks[i]();
      }
      WebSocket.__tasks = [];
    }, 0);
  };
  
  /**
   * Called by Flash to notify WebSockets events are fired.
   */
  WebSocket.__onFlashEvent = function() {
    setTimeout(function() {
      try {
        // Gets events using receiveEvents() instead of getting it from event object
        // of Flash event. This is to make sure to keep message order.
        // It seems sometimes Flash events don't arrive in the same order as they are sent.
        var events = WebSocket.__flash.receiveEvents();
        for (var i = 0; i < events.length; ++i) {
          WebSocket.__instances[events[i].webSocketId].__handleEvent(events[i]);
        }
      } catch (e) {
        console.error(e);
      }
    }, 0);
    return true;
  };
  
  // Called by Flash.
  WebSocket.__log = function(message) {
    console.log(decodeURIComponent(message));
  };
  
  // Called by Flash.
  WebSocket.__error = function(message) {
    console.error(decodeURIComponent(message));
  };
  
  WebSocket.__addTask = function(task) {
    if (WebSocket.__flash) {
      task();
    } else {
      WebSocket.__tasks.push(task);
    }
  };
  
  /**
   * Test if the browser is running flash lite.
   * @return {boolean} True if flash lite is running, false otherwise.
   */
  WebSocket.__isFlashLite = function() {
    if (!window.navigator || !window.navigator.mimeTypes) {
      return false;
    }
    var mimeType = window.navigator.mimeTypes["application/x-shockwave-flash"];
    if (!mimeType || !mimeType.enabledPlugin || !mimeType.enabledPlugin.filename) {
      return false;
    }
    return mimeType.enabledPlugin.filename.match(/flashlite/i) ? true : false;
  };
  
  if (!window.WEB_SOCKET_DISABLE_AUTO_INITIALIZATION) {
    if (window.addEventListener) {
      window.addEventListener("load", function(){
        WebSocket.__initialize();
      }, false);
    } else {
      window.attachEvent("onload", function(){
        WebSocket.__initialize();
      });
    }
  }
  
})();

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io, global) {

  /**
   * Expose constructor.
   *
   * @api public
   */

  exports.XHR = XHR;

  /**
   * XHR constructor
   *
   * @costructor
   * @api public
   */

  function XHR (socket) {
    if (!socket) return;

    io.Transport.apply(this, arguments);
    this.sendBuffer = [];
  };

  /**
   * Inherits from Transport.
   */

  io.util.inherit(XHR, io.Transport);

  /**
   * Establish a connection
   *
   * @returns {Transport}
   * @api public
   */

  XHR.prototype.open = function () {
    this.socket.setBuffer(false);
    this.onOpen();
    this.get();

    // we need to make sure the request succeeds since we have no indication
    // whether the request opened or not until it succeeded.
    this.setCloseTimeout();

    return this;
  };

  /**
   * Check if we need to send data to the Socket.IO server, if we have data in our
   * buffer we encode it and forward it to the `post` method.
   *
   * @api private
   */

  XHR.prototype.payload = function (payload) {
    var msgs = [];

    for (var i = 0, l = payload.length; i < l; i++) {
      msgs.push(io.parser.encodePacket(payload[i]));
    }

    this.send(io.parser.encodePayload(msgs));
  };

  /**
   * Send data to the Socket.IO server.
   *
   * @param data The message
   * @returns {Transport}
   * @api public
   */

  XHR.prototype.send = function (data) {
    this.post(data);
    return this;
  };

  /**
   * Posts a encoded message to the Socket.IO server.
   *
   * @param {String} data A encoded message.
   * @api private
   */

  function empty () { };

  XHR.prototype.post = function (data) {
    var self = this;
    this.socket.setBuffer(true);

    function stateChange () {
      if (this.readyState == 4) {
        this.onreadystatechange = empty;
        self.posting = false;

        if (this.status == 200){
          self.socket.setBuffer(false);
        } else {
          self.onClose();
        }
      }
    }

    function onload () {
      this.onload = empty;
      self.socket.setBuffer(false);
    };

    this.sendXHR = this.request('POST');

    if (global.XDomainRequest && this.sendXHR instanceof XDomainRequest) {
      this.sendXHR.onload = this.sendXHR.onerror = onload;
    } else {
      this.sendXHR.onreadystatechange = stateChange;
    }

    this.sendXHR.send(data);
  };

  /**
   * Disconnects the established `XHR` connection.
   *
   * @returns {Transport}
   * @api public
   */

  XHR.prototype.close = function () {
    this.onClose();
    return this;
  };

  /**
   * Generates a configured XHR request
   *
   * @param {String} url The url that needs to be requested.
   * @param {String} method The method the request should use.
   * @returns {XMLHttpRequest}
   * @api private
   */

  XHR.prototype.request = function (method) {
    var req = io.util.request(this.socket.isXDomain())
      , query = io.util.query(this.socket.options.query, 't=' + +new Date);

    req.open(method || 'GET', this.prepareUrl() + query, true);

    if (method == 'POST') {
      try {
        if (req.setRequestHeader) {
          req.setRequestHeader('Content-type', 'text/plain;charset=UTF-8');
        } else {
          // XDomainRequest
          req.contentType = 'text/plain';
        }
      } catch (e) {}
    }

    return req;
  };

  /**
   * Returns the scheme to use for the transport URLs.
   *
   * @api private
   */

  XHR.prototype.scheme = function () {
    return this.socket.options.secure ? 'https' : 'http';
  };

  /**
   * Check if the XHR transports are supported
   *
   * @param {Boolean} xdomain Check if we support cross domain requests.
   * @returns {Boolean}
   * @api public
   */

  XHR.check = function (socket, xdomain) {
    try {
      var request = io.util.request(xdomain),
          usesXDomReq = (global.XDomainRequest && request instanceof XDomainRequest),
          socketProtocol = (socket && socket.options && socket.options.secure ? 'https:' : 'http:'),
          isXProtocol = (socketProtocol != global.location.protocol);
      if (request && !(usesXDomReq && isXProtocol)) {
        return true;
      }
    } catch(e) {}

    return false;
  };

  /**
   * Check if the XHR transport supports cross domain requests.
   *
   * @returns {Boolean}
   * @api public
   */

  XHR.xdomainCheck = function () {
    return XHR.check(null, true);
  };

})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
  , this
);
/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io) {

  /**
   * Expose constructor.
   */

  exports.htmlfile = HTMLFile;

  /**
   * The HTMLFile transport creates a `forever iframe` based transport
   * for Internet Explorer. Regular forever iframe implementations will 
   * continuously trigger the browsers buzy indicators. If the forever iframe
   * is created inside a `htmlfile` these indicators will not be trigged.
   *
   * @constructor
   * @extends {io.Transport.XHR}
   * @api public
   */

  function HTMLFile (socket) {
    io.Transport.XHR.apply(this, arguments);
  };

  /**
   * Inherits from XHR transport.
   */

  io.util.inherit(HTMLFile, io.Transport.XHR);

  /**
   * Transport name
   *
   * @api public
   */

  HTMLFile.prototype.name = 'htmlfile';

  /**
   * Creates a new Ac...eX `htmlfile` with a forever loading iframe
   * that can be used to listen to messages. Inside the generated
   * `htmlfile` a reference will be made to the HTMLFile transport.
   *
   * @api private
   */

  HTMLFile.prototype.get = function () {
    this.doc = new window[(['Active'].concat('Object').join('X'))]('htmlfile');
    this.doc.open();
    this.doc.write('<html></html>');
    this.doc.close();
    this.doc.parentWindow.s = this;

    var iframeC = this.doc.createElement('div');
    iframeC.className = 'socketio';

    this.doc.body.appendChild(iframeC);
    this.iframe = this.doc.createElement('iframe');

    iframeC.appendChild(this.iframe);

    var self = this
      , query = io.util.query(this.socket.options.query, 't='+ +new Date);

    this.iframe.src = this.prepareUrl() + query;

    io.util.on(window, 'unload', function () {
      self.destroy();
    });
  };

  /**
   * The Socket.IO server will write script tags inside the forever
   * iframe, this function will be used as callback for the incoming
   * information.
   *
   * @param {String} data The message
   * @param {document} doc Reference to the context
   * @api private
   */

  HTMLFile.prototype._ = function (data, doc) {
    this.onData(data);
    try {
      var script = doc.getElementsByTagName('script')[0];
      script.parentNode.removeChild(script);
    } catch (e) { }
  };

  /**
   * Destroy the established connection, iframe and `htmlfile`.
   * And calls the `CollectGarbage` function of Internet Explorer
   * to release the memory.
   *
   * @api private
   */

  HTMLFile.prototype.destroy = function () {
    if (this.iframe){
      try {
        this.iframe.src = 'about:blank';
      } catch(e){}

      this.doc = null;
      this.iframe.parentNode.removeChild(this.iframe);
      this.iframe = null;

      CollectGarbage();
    }
  };

  /**
   * Disconnects the established connection.
   *
   * @returns {Transport} Chaining.
   * @api public
   */

  HTMLFile.prototype.close = function () {
    this.destroy();
    return io.Transport.XHR.prototype.close.call(this);
  };

  /**
   * Checks if the browser supports this transport. The browser
   * must have an `Ac...eXObject` implementation.
   *
   * @return {Boolean}
   * @api public
   */

  HTMLFile.check = function () {
    if (typeof window != "undefined" && (['Active'].concat('Object').join('X')) in window){
      try {
        var a = new window[(['Active'].concat('Object').join('X'))]('htmlfile');
        return a && io.Transport.XHR.check();
      } catch(e){}
    }
    return false;
  };

  /**
   * Check if cross domain requests are supported.
   *
   * @returns {Boolean}
   * @api public
   */

  HTMLFile.xdomainCheck = function () {
    // we can probably do handling for sub-domains, we should
    // test that it's cross domain but a subdomain here
    return false;
  };

  /**
   * Add the transport to your public io.transports array.
   *
   * @api private
   */

  io.transports.push('htmlfile');

})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io, global) {

  /**
   * Expose constructor.
   */

  exports['xhr-polling'] = XHRPolling;

  /**
   * The XHR-polling transport uses long polling XHR requests to create a
   * "persistent" connection with the server.
   *
   * @constructor
   * @api public
   */

  function XHRPolling () {
    io.Transport.XHR.apply(this, arguments);
  };

  /**
   * Inherits from XHR transport.
   */

  io.util.inherit(XHRPolling, io.Transport.XHR);

  /**
   * Merge the properties from XHR transport
   */

  io.util.merge(XHRPolling, io.Transport.XHR);

  /**
   * Transport name
   *
   * @api public
   */

  XHRPolling.prototype.name = 'xhr-polling';

  /** 
   * Establish a connection, for iPhone and Android this will be done once the page
   * is loaded.
   *
   * @returns {Transport} Chaining.
   * @api public
   */

  XHRPolling.prototype.open = function () {
    var self = this;

    io.Transport.XHR.prototype.open.call(self);
    return false;
  };

  /**
   * Starts a XHR request to wait for incoming messages.
   *
   * @api private
   */

  function empty () {};

  XHRPolling.prototype.get = function () {
    if (!this.open) return;

    var self = this;

    function stateChange () {
      if (this.readyState == 4) {
        this.onreadystatechange = empty;

        if (this.status == 200) {
          self.onData(this.responseText);
          self.get();
        } else {
          self.onClose();
        }
      }
    };

    function onload () {
      this.onload = empty;
      this.onerror = empty;
      self.onData(this.responseText);
      self.get();
    };

    function onerror () {
      self.onClose();
    };

    this.xhr = this.request();

    if (global.XDomainRequest && this.xhr instanceof XDomainRequest) {
      this.xhr.onload = onload;
      this.xhr.onerror = onerror;
    } else {
      this.xhr.onreadystatechange = stateChange;
    }

    this.xhr.send(null);
  };

  /**
   * Handle the unclean close behavior.
   *
   * @api private
   */

  XHRPolling.prototype.onClose = function () {
    io.Transport.XHR.prototype.onClose.call(this);

    if (this.xhr) {
      this.xhr.onreadystatechange = this.xhr.onload = this.xhr.onerror = empty;
      try {
        this.xhr.abort();
      } catch(e){}
      this.xhr = null;
    }
  };

  /**
   * Webkit based browsers show a infinit spinner when you start a XHR request
   * before the browsers onload event is called so we need to defer opening of
   * the transport until the onload event is called. Wrapping the cb in our
   * defer method solve this.
   *
   * @param {Socket} socket The socket instance that needs a transport
   * @param {Function} fn The callback
   * @api private
   */

  XHRPolling.prototype.ready = function (socket, fn) {
    var self = this;

    io.util.defer(function () {
      fn.call(self);
    });
  };

  /**
   * Add the transport to your public io.transports array.
   *
   * @api private
   */

  io.transports.push('xhr-polling');

})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
  , this
);

/**
 * socket.io
 * Copyright(c) 2011 LearnBoost <dev@learnboost.com>
 * MIT Licensed
 */

(function (exports, io, global) {
  /**
   * There is a way to hide the loading indicator in Firefox. If you create and
   * remove a iframe it will stop showing the current loading indicator.
   * Unfortunately we can't feature detect that and UA sniffing is evil.
   *
   * @api private
   */

  var indicator = global.document && "MozAppearance" in
    global.document.documentElement.style;

  /**
   * Expose constructor.
   */

  exports['jsonp-polling'] = JSONPPolling;

  /**
   * The JSONP transport creates an persistent connection by dynamically
   * inserting a script tag in the page. This script tag will receive the
   * information of the Socket.IO server. When new information is received
   * it creates a new script tag for the new data stream.
   *
   * @constructor
   * @extends {io.Transport.xhr-polling}
   * @api public
   */

  function JSONPPolling (socket) {
    io.Transport['xhr-polling'].apply(this, arguments);

    this.index = io.j.length;

    var self = this;

    io.j.push(function (msg) {
      self._(msg);
    });
  };

  /**
   * Inherits from XHR polling transport.
   */

  io.util.inherit(JSONPPolling, io.Transport['xhr-polling']);

  /**
   * Transport name
   *
   * @api public
   */

  JSONPPolling.prototype.name = 'jsonp-polling';

  /**
   * Posts a encoded message to the Socket.IO server using an iframe.
   * The iframe is used because script tags can create POST based requests.
   * The iframe is positioned outside of the view so the user does not
   * notice it's existence.
   *
   * @param {String} data A encoded message.
   * @api private
   */

  JSONPPolling.prototype.post = function (data) {
    var self = this
      , query = io.util.query(
             this.socket.options.query
          , 't='+ (+new Date) + '&i=' + this.index
        );

    if (!this.form) {
      var form = document.createElement('form')
        , area = document.createElement('textarea')
        , id = this.iframeId = 'socketio_iframe_' + this.index
        , iframe;

      form.className = 'socketio';
      form.style.position = 'absolute';
      form.style.top = '0px';
      form.style.left = '0px';
      form.style.display = 'none';
      form.target = id;
      form.method = 'POST';
      form.setAttribute('accept-charset', 'utf-8');
      area.name = 'd';
      form.appendChild(area);
      document.body.appendChild(form);

      this.form = form;
      this.area = area;
    }

    this.form.action = this.prepareUrl() + query;

    function complete () {
      initIframe();
      self.socket.setBuffer(false);
    };

    function initIframe () {
      if (self.iframe) {
        self.form.removeChild(self.iframe);
      }

      try {
        // ie6 dynamic iframes with target="" support (thanks Chris Lambacher)
        iframe = document.createElement('<iframe name="'+ self.iframeId +'">');
      } catch (e) {
        iframe = document.createElement('iframe');
        iframe.name = self.iframeId;
      }

      iframe.id = self.iframeId;

      self.form.appendChild(iframe);
      self.iframe = iframe;
    };

    initIframe();

    // we temporarily stringify until we figure out how to prevent
    // browsers from turning `\n` into `\r\n` in form inputs
    this.area.value = io.JSON.stringify(data);

    try {
      this.form.submit();
    } catch(e) {}

    if (this.iframe.attachEvent) {
      iframe.onreadystatechange = function () {
        if (self.iframe.readyState == 'complete') {
          complete();
        }
      };
    } else {
      this.iframe.onload = complete;
    }

    this.socket.setBuffer(true);
  };
  
  /**
   * Creates a new JSONP poll that can be used to listen
   * for messages from the Socket.IO server.
   *
   * @api private
   */

  JSONPPolling.prototype.get = function () {
    var self = this
      , script = document.createElement('script')
      , query = io.util.query(
             this.socket.options.query
          , 't='+ (+new Date) + '&i=' + this.index
        );

    if (this.script) {
      this.script.parentNode.removeChild(this.script);
      this.script = null;
    }

    script.async = true;
    script.src = this.prepareUrl() + query;
    script.onerror = function () {
      self.onClose();
    };

    var insertAt = document.getElementsByTagName('script')[0]
    insertAt.parentNode.insertBefore(script, insertAt);
    this.script = script;

    if (indicator) {
      setTimeout(function () {
        var iframe = document.createElement('iframe');
        document.body.appendChild(iframe);
        document.body.removeChild(iframe);
      }, 100);
    }
  };

  /**
   * Callback function for the incoming message stream from the Socket.IO server.
   *
   * @param {String} data The message
   * @api private
   */

  JSONPPolling.prototype._ = function (msg) {
    this.onData(msg);
    if (this.open) {
      this.get();
    }
    return this;
  };

  /**
   * The indicator hack only works after onload
   *
   * @param {Socket} socket The socket instance that needs a transport
   * @param {Function} fn The callback
   * @api private
   */

  JSONPPolling.prototype.ready = function (socket, fn) {
    var self = this;
    if (!indicator) return fn.call(this);

    io.util.load(function () {
      fn.call(self);
    });
  };

  /**
   * Checks if browser supports this transport.
   *
   * @return {Boolean}
   * @api public
   */

  JSONPPolling.check = function () {
    return 'document' in global;
  };

  /**
   * Check if cross domain requests are supported
   *
   * @returns {Boolean}
   * @api public
   */

  JSONPPolling.xdomainCheck = function () {
    return true;
  };

  /**
   * Add the transport to your public io.transports array.
   *
   * @api private
   */

  io.transports.push('jsonp-polling');

})(
    'undefined' != typeof io ? io.Transport : module.exports
  , 'undefined' != typeof io ? io : module.parent.exports
  , this
);
