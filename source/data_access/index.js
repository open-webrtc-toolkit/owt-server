'use strict';

global.config = global.config || {};
global.config.mongo = global.config.mongo || {};
global.config.mongo.dataBaseURL = global.config.mongo.dataBaseURL || 'localhost/nuvedb';
var databaseUrl = global.config.mongo.dataBaseURL;

// Connect to MongoDB
var connectOption = {
  useMongoClient: true,
  reconnectTries: 60 * 15,
  reconnectInterval: 1000
};
var mongoose = require('mongoose');
mongoose.Promise = Promise;
mongoose.connect('mongodb://' + databaseUrl, connectOption)
  .catch(function (err) {
    console.log(err.message);
  });
mongoose.connection.on('error', function(err) {
  console.log(err.message);
});
exports.token = require('./interface/tokenInterface');
exports.room = require('./interface/roomInterface');
exports.service = require('./interface/serviceInterface');
