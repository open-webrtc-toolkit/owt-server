// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var logger = require('./logger').logger;
var log = logger.getLogger('WorkingNode');

var cxxLogger;
try {
    cxxLogger = require('./logger/build/Release/logger');
} catch (e) {
    log.debug('No native logger for reconfiguration');
}

var rpc = require('./amqp_client')();

var controller;
function init_controller() {
    log.info('pid:', process.pid);

    var nodeConfig = JSON.parse(process.argv[4]);
    var rpcID = process.argv[2];
    var parentID = process.argv[3];
    var purpose = nodeConfig.purpose;
    var clusterWorkerIP = nodeConfig.cluster.worker.ip;

    global.config = nodeConfig;

    log.info('Connecting to rabbitMQ server...');
    rpc.connect(global.config.rabbit, function () {
        rpc.asRpcClient(function(rpcClient) {
            controller = require('./' + purpose)(rpcClient, rpcID, parentID, clusterWorkerIP);
            var rpcAPI = (controller.rpcAPI || controller);

            rpc.asRpcServer(rpcID, rpcAPI, function(rpcServer) {
                log.info(rpcID + ' as rpc server ready');
                rpc.asMonitor(function (data) {
                    if (data.reason === 'abnormal' || data.reason === 'error' || data.reason === 'quit') {
                        if (controller && typeof controller.onFaultDetected === 'function') {
                            controller.onFaultDetected(data.message);
                        }
                    }
                }, function (monitor) {
                    log.info(rpcID + ' as monitor ready');
                    process.send('READY');
                    setInterval(() => {
                      process.send('IMOK');
                    }, 1000);
                }, function(reason) {
                    process.send('ERROR');
                    log.error(reason);
                });
            }, function(reason) {
                process.send('ERROR');
                log.error(reason);
            });
        }, function(reason) {
            process.send('ERROR');
            log.error(reason);
        });
    }, function(reason) {
        process.send('ERROR');
        log.error('Node connect to rabbitMQ server failed, reason:', reason);
    });
};

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        if (controller && typeof controller.close === 'function') {
            controller.close();
        }
        process.exit();
    });
});

['SIGHUP', 'SIGPIPE'].map(function (sig) {
    process.on(sig, function () {
        log.warn(sig, 'caught and ignored');
    });
});

process.on('exit', function () {
    rpc.disconnect();
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
});

process.on('SIGUSR2', function() {
    logger.reconfigure();
    if (cxxLogger) {
        cxxLogger.configure();
    }
});

(function main() {
    init_controller();
})();
