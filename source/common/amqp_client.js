/*global require, exports*/
'use strict';
var amqp = require('amqp');
var log = require('./logger').logger.getLogger('AmqpClient');
var TIMEOUT = 1000;
var REMOVAL_TIMEOUT = 7 * 24 * 3600 * 1000;

module.exports = function() {
    var that = {};

    var connection,
        rpc_exc,
        rpc_client,
        rpc_server,
        monitor_exc;

    var declareExchange = function(name, type, on_ok, on_failure) {
        if (connection) {
            var ok = false;
            var exc = connection.exchange(name, {type: type}, function (exchange) {
                log.info('Exchange ' + exchange.name + ' is open');
                ok = true;
                on_ok(exc);
            });

            setTimeout(function() {
                if (!ok) {
                    log.error('Declare exchange [name: ' + name + ', type: ' + type + '] failed.');
                    on_failure('Declare exchange [name: ' + name + ', type: ' + type + '] failed.');
                }
            }, 5000);
        } else {
            on_failure('amqp connection is not ready');
        }
    };

    var rpcClient = function(conn, exc, on_ready) {
        var handler = {};//neccesary? maybe 'this' is clearer.

        var call_map = {},
            corrID = 0,
            ready = false,
            reply_q = conn.queue('', function (q) {
                log.info('Reply queue for rpc client ' + q.name + ' is open');

                reply_q.bind(exc.name, reply_q.name, function () {
                    reply_q.subscribe(function (message) {
                        try {
                            log.debug('New message received', message);

                            if(call_map[message.corrID] !== undefined) {
                                log.debug('Callback', message.type, ' - ', message.data);
                                clearTimeout(call_map[message.corrID].timer);
                                call_map[message.corrID].fn[message.type].call({}, message.data, message.err);
                                if (call_map[message.corrID].fn['onStatus']) {
                                //FIXME: if the rpc contains a 'onStatus' callback, it will not be deleted immediately, but wait for a time span of REMOVAL_TIMEOUT then delete, which will lead to the issue that the status update would not be observed by the caller thereafter.
                                    setTimeout(function() {
                                        (call_map[message.corrID] !== undefined) &&  (delete call_map[message.corrID]);
                                    }, REMOVAL_TIMEOUT);
                                } else {
                                    (call_map[message.corrID] !== undefined) && (delete call_map[message.corrID]);
                                }
                            } else {
                              log.warn('Late rpc reply:', message);
                            }
                        } catch(err) {
                            log.error('Error processing response: ', err);
                        }
                    });
                    ready = true;
                    on_ready();
                });
            });

        handler.remoteCall = function(to, method, args, callbacks) {
            log.debug('remoteCall, corrID:', corrID, 'to:', to, 'method:', method);
            if (ready) {
                corrID ++;
                call_map[corrID] = {};
                call_map[corrID].fn = callbacks || {callback: function() {}};
                call_map[corrID].timer = setTimeout(function() {
                    if (call_map[corrID]) {
                        for (var i in call_map[corrID].fn) {
                            (typeof call_map[corrID].fn[i] === 'function' ) && call_map[corrID].fn[i]('timeout');
                        }
                        delete call_map[corrID];
                    }
                }, TIMEOUT);

                exc.publish(to, {method: method, args: args, corrID: corrID, replyTo: reply_q.name});
            } else {
                for (var i in callbacks) {
                    (typeof callbacks[i] === 'function' ) && callbacks[i]('error', 'rpc client is not ready');
                }
            }
        };

        handler.remoteCast = function(to, method, args) {
            exc.publish(to, {method: method, args: args});
        };

        handler.close = function() {
            for (var i in call_map) {
                 clearTimeout(call_map[i].timer);
            }
            call_map = {};
            reply_q && reply_q.destroy();
        };

        return handler;
    };

    var rpcServer = function(conn, exc, id, methods, on_ready) {
        var handler = {};

        var request_q = connection.queue(id, function (queueCreated) {
            log.info('Request queue for rpc server ' + queueCreated.name + ' is open');

            request_q.bind(exc.name, id, function() {
                request_q.subscribe(function (message) {
                    try {
                        log.debug('New message received', message);
                        message.args = message.args || [];
                        if (message.replyTo && message.corrID !== undefined) {
                            message.args.push(function(type, result, err) {
                                exc.publish(message.replyTo, {data: result, corrID: message.corrID, type: type, err: err});
                            });
                        }
                        methods[message.method].apply(methods, message.args);
                    } catch (error) {
                        log.error('message:', message);
                        log.error('Error processing call: ', error);
                    }
                });
                on_ready();
            });
        });

        handler.close = function() {
            request_q.destroy();
        };

        return handler;
    };

    that.connect = function(addr, on_ok, on_failure) {
        log.info('Connecting to rabbitMQ server:', addr);
        var conn = amqp.createConnection(addr);
        conn.on('ready', function() {
            log.info('Connecting to rabbitMQ server OK');
            connection = conn;
            on_ok();
        });

        conn.on('error', function(e) {
            connection = undefined;
            log.info('Connecting to rabbitMQ server error', e);
            on_failure('amqp connection error');
        });
    };

    that.disconnect = function() {
        rpc_server && rpc_server.close();
        rpc_server = undefined;
        rpc_client && rpc_client.close();
        rpc_client = undefined;
        rpc_exc && rpc_exc.destroy(true);
        rpc_exc = undefined;
        connection && connection.disconnect();
        connection = undefined;
    };

    that.asRpcServer = function(id, methods, on_ok, on_failure) {
        if (rpc_server) {
            on_ok();
            return that;
        }

        var init_rpc_server = function() {
            var timer = setTimeout(on_failure, 10 * 1000, 'Initializing rpc server timeout');
            rpc_server = rpcServer(connection, rpc_exc, id, methods, function() {
                clearTimeout(timer);
                on_ok();
            });
        };

        if (connection) {
            if (rpc_exc) {
                init_rpc_server();
            } else {
                declareExchange('woogeenRpc', 'direct', function(exc) {
                    rpc_exc = exc;
                    init_rpc_server();
                }, on_failure);
            }
        } else {
            on_failure('amqp connection not ready');
        }

        return that;
    };

    that.asRpcClient = function(on_ok, on_failure) {
        if (rpc_client) {
            return on_ok();
        }

        var init_rpc_client = function() {
            var timer = setTimeout(on_failure, 10 * 1000, 'Initializing rpc client timeout');
            rpc_client = rpcClient(connection, rpc_exc, function() {
                clearTimeout(timer);
                on_ok();
            });
        };

        if (connection) {
            if (rpc_exc) {
                init_rpc_client();
            } else {
                declareExchange('woogeenRpc', 'direct', function(exc) {
                    rpc_exc = exc;
                    init_rpc_client();
                }, on_failure);
            }
        } else {
            on_failure('amqp connection not ready');
        }

        return that;
    };

    that.callRpc = function(to, method, args, callback) {
        if (rpc_client) {
            rpc_client.remoteCall(to, method, args, callback);
        } else {
            callback('error', 'Rpc client is not ready.');
        }
    };

    that.remoteCast = function(to, method, args) {
        if (rpc_client) {
            rpc_client.remoteCast(to, method, args);
        }
    };

    that.monitor = function(target, evt, callback) {
    };

    that.unmonitor = function(target, evt) {
    };

    return that;
};

