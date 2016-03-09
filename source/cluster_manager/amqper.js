/*global require, exports*/
'use strict';
var amqp = require('amqp');
var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('AMQPER');

// Configuration default values
var config = require('./config').rabbit || {};
config.host = config.host || 'localhost';
config.port = config.port || 5672;

var TIMEOUT = 5000;

// This timeout shouldn't be too low because it won't listen to onReady responses from ErizoJS
var REMOVAL_TIMEOUT = 300000;

var corrID = 0;
var map = {};   //{corrID: {fn: callback, to: timeout}}
var connection, rpc_exc, broadcast_exc, clientQueue;

var addr = config.url ? {url: config.url} : {host: config.host, port: config.port};
var rpcPublic = {};

exports.timeout = TIMEOUT;

exports.setPublicRPC = function(methods) {
    rpcPublic = methods;
};

exports.connect = function(callback) {

    // Create the amqp connection to rabbitMQ server
    connection = amqp.createConnection(addr);
    connection.on('ready', function () {

        //Create a direct exchange
        rpc_exc = connection.exchange('rpcExchange', {type: 'direct'}, function (exchange) {
            try {
                log.info('Exchange ' + exchange.name + ' is open');

                //Create the queue for receiving messages
                clientQueue = connection.queue('', function (q) {
                    log.info('ClientQueue ' + q.name + ' is open');

                    clientQueue.bind('rpcExchange', clientQueue.name, callback);

                    clientQueue.subscribe(function (message) {
                        try {
                            log.debug('New message received', message);

                            if(map[message.corrID] !== undefined) {
                                log.debug('Callback', message.type, ' - ', message.data);
                                clearTimeout(map[message.corrID].to);
                                if (message.type === 'onReady') map[message.corrID].fn[message.type].call({}, message.data);
                                else map[message.corrID].fn[message.type].call({}, message.data);
                                setTimeout(function() {
                                    if (map[message.corrID] !== undefined) delete map[message.corrID];
                                }, REMOVAL_TIMEOUT);
                            }
                        } catch(err) {
                            log.error('Error processing response: ', err);
                        }
                    });
                });
            } catch (err) {
                log.error('Error in exchange ', exchange.name, ' - error - ', err);
            }
        });

        //Create a fanout exchange
        broadcast_exc = connection.exchange('broadcastExchange', {type: 'topic'}, function (exchange) {
            log.info('Exchange ' + exchange.name + ' is open');
        });
    });

    connection.on('error', function(e) {
       log.error('Connection error...', e);
    });
};

exports.bind = function(id, callback) {

    //Create the queue for receive messages
    var q = connection.queue(id, function (queueCreated) {
        try {
            log.info('Queue ' + queueCreated.name + ' is open');

            q.bind('rpcExchange', id, callback);
            q.subscribe(function (message) {
                try {
                    log.debug('New message received', message);
                    message.args = message.args || [];
                    message.args.push(function(type, result) {
                        rpc_exc.publish(message.replyTo, {data: result, corrID: message.corrID, type: type});
                    });
                    rpcPublic[message.method].apply(rpcPublic, message.args);
                } catch (error) {
                    log.error('message:', message);
                    log.error('Error processing call: ', error);
                }
            });
        } catch (err) {
            log.error('Error in exchange ', queueCreated.name, ' - error - ', err);
        }
    });
};

//Subscribe to 'topic'
exports.bind_broadcast = function(id, callback) {

    //Create the queue for receive messages
    var q = connection.queue('', function (queueCreated) {
        try {
            log.info('Queue ' + queueCreated.name + ' is open');
            q.bind('broadcastExchange', id);
            q.subscribe(function (m) {callback(m);});
        } catch (err) {
            log.error('Error in exchange ', queueCreated.name, ' - error - ', err);
        }
    });
};

/*
 * Publish broadcast messages to 'topic'
 */
exports.broadcast = function(topic, message) {
    broadcast_exc.publish(topic, message);
};

/*
 * Calls remotely the 'method' function defined in rpcPublic of 'to'.
 */
exports.callRpc = function(to, method, args, callbacks) {
    corrID ++;
    map[corrID] = {};
    map[corrID].fn = callbacks;
    map[corrID].to = setTimeout(callbackError, TIMEOUT, corrID);
    rpc_exc.publish(to, {method: method, args: args, corrID: corrID, replyTo: clientQueue.name});
};


var callbackError = function(corrID) {
    for (var i in map[corrID].fn) {
        map[corrID].fn[i]('timeout');
    }
    delete map[corrID];
};
