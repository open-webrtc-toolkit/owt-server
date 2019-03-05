// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var mongoose = require('mongoose');
var Schema = mongoose.Schema;

var ServiceSchema = new Schema({
  name: {
    type: String,
    required: true,
    unique: true
  },
  key: {
    type: String,
    required: true
  },
  encrypted: {
    type: Boolean
  },
  rooms:[
    { type: Schema.Types.ObjectId, ref: 'Room' }
  ]
});

module.exports = mongoose.model('Service', ServiceSchema);