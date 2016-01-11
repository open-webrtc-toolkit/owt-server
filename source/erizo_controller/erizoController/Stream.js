/*global exports*/
exports.Stream = function (spec) {
    'use strict';

    var that = {},
        videoRecorder = '',
        audioRecorder = '',
        dataSubscribers = [];

    that.getID = function () {
        return spec.id;
    };

    that.getSocket = function () {
        return spec.socket;
    };

    // Indicates if the stream has audio activated
    that.hasAudio = function () {
        return !!spec.audio;
    };

    // Indicates if the stream has video activated
    that.hasVideo = function () {
        return !!spec.video;
    };

    // Indicates if the stream has data activated
    that.hasData = function () {
        return !!spec.data;
    };

    // Indicates if the stream has screen activated
    that.hasScreen = function () {
        return (!!spec.video) && spec.video.device === 'screen';
    };

    // Indicates if the stream is mixed stream
    that.isMixed = function () {
        return (!!spec.video) && (spec.video.device === 'mcu');
    };

    // Indicates if the stream has the resolution
    that.hasResolution = function (resolution) {
        if (that.isMixed() && spec.video.resolutions instanceof Array && typeof resolution === 'object') {
            for (var i in spec.video.resolutions) {
                if (spec.video.resolutions[i].width === resolution.width && spec.video.resolution[i].height === resolution.height)
                    return true;
            }
        }
        return false;
    };

    // Retrieves attributes from stream
    that.getAttributes = function () {
        return spec.attributes;
    };

    // Set attributes for this stream
    that.setAttributes = function (attrs) {
        spec.attributes = attrs;
    };

    // Retrieves subscribers to data
    that.getDataSubscribers = function () {
        return dataSubscribers;
    };

    // Add a new data subscriber to this stream
    that.addDataSubscriber = function (id) {
        if (dataSubscribers.indexOf(id) === -1) {
            dataSubscribers.push(id);
        }
    };

    // Removes a data subscriber from this stream
    that.removeDataSubscriber = function (id) {
        var index = dataSubscribers.indexOf(id);
        if (index !== -1) {
            dataSubscribers.splice(index, 1);
        }
    };

    // Gets the video recorder
    that.getVideoRecorder = function() {
        return videoRecorder;
    };

    // Gets the audio recorder
    that.getAudioRecorder = function() {
        return audioRecorder;
    };

    // Sets the video recorder
    that.setVideoRecorder = function(rec) {
        videoRecorder = rec;
    };

    // Sets the audio recorder
    that.setAudioRecorder = function(rec) {
        audioRecorder = rec;
    };

    // Returns the public specification of this stream
    that.getPublicStream = function () {
        return {id: spec.id, audio: spec.audio, video: spec.video, data: spec.data, from: spec.from, attributes: spec.attributes};
    };

    that.getOwner = function () {
        return spec.from;
    };

    that.getStatus = function () {
        return spec.status === 'initializing' ? 'initializing' : 'ready';
    };

    that.setStatus = function (status) {
        spec.status = status;
    };

    return that;
};
