// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
const log4js = require('log4js');
log4js.configure('./log4js_configuration.json');

log4js.reconfigure = function() {
  log4js.configure('./log4js_configuration.json');
};

exports.logger = log4js;
