'use strict';
var mongoose = require('mongoose');
var Schema = mongoose.Schema;

var KeySchema = new Schema({
  _id: Number,
  key: String
});

module.exports = mongoose.model('Key', KeySchema);
