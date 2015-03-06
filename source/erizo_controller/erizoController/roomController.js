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
        // {id: erizoJS_id}
        publishers = {},

        // {erizoJS_id: {publishers: [ids], ka_count: count}}
        erizos = {},

        // {id: ExternalOutput}
        externalOutputs = {};

    var amqper = spec.amqper;
    var agentId = spec.agent_id;
    var roomId = spec.id;

    var KEEPALIVE_INTERVAL = 5*1000;
    var TIMEOUT_LIMIT = 2;

    var eventListeners = [];

    var callbackFor = function(erizo_id) {

        return function(ok) {
            if (!erizos[erizo_id]) return;

            if (ok !== true) {
                erizos[erizo_id].ka_count ++;

                if (erizos[erizo_id].ka_count > TIMEOUT_LIMIT) {

                    for (var p in erizos[erizo_id].publishers) {
                        dispatchEvent("unpublish", erizos[erizo_id].publishers[p]);
                    }
                    amqper.callRpc("ErizoAgent", "deleteErizoJS", [erizo_id], {callback: function(){}}); 
                    delete erizos[erizo_id];
                }
            } else {
                erizos[erizo_id].ka_count = 0;
            }
        }
    };

    var sendKeepAlive = function() {
        for (var e in erizos) {
            amqper.callRpc("ErizoJS_" + e, "keepAlive", [], {callback: callbackFor(e)});
        }
    };

    setInterval(sendKeepAlive, KEEPALIVE_INTERVAL);

    var getErizoJS = function(mixer_id, callback) {
        // Currently we want to make sure the publishers and the mixer they associate with
        // are run in the same process. This is the requirement of in-process mixer.
        if (!GLOBAL.config.erizoController.outOfProcessMixer && mixer_id !== undefined && publishers[mixer_id] !== undefined) {
            callback(publishers[mixer_id]);
            return;
        }
    	amqper.callRpc("ErizoAgent_" + agentId, "createErizoJS", [], {callback: function(erizo_id) {
            log.debug("Answer", erizo_id);
            if (erizo_id !== 'timeout' && !erizos[erizo_id]) {
                erizos[erizo_id] = {publishers: [], ka_count: 0};
            }
            callback(erizo_id);
            amqper.callRpc('ErizoJS_'+erizo_id, 'setControllerId', [exports.myId, roomId], {});
        }});
    };
/*
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
        amqper.callRpc("ErizoAgent_" + agentId, "createErizoJS", [publisher_id], {callback: function(erizo_id) {
            log.debug("Answer", erizo_id);
            erizos[publisher_id] = erizo_id;
            callback();
            amqper.callRpc('ErizoJS_'+erizo_id, 'setControllerId', [exports.myId, roomId], {});
        }});
    };
*/
    var getErizoQueue = function(publisher_id) {
        return "ErizoJS_" + publishers[publisher_id];
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

        if (publishers[id] !== undefined)
            return;

        // We create a new ErizoJS with the id.
        getErizoJS(id, function(erizo_id) {
            log.info("Erizo created");
            // Track publisher locally
            publishers[id] = erizo_id;
            subscribers[id] = [];

            // then we call its initMixer method.
            var args = [id, GLOBAL.config.erizoController.outOfProcessMixer, roomConfig];
            amqper.callRpc(getErizoQueue(id), "initMixer", args, {callback: callback});

            erizos[erizo_id].publishers.push(id);

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

            getErizoJS(mixer_id, function(erizo_id) {
                // Track publisher locally
                publishers[publisher_id] = erizo_id;
                subscribers[publisher_id] = [];

                // then we call its addExternalInput method.
                var mixer = {id: mixer_id, oop: GLOBAL.config.erizoController.outOfProcessMixer};
                var args = [publisher_id, options, mixer];
                amqper.callRpc(getErizoQueue(publisher_id), "addExternalInput", args, {callback: callback, onReady: onReady});

                erizos[erizo_id].publishers.push(publisher_id);

            });
        } else {
            log.info("Publisher already set for", publisher_id);
        }
    };

    that.addExternalOutput = function (video_publisher_id, audio_publisher_id, output_id, mixer_id, url, interval, callback) {
        if (publishers[video_publisher_id] !== undefined && publishers[audio_publisher_id] !== undefined) {
            log.info("Adding ExternalOutput to " + video_publisher_id + " and " + audio_publisher_id + " with url: " + url);

            var externalOutput = externalOutputs[output_id];
            if (externalOutput) {
                return callback({
                    success: false,
                    text: 'the external output is busy'
                });
            }

            getErizoJS(mixer_id, function(erizo_id) {
                var erizoVideo = getErizoQueue(video_publisher_id);
                var erizoAudio = getErizoQueue(audio_publisher_id);
                if (erizoVideo !== erizoAudio) {
                    return callback({
                        success: false,
                        text: 'cannot make external output for video and audio from different erizo nodes'
                    });
                }

                var args = [video_publisher_id, audio_publisher_id, output_id, url, interval];

                amqper.callRpc(erizo_id, "addExternalOutput", args, {callback: function (result) {
                    if (result === 'success') {
                        // Track external outputs
                        externalOutputs[output_id] = {video: video_publisher_id, audio: audio_publisher_id, erizo: erizo_id};

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

            amqper.callRpc(externalOutput.erizo, "removeExternalOutput", args, {callback: function (result) {
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

    that.processSignaling = function (streamId, peerId, msg) {
        if (publishers[streamId] !== undefined) {

            log.info("Sending signaling mess to erizoJS of st ", streamId, ' of peer ', peerId);

            var args = [streamId, peerId, msg];

            amqper.callRpc(getErizoQueue(streamId), "processSignaling", args, {});

        }
    };

    /*
     * Adds a publisher to the room. This creates a new OneToManyProcessor
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the OneToManyProcessor.
     */
    that.addPublisher = function (publisher_id, mixer_id, unmix, callback) {

        if (publishers[publisher_id] === undefined) {

            log.info('Adding publisher peer_id', publisher_id);

            // We create a new ErizoJS with the publisher_id.
            getErizoJS(mixer_id, function (erizo_id) {

                if (erizo_id === 'timeout') {
                    log.error('No Agents Available');
                    callback('timeout');
                    return;
                }

                // Track publisher locally
                publishers[publisher_id] = erizo_id;
                subscribers[publisher_id] = [];

                // then we call its addPublisher method.
                var mixer = {id: mixer_id, oop: GLOBAL.config.erizoController.outOfProcessMixer};
                var args = [publisher_id, mixer, unmix];
	            amqper.callRpc(getErizoQueue(publisher_id), 'addPublisher', args, {callback: callback});

                erizos[erizo_id].publishers.push(publisher_id);
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
    that.addSubscriber = function (subscriber_id, publisher_id, options, callback) {
        if (subscriber_id === null) {
            callback('error', 'invalid id');
            return;
        }

        if (publishers[publisher_id] !== undefined && subscribers[publisher_id].indexOf(subscriber_id) === -1) {

            log.info("Adding subscriber ", subscriber_id, ' to publisher ', publisher_id);

            if (options.audio === undefined) options.audio = true;
            if (options.video === undefined) options.video = true;

            var args = [subscriber_id, publisher_id, options];

            amqper.callRpc(getErizoQueue(publisher_id), "addSubscriber", args, {callback: callback});

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

            var video_output_id = -1;
            var audio_output_id = -1;
            for (var i in externalOutputs) {
                if (externalOutputs.hasOwnProperty(i)) {
                    if (externalOutputs[i].video === publisher_id) {
                        video_output_id = i;
                    }

                    if (externalOutputs[i].audio === publisher_id) {
                        audio_output_id = i;
                    }
                }
            }

            // Stop the video related output
            if (video_output_id !== -1) {
                log.info('Removing external output', video_output_id);
                var args = [video_output_id, false];
                amqper.callRpc(getErizoQueue(video_output_id), "removeExternalOutput", args, {callback: function (result) {}});

                // Remove the external output track anyway
                delete externalOutputs[video_output_id];
            }

            // Stop the audio related output
            if (audio_output_id !== -1 && audio_output_id !== video_output_id) {
                log.info('Removing external output', audio_output_id);
                var args = [audio_output_id, false];
                amqper.callRpc(getErizoQueue(audio_output_id), "removeExternalOutput", args, {callback: function (result) {}});

                // Remove the external output track anyway
                delete externalOutputs[audio_output_id];
            }

            var args = [publisher_id];
            amqper.callRpc(getErizoQueue(publisher_id), "removePublisher", args, undefined);

            // Remove tracks
            var index = erizos[publishers[publisher_id]].publishers.indexOf(publisher_id);
            erizos[publishers[publisher_id]].publishers.splice(index, 1);

            log.info('Removing subscribers', publisher_id);
            delete subscribers[publisher_id];
            log.info('Removing publisher', publisher_id);
            delete publishers[publisher_id];
            log.info('Removed all');
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
            amqper.callRpc(getErizoQueue(publisher_id), "removeSubscriber", args, undefined);

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
                    amqper.callRpc(getErizoQueue(publisher_id), "removeSubscriber", args, undefined);

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
        amqper.callRpc(getErizoQueue(publisher_id), 'addToMixer', [publisher_id, mixer_id], {callback: callback});

    };

    that.removeFromMixer = function removeFromMixer (publisher_id, mixer_id, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('error', 'stream not published');
        }
        amqper.callRpc(getErizoQueue(publisher_id), 'removeFromMixer', [publisher_id, mixer_id], {callback: callback});
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
        amqper.callRpc(getErizoQueue(mixer_id), "getRegion", args, {callback: function (result) {
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
        amqper.callRpc(getErizoQueue(mixer_id), "setRegion", args, {callback: callback});
    };

    that.setVideoBitrate = function (mixer_id, publisher_id, bitrate, callback) {
        if (publishers[publisher_id] === undefined) {
            return callback('stream not published');
        }
        if (publishers[mixer_id] === undefined) {
            return callback('mixer is not available');
        }
        var args = [mixer_id, publisher_id, bitrate];
        amqper.callRpc(getErizoQueue(mixer_id), "setVideoBitrate", args, {callback: callback});
    };

    that.addRTSPOut = function (mixer_id, callback) {
        if (publishers[mixer_id] !== undefined) {
            var args = [mixer_id, mixer_id, '', '', 0];
            amqper.callRpc(getErizoQueue(mixer_id), 'addExternalOutput', args, {callback: function (result) {
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
            amqper.callRpc(getErizoQueue(publisher_id), 'subscribeStream', [publisher_id, subscriber_id, true], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['audio-in-off'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Disabling [audio] subscriber', subscriber_id, 'in', publisher_id);
            amqper.callRpc(getErizoQueue(publisher_id), 'unsubscribeStream', [publisher_id, subscriber_id, true], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['video-in-on'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Enabling [video] subscriber', subscriber_id, 'in', publisher_id);
            amqper.callRpc(getErizoQueue(publisher_id), 'subscribeStream', [publisher_id, subscriber_id, false], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['video-in-off'] = function (publisher_id, subscriber_id, callback) {
        var index = subscribers[publisher_id].indexOf(subscriber_id);
        if (index !== -1) {
            log.info('Disabling [video] subscriber', subscriber_id, 'in', publisher_id);
            amqper.callRpc(getErizoQueue(publisher_id), 'unsubscribeStream', [publisher_id, subscriber_id, false], {callback: callback});
        } else {
            callback('error');
        }
    };

    that['audio-out-on'] = function (publisher_id, unused, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info('Enabling [audio] publisher', publisher_id);
            amqper.callRpc(getErizoQueue(publisher_id), 'publishStream', [publisher_id, true], {callback: function (err) {
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
            amqper.callRpc(getErizoQueue(publisher_id), 'unpublishStream', [publisher_id, true], {callback: function (err) {
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
            amqper.callRpc(getErizoQueue(publisher_id), 'publishStream', [publisher_id, false], {callback: function (err) {
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
            amqper.callRpc(getErizoQueue(publisher_id), 'unpublishStream', [publisher_id, false], {callback: function (err) {
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
