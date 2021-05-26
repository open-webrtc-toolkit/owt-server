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

exports.connect = function (options) {
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
    amqper.disconnect();
};

/*
 * Calls remotely the 'method' function defined in rpcPublic of 'to'.
 */
exports.callRpc = function (to, method, args, callbacks, timeout) {
    if (rpcClient) {
        rpcClient.remoteCall(to, method, args, callbacks, timeout);
    }
};
