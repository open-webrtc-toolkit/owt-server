// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

global.config = global.config || {};
global.config.mongo = global.config.mongo || {};
global.config.mongo.dataBaseURL = global.config.mongo.dataBaseURL || 'localhost/owtdb';
var databaseUrl = global.config.mongo.dataBaseURL;

var fs = require('fs');
var cipher = require('../cipher');

// Connect to MongoDB
var connectOption = {
};

var mongoose = require('mongoose');
mongoose.plugin(schema => { schema.options.usePushEach = true });
mongoose.Promise = Promise;

var setupConnection = function () {
  if (databaseUrl.indexOf('mongodb://') !== 0) {
    databaseUrl = 'mongodb://' + databaseUrl;
  }
  mongoose.connect(databaseUrl, connectOption)
    .catch(function (err) {
      console.log(err.message);
    });
  mongoose.connection.on('error', function(err) {
    console.log(err.message);
  });
};

if (fs.existsSync(cipher.astore)) {
  cipher.unlock(cipher.k, cipher.astore, function cb (err, authConfig) {
    if (!err) {
      if (authConfig.mongo) {
        connectOption.user = authConfig.mongo.username;
        connectOption.pass = authConfig.mongo.password;
      }
      setupConnection();
    } else {
      console.error('Failed to get mongodb auth:', err);
      setupConnection();
    }
  });
} else {
  setupConnection();
}

exports.token = require('./interface/tokenInterface');
exports.room = require('./interface/roomInterface');
exports.service = require('./interface/serviceInterface');
