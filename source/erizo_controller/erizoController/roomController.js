/*global require, exports, console, setInterval, clearInterval*/

var logger = require('./../common/logger').logger;

// Logger
var log = logger.getLogger("RoomController");


exports.RoomController = function (spec) {
    "use strict";

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

    var KEEPALIVE_INTERVAL = 5*1000;

    var eventListeners = [];

    var recordingUrl = null;

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
        });
    };

    that.addExternalInput = function (publisher_id, url, callback) {

        if (publishers[publisher_id] === undefined) {

            log.info("Adding external input peer_id ", publisher_id);

            createErizoJS(publisher_id, undefined, function() {
            // then we call its addPublisher method.
            var args = [publisher_id, url];
            rpc.callRpc(getErizoQueue(publisher_id), "addExternalInput", args, {callback: callback});

            // Track publisher locally
            publishers[publisher_id] = publisher_id;
            subscribers[publisher_id] = [];

            });
        } else {
            log.info("Publisher already set for", publisher_id);
        }
    };

    that.addExternalOutput = function (publisher_id, url, callback) {
        if (publishers[publisher_id] !== undefined) {
            log.info("Adding ExternalOutput to " + publisher_id + " url " + url);

            var args = [publisher_id, url];

            rpc.callRpc(getErizoQueue(publisher_id), "addExternalOutput", args, undefined);

            // Track external outputs
            externalOutputs[url] = publisher_id;

            // Track publisher locally
            publishers[publisher_id] = publisher_id;
            subscribers[publisher_id] = [];
            callback('success');
        } else {
            callback('error');
        }
    };

    that.removeExternalOutput = function (url, callback) {
        var publisher_id = externalOutputs[url];

        if (publisher_id !== undefined && publishers[publisher_id] != undefined) {
            log.info("Stopping ExternalOutput: url " + url);

            var args = [publisher_id, url];
            rpc.callRpc(getErizoQueue(publisher_id), "removeExternalOutput", args, undefined);

            // Remove track
            delete externalOutputs[url];
            callback('success');
        } else {
            callback('error', 'This stream is not being recorded');
        }
    };

    /*
     * Adds a publisher to the room. This creates a new OneToManyProcessor
     * and a new WebRtcConnection. This WebRtcConnection will be the publisher
     * of the OneToManyProcessor.
     */
    that.addPublisher = function (publisher_id, sdp, mixer_id, callback, onReady) {

        if (publishers[publisher_id] === undefined) {

            log.info("Adding publisher peer_id ", publisher_id);

            sdp = unescapeSdp(sdp);

            // We create a new ErizoJS with the publisher_id.
            createErizoJS(publisher_id, mixer_id, function(erizo_id) {
                log.info("Erizo created");
                // then we call its addPublisher method.
                var mixer = {id: mixer_id, oop: GLOBAL.config.erizoController.outOfProcessMixer};
                var args = [publisher_id, sdp, mixer];
                rpc.callRpc(getErizoQueue(publisher_id), "addPublisher", args, {callback: callback, onReady: onReady});

                // Track publisher locally
                publishers[publisher_id] = publisher_id;
                subscribers[publisher_id] = [];
            });

        } else {
            log.info("Publisher already set for", publisher_id);
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

    that.startRecorder = function (mixer_id, url, interval, callback) {
        if (publishers[mixer_id] !== undefined) {
            if (recordingUrl) {
                return callback({
                    success: false,
                    text: 'recorder busy'
                });
            }
            recordingUrl = url;
            rpc.callRpc(getErizoQueue(mixer_id), 'startRecorder', [mixer_id, url, interval], {callback: function (result) {
                if (result === 'success') {
                    return callback({
                        success: true,
                        text: recordingUrl
                    });
                }
                recordingUrl = null;
                callback({
                    success: false,
                    text: 'recorder failed'
                });
            }});
        } else {
            callback({
                success: false,
                text: 'mixer not found'
            });
        }
    };

    that.stopRecorder = function (mixer_id, callback) {
        if (publishers[mixer_id] !== undefined) {
            if (!recordingUrl) {
                return callback({
                    success: false,
                    text: 'no recorder process'
                });
            }
            rpc.callRpc(getErizoQueue(mixer_id), 'stopRecorder', [mixer_id], {callback: function (result) {
                if (result === 'success') {
                    callback({
                        success: true,
                        text: recordingUrl
                    });
                    recordingUrl = null;
                    return;
                }
                callback({
                    success: false,
                    text: 'recorder failed'
                });
            }});
        } else {
            callback({
                success: false,
                text: 'mixer not found'
            });
        }
    };

    that.publisherNum = function () {
        return Object.keys(publishers).length;
    };

    return that;
};
