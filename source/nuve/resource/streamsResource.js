'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('StreamsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing streams for room ', req.params.room, 'and service', req.authData.service._id);
    cloudHandler.getStreamsInRoom (req.params.room, function (streams) {
        if (streams === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(streams);
    });
};

exports.get = function (req, res, next) {
    var stream = req.params.stream;
    cloudHandler.getStreamsInRoom(req.params.room, function (streams) {
        if (streams === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        for (var index in streams) {
            if (streams[index].id === stream) {
                log.debug('Found stream', stream);
                res.send(streams[index]);
                return;
            }
        }
        log.error('Stream', req.params.stream, 'does not exist');
        return next(new e.NotFoundError('Stream not found'));
    });
};

exports.addStreamingIn = function (req, res, next) {
    var pub_req = req.body;
    pub_req.type = 'streaming';
    cloudHandler.addStreamingIn(req.params.room, pub_req, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.patch = function (req, res, next) {
    var stream = req.params.stream;
    var cmds = req.body;
    cloudHandler.controlStream(req.params.room, stream, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.delete = function (req, res, next) {
    cloudHandler.deleteStream(req.params.room, req.params.stream, function (result) {
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
