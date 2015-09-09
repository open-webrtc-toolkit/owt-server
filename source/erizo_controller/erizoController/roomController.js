/*global require, exports, setInterval*/
'use strict';

var logger = require('./../common/logger').logger;
var erizoController = require('./erizoController');
// Logger
var log = logger.getLogger('RoomController');

exports.RoomController = function (spec) {
    var that = {},
        // {id: array of subscribers}
        subscribers = {},
        // {id: OneToManyProcessor}
        publishers = {},

        erizos = {},

        // {id: ExternalOutput}
        externalOutputs = {};

    var rpc = spec.rpc;
    var agentId = spec.agent_id;
    var roomId = spec.id;

    var KEEPALIVE_INTERVAL = 5*1000;

    var eventListeners = [];

    var callbackFor = function(erizo_id, publisher_id) {
        return function(ok) {
            if (ok !== true) {
                dispatchEvent("unpublish", publisher_id);
                rpc.callRpc("ErizoAgent_" + agentId, "deleteErizoJS", [erizo_id], {callback: function(){
                    delete erizos[publisher_id];
                }});
            }
        }
    };

    var sendKeepAlive = function() {
        for (var publisher_id in erizos) {
            var erizo_id = erizos[publisher_id];
            rpc.callRpc(getErizoQueue(publisher_id), "keepAlive", [], {callback: callbackFor(erizo_id, publisher_id)});
        }
    };

    setInterval(sendKeepAlive, KEEPALIVE_INTERVAL);

    var createErizoJS = function(publisher_id, mixer_id, callback) {
        if (erizos[publisher_id] !== undefined) {
            callback();
            return;
        }
        // Currently we want to make sure the publishers and the mixer they associate with
        // are run in the same process. This is the requirement of in-process mixer.
        if (!GLOBAL.config.erizoController.outOfProcessMixer && mixer_id !== undefined && erizos[mixer_id] !== undefined) {
            erizos[publisher_id] = erizos[mixer_id];
            callback();
            return;
        }
        rpc.callRpc("ErizoAgent_" + agentId, "createErizoJS", [publisher_id], {callback: function(erizo_id) {
            log.debug("Answer", erizo_id);
            erizos[publisher_id] = erizo_id;
            callback();
            rpc.callRpc('ErizoJS_'+erizo_id, 'setControllerId', [exports.myId, roomId], {});
        }});
    };

    var getErizoQueue = function(publisher_id) {
        return "ErizoJS_" + erizos[publisher_id];
    };

    var dispatchEvent = function(type, event) {
        for (var event_id in eventListeners) {
            eventListeners[event_id](type, event);    
        }
        
    };

    var unescapeSdp = function(sdp) {
        var reg = new RegExp(/\\\//g);
        var newSdp = sdp.replace(reg, '/');
        return newSdp;
    };

    that.addEventListener = function(eventListener) {
        eventListeners.push(eventListener);
    };

    /*
     * Initialize the mixer in the room.
     */
    that.initMixer = function (id, roomConfig, callback) {
        log.info("Adding mixer id ", id);

        // We create a new ErizoJS with the id.
        createErizoJS(id, id, function(erizo_id) {
            log.info("Erizo created");
            // then we call its initMixer method.
            var args = [id, GLOBAL.config.erizoController.outOfProcessMixer, roomConfig];
            rpc.callRpc(getErizoQueue(id), "initMixer", args, {callback: callback});

            // Track publisher locally
            publishers[id] = id;
            subscribers[id] = [];
            setTimeout(function () {
                that.addRTSPOut(id, function (resp) {
                    log.info('rtsp streaming', id, '->', resp);
                });
            }, 1);
        });
    };

    that.addExternalInput = function (publisher_id, options, mixer_id, callback, onReady) {

        if (publishers[publisher_id] === undefined) {

            log.info("Adding external input peer_id ", publisher_id, ", mixer id", mixer_id);

            createErizoJS(publisher_id, mixer_id, function() {
                // then we call its addExternalInput method.
                var mixer = {id: mixer_id, oop: GLOBAL.config.erizoController.outOfProcessMixer};
                var args = [publisher_id, options, mixer];
                rpc.callRpc(getErizoQueue(publisher_id), "addExternalInput", args, {callback: callback, onReady: onReady});

                // Track publisher locally
                publishers[publisher_id] = publisher_id;
                subscribers[publisher_id] = [];

            });
        } else {
            log.info("Publisher already set for", publisher_id);
        }
    };

    that.addExternalOutput = function (video_publisher_id, audio_publisher_id, output_id, mixer_id, url, interval, callback) {
        if (publishers[video_publisher_id] !== undefined && publishers[audio_publisher_id] !== undefined) {
            log.info("Adding ExternalOutput to " + video_publisher_id + " and " + audio_publisher_id + " with url: " + url);

            createErizoJS(output_id, mixer_id, function() {
                var externalOutput = externalOutputs[output_id];
                if (externalOutput) {
                    return callback({
                        success: false,
                        text: 'the external output is busy'
                    });
                }

                var erizoVideo = getErizoQueue(video_publisher_id);
                var erizoAudio = getErizoQueue(audio_publisher_id);
                if (erizoVideo !== erizoAudio) {
                    return callback({
                        success: false,
                        text: 'cannot make external output for video and audio from different erizo nodes'
                    });
                }

                var args = [video_publisher_id, audio_publisher_id, output_id, url, interval];

                rpc.callRpc(getErizoQueue(output_id), "addExternalOutput", args, {callback: function (result) {
                    if (result === 'success') {
                        // Track external outputs
                        externalOutputs[output_id] = {video: video_publisher_id, audio: audio_publisher_id};

                        return callback({
                            success: true,
                            text: url
                        });
                    }

                    callback({
                        success: false,
                        text: result
                    });
                }});
            });
        } else {
            callback({
                success: false,
                text: 'target stream(s) not found'
            });
        }
    };

    that.removeExternalOutput = function (output_id, close, callback) {
        // FIXME: It seems there is no need to verify the video_publisher_id and audio_publisher_id
        if (close === undefined) {
            close = true;
        }

        var externalOutput = externalOutputs[output_id];
        var video_publisher_id = null;
        var audio_publisher_id = null;

        if (externalOutput) {
            video_publisher_id = externalOutput.video;
            audio_publisher_id = externalOutput.audio;
        }

        if (!externalOutput || !video_publisher_id || !audio_publisher_id) {
            if (!close) {
                return callback({
                    success: true,
                    text: 'no external output context needs to be cleaned'
                });
            }

            return callback({
                success: false,
                text: 'no external output ongoing'
            });
        }

        var erizoVideo = getErizoQueue(video_publisher_id);
        var erizoAudio = getErizoQueue(audio_publisher_id);
        if (erizoVideo !== erizoAudio) {
            return callback({
                success: false,
                text: 'cannot stop video and audio external output from different erizo nodes'
            });
        }

        if (publishers[video_publisher_id] !== undefined && publishers[audio_publisher_id] !== undefined) {
            log.info("Stopping ExternalOutput: " + output_id);

            var args = [output_id, close];

            rpc.callRpc(getErizoQueue(output_id), "removeExternalOutput", args, {callback: function (result) {
                if (result === 'success') {
                    // Remove the track
                    delete externalOutputs[output_id];
                    return callback({
                        success: true,
                        text: output_id
                    });
                }

                callback({
                    success: false,
                    text: 'stop external output failed'
                });
            }});
        } else {
            callback({
                success: false,
                text: 'Not valid external output to stop'
            });
        }
    };

    /*
     * Adds a publisher to the room. This creates a new OneToManyProcessor
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the OneToManyProcessor.
     */
    that.addPublisher = function (publisher_id, sdp, mixer_id, unmix, callback, onReady) {

        if (publishers[publisher_id] === undefined) {

            log.info('Adding publisher peer_id', publisher_id);

            sdp = unescapeSdp(sdp);

            // We create a new ErizoJS with the publisher_id.
            createErizoJS(publisher_id, mixer_id, function () {
                log.info('Erizo created');
                // then we call its addPublisher method.
                var mixer = {id: mixer_id, oop: GLOBAL.config.erizoController.outOfProcessMixer};
                var args = [publisher_id, sdp, mixer, unmix];
                rpc.callRpc(getErizoQueue(publisher_id), 'addPublisher', args, {callback: callback, onReady: onReady});

                // Track publisher locally
                publishers[publisher_id] = publisher_id;
                subscribers[publisher_id] = [];
            });

        } else {
            log.info('Publisher already set for', publisher_id);
            callback('error', 'publisher already added');
        }
    };

    /*
     * Adds a subscriber to the room. This creates a new WebRtcConnection.
     * This WebRtcConnection will be added to the subscribers list of the
     * OneToManyProcessor.
     */
    that.addSubscriber = function (subscriber_id, publisher_id, audio, video, sdp, callback, onReady) {
        if (typeof sdp !== 'string') sdp = JSON.stringify(sdp);

        if (sdp === null || subscriber_id === null) {
            callback('error', 'invalid sdp or id');
            return;
        }

        sdp = unescapeSdp(sdp);

        if (publishers[publisher_id] !== undefined && subscribers[publisher_id].indexOf(subscriber_id) === -1 && sdp.match('OFFER') !== null) {

            log.info("Adding subscriber ", subscriber_id, ' to publisher ', publisher_id);

            if (audio === undefined) audio = true;
            if (video === undefined) video = true;

            var args = [subscriber_id, publisher_id, audio, video, sdp];

            rpc.callRpc(getErizoQueue(publisher_id), "addSubscriber", args, {callback: callback, onReady: onReady});

            // Track subscriber locally
            subscribers[publisher_id].push(subscriber_id);
        } else {
            log.info('subscriber', subscriber_id, 'already added');
            callback('error', 'subscriber already added');
        }
    };

    /*
     * Removes a publisher from the room. This also deletes the associated OneToManyProcessor.
     */
    that.removePublisher = function (publisher_id) {

        if (subscribers[publisher_id] !== undefined && publishers[publisher_id] !== undefined) {

            var output_id = -1;
            for (var i in externalOutputs) {
                if (externalOutputs.hasOwnProperty(i) && (externalOutputs[i].video === publisher_id || externalOutputs[i].audio === publisher_id)) {
                    output_id = i;
                    break;
                }
            }

            if (output_id !== -1) {
                log.info('Removing external output', output_id);
                var args = [output_id, false];
                rpc.callRpc(getErizoQueue(output_id), "removeExternalOutput", args, {callback: function (result) {}});

                // Remove the external output track anyway
                delete externalOutputs[output_id];
            }

            var args = [publisher_id];
            rpc.callRpc(getErizoQueue(publisher_id), "removePublisher", args, undefined);

            // Remove tracks
            log.info('Removing subscribers', publisher_id);
            delete subscribers[publisher_id];
            log.info('Removing publisher', publisher_id);
            delete publishers[publisher_id];
            log.info('Removed all');
            delete erizos[publisher_id];
            log.info('Removing muxer', publisher_id, ' muxers left ', Object.keys(publishers).length );
        }
    };

    /*
     * Removes a subscriber from the room. This also removes it from the associated OneToManyProcessor.
     */
    that.removeSubscriber = function (subscriber_id, publisher_id) {

        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Removing subscriber ', subscriber_id, 'to muxer ', publisher_id);

            var args = [subscriber_id, publisher_id];
            rpc.callRpc(getErizoQueue(publisher_id), "removeSubscriber", args, undefined);

            // Remove track
            subscribers[publisher_id].splice(index, 1);
        }
    };

    /*
     * Removes all the subscribers related with a client.
     */
    that.removeSubscriptions = function (subscriber_id) {

        var publisher_id, index;

        log.info('Removing subscriptions of ', subscriber_id);


        for (publisher_id in subscribers) {
            if (subscribers.hasOwnProperty(publisher_id)) {
                index = subscribers[publisher_id].indexOf(subscriber_id);
                if (index !== -1) {
                    log.info('Removing subscriber ', subscriber_id, 'to muxer ', publisher_id);

                    var args = [subscriber_id, publisher_id];
                    rpc.callRpc(getErizoQueue(publisher_id), "removeSubscriber", args, undefined);

                    // Remove tracks
                    subscribers[publisher_id].splice(index, 1);
                }
            }
        }
    };

    that.addToMixer = function addToMixer (publisher_id, mixer_id, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('error', 'stream not published');
        }
        rpc.callRpc(getErizoQueue(publisher_id), 'addToMixer', [publisher_id, mixer_id], {callback: callback});

    };

    that.removeFromMixer = function removeFromMixer (publisher_id, mixer_id, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('error', 'stream not published');
        }
        rpc.callRpc(getErizoQueue(publisher_id), 'removeFromMixer', [publisher_id, mixer_id], {callback: callback});
    };

    that.publisherNum = function () {
        return Object.keys(publishers).length;
    };

    that.getRegion = function (mixer_id, publisher_id, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback({
                success: false,
                text: 'stream not published'
            });
        }
        if (publishers[mixer_id] === undefined) {
            return callback({
                success: false,
                text: 'mixer is not available'
            });
        }
        var args = [mixer_id, publisher_id];
        rpc.callRpc(getErizoQueue(mixer_id), "getRegion", args, {callback: function (result) {
            if (result) {
                return callback({
                    success: true,
                    text: result
                });
            }

            callback({
                success: false,
                text: 'Cannot find the participant in the mixed video'
            });
        }});
    };

    that.setRegion = function (mixer_id, publisher_id, region_id, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('stream not published');
        }
        if (publishers[mixer_id] === undefined) {
            return callback('mixer is not available');
        }
        var args = [mixer_id, publisher_id, region_id];
        rpc.callRpc(getErizoQueue(mixer_id), "setRegion", args, {callback: callback});
    };

    that.setVideoBitrate = function (mixer_id, publisher_id, bitrate, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('stream not published');
        }
        if (publishers[mixer_id] === undefined) {
            return callback('mixer is not available');
        }
        var args = [mixer_id, publisher_id, bitrate];
        rpc.callRpc(getErizoQueue(mixer_id), "setVideoBitrate", args, {callback: callback});
    };

    that.addRTSPOut = function (mixer_id, callback) {
        if (publishers[mixer_id] !== undefined) {
            var args = [mixer_id, mixer_id, '', '', 0];
            rpc.callRpc(getErizoQueue(mixer_id), 'addExternalOutput', args, {callback: function (result) {
                callback(result);
            }});
        } else {
            callback('mixer not found');
        }
    };

    that['audio-in-on'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Enabling [audio] subscriber', subscriber_id, 'in', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'subscribeStream', [publisher_id, subscriber_id, true], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['audio-in-off'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Disabling [audio] subscriber', subscriber_id, 'in', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'unsubscribeStream', [publisher_id, subscriber_id, true], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['video-in-on'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Enabling [video] subscriber', subscriber_id, 'in', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'subscribeStream', [publisher_id, subscriber_id, false], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['video-in-off'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Disabling [video] subscriber', subscriber_id, 'in', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'unsubscribeStream', [publisher_id, subscriber_id, false], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['audio-out-on'] = function (publisher_id, unused, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Enabling [audio] publisher', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'publishStream', [publisher_id, true], {callback: function (err) {
                if (!err) {
                    erizoController.handleEventReport('updateStream', roomId, {event: 'AudioEnabled', id: publisher_id, data: null});
                }
                callback(err);
            }});
        } else {
            callback('error');
        }
    };

    that['audio-out-off'] = function (publisher_id, unused, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Disabling [audio] publisher', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'unpublishStream', [publisher_id, true], {callback: function (err) {
                if (!err) {
                    erizoController.handleEventReport('updateStream', roomId, {event: 'AudioDisabled', id: publisher_id, data: null});
                }
                callback(err);
            }});
        } else {
            callback('error');
        }
    };

    that['video-out-on'] = function (publisher_id, unused, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Enabling [video] publisher', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'publishStream', [publisher_id, false], {callback: function (err) {
                if (!err) {
                    erizoController.handleEventReport('updateStream', roomId, {event: 'VideoEnabled', id: publisher_id, data: null});
                }
                callback(err);
            }});
        } else {
            callback('error');
        }
    };

    that['video-out-off'] = function (publisher_id, unused, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Disabling [video] publisher', publisher_id);
            rpc.callRpc(getErizoQueue(publisher_id), 'unpublishStream', [publisher_id, false], {callback: function (err) {
                if (!err) {
                    erizoController.handleEventReport('updateStream', roomId, {event: 'VideoDisabled', id: publisher_id, data: null});
                }
                callback(err);
            }});
        } else {
            callback('error');
        }
    };

    return that;
};
