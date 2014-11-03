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
    this.isMixed = function () {
      return (!!spec.video) && (spec.video.category === 'mix'); // category: 'mix', 'single'
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
    this.isMix = function () {
      return false;
    };
  }

  function WoogeenRemoteStream (spec) {
    WoogeenStream.call(this, spec);
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