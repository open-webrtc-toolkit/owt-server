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

var rpc = require('./amqpClient')();

var controller;
function init_controller() {
    log.info('pid:', process.pid);

    var nodeConfig = JSON.parse(process.argv[4]);
    var rpcID = process.argv[2];
    var parentID = process.argv[3];
    var purpose = nodeConfig.purpose;
    var clusterWorkerIP = nodeConfig.cluster.worker.ip;
    var enableGRPC = nodeConfig.agent.enable_grpc || false;

    global.config = nodeConfig;

    log.info('Connecting to rabbitMQ server...');
    rpc.connect(global.config.rabbit, function () {
        rpc.asRpcClient(function(rpcClient) {
            if (enableGRPC) {
                rpcClient = {
                    remoteCast: () => {log.info('fake remote cast')},
                    remoteCall: () => {log.info('fake remote call')},
                };
            }
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
                    // Send RPC server address
                    if (enableGRPC) {
                        log.info('Start grpc server');
                        var grpcTools = require('./grpcTools');
                        grpcTools.startServer(purpose, controller.grpcInterface)
                            .then((port) => {
                                const rpcAddress = clusterWorkerIP + ':' + port;
                                process.send('READY:' + rpcAddress);
                            }).catch((err) => {
                                log.warn('Start grpc server failed:', e);
                            });
                    } else {
                        process.send('READY');
                    }
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

var exiting = false;
['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
        if (exiting) {
            return;
        }
        exiting = true;
        log.warn('Exiting on', sig);
        if (controller && typeof controller.close === 'function') {
            controller.close();
        }
        try {
            await rpc.disconnect();
        } catch(e) {
            log.warn('Exiting e:', e);
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
    log.info('Process exit');
    //rpc.disconnect();
});

process.on('uncaughtException', async (err) => {
    log.error(err);
    try {
        await rpc.disconnect();
    } catch(e) {
        log.warn('Exiting e:', e);
    }
    process.exit(1);
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ', reason.stack);
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
