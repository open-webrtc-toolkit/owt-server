/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('StreamsResource');

/*
 * Gets the service and the room for the proccess of the request.
 */
var doInit = function (currentService, roomId, callback) {
    dataAccess.room.get(currentService._id, roomId, function (room) {
        if (room) {
            callback(room);
        } else {
            callback();
        }
    });
};

exports.getList = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    var currentService = authData.service;

    doInit(currentService, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' not found');
            res.status(404).send('Room not found');
            return;
        }

        log.debug('Representing streams for room ', currentRoom._id, 'and service', currentService._id);
        cloudHandler.getStreamsInRoom (currentRoom._id, function (streams) {
            if (streams === 'error') {
                //res.status(404).send('Operation failed');
                res.send([]);
                return;
            }
            res.send(streams);
        });
    });
};

exports.get = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var stream = req.params.stream;
        cloudHandler.getStreamsInRoom(currentRoom._id, function (streams) {
            if (streams === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            for (var index in streams) {
                if (streams[index].id === stream) {
                    log.debug('Found stream', stream);
                    res.send(streams[index]);
                    return;
                }
            }
            log.error('Stream', req.params.stream, 'does not exist');
            res.status(404).send('Stream does not exist');
            return;
        });
    });
};

exports.addStreamingIn = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var pub_req = req.body;
        pub_req.type = 'streaming';
        cloudHandler.addStreamingIn(currentRoom._id, pub_req, function (result) {
            if (result === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(result);
            return;
        });
    });
};

exports.patch = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var stream = req.params.stream,
            cmds = req.body;
        cloudHandler.controlStream(currentRoom._id, stream, cmds, function (result) {
            if (result === 'error') {
                res.status(404).send('Operation failed');
                return;
            }
            res.send(result);
            return;
        });
    });
};

exports.delete = function (req, res) {
    var authData = req.authData || {};

    if (authData.service === undefined) {
        log.info('Service not found');
        res.status(404).send('Service not found');
        return;
    }

    doInit(authData.service, req.params.room, function (currentRoom) {
        if (currentRoom === undefined) {
            log.info('Room ', req.params.room, ' does not exist');
            res.status(404).send('Room does not exist');
            return;
        }

        var stream = req.params.stream;
        cloudHandler.deleteStream(currentRoom._id, stream, function (result) {
            if (result === 'error') {
                res.status(404).send('Operation failed');
            } else {
                res.send();
            }
        });
    });
};
