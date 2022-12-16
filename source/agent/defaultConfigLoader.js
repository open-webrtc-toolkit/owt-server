// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var fs = require('fs');
var toml = require('toml');
var networkHelper = require('./networkHelper');

module.exports.load = function() {
  var config;
  try {
    config = toml.parse(fs.readFileSync('./agent.toml')) || {};
    config.agent = config.agent || {};
    config.agent.maxProcesses = config.agent.maxProcesses || 16;
    config.agent.prerunProcesses = config.agent.prerunProcesses || 2;

    config.cluster = config.cluster || {};
    config.cluster.name = config.cluster.name || 'owt-cluster';
    config.cluster.worker = config.cluster.worker || {};
    config.cluster.worker.ip = config.cluster.worker.ip || (networkHelper.getAddress("firstEnumerated") || {}).ip || 'unkown';
    config.cluster.worker.join_retry = config.cluster.worker.join_retry || 60;
    config.cluster.worker.load = config.cluster.worker.load || {};
    config.cluster.worker.load.max = config.cluster.max_load || 0.85;
    config.cluster.worker.load.period = config.cluster.report_load_interval || 1000;
    config.cluster.worker.load.item = {
      name: 'network',
      interf: 'lo',
      max_scale: config.cluster.network_max_scale || 1000
    };

    config.capacity = config.capacity || {};
    config.capacity.isps = config.capacity.isps || [];
    config.capacity.regions = config.capacity.regions || [];

    return config;
  } catch (e) {
    console.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
    process.exit(1);
  }

};
