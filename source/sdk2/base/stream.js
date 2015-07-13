/* global webkitURL, chrome */
(function () {
  'use strict';

/**
 * @namespace Woogeen.Stream
 * @classDesc Handles the WebRTC (audio, video) stream, identifies the stream, and identifies the location where the stream should be displayed. There are two stream classes: LocalStream and RemoteStream.
 */
  function WoogeenStream (spec) {
    // if (!(this instanceof WoogeenStream)) {
    //   return new WoogeenStream();
    // }
    this.mediaStream = spec.mediaStream;
    spec.attributes = spec.attributes || {};
    this.url = function () {
      if(typeof spec.url === 'string' && spec.url !== '') {
        return spec.url;
      }
      return undefined;
    };
/**
   * @function hasVideo
   * @desc This function returns true when stream has video track otherwise false.
   * @memberOf Woogeen.Stream
   * @instance
   * @return true The stream has video.<br>false The stream does not have video.
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream hasVideo:', stream.hasVideo());
</script>
   */
    this.hasVideo = function () {
      return !!spec.video;
    };
/**
   * @function hasAudio
   * @desc This function returns true when stream has audio track otherwise false.
   * @memberOf Woogeen.Stream
   * @instance
   * @return true The stream has audio.<br>false The stream does not have audio.
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream hasAudio:', stream.hasAudio());
</script>
   */
    this.hasAudio = function () {
      return !!spec.audio;
    };
/**
   * @function attributes
   * @desc This function returns all user-defined attributes in stream.
   * @memberOf Woogeen.Stream
   * @instance
   * @return All the user-defined attributes.
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream attibutes:', stream.attributes());
</script>
   */
    this.attributes = function () {
      return spec.attributes;
    };
/**
   * @function attr
   * @desc This function sets user-defined value in attributes when value is provided; otherwise returns corresponding attribute.
   * @memberOf Woogeen.Stream
   * @instance
   * @param {string} key attribute key.
   * @param {string} value attribute value.
   * @return Existing attribute value if it’s not specified in parameter
   * @example
<script type="text/JavaScript”>
stream.attr(“custom_key”, “custom_value”);
</script>
   */
    this.attr = function (key, value) {
      if (arguments.length > 1) {
        spec.attributes[key] = value;
      }
      return spec.attributes[key];
    };
/**
   * @function id
   * @desc This function returns stream Id assigned by server.
<br><b>Remarks:</b><br>
For local stream, it returns undefined if the stream has not been published; once published, stream Id should be updated by server.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {string} Stream Id assigned by server.
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream added:', stream.id());
</script>
   */
    this.id = function () {
      return spec.id;
    };
//TODO: Named hasScreen in API doc?
/**
   * @function isScreen
   * @desc This function returns true when stream's video track is from screen sharing otherwise false.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream has screen.<br>false The stream does not have screen.
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream hasScreen:', stream.hasScreen());
</script>
   */
    this.isScreen = function () {
      return (!!spec.video) && (spec.video.device === 'screen'); // device: 'camera', 'screen'
    };
    this.bitRate = {
      maxVideoBW: undefined,
      maxAudioBW: undefined
    }; // mutable;
    this.toJson = function () {
      return {
        id: this.id(),
        audio: spec.audio,
        video: spec.video,
        attributes: spec.attributes
      };
    };
  }
/**
   * @function close
   * @desc This function closes the stream.
<br><b>Remarks:</b><br>
The local stream closes once it’s done. If the stream has audio and/or video, it also stops capturing camera/microphone. If the stream is published, the function also un-publishes it. If the stream is showing in an HTML element, the stream would be hidden. Once a LocalStream is closed, it is no longer usable.
   * @memberOf Woogeen.Stream
   * @instance
   * @example
<script type="text/JavaScript”>
var stream = Woogeen.Stream({audio:true, video:true, data: false, attributes:
{name:'WoogeenStream'}});
stream.close();
</script>
   */
  WoogeenStream.prototype.close = function() {
    if (typeof this.hide === 'function') {this.hide();}
    if (this.mediaStream && typeof this.mediaStream.stop === 'function') {
      this.mediaStream.stop();
    }
    this.mediaStream = null;
    if (typeof this.unpublish === 'function') {
      this.unpublish();
    }
    // close peer connection if necessary
    if (this.channel && typeof this.channel.close === 'function') {
      this.channel.close();
    }
  };

  WoogeenStream.prototype.createObjectURL = function() {
    if (!this.mediaStream) {return '';}
    return (window.URL || webkitURL).createObjectURL(this.mediaStream);
  };
  //TODO: fill return description and type
/**
   * @function disableAudio
   * @desc This function disables underlying audio track in the stream if it has audio capacity; otherwise it does nothing.
<br><b>Remarks:</b><br>
For local stream, it stops sending audio packets; for remote stream, it stops decoding audio.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {TYPE} DESCRIPTION
   * @example
<script type="text/JavaScript”>
stream.disableAudio();
</script>
   */
  WoogeenStream.prototype.disableAudio = function(tracknum) {
    var self = this;
    if (self.hasAudio() && self.mediaStream) {
      if (tracknum === undefined) {
        tracknum = 0;
      }
      if (tracknum === -1) {
        return self.mediaStream.getAudioTracks().map(function (track) {
          if (track.enabled) {
            track.enabled = false;
            return true;
          }
          return false;
        });
      }
      var tracks = self.mediaStream.getAudioTracks();
      if (tracks && tracks[tracknum] && tracks[tracknum].enabled) {
        tracks[tracknum].enabled = false;
        return true;
      }
    }
    return false;
  };
  //TODO: fill return description and type
/**
   * @function enableAudio
   * @desc This function enables underlying audio track in the stream if it has audio capacity.
<br><b>Remarks:</b><br>
For local stream, it continues sending audio; for remote stream, it continues decoding audio.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {TYPE} DESCRIPTION
   * @example
<script type="text/JavaScript”>
stream.enableAudio();
</script>
   */
  WoogeenStream.prototype.enableAudio = function(tracknum) {
    var self = this;
    if (self.hasAudio() && self.mediaStream) {
      if (tracknum === undefined) {
        tracknum = 0;
      }
      if (tracknum === -1) {
        return self.mediaStream.getAudioTracks().map(function (track) {
          if (track.enabled !== true) {
            track.enabled = true;
            return true;
          }
          return false;
        });
      }
      var tracks = self.mediaStream.getAudioTracks();
      if (tracks && tracks[tracknum] && tracks[tracknum].enabled !== true) {
        tracks[tracknum].enabled = true;
        return true;
      }
    }
    return false;
  };

  //TODO: fill return description and type
/**
   * @function disableVideo
   * @desc This function disables underlying video track in the stream if it has video capacity; otherwise it does nothing.
<br><b>Remarks:</b><br>
For local stream, it stops sending video packets; for remote stream, it stops decoding video.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {TYPE} DESCRIPTION
   * @example
<script type="text/JavaScript”>
stream.disableVideo();
</script>
   */
  WoogeenStream.prototype.disableVideo = function(tracknum) {
    var self = this;
    if (self.hasVideo() && self.mediaStream) {
      if (tracknum === undefined) {
        tracknum = 0;
      }
      if (tracknum === -1) {
        return self.mediaStream.getVideoTracks().map(function (track) {
          if (track.enabled) {
            track.enabled = false;
            return true;
          }
          return false;
        });
      }
      var tracks = self.mediaStream.getVideoTracks();
      if (tracks && tracks[tracknum] && tracks[tracknum].enabled) {
        tracks[tracknum].enabled = false;
        return true;
      }
    }
    return false;
  };

  //TODO: fill return description and type
/**
   * @function enableVideo
   * @desc This function enables underlying video track in the stream if it has video capacity.
<br><b>Remarks:</b><br>
For local stream, it continues sending video; for remote stream, it continues decoding video.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {TYPE} DESCRIPTION
   * @example
<script type="text/JavaScript”>
stream.enableVideo();
</script>
   */
  WoogeenStream.prototype.enableVideo = function(tracknum) {
    var self = this;
    if (self.hasVideo() && self.mediaStream) {
      if (tracknum === undefined) {
        tracknum = 0;
      }
      if (tracknum === -1) {
        return self.mediaStream.getVideoTracks().map(function (track) {
          if (track.enabled !== true) {
            track.enabled = true;
            return true;
          }
          return false;
        });
      }
      var tracks = self.mediaStream.getVideoTracks();
      if (tracks && tracks[tracknum] && tracks[tracknum].enabled !== true) {
        tracks[tracknum].enabled = true;
        return true;
      }
    }
    return false;
  };

  WoogeenStream.prototype.playAudio = function(onSuccess, onFailure) {
    if (typeof this.signalOnPlayAudio === 'function') {
      return this.signalOnPlayAudio(onSuccess, onFailure);
    }
    if (typeof onFailure === 'function') {
      onFailure('unable to call playAudio');
    }
  };

  WoogeenStream.prototype.pauseAudio = function(onSuccess, onFailure) {
    if (typeof this.signalOnPauseAudio === 'function') {
      return this.signalOnPauseAudio(onSuccess, onFailure);
    }
    if (typeof onFailure === 'function') {
      onFailure('unable to call pauseAudio');
    }
  };

  WoogeenStream.prototype.playVideo = function(onSuccess, onFailure) {
    if (typeof this.signalOnPlayVideo === 'function') {
      return this.signalOnPlayVideo(onSuccess, onFailure);
    }
    if (typeof onFailure === 'function') {
      onFailure('unable to call playVideo');
    }
  };

  WoogeenStream.prototype.pauseVideo = function(onSuccess, onFailure) {
    if (typeof this.signalOnPauseVideo === 'function') {
      return this.signalOnPauseVideo(onSuccess, onFailure);
    }
    if (typeof onFailure === 'function') {
      onFailure('unable to call pauseVideo');
    }
  };

  function WoogeenLocalStream (spec) {
    WoogeenStream.call(this, spec);
  }

  function WoogeenRemoteStream (spec) {
    WoogeenStream.call(this, spec);
/**
   * @function isMixed
   * @desc This function returns true when stream's video track is mixed by server otherwise false.
<br><b>Remarks:</b><br>
Only for Woogeen.RemoteStream.
   * @memberOf Woogeen.RemoteStream
   * @instance
   * @return {boolean} true The stream is mixed stream.<br>false The stream is not mixed stream
   * @example
<script type="text/JavaScript”>
L.Logger.info('stream isMixed:', stream.isMixed());
</script>
   */
    this.isMixed = function () {
      return (!!spec.video) && (spec.video.device === 'mcu');
    };
    this.from = spec.from;
    var listeners = {};
    var self = this;
    Object.defineProperties(this, {
      on: {
        get: function () {
          return function (event, listener) {
            listeners[event] = listeners[event] || [];
            listeners[event].push(listener);
            return self;
          };
        }
      },
      emit: {
        get: function () {
          return function (event) {
            if (listeners[event]) {
              var args = [].slice.call(arguments, 1);
              listeners[event].map(function (fn) {
                fn.apply(self, args);
              });
            }
            return self;
          };
        }
      },
      removeListener: {
        get: function () {
          return function (event, cb) {
            if (cb === undefined) {
              listeners[event] = [];
            } else {
              if (listeners[event]) {
                listeners[event].map(function (fn, index) {
                  if (fn === cb) {
                    listeners[event].splice(index, 1);
                  }
                });
              }
            }
            return self;
          };
        }
      },
      clearListeners: {
        get: function () {
          return function () {
            listeners = {};
            return self;
          };
        }
      }
    });
  }

  WoogeenLocalStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenRemoteStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenLocalStream.prototype.constructor = WoogeenLocalStream;
  WoogeenRemoteStream.prototype.constructor = WoogeenRemoteStream;

  function isLegacyChrome () {
    return window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./) !== null &&
      window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] <= 35;
  }

  function isLegacyIE () {
    return window.navigator.appVersion.indexOf('Trident') > -1 &&
      window.navigator.appVersion.indexOf('rv') > -1;
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
    if (typeof option === 'object' && option !== null && option.url !== undefined) {
      var localStream = new Woogeen.LocalStream(option);
      if (typeof callback === 'function') {
        callback(null, localStream);
      }
      return;
    }
    if (typeof getMedia !== 'function' && !isLegacyIE()) {
      if (typeof callback === 'function') {
        callback({
          code: 1100,
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
                code: 1103,
                msg: 'browser screen sharing not supported'
              });
              return;
            }
          }
        }

        mediaOption.video = JSON.parse(JSON.stringify(supportedVideoList[option.video.resolution] || supportedVideoList.unspecified));
        if(!isLegacyIE()){
          if (!isLegacyChrome() && option.video.frameRate instanceof Array && option.video.frameRate.length >= 2) {
            mediaOption.video.mandatory.minFrameRate = option.video.frameRate[0];
            mediaOption.video.mandatory.maxFrameRate = option.video.frameRate[1];
          }
        }
      }
      if (option.audio) {
        mediaOption.audio = true;
      }
    } else {
      if (typeof callback === 'function') {
        callback({
          code: 1104,
          msg: 'invalid media option'
        });
        return;
      }
    }

    var onSuccess = function (mediaStream) {
      option.mediaStream = mediaStream;
      var localStream = new Woogeen.LocalStream(option);
      if (option.video && option.video.device === 'screen') {
        // when <Stop sharing> button in Browser was pressed, `onended' would
        // be triggered; then we need to close the screen sharing stream.
        // `onended' is a EventHandler containing the action
        // to perform when an ended event is fired on the object,
        // that is when the streaming is terminating.
        mediaStream.onended = function () {
          localStream.close();
        };
      }
      if (mediaOption.video) {
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
      }
      if (typeof callback === 'function') {
        callback(null, localStream);
      }
    };

    var onFailure = function (error) {
      var err = {code: 1100, msg: error.name || error};
      switch (err.msg) {
      // below - internally handled
      case 'Starting video failed':       // firefox: camera possessed by other process?
      case 'TrackStartError':             // chrome: probably resolution not supported
        option.video = {
          device: option.video.device,
          extensionId: option.video.extensionId
        };
        if (init_retry > 0) {
          setTimeout(function () {
            createLocalStream(option, callback, init_retry-1);
          }, 1);
          return;
        } else {
          err.msg = 'MEDIA_OPTION_INVALID';
          err.code = 1104;
        }
        break;
      // below - exposed
      case 'DevicesNotFoundError':        // chrome
        err.msg = 'DEVICES_NOT_FOUND';
        err.code = 1102;
        break;
      case 'NotSupportedError':           // chrome
        err.msg = 'NOT_SUPPORTED';
        err.code = 1105;
        break;
      case 'PermissionDeniedError':       // chrome
        err.msg = 'PERMISSION_DENIED';
        err.code = 1101;
        break;
      case 'PERMISSION_DENIED':           // firefox
        err.code = 1101;
        break;
      case 'ConstraintNotSatisfiedError': // chrome
        err.msg = 'CONSTRAINT_NOT_SATISFIED';
        err.code = 1106;
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
      var extensionId = option.video.extensionId || 'pndohhifhheefbpeljcmnhnkphepimhe';
      mediaOption.audio = false;
      try {
        chrome.runtime.sendMessage(extensionId, {getStream: true}, function (response) {
          if (response === undefined) {
            if (typeof callback === 'function') {
              callback({
                code: 1103,
                msg: 'screen sharing plugin inaccessible'
              });
            }
            return;
          }
          mediaOption.video.mandatory.chromeMediaSource = 'desktop';
          mediaOption.video.mandatory.chromeMediaSourceId = response.streamId;
          getMedia.apply(navigator, [mediaOption, onSuccess, onFailure]);
        });
      } catch (err) {
        if (typeof callback === 'function') {
          callback({
            code: 1103,
            msg: 'screen sharing plugin inaccessible',
            err: err
          });
        }
      }
      return;
    }
    if(!isLegacyIE()){
      getMedia.apply(navigator, [mediaOption, onSuccess, onFailure]);
    }else{
      navigator.getUserMedia(mediaOption, onSuccess, onFailure);
    }
  }
/**
   * @function create
   * @desc This factory returns a Woogeen.LocalStream instance with user defined options.<br>
<br><b>Remarks:</b><br>
When the video/audio parameters are not supported by the browser, a fallback parameter set will be used; if the fallback also fails, a callback is invoked with an error defined below as follows:
<ul>
    <li><b>DEVICES_NOT_FOUND</b> – no camera or microphone available.</li>
    <li><b>PERMISSION_DENIED</b> – access camera/microphone denied.</li>
    <li><b>MEDIA_OPTION_INVALID</b> – video/audio parameters are invalid on browser and fallback fails.</li>
    <li><b>CONSTRAINT_NOT_SATISFIED</b> – one of the mandatory constraints could not be satisfied.</li>
    <li><b>STREAM_HAS_NO_MEDIA_ATTRIBUTES</b> – both audio and video are false.</li>
    <li><b>UNDEFINED</b> – uncategorized.</li>
</ul>
<br>parameter 'options':
<ul>
    <li>audio: true/false.</li>
    <li>video: device, resolution, frameRate.</li>
        <ul>
            <li>Valid device list:</li>
                <ul>
                    <li>'camera' for stream from camera;</li>
                    <li>'screen' for stream from screen;<br>
                    Screen stream creating can be done only when your web-app is in HTTPS/SSL environment.
                    </li>
                </ul>
            <li>Valid resolution list:</li>
                <ul>
                    <li>'unspecified'</li>
                    <li>'sif'</li>
                    <li>'vga'</li>
                    <li>'hd720p'</li>
                    <li>'hd1080p'</li>
                </ul>
            <li>frameRate should be an array as [min_frame_rate, max_frame_rate], in which each element should be a proper number, e.g., [20, 30].</li>
            <li><b>Note</b>: Firefox currently does not fully support resolution or frameRate setting.</li>
        </ul>
</ul>
   * @memberOf Woogeen.LocalStream
   * @static
   * @param {json} options Share screen options.
   * @param {function} callback callback(err, localStream) will be invoked when LocalStream creation is done.
   * @return {Woogeen.LocalStream} An instance of Woogeen.LocalStream.
   * @example
<script type="text/javascript>
// LocalStream
var localStream;
Woogeen.LocalStream.create({
video: {
device: 'camera',
resolution: 'vga',
},
audio: true
}, function (err, stream) {
if (err) {
return console.log('create LocalStream failed:', err);
}
localStream = stream;
});
// RemoteStream
conference.on('stream-added', function (event) {
var remoteStream = event.stream;
console.log('stream added:', stream.id());
});
</script>
   */
  WoogeenLocalStream.create = function() {
    createLocalStream.apply(this, arguments);
  };

  Woogeen.Stream = WoogeenStream;
  //TODO: fill description of LocalSteam and RemoteStream
/**
 * @namespace Woogeen.LocalStream
 * @extends Woogeen.Stream
 * @classDesc To be filled...
 */
  Woogeen.LocalStream = WoogeenLocalStream;
/**
 * @namespace Woogeen.RemoteStream
 * @extends Woogeen.Stream
 * @classDesc To be filled...
 */
  Woogeen.RemoteStream = WoogeenRemoteStream;

  // return {
  //   Stream: WoogeenStream,
  //   LocalStream: WoogeenLocalStream,
  //   RemoteStream: WoogeenRemoteStream
  // };
}());
