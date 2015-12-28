/*global require, exports, , setInterval, clearInterval*/

var addon = require('./../../bindings/mcu/build/Release/addon');
var logger = require('./../common/logger').logger;

// Logger
var log = logger.getLogger("RtspIn");

exports.RtspIn = function (spec) {
    "use strict";

    var that = {},
        audio = false,
        video = false,
        rtspClient = undefined;

    /*
     * Given a WebRtcConnection waits for the state CANDIDATES_GATHERED for set remote SDP.
     */
    var initRtspClient = function (rtspClient, on_status) {
        var terminated = false;
        var audio_codec_list = [],
            video_codec_list = [];

        rtspClient.init(function (msg){

          var localSdp, answer;
          log.info("RtspIn Addon status:" + msg);

          if (msg.startsWith('videoCodec')) {
              video_codec_list.push(msg.split(':')[1]);
          } else if (msg.startsWith('audioCodec')) {
              audio_codec_list.push(msg.split(':')[1]);
          } else if (msg === 'success') {
              on_status({type: 'ready', audio_codecs: audio_codec_list, video_codecs: video_codec_list});
          } else {
              on_status(msg);
          }
        });

        on_status({type: 'initializing'});
    };


    that.init = function (url, expect_audio, expect_video, transport, buffer_size, on_status) {
        audio = expect_audio;
        video = expect_video;
        rtspClient = new addon.RtspIn(url, audio, video, transport, buffer_size);

        initRtspClient(rtspClient, on_status);
    };

    that.close = function () {
        rtspClient.close();
    };

    that.addDestination = function (track, dest) {
        if (audio && track === 'audio') {
            rtspClient.addDestination('audio', dest);
            return;
        } else if (video && track === 'video') {
            rtspClient.addDestination('video', dest);
            return;
        } else {
            log.warn("Wrong track:"+track);
        }
    };

    that.removeDestination = function (track, dest) {
        if (audio && track === 'audio') {
            rtspClient.removeDestination('audio', dest);
            return;
        } else if (video && track === 'video') {
            rtspClient.removeDestination('video', dest);
            return;
        }

        log.warn("Wrong track:"+track);
    };

    return that;
};
