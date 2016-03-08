/*global require*/
'use strict';

var config = require('./config');
var amqper = require('./amqper');
var logger = require('./logger').logger;
var ClusterManager = require('./clusterManager');

var log = logger.getLogger('Main');

function startup () {
    var enableService = function () {
        var spec = {initialTime: config.initial_time || 10 * 1000/*MS*/,
                    checkAlivePeriod: config.check_alive_period || 1000/*MS*/,
                    checkAliveCount: config.check_alive_count || 10,
                    scheduleKeepTime: config.schedule_reserve_time || 60 * 1000/*MS*/,
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

    amqper.connect(function () {
        amqper.bind(config.clusterName || 'woogeenCluster', function () {
            log.info('clusterManager up!');
            enableService();
        });
    });
}

startup();
