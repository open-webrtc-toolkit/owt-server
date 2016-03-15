/*global require, process*/
'use strict';

var amqper = require('./amqper');
var log = require('./logger').logger.getLogger('Main');
var ClusterManager = require('./clusterManager');
var toml = require('toml');
var fs = require('fs');
var config;
try {
  config = toml.parse(fs.readFileSync('./cluster_manager.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

function startup () {
    var enableService = function () {
        var spec = {initialTime: config.manager.initial_time || 10 * 1000/*MS*/,
                    checkAlivePeriod: config.manager.check_alive_period || 1000/*MS*/,
                    checkAliveCount: config.manager.check_alive_count || 10,
                    scheduleKeepTime: config.manager.schedule_reserve_time || 60 * 1000/*MS*/,
                    generalStrategy: config.strategy.general || 'last-used',
                    portalStrategy: config.strategy.portal || 'last-used',
                    webrtcStrategy: config.strategy.webrtc || 'last-used',
                    rtspStrategy: config.strategy.rtsp || 'round-robin',
                    fileStrategy: config.strategy.file || 'randomly-pick',
                    audioStrategy: config.strategy.audio || 'most-used',
                    videoStrategy: config.strategy.video || 'least-used'
                   };
        var api = new ClusterManager.API(spec);
        amqper.setPublicRPC(api);
    };

    amqper.connect(config.rabbit, function () {
        amqper.bind(config.clusterName || 'woogeenCluster', function () {
            log.info('clusterManager up!');
            enableService();
        });
    });
}

startup();
