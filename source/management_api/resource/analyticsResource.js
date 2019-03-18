'use strict';
var requestHandler = require('../requestHandler');
var e = require('../errors');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('AnalyticsResource');

exports.getList = function (req, res, next) {
    log.debug('Representing analytics for room ', req.params.room, 'and service', req.authData.service._id);
    requestHandler.getSubscriptionsInRoom (req.params.room, 'analytics', function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

exports.add = function (req, res, next) {
    log.debug('Add analytics for room', req.params.room);
    var subDesc = {
      type: 'analytics',
      connection: {
        algorithm: req.body.algorithm
      },
      media: req.body.media
    };
    requestHandler.addServerSideSubscription(req.params.room, subDesc, function (result, err) {
        if (result === 'error') {
            return next(err);
        }
        res.send(result);
        return;
    });
};

exports.delete = function (req, res, next) {
    var subId = req.params.id;
    requestHandler.deleteSubscription(req.params.room, subId, function (result) {
        log.debug('result', result);
        if (result === 'error') {
            next(new e.CloudError('Operation failed'));
        } else {
            res.send();
        }
    });
};
