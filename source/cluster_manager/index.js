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

config.manager = config.manager || {};
config.manager.name = config.manager.name || 'woogeen-cluster';
config.manager.initial_time = config.manager.initial_time || 10 * 1000;
config.manager.check_alive_interval = config.manager.check_alive_interval || 1000;
config.manager.check_alive_count = config.manager.check_alive_count || 10;
config.manager.schedule_reserve_time = config.manager.schedule_reserve_time || 60 * 1000;

config.strategy = config.strategy || {};
config.strategy.general = config.strategy.general || 'round-robin';
config.strategy.portal = config.strategy.portal || 'last-used';
config.strategy.webrtc = config.strategy.webrtc || 'last-used';
config.strategy.rtsp = config.strategy.rtsp || 'round-robin';
config.strategy.file = config.strategy.file || 'randomly-pick';
config.strategy.audio = config.strategy.audio || 'most-used';
config.strategy.video = config.strategy.video || 'least-used';

config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

function startup () {
    var enableService = function () {
        var spec = {initialTime: config.manager.initial_time,
                    checkAlivePeriod: config.manager.check_alive_interval,
                    checkAliveCount: config.manager.check_alive_count,
                    scheduleKeepTime: config.manager.schedule_reserve_time,
                    generalStrategy: config.strategy.general,
                    portalStrategy: config.strategy.portal,
                    webrtcStrategy: config.strategy.webrtc,
                    rtspStrategy: config.strategy.rtsp,
                    fileStrategy: config.strategy.file,
                    audioStrategy: config.strategy.audio,
                    videoStrategy: config.strategy.video
                   };
        var api = new ClusterManager.API(spec);
        amqper.setPublicRPC(api);
    };

    amqper.connect(config.rabbit, function () {
        amqper.bind(config.manager.name || 'woogeen-cluster', function () {
            log.info('clusterManager up!');
            enableService();
        });
    });
}

startup();
