/*global exports, require*/
'use strict';
var dataAccess = require('../data_access');
var cloudHandler = require('../cloudHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('StreamingOutsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing streaming-outs for room ', req.params.room, 'and service', req.authData.service._id);
    cloudHandler.getSubscriptionsInRoom (req.params.room, 'streaming', function (streamingOuts) {
        if (streamingOuts === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(streamingOuts);
    });
};

exports.add = function (req, res, next) {
    var sub_req = req.body;
    sub_req.type = 'streaming';
    var sub_req = {
      type: 'streaming',
      connection: {
        url: req.body.url
      },
      media: req.body.media
    };
    cloudHandler.addServerSideSubscription(req.params.room, sub_req, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.patch = function (req, res, next) {
    var sub_id = req.params.id,
        cmds = req.body;
    cloudHandler.controlSubscription(req.params.room, sub_id, cmds, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.delete = function (req, res, next) {
    var sub_id = req.params.id;
    cloudHandler.deleteSubscription(req.params.room, sub_id, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
