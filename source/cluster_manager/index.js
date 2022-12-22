// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var amqper = require('./amqpClient')();
var logger = require('./logger').logger;
var log = logger.getLogger('Main');
var ClusterManager = require('./clusterManager');
var toml = require('toml');
var fs = require('fs');
var config;
var clusterManager;
try {
  config = toml.parse(fs.readFileSync('./cluster_manager.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

config.manager = config.manager || {};
config.manager.name = config.manager.name || 'owt-cluster';
config.manager.initial_time = config.manager.initial_time || 10 * 1000;
config.manager.check_alive_interval = config.manager.check_alive_interval || 1000;
config.manager.check_alive_count = config.manager.check_alive_count || 10;
config.manager.schedule_reserve_time = config.manager.schedule_reserve_time || 60 * 1000;

config.strategy = config.strategy || {};
config.strategy.general = config.strategy.general || 'round-robin';
config.strategy.portal = config.strategy.portal || 'last-used';
config.strategy.conference = config.strategy.conference || 'last-used';
config.strategy.webrtc = config.strategy.webrtc || 'last-used';
config.strategy.sip = config.strategy.sip || 'round-robin';
config.strategy.streaming = config.strategy.streaming || 'round-robin';
config.strategy.recording = config.strategy.recording || 'randomly-pick';
config.strategy.audio = config.strategy.audio || 'most-used';
config.strategy.video = config.strategy.video || 'least-used';
config.strategy.analytics = config.strategy.analytics || 'least-used';

config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

config.cloud.url = config.cloud.url;
config.cloud.region = config.cloud.region;
config.cloud.clusterID = config.cloud.clusterID;

function startup () {
    var id = Math.floor(Math.random() * 1000000000);
    var spec = {
        initialTime: config.manager.initial_time,
        checkAlivePeriod: config.manager.check_alive_interval,
        checkAliveCount: config.manager.check_alive_count,
        scheduleKeepTime: config.manager.schedule_reserve_time,
        strategy: config.strategy,
        url: config.cloud.url, region: config.cloud.region, clusterID: config.cloud.clusterID
    };

    if (config.manager.enable_grpc) {
        const grpcTools = require('./grpcTools');
        let gHostname = 'localhost';
        let gPort = 10080;
        if (config.manager.grpc_host) {
            [gHostname, gPort] = config.manager.grpc_host.split(':');
            gPort = Number(gPort) || 10080;
        }
        const manager = new ClusterManager.ClusterManager(
            config.manager.name, id, spec);
        manager.serve();
        grpcTools.startServer('clusterManager', manager.grpcInterface, gPort)
        .then((port) => {
            // Send RPC server address
            const rpcAddress = gHostname + ':' + port;
            log.info('As gRPC server ok', rpcAddress);
        }).catch((err) => {
            log.error('Start grpc server failed:', err);
        });
        return;
    }

    var enableService = function () {
        amqper.asTopicParticipant(config.manager.name + '.management', function(channel) {
            log.info('Cluster manager up! id:', id);
            clusterManager = new ClusterManager.manager(channel, config.manager.name, id, spec);
            clusterManager.run(channel);
        }, function(reason) {
            log.error('Cluster manager initializing failed, reason:', reason);
            process.kill(process.pid, 'SIGINT');
        });
    };

    amqper.connect(config.rabbit, function () {
        enableService();
    }, function(reason) {
        log.error('Cluster manager connect to rabbitMQ server failed, reason:', reason);
        process.kill(process.pid, 'SIGINT');
    });
}

startup();

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
        log.warn('Exiting on', sig);
        try {
            clusterManager.leave();
            await amqper.disconnect();
        } catch (e) {
            log.warn('Disconnect:', e);
        }
        process.exit();
    });
});

process.on('exit', function () {
    log.info('Process exit');
    // amqper.disconnect();
});

process.on('SIGUSR2', function() {
    logger.reconfigure();
});