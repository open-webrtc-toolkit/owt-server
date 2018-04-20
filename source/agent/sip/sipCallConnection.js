/*global require, exports, GLOBAL*/
'use strict';
require = require('module')._load('./AgentLoader');
var woogeenSipGateway = require('./sipIn/build/Release/sipIn');
var AudioFrameConstructor = woogeenSipGateway.AudioFrameConstructor;
var VideoFrameConstructor = woogeenSipGateway.VideoFrameConstructor;
var AudioFramePacketizer = woogeenSipGateway.AudioFramePacketizer;
var VideoFramePacketizer = woogeenSipGateway.VideoFramePacketizer;
var path = require('path');
var logger = require('./logger').logger;
var log = logger.getLogger('SipCallConnection');
exports.SipCallConnection = function (spec, onMediaUpdate) {
    var that = {},
        input = true,
        output = true,
        gateway = spec.gateway,
        clientID = spec.clientID,
        audio = spec.audio,
        video = spec.video,
        support_red = spec.red,
        support_ulpfec = spec.ulpfec,
        audioFrameConstructor,
        audioFramePacketizer,
        videoFrameConstructor,
        videoFramePacketizer,
        sip_callConnection;

    sip_callConnection = new woogeenSipGateway.SipCallConnection(gateway, clientID);
    if (audio) {
        // sip->mcu
        audioFrameConstructor = new AudioFrameConstructor();
        audioFrameConstructor.bindTransport(sip_callConnection);

        // mcu->sip
        audioFramePacketizer = new AudioFramePacketizer();
        audioFramePacketizer.bindTransport(sip_callConnection);
    }
    if (video) {
        videoFrameConstructor = new VideoFrameConstructor();
        videoFrameConstructor.bindTransport(sip_callConnection);
        videoFrameConstructor.addEventListener('mediaInfo', function (mediaUpdate) {
            onMediaUpdate(clientID, 'in', mediaUpdate);
        });

        videoFramePacketizer = new VideoFramePacketizer(support_red, support_ulpfec);
        videoFramePacketizer.bindTransport(sip_callConnection);
    }

    that.close = function (direction) {
        log.debug('SipCallConnection close');
        if (direction.output) {
          //audio && audioFramePacketizer && audioFramePacketizer.close();
          //video && videoFramePacketizer && videoFramePacketizer.close();
          output = false;
        }
        // sip_callConnection && sip_callConnection.close();
        if (direction.input) {
          //audio && audioFrameConstructor && audioFrameConstructor.close();
          //video && videoFrameConstructor && videoFrameConstructor.close();
          input = false;
        }
        if (!(input || output) ) {
            if (audio && audioFramePacketizer) {
                audioFramePacketizer.unbindTransport();
                audioFramePacketizer.close();
                audioFramePacketizer = undefined;
            }

            if (video && videoFramePacketizer) {
                videoFramePacketizer.unbindTransport();
                videoFramePacketizer.close();
                videoFramePacketizer = undefined;
            }

            if (audio && audioFrameConstructor) {
                audioFrameConstructor.unbindTransport();
                audioFrameConstructor.close();
                audioFrameConstructor = undefined;
            }

            if (video && videoFrameConstructor) {
                videoFrameConstructor.unbindTransport();
                videoFrameConstructor.close();
                videoFrameConstructor = undefined;
            }

            sip_callConnection && sip_callConnection.close();
            sip_callConnection = undefined;
            log.debug('Completely close');
        }
    };

    that.addDestination = function (track, dest) {
        if (audio && track === 'audio' && audioFrameConstructor) {
            audioFrameConstructor.addDestination(dest);
            return;
        } else if (video && track === 'video' && videoFrameConstructor) {
            videoFrameConstructor.addDestination(dest);
            return;
        }

        log.warn('Wrong track:'+track);
    };

    that.removeDestination = function (track, dest) {
        if (audio && track === 'audio' && audioFrameConstructor) {
            audioFrameConstructor.removeDestination(dest);
            return;
        } else if (video && track === 'video' && videoFrameConstructor) {
            videoFrameConstructor.removeDestination(dest);
            return;
        }

        log.warn('Wrong track:'+track);
    };

    that.receiver = function (track) {
        if (audio && track === 'audio') {
            return audioFramePacketizer;
        }

        if (video && track === 'video') {
            return videoFramePacketizer;
        }

        log.error('receiver error');
        return undefined;
    };

    that.requestKeyFrame = function() {
        if (video && videoFrameConstructor)
            videoFrameConstructor.requestKeyFrame();
    };
    return that;
};
