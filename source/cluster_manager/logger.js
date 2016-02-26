
/*global GLOBAL, require, exports*/
'use strict';
var log4js = require('log4js');

GLOBAL.config.logger = GLOBAL.config.logger || {};

var log_file = GLOBAL.config.logger.config_file ||  './log4js_configuration.json';

log4js.configure(log_file);

module.exports.logger = log4js;
