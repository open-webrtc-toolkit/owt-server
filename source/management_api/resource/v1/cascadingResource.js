// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var requestHandler = require('../../requestHandler');
var e = require('../../errors');

var logger = require('../../logger').logger;

// Logger
var log = logger.getLogger('CascadingResource');

exports.startEventCascading = function (req, res, next) {
    var pub_req = req.body;
    pub_req.type = 'cascading';
    requestHandler.startEventCascading(req.params.room, pub_req, function (result) {
        if (result === 'error') {
            return next(new e.CloudError('Operation failed'));
        }
        res.send(result);
    });
};

