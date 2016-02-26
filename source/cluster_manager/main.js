/*global require*/
'use strict'

GLOBAL.config = require('./config');

var amqper = require('./amqper');
var logger = require('./logger').logger;
var ClusterManager = require('./clusterManager');

var log = logger.getLogger('Main');

var startup = function () {
    var enableService = function () {
        var spec = {initialTime: GLOBAL.config.initial_time || 10 * 1000/*MS*/,
                    checkAlivePeriod: GLOBAL.config.check_alive_period || 1000/*MS*/,
                    checkAliveCount: GLOBAL.config.check_alive_count || 10,
                    scheduleKeepTime: GLOBAL.config.schedule_reserve_time || 60 * 1000/*MS*/,
                    generalStrategy: GLOBAL.config.strategy.general || 'last-used',
                    portalStrategy: GLOBAL.config.strategy.portal || 'last-used',
                    webrtcStrategy: GLOBAL.config.strategy.webrtc || 'last-used',
                    rtspStrategy: GLOBAL.config.strategy.rtsp || 'round-robin',
                    fileStrategy: GLOBAL.config.strategy.file || 'randomly-pick',
                    audioStrategy: GLOBAL.config.strategy.audio || 'most-used',
                    videoStrategy: GLOBAL.config.strategy.video || 'least-used'
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
};

startup();
