// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var dataAccess = require('../../data_access');
var requestHandler = require('../../requestHandler');
var e = require('../../errors');

var logger = require('../../logger').logger;

// Logger
var log = logger.getLogger('StreamsResource');
var convertStream = require('../streamAdapter').convertToV11Stream;

exports.getList = function (req, res, next) {
    log.debug('Representing streams for room ', req.params.room, 'and service', req.authData.service._id);
    requestHandler.getStreamsInRoom (req.params.room, function (streams) {
        if (streams === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        streams.map(convertStream);
        res.send(streams);
    });
};

exports.get = function (req, res, next) {
    var stream = req.params.stream;
    requestHandler.getStreamsInRoom(req.params.room, function (streams) {
        if (streams === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        for (var index in streams) {
            if (streams[index].id === stream) {
                log.debug('Found stream', stream);
                convertStream(stream[index]);
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
    requestHandler.addStreamingIn(req.params.room, pub_req, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        convertStream(result);
        res.send(result);
    });
};

exports.patch = function (req, res, next) {
    var stream = req.params.stream;
    var cmds = req.body;
    requestHandler.controlStream(req.params.room, stream, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        convertStream(result);
        res.send(result);
    });
};

exports.delete = function (req, res, next) {
    requestHandler.deleteStream(req.params.room, req.params.stream, function (result) {
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
