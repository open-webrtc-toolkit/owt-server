/*global exports, require, setTimeout, clearTimeout*/
'use strict';
var amqp = require('amqp');
var rpcPublic = require('./rpcPublic');
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
        log.info('Conected to rabbitMQ server');

        //Create a direct exchange
        exc = connection.exchange('woogeenRpc', {type: 'direct'}, function (exchange) {
            log.info('Exchange ' + exchange.name + ' is open');

            //Create the queue for receive messages
            var q = connection.queue('nuveQueue', function (queue) {
                log.info('Queue ' + queue.name + ' is open');

                q.bind('woogeenRpc', 'nuve');
                q.subscribe(function (message) {
                    rpcPublic[message.method](message.args, function (type, result) {
                        exc.publish(message.replyTo, {data: result, corrID: message.corrID, type: type});
                    });
                });
            });

            //Create the queue for send messages
            clientQueue = connection.queue('', function (q) {
                log.info('ClientQueue ' + q.name + ' is open');
                clientQueue.bind('woogeenRpc', clientQueue.name);
                clientQueue.subscribe(function (message) {
                    if (map[message.corrID] !== undefined) {
                        map[message.corrID].fn[message.type](message.data);
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
exports.callRpc = function (to, method, args, callbacks) {
    corrID += 1;
    map[corrID] = {};
    map[corrID].fn = callbacks;
    map[corrID].to = setTimeout(callbackError, TIMEOUT, corrID);
    var send = {method: method, args: args, corrID: corrID, replyTo: clientQueue.name};
    exc.publish(to, send);
};
