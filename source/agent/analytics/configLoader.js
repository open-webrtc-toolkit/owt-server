// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var fs = require('fs');
var path = require('path');
var toml = require('toml');
var networkHelper = require('./networkHelper');

module.exports.load = () => {
  var config;
  try {
    config = toml.parse(fs.readFileSync('./agent.toml')) || {};
    config.agent = config.agent || {};
    config.agent.maxProcesses = config.agent.maxProcesses || -1;
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
      name: 'cpu'
    };

    var resource = config.cluster.load_items;
    //In current environment, always report CPU resource

    if (resource.toLowerCase().indexOf("gpu") !== -1) {
      config.cluster.worker.load.item.gpu = true;
    }

    if (resource.toLowerCase().indexOf("vpu") !== -1) {
      config.cluster.worker.load.item.vpu = true;
    }

    config.internal.ip_address = config.internal.ip_address || '';
    config.internal.network_interface = config.internal.network_interface || undefined;
    config.internal.minport = config.internal.minport || 0;
    config.internal.maxport = config.internal.maxport || 0;

    if (!config.internal.ip_address) {
      let addr = networkHelper.getAddress(config.internal.network_interface || "firstEnumerated");
      if (!addr) {
        console.error("Can't get internal IP address");
        process.exit(1);
      }

      config.internal.ip_address = addr.ip;
    }

    config.analytics = config.analytics || {};
    config.analytics.libpath = config.analytics.libpath || 'lib';

    config.capacity = config.capacity || {};
    // add plugin library to ld library path
    process.env.LD_LIBRARY_PATH = [
      path.resolve(__dirname, config.analytics.libpath),
      process.env.LD_LIBRARY_PATH || '',
    ].join(':');
    try {
      const plugins = toml.parse(fs.readFileSync('./plugin.cfg'));
      config.capacity.algorithms = Object.keys(plugins);
    } catch (e) {
      config.capacity.algorithms = [];
    }

    return config;
  } catch (e) {
    console.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
    process.exit(1);
  }

};
