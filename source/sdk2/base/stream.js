/* global webkitURL, chrome */
(function () {
  'use strict';
/**
    *@namespace Woogeen
    *@desc TODO:Description of namespace Woogeen.
    */
/**
 * @class Woogeen.Stream
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
<script type="text/JavaScript">
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
<script type="text/JavaScript">
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
<script type="text/JavaScript">
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
   * @return Existing attribute value if it's not specified in parameter
   * @example
<script type="text/JavaScript">
stream.attr("custom_key", "custom_value");
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
For local stream, it returns null if the stream has not been published; once published, stream Id should be updated by server.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {string} Stream Id assigned by server.
   * @example
<script type="text/JavaScript">
L.Logger.info('stream added:', stream.id());
</script>
   */
    this.id = function () {
      return spec.id || null;
    };
/**
   * @function isScreen
   * @desc This function returns true when stream's video track is from screen sharing otherwise false.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream is from screen;<br>otherwise false.
   * @example
<script type="text/JavaScript">
L.Logger.info('stream is from screen?', stream.isScreen());
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
If the stream has audio and/or video, it also stops capturing camera/microphone. If the stream is published to a conference, the function also un-publishes it. If the stream is published to a P2P session, the function does NOT un-publish it. If the stream is showing in an HTML element, the stream would be hidden. Once a LocalStream is closed, it is no longer usable.
   * @memberOf Woogeen.Stream
   * @instance
   * @example
<script type="text/JavaScript">
var stream = Woogeen.Stream({audio:true, video:true, data: false, attributes:
{name:'WoogeenStream'}});
stream.close();
</script>
   */
  WoogeenStream.prototype.close = function() {
    if (typeof this.hide === 'function') {this.hide();}
    if (this.mediaStream) {
      this.mediaStream.getTracks().map(function(track) {
        if (typeof track.stop === 'function') {
          track.stop();
        }
      });
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
/**
   * @function disableAudio
   * @desc This function disables underlying audio track in the stream if it has audio capacity; otherwise it does nothing.
<br><b>Remarks:</b><br>
For remote stream, it stops decoding audio; for local stream, it also stops capturing audio.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream has audio and the audio track is enabled previously; <br> otherwise false.
   * @example
<script type="text/JavaScript">
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
/**
   * @function enableAudio
   * @desc This function enables underlying audio track in the stream if it has audio capacity.
<br><b>Remarks:</b><br>
For remote stream, it continues decoding audio; for local stream, it also continues capturing audio.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream has audio and the audio track is disabled previously; <br> otherwise false.
   * @example
<script type="text/JavaScript">
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

/**
   * @function disableVideo
   * @desc This function disables underlying video track in the stream if it has video capacity; otherwise it does nothing.
<br><b>Remarks:</b><br>
For remote stream, it stops decoding video; for local stream, it also stops capturing video.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream has video and the video track is enabled previously; <br> otherwise false.
   * @example
<script type="text/JavaScript">
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

/**
   * @function enableVideo
   * @desc This function enables underlying video track in the stream if it has video capacity.
<br><b>Remarks:</b><br>
For remote stream, it continues decoding video; for local stream, it also continues capturing video.
   * @memberOf Woogeen.Stream
   * @instance
   * @return {boolean} true The stream has video and the video track is disabled previously; <br> otherwise false.
   * @example
<script type="text/JavaScript">
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

  function WoogeenLocalStream (spec) {
    WoogeenStream.call(this, spec);
  }

  function WoogeenRemoteStream (spec) {
    WoogeenStream.call(this, spec);
/**
   * @function isMixed
   * @desc This function returns true when stream's video track is mixed by server otherwise false.
<br><b>Remarks:</b><br>
Deprecated, use <code>instanceof Woogeen.RemoteMixedStream</code> instead.
   * @memberOf Woogeen.RemoteStream
   * @instance
   * @return {boolean} true The stream is mixed stream.<br>false The stream is not mixed stream
   * @example
<script type="text/JavaScript">
L.Logger.info('stream isMixed:', stream.isMixed());
</script>
   */
    this.isMixed = function () {
      return false;
    };

    this.from = spec.from;
    var listeners = {};
    var self = this;
    Object.defineProperties(this, {
/**
   * @function on
   * @desc This function registers a listener for a specified event, which would be called when the event occurred.
<br><b>Remarks:</b><br>
Reserved events from MCU:<br>
<table class="params table table-striped">
<thead>
  <tr><th align="center">Event Name</th><th align="center">Description</th><th align="center">Status</th></tr>
</thead>
<tbody>
  <tr><td align="center"><code>VideoLayoutChanged</code></td><td align="center">Video layout of a mix (remote) stream changed</td><td align="center">stable</td></tr>
  <tr><td align="center"><code>VideoEnabled</code></td><td align="center">Video track of a remote stream enabled</td><td align="center">reserved</td></tr>
  <tr><td align="center"><code>VideoDisabled</code></td><td align="center">Video track of a remote stream disabled</td><td align="center">reserved</td></tr>
  <tr><td align="center"><code>AudioEnabled</code></td><td align="center">Audio track of a remote stream enabled</td><td align="center">reserved</td></tr>
  <tr><td align="center"><code>AudioDisabled</code></td><td align="center">Audio track of a remote stream disabled</td><td align="center">reserved</td></tr>
</tbody>
</table>
User-defined events and listeners are also supported, See {@link Woogeen.RemoteStream#emit|stream.emit(event, data)} method.
   * @memberOf Woogeen.RemoteStream
   * @param {string} event Event name.
   * @param {function} listener(data) Callback function.
   * @instance
   * @example
<script type="text/JavaScript">
if (stream.isMixed()) {
  stream.on('VideoLayoutChanged', function () {
    L.Logger.info('stream', stream.id(), 'video layout changed');
  });
}
</script>
   */
      on: {
        get: function () {
          return function (event, listener) {
            listeners[event] = listeners[event] || [];
            listeners[event].push(listener);
            return self;
          };
        }
      },
/**
   * @function emit
   * @desc This function triggers a specified event, which would invoke corresponding event listener(s).
   * @memberOf Woogeen.RemoteStream
   * @param {string} event Event name.
   * @param {user-defined} data Data fed to listener function.
   * @instance
   */
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
/**
   * @function removeListener
   * @desc This function removes listener(s) for a specified event. If listener is unspecified, all the listener(s) of the event would be removed; or if the listener is in the event listener list, it would be removed; otherwise this function does nothing.
   * @memberOf Woogeen.RemoteStream
   * @param {string} event Event name.
   * @param {function} listener Corresponding callback function (optional).
   * @instance
   */
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
/**
   * @function clearListeners
   * @desc This function removes all registered listener(s) for all events on the stream.
   * @memberOf Woogeen.RemoteStream
   * @instance
   */
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

  function WoogeenRemoteMixedStream (spec) {
    WoogeenRemoteStream.call(this, spec);
/**
   * @function resolutions
   * @desc This function returns an array of supported resolutions for mixed stream.
   * @memberOf Woogeen.RemoteMixedStream
   * @instance
   * @return {Array}
   */
    this.resolutions = function () {
      if (spec.video.resolutions instanceof Array) {
        return spec.video.resolutions.map(function (resolution) {
          return resolution;
        });
      }
      return [];
    };

    this.isMixed = function () {
      return true;
    };
  }

  WoogeenLocalStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenRemoteStream.prototype = Object.create(WoogeenStream.prototype);
  WoogeenRemoteMixedStream.prototype = Object.create(WoogeenRemoteStream.prototype);
  WoogeenLocalStream.prototype.constructor = WoogeenLocalStream;
  WoogeenRemoteStream.prototype.constructor = WoogeenRemoteStream;
  WoogeenRemoteMixedStream.prototype.constructor = WoogeenRemoteMixedStream;


  function isLegacyChrome () {
    return window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./) !== null &&
      window.navigator.appVersion.match(/Chrome\/([\w\W]*?)\./)[1] <= 35;
  }

  function isLegacyIE () {
    return window.navigator.appVersion.indexOf('Trident') > -1 &&
      window.navigator.appVersion.indexOf('rv') > -1;
  }

  function isFirefox () {
    return window.navigator.userAgent.match("Firefox") !== null;
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
          code: 1107,
          msg: 'USER_INPUT_INVALID'
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
      if (isFirefox()) {
        mediaOption = { video: { mediaSource: 'window' || 'screen' }};
        getMedia.apply(navigator, [mediaOption, onSuccess, onFailure]);
        return;
      }
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
When the video/audio parameters are not supported by the browser, a fallback parameter set will be used; if the fallback also fails, the callback (if specified) is invoked with an error. See details in callback description.
<br><b>options:</b>
<ul>
    <li>audio: true/false.</li>
    <li>video: device, resolution, frameRate, extensionId.</li>
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
            <li>extensionId is id of Chrome Extension for screen sharing. If not provided, the id of <a href="https://chrome.google.com/webstore/detail/webrtc-desktop-sharing-ex/pndohhifhheefbpeljcmnhnkphepimhe">WebRTC Desktop Sharing Extension</a> would be used.</li>
            <li><b>Note</b>: Firefox currently does not fully support resolution or frameRate setting.</li>
        </ul>
    <li>url: RTSP stream URL</li>
</ul>
<br><b>callback:</b>
<br>Upon success, err is null, and localStream is an instance of Woogeen.LocalStream; upon failure localStream is undefined and err is one of the following:<br>
<ul>
  <li><b>{code: 1100, msg: xxx}</b> - general stream creation error, e.g., no WebRTC support in browser, uncategorized error, etc.</li>
  <li><b>{code: 1101, msg: 'PERMISSION_DENIED'}</b> – access media (camera, microphone, etc) denied.</li>
  <li><b>{code: 1102, msg: 'DEVICES_NOT_FOUND'}</b> – no camera or microphone available.</li>
  <li><b>{code: 1103, msg: xxx}</b> - error in accessing screen sharing plugin: not supported, not installed or disabled.</li>
  <li><b>{code: 1104, msg: 'MEDIA_OPTION_INVALID'}</b> – video/audio parameters are invalid on browser and fallback fails.</li>
  <li><b>{code: 1105, msg: 'NOT_SUPPORTED'}</b> - media option not supported by the browser.</li>
  <li><b>{code: 1106, msg: 'CONSTRAINT_NOT_SATISFIED'}</b> – one of the mandatory constraints could not be satisfied.</li>
  <li><b>{code: 1107, msg: 'USER_INPUT_INVALID'}</b> – user input media option is invalid.</li>
</ul>
   * @memberOf Woogeen.LocalStream
   * @static
   * @param {json} options Stream creation options.
   * @param {function} callback callback(err, localStream) will be invoked when LocalStream creation is done.
   * @example
<script type="text/javascript">
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
</script>
   */
  WoogeenLocalStream.create = function() {
    createLocalStream.apply(this, arguments);
  };

  Woogeen.Stream = WoogeenStream;

/**
 * @class Woogeen.LocalStream
 * @extends Woogeen.Stream
 * @classDesc Stream from browser constructed from camera, screen, external input(e.g. rtsp)... Use create(options, callback) factory to create an instance.
 */
  Woogeen.LocalStream = WoogeenLocalStream;
/**
 * @class Woogeen.RemoteStream
 * @extends Woogeen.Stream
 * @classDesc Stream from server retrieved by 'stream-added' event. RemoteStreams are automatically constructed upon the occurrence of the event.
<br><b>Example:</b>
```
<script type="text/javascript">
conference.on('stream-added', function (event) {
  var remoteStream = event.stream;
console.log('stream added:', stream.id());
});
</script>
```
 */
  Woogeen.RemoteStream = WoogeenRemoteStream;
/**
 * @class Woogeen.RemoteMixedStream
 * @extends Woogeen.RemoteStream
 * @classDesc A RemoteStream whose video track is mixed by server.
 */
  Woogeen.RemoteMixedStream = WoogeenRemoteMixedStream;

}());
