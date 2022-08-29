// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';
var fs = require('fs');
var amqper = require('./../amqpClient')();
var log = require('./../logger').logger.getLogger('RPC');
var cipher = require('../cipher');
var TIMEOUT = 3000;
var rpcClient;

const enableGrpc = global.config?.server?.enable_grpc || false;
const grpcTools = require('../grpcTools');
const grpcNode = {}; // workerNode => grpcClient
const GRPC_TIMEOUT = 2000;

exports.connect = function (options) {
    if (enableGrpc) {
        return;
    }
    amqper.connect(options, function () {
        amqper.asRpcClient(function(rpcCli) {
            rpcClient = rpcCli;
        }, function(reason) {
            log.error('Initializing as rpc client failed, reason:', reason);
            stopServers();
            process.exit();
      });
    }, function(reason) {
        log.error('Connect to rabbitMQ server failed, reason:', reason);
        process.exit();
    });
};

exports.disconnect = function() {
    if (enableGrpc) {
        // Clean gRPC clients
        return;
    }
    amqper.disconnect();
};

function toGrpc(to, method, args, callbacks, timeout) {
    const clusterMethods = ['schedule', 'getScheduled', 'getWorkerAttr'];
    const nodeManagerMethods = ['getNode', 'queryNode', 'recycleNode'];
    const opt = () => ({deadline: new Date(Date.now() + GRPC_TIMEOUT)});

    let type = 'conference';
    if (clusterMethods.includes(method)) {
        type = 'clusterManager';
    } else if (nodeManagerMethods.includes(method)) {
        type = 'nodeManager';
    } else if (method === 'handleSipUpdate') {
        type = 'sipPortal';
        to = global.config.cluster.sip_portal || 'localhost:9090';
    }
    if (!grpcNode[to]) {
        log.debug('Start gRPC client:', type, to);
        grpcNode[to] = grpcTools.startClient(type, to);
    }
    const cb = callbacks['callback'] || function() {};
    if (method === 'schedule') {
        const req = {
            purpose: args[0],
            task: args[1],
            preference: (typeof args[2] === 'object') ? args[2] : {},
            reserveTime: args[3]
        };
        grpcNode[to].schedule(req, opt(), (err, result) => {
            if (err) {
                log.info('schedule error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'getScheduled') {
        const req = {
            purpose: args[0],
            task: args[1]
        };
        grpcNode[to].getScheduled(req, opt(), (err, result) => {
            log.info('getScheduled:', result);
            if (err) {
                log.info('getScheduled error:', err);
                cb('error');
            } else {
                const workerId = result.message;
                grpcNode[to].getWorkerAttr({id: workerId}, opt(), (err, result) => {
                    log.info('getWorkerAtt:', result);
                    if (err) {
                        log.info('getWorkerAttr error:', err);
                        cb('error');
                    } else {
                        const addr = result.info.ip + ':' + result.info.grpcPort;
                        cb(addr);
                    }
                });
            }
        });
    } else if (method === 'getNode') {
        const req = {info: args[0]};
        grpcNode[to].getNode(req, opt(), (err, result) => {
            if (err) {
                log.info('getNode error:', err);
                cb('error');
            } else {
                cb(result.message);
            }
        });
    } else if (method === 'queryNode') {
        const req = {info: {task: args[0]}};
        grpcNode[to].queryNode(req, opt(), (err, result) => {
            if (err) {
                log.info('queryNode error:', err);
                cb('error');
            } else {
                cb(result.message);
            }
        });
    } else if (method === 'getParticipants') {
        grpcNode[to].getParticipants({}, opt(), (err, result) => {
            if (err) {
                log.info('getParticipants error:', err);
                cb('error');
            } else {
                cb(result.result);
            }
        });
    } else if (method === 'controlParticipant') {
        const req = {
            participantId: args[0],
            jsonPatch: JSON.stringify(args[1])
        };
        grpcNode[to].controlParticipant(req, opt(), (err, result) => {
            if (err) {
                log.info('controlParticipant error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'dropParticipant') {
        const req = {id: args[0]};
        grpcNode[to].dropParticipant(req, opt(), (err, result) => {
            if (err) {
                log.info('dropParticipant error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'getStreams') {
        grpcNode[to].getStreams({}, opt(), (err, result) => {
            if (err) {
                log.info('getStreams error:', err);
                cb('error');
            } else {
                const streams = result.result;
                streams.forEach((stream) => {
                    if (stream.info.attributes) {
                        stream.info.attributes = JSON.parse(stream.info.attributes);
                    }
                });
                cb(streams);
            }
        });
    } else if (method === 'controlStream') {
        const req = {
            id: args[0],
            commands: args[1]
        };
        req.commands = req.commands.map((command) => {
            return JSON.stringify(command);
        });
        grpcNode[to].controlStream(req, opt(), (err, result) => {
            if (err) {
                log.info('controlStream error:', err);
                cb('error');
            } else {
                if (result.info.attributes) {
                    result.info.attributes = JSON.parse(result.info.attributes);
                }
                cb(result);
            }
        });
    } else if (method === 'deleteStream') {
        grpcNode[to].deleteStream({id: args[0]}, opt(), (err, result) => {
            if (err) {
                log.info('deleteStream error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'addStreamingIn') {
        const req = {
            roomId: args[0],
            pubInfo: args[1]
        };
        grpcNode[to].addStreamingIn(req, opt(), (err, result) => {
            if (err) {
                log.info('addStreamingIn error:', err);
                cb('error');
            } else {
                if (result.info.attributes) {
                    result.info.attributes = JSON.parse(result.info.attributes);
                }
                cb(result);
            }
        });
    } else if (method === 'getSubscriptions') {
        const req = {type: args[0]};
        grpcNode[to].getSubscriptions(req, opt(), (err, result) => {
            if (err) {
                log.info('getSubscriptions error:', err);
                cb('error');
            } else {
                cb(result.result);
            }
        });
    } else if (method === 'getSubscriptionInfo') {
        const req = {id: args[0]};
        grpcNode[to].getSubscriptionInfo(req, opt(), (err, result) => {
            if (err) {
                log.info('getSubscriptionInfo error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'addServerSideSubscription') {
        const req = {
            roomId: args[0],
            subInfo: args[1]
        };
        if (req.subInfo.media) {
            if (req.subInfo.media.audio === false) {
                delete req.subInfo.media.audio;
            }
            if (req.subInfo.media.video === false) {
                delete req.subInfo.media.video;
            }
        }
        grpcNode[to].addServerSideSubscription(req, opt(), (err, result) => {
            if (err) {
                log.info('addServerSideSubscription error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'controlSubscription') {
        const req = {
            id: args[0],
            commands: args[1]
        };
        req.commands = req.commands.map((command) => {
            return JSON.stringify(command);
        });
        grpcNode[to].controlSubscription(req, opt(), (err, result) => {
            if (err) {
                log.info('controlSubscription error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'deleteSubscription') {
        const req = {
            id: args[0],
            type: args[1]
        };
        grpcNode[to].deleteSubscription(req, opt(), (err, result) => {
            if (err) {
                log.info('deleteSubscription error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'destroy') {
        grpcNode[to].destroy({}, opt(), (err, result) => {
            if (err) {
                log.info('destroy error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'getSipCalls') {
        grpcNode[to].getSipCalls({}, opt(), (err, result) => {
            if (err) {
                log.info('getSipCalls error:', err);
                cb('error');
            } else {
                cb(result.result);
            }
        });
    } else if (method === 'getSipCall') {
        grpcNode[to].getSipCall({id: args[0]}, opt(), (err, result) => {
            if (err) {
                log.info('getSipCall error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'makeSipCall') {
        const req = {
            id: args[0],
            peer: args[1].peerURI,
            mediaIn: args[1].mediaIn,
            mediaOut: args[1].mediaOut
        };
        grpcNode[to].makeSipCall(req, opt(), (err, result) => {
            if (err) {
                log.info('makeSipCall error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'controlSipCall') {
        const req = {
            id: args[0],
            commands: args[1]
        };
        req.commands = req.commands.map((command) => {
            return JSON.stringify(command);
        });
        grpcNode[to].controlSipCall(req, opt(), (err, result) => {
            if (err) {
                log.info('controlSipCall error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'endSipCall') {
        grpcNode[to].endSipCall({id: args[0]}, opt(), (err, result) => {
            if (err) {
                log.info('endSipCall error:', err);
                cb('error');
            } else {
                cb(result);
            }
        });
    } else if (method === 'handleSipUpdate') {
        const req = {
            type: args[0].type,
            roomId: args[0].room_id,
            sip: args[0].sip
        };
        grpcNode[to].handleSipUpdate(req, opt(), (err, result) => {
            if (err) {
                log.info('handleSipUpdate error:', err);
                cb('error');
            } else {
                cb(result.message);
            }
        });
    } else {
        log.warn('Unknown RPC method:', method);
    }
}

/*
 * Calls remotely the 'method' function defined in rpcPublic of 'to'.
 */
exports.callRpc = function (to, method, args, callbacks, timeout) {
    if (enableGrpc) {
        toGrpc(to, method, args, callbacks, timeout);
    }
    if (rpcClient) {
        rpcClient.remoteCall(to, method, args, callbacks, timeout);
    }
};
