/*global exports, require, setTimeout, clearTimeout*/
'use strict';
var amqp = require('amqp');
var log = require('./../logger').logger.getLogger('RPC');
var TIMEOUT = 3000;
var corrID = 0;
var map = {};   //{corrID: {fn: callback, to: timeout}}
var clientQueue;
var connection;
var exc;

exports.connect = function (addr) {
    connection = amqp.createConnection(addr);
    connection.on('ready', function () {
        log.info('Connected to rabbitMQ server');

        //Create a direct exchange
        exc = connection.exchange('woogeenRpc', {type: 'direct'}, function (exchange) {
            log.info('Exchange ' + exchange.name + ' is open');

            //Create the queue for send messages
            clientQueue = connection.queue('', function (q) {
                log.info('ClientQueue ' + q.name + ' is open');
                clientQueue.bind('woogeenRpc', clientQueue.name);
                clientQueue.subscribe(function (message) {
                    if (map[message.corrID] !== undefined) {
                        map[message.corrID].fn[message.type](message.data, message.err);
                        clearTimeout(map[message.corrID].to);
                        delete map[message.corrID];
                    }
                });
            });
        });
    });
};

exports.disconnect = function() {
    if (connection) {
        connection.disconnect();
        connection = undefined;
    }
};

var callbackError = function (corrID) {
    for (var i in map[corrID].fn) {
        map[corrID].fn[i]('timeout');
    }
    delete map[corrID];
};

/*
 * Calls remotely the 'method' function defined in rpcPublic of 'to'.
 */
exports.callRpc = function (to, method, args, callbacks, timeout) {
    if (!clientQueue) {
        for (var i in callbacks) {
            callbacks[i]('rpc client not ready');
        }
        return;
    }
    corrID += 1;
    map[corrID] = {};
    map[corrID].fn = callbacks;
    map[corrID].to = setTimeout(callbackError, ((typeof timeout === 'number' && timeout) ? timeout : TIMEOUT), corrID);
    var send = {method: method, args: args, corrID: corrID, replyTo: clientQueue.name};
    exc.publish(to, send);
};
