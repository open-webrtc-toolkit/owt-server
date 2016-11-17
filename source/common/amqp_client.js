/*global require, exports*/
'use strict';
var amqp = require('amqp');
var log = require('./logger').logger.getLogger('AmqpClient');
var TIMEOUT = 2000;
var REMOVAL_TIMEOUT = 7 * 24 * 3600 * 1000;

var declareExchange = function(conn, name, type, autoDelete, on_ok, on_failure) {
    var ok = false;
    var exc = conn.exchange(name, {type: type, autoDelete: autoDelete}, function (exchange) {
        log.info('Exchange ' + exchange.name + ' is open');
        ok = true;
        on_ok(exc);
    });

    setTimeout(function() {
        if (!ok) {
            log.error('Declare exchange [name: ' + name + ', type: ' + type + '] failed.');
            on_failure('Declare exchange [name: ' + name + ', type: ' + type + '] failed.');
        }
    }, 3000);
};

var rpcClient = function(bus, conn, on_ready, on_failure) {
    var handler = {bus: bus};

    var call_map = {},
        corrID = 0,
        ready = false,
        reply_q,
        exc;

    declareExchange(conn, 'woogeenRpc', 'direct', true, function (exc_got) {
        exc = exc_got;
        var timer = setTimeout(function () {
            if (!ready) {
                exc.destroy(true);
                on_failure('Declare reply queue or rpc-client failed.');
            }
        }, 2000);

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
    }, on_failure);

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
        exc && exc.publish(to, {method: method, args: args});
    };

    handler.close = function() {
        for (var i in call_map) {
             clearTimeout(call_map[i].timer);
        }
        call_map = {};
        reply_q && reply_q.destroy();
        reply_q = undefined;
        exc && exc.destroy(true);
        exc = undefined;
    };

    return handler;
};

var rpcServer = function(bus, conn, id, methods, on_ready, on_failure) {
    var handler = {bus: bus};

    var exc, request_q;

    declareExchange(conn, 'woogeenRpc', 'direct', true, function (exc_got) {
        exc = exc_got;
        var ready = false;
        var timer = setTimeout(function () {
            if (!ready) {
                exc.destroy(true);
                on_failure('Declare request queue or rpc-server failed.');
            }
        }, 2000);

        request_q = conn.queue(id, function (queueCreated) {
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
                ready = true;
                on_ready();
            });
        });
    }, on_failure);

    handler.close = function() {
        request_q && request_q.destroy();
        request_q = undefined;
        exc && exc.destroy(true);
        exc = undefined;
    };

    return handler;
};

var topicParticipant = function(bus, conn, excName, on_ready, on_failure) {
    var that = {bus: bus},
        message_processor;

    var exc, msg_q;

    declareExchange(conn, excName, 'topic', false, function (exc_got) {
        exc = exc_got;
        var ready = false;
        var timer = setTimeout(function () {
            if (!ready) {
                exc.destroy(true);
                on_failure('Declare queue for topic[' + excName + '] failed.');
            }
        }, 2000);

        msg_q = conn.queue('', function (queueCreated) {
            log.info('Message queue for topic participant is open:', queueCreated.name);
            ready = true;
            clearTimeout(timer);
            on_ready();
        });

    }, on_failure);

    that.subscribe = function(patterns, on_message, on_ok) {
        message_processor = on_message;
        patterns.map(function(pattern) {
            msg_q.bind(exc.name, pattern, function() {
                log.debug('Follow topic [' + pattern + '] ok.');
            });
        });
        msg_q.subscribe(function(message) {
            try {
                message_processor && message_processor(message);
            } catch (error) {
                log.error('Error processing topic message:', message, 'and error:', error);
            }
        });
        on_ok();
    };

    that.unsubscribe = function(patterns) {
        message_processor = undefined;
        patterns.map(function(pattern) {
            msg_q.unbind(exc.name, pattern);
            log.debug('Ignore topic [' + pattern + ']');
        });
    };

    that.publish = function(topic, data) {
        exc.publish(topic, data);
    };

    that.close = function () {
        msg_q.destroy();
        exc.destroy(true);
    };

    return that;
};

module.exports = function() {
    var that = {};

    var connection,
        rpc_client,
        rpc_server,
        topic_participants = {},
        monitor_exc;

    var close = function () {
        rpc_server && rpc_server.close();
        rpc_server = undefined;
        rpc_client && rpc_client.close();
        rpc_client = undefined;
        for (var group in topic_participants) {
            topic_participants[group].close();
        }
        topic_participants = {};
    };

    that.connect = function(hostPort, on_ok, on_failure) {
        log.info('Connecting to rabbitMQ server:', hostPort);
        var conn = amqp.createConnection(hostPort);
        conn.on('ready', function() {
            log.info('Connecting to rabbitMQ server OK');
            connection = conn;
            on_ok();
        });

        conn.on('error', function(e) {
            close();
            connection = undefined;
            log.info('Connection to rabbitMQ server error', e);
            on_failure('amqp connection error');
        });

        conn.on('disconnect', function() {
            if (connection) {
                close();
                connection = undefined;
                log.info('Connection to rabbitMQ server disconnected.');
                on_failure('amqp connection disconnected');
            }
        });
    };

    that.disconnect = function() {
        close();
        connection && connection.disconnect();
        connection = undefined;
    };

    that.asRpcServer = function(id, methods, on_ok, on_failure) {
        if (rpc_server) {
            return on_ok(rpc_server);
        }

        if (connection) {
            rpc_server = rpcServer(that, connection, id, methods, function () { on_ok(rpc_server); }, on_failure);
        } else {
            on_failure('amqp connection not ready');
        }
    };

    that.asRpcClient = function(on_ok, on_failure) {
        if (rpc_client) {
            return on_ok(rpc_client);
        }

        if (connection) {
            rpc_client = rpcClient(that, connection, function () { on_ok(rpc_client); }, on_failure);
        } else {
            on_failure('amqp connection not ready');
        }
    };

    that.asTopicParticipant = function(group, on_ok, on_failure) {
        if (connection) {
            if (topic_participants[group]) {
                return on_ok(topic_participants[group]);
            }

            var topic_participant = topicParticipant(that, connection, group, function() {
                topic_participants[group] = topic_participant;
                on_ok(topic_participant);
            }, on_failure);
        } else {
            on_failure('amqp connection not ready');
        }
    };

    that.monitor = function(target, evt, callback) {
    };

    that.unmonitor = function(target, evt) {
    };

    return that;
};

