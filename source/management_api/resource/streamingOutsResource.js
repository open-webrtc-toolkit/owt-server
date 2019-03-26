// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var dataAccess = require('../data_access');
var requestHandler = require('../requestHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('StreamingOutsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing streaming-outs for room ', req.params.room, 'and service', req.authData.service._id);
    requestHandler.getSubscriptionsInRoom (req.params.room, 'streaming', function (streamingOuts) {
        if (streamingOuts === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(streamingOuts);
    });
};

const guessProtocol = (url) => {
  if (url.startsWith('rtmp://')) {
    return 'rtmp';
  } else if (url.startsWith('rtsp://')) {
    return 'rtsp';
  } else if (url.endsWith('.m3u8')) {
    return 'hls';
  } else if (url.endsWith('.mpd')) {
    return 'dash';
  } else {
    return 'unknown';
  }
};

exports.add = function (req, res, next) {
    var sub_req = req.body;
    sub_req.type = 'streaming';
    var sub_req = {
      type: 'streaming',
      connection: {
        protocol: req.body.protocol || guessProtocol(req.body.url), /*FIXME: req.body.protocol should be mandatorily specified*/
        url: req.body.url
      },
      media: req.body.media
    };

    if (sub_req.connection.protocol === 'hls') {
      sub_req.connection.parameters = req.body.parameters || {hlsTime: 2, hlsListSize: 5};
    } else if (sub_req.connection.protocol === 'dash') {
      sub_req.connection.parameters = req.body.parameters || {dashSegDuration: 2, dashWindowSize: 5};
    }

    requestHandler.addServerSideSubscription(req.params.room, sub_req, function (result, err) {
        if (result === 'error') {
            return next(err);
        }
        res.send(result);
    });
};

exports.patch = function (req, res, next) {
    var sub_id = req.params.id,
        cmds = req.body;
    requestHandler.controlSubscription(req.params.room, sub_id, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.delete = function (req, res, next) {
    var sub_id = req.params.id;
    requestHandler.deleteSubscription(req.params.room, sub_id, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
