// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var util = require('util');
var os = require("os");
var path = require('path');
var fs = require('fs');
var amqp = require('amqplib');
var logger = require('./logger').logger;

var log = logger.getLogger('AmqpClient');
var cipher = require('./cipher');
var TIMEOUT = 5000;
var purpose = path.basename(!process.pkg ? __dirname : path.dirname(process.execPath));
var pid = process.pid;

const RPC_EXC = {
    name: 'owtRpc',
    type: 'direct',
    options: {
        autoDelete: false,
        durable: true,
    }
};

const MONITOR_EXC = {
    name: 'owtMonitoring',
    type: 'topic',
    options: {
        autoDelete: false,
        durable: true,
    }
};

const Q_OPTION = {
    durable: true,
    autoDelete: true,
    messageTtl: 2 * 60 * 1000, //message auto delete when no be got gt 2min
    expires: 5 * 60 * 1000 //auto delete queue when no customers gt 5min
};

var getQueueName = function (role, index) {
    log.info("getQueueName:", purpose);
    let hostIp = os.hostname();
    if (global.config && global.config.cluster && global.config.cluster.worker && global.config.cluster.worker.ip) {
        hostIp = global.config.cluster.worker.ip;
    } else if (global.config && global.config.server && global.config.server.hostname) {
        hostIp = global.config.server.hostname;
    }

    return `${purpose}-${role}@${hostIp}-${pid}-${index}-${Date.now()}`
}

var wrapIdx = 0;

class MsgSender {
    constructor(amqpCli, exchange = RPC_EXC) {
        this.bus = amqpCli;
        this.exchange = exchange;
        this.log = logger.getLogger('MsgSender');
        this.ready = false;
        this.closed = false;
    };

    setup() {
        let self = this;
        return new Promise((resolve, reject) => {
            if (self.closed) {
                reject("has been closed");
            }

            if (self.ready) {
                resolve();
            }

            if (!self.bus.channel) {
                reject("rabbitmq channel is null");
            }

            self.bus.channel.assertExchange(self.exchange.name, self.exchange.type, self.exchange.options).then(() => {
                self.log.info('assertExchange', JSON.stringify(self.exchange));
                self.ready = true;
                resolve();
            }).catch((e) => {
                self.log.error(`${self.bus.idx} setup failed ${util.inspect(e)}`);
                reject(e);
            });
        });
    }

    async notify(to, method, args) {
        return this.rpcCall(to, {method, args});
    };

    async remoteCast(to, method, args) {
        return this.notify(to, method, args);
    };

    async rpcCall(to, content) {
        let self = this;
        if (!await self.isLoss()) {
            try {
                content = JSON.stringify(content);
                self.bus.channel.publish(self.exchange.name, to, Buffer.from(content));
            } catch (e) {
                self.log.error('Failed to publish:', util.inspect(e));
            }
        } else {
            self.log.error('Failed to publish:', to, content);
        }
    };

    async isLoss(count = 60) {
        let self = this;
        if (self.closed) {
            self.log.warn("is closed");
            return true;
        } else if (self.ready && !self.closed && self.bus.channel) {
            return false;
        } else {
            return new Promise(function (resolve, reject) {
                let interval = setInterval(function () {
                    count > 0 && count--;
                    if (self.closed) {
                        clearInterval(interval);
                        resolve(true);
                    } else if (self.ready && !self.closed && self.bus.channel) {
                        clearInterval(interval);
                        resolve(false);
                    } else if (count === 0) {
                        clearInterval(interval);
                        resolve(true);
                    }
                }, 50);
            });
        }
    }

    publish(topic, data) {
        return this.rpcCall(topic, data);
    }

    async close() {
        this.ready = false;
        this.closed = true;
    }
};

class MsgSenderReceiver extends MsgSender {
    constructor(amqpCli, exchange = RPC_EXC, queue, options = Q_OPTION) {
        super(amqpCli, exchange);
        this.options = options;
        this.queue = queue ? queue : getQueueName(exchange.type, this.bus.idx)
        this.subscriptions = new Map(); //{patterns, cb}
        this.log = logger.getLogger('MsgSenderReceiver');
        this.rebinded = false;
    }

    translateToRegx(regx) {
        if (!regx) {
            return;
        }

        let regxParts = regx.split('.');
        let regxStr = "";
        let len = regxParts.length;
        for (let i in regxParts) {
            let part = regxParts[i];
            switch (part) {
                case "*":
                    if (i == len - 1) {
                        regxStr += "[.]";
                    }
                    regxStr += "[^.]+?";
                    break;
                case "#":
                    regxStr += ".*";
                    break;
                default:
                    if (i == 0) {
                        regxStr = part;
                    } else {
                        regxStr += "[.]" + part;
                    }
                    break;
            }
        }
        return new RegExp(`^${regxStr}$`);
    }

    setup() {
        let self = this;
        let channel = null;
        self.rebinded = false;
        return super.setup().then(() => {
            channel = this.bus.channel;
            return channel.assertQueue(self.queue, self.options);
        }).then((result) => {
            if (result) {
                self.log.info('setup ok', "queue:", result.queue);
                self.queue = result.queue;

                let resubscribe = [];
                self.subscriptions.forEach((sub, tag) => {
                    resubscribe.push({oldTag: tag, patterns: sub.patterns.concat(), cb: sub.cb, onOk: sub.onOk})
                });

                let rebind = resubscribe.map((sub) => {
                    return new Promise((resolve) => {
                        if (self.subscriptions.delete(sub.oldTag)) {
                            self.subscribe(sub.patterns, sub.cb, () => {
                                sub.onOk ? sub.onOk() : null;
                                channel.cancel(sub.oldTag).then((ret) => {
                                    self.log.info('cancel old consumerTag:', sub.oldTag, "ret:", ret);
                                    resolve();
                                }).catch((err) => {
                                    self.log.error('Failed to cancel old consumerTag:', sub.oldTag);
                                    resolve();
                                });
                            });
                        } else {
                            resolve();
                        }
                    });
                });
                return Promise.all(rebind).then(()=>{
                    self.rebinded = true;
                });
            } else {
                self.log.error('setup failed', "queue:", self.queue);
            }
        });
    }

    async subscribe(patterns, onMessage, onOk) {
        let self = this;
        self.log.debug('subscribe:', this.queue, patterns);
        patterns = patterns.concat();
        const tagTail = Math.floor(Math.random() * 1000000000)
        const consumerTag = `${patterns.toString()}.${tagTail}`
        if (!(await self.isLoss(-1))) {
            const channel = self.bus.channel;
            let bindQueuePromise = patterns.map((pattern) => {
                return channel.bindQueue(self.queue, self.exchange.name, pattern)
                    .then(() => self.log.info(`exchange:${self.exchange.name},queue:${self.queue} Follow [${pattern}] ok.`))
                    .catch((err) => {
                        // queue might be delete
                        self.subscriptions.set(consumerTag, {patterns, cb: onMessage, onOk: onOk});
                        self.ready = false;
                        throw new Error(`Failed to bind queue:${self.queue} on exchange:${self.exchange.name} for pattern:${pattern}`);
                    });
            });
            Promise.all(bindQueuePromise).then(() => {
                channel.consume(self.queue, (rawMessage) => {
                    if (!rawMessage || !rawMessage.hasOwnProperty("content")) {
                        // queue might be delete
                        self.ready = false;
                        throw new Error(`subscribe failed patterns:${patterns}, exchange:${self.exchange.name}, queue:${self.queue} might be delete`);
                    }
                    try {
                        const msg = JSON.parse(rawMessage.content.toString());
                        const routingKey = rawMessage.fields.routingKey;
                        self.subscriptions.forEach((sub, tag) => {
                            let callback = undefined;
                            sub.patterns.forEach((pattern, index) => {
                                if (!callback) {
                                    let regx = self.translateToRegx(pattern);
                                    if (regx && regx.test(routingKey)) {
                                        callback = sub.cb;
                                    }
                                }
                            });

                            if (callback) {
                                self.log.debug(`exchange:${self.exchange.name},queue:${self.queue} on message:`, msg, rawMessage.fields.routingKey);
                                callback(msg, rawMessage.fields.routingKey);
                            }
                        });
                    } catch (error) {
                        self.log.error(`Error processing exchange:${self.exchange.name},queue:${self.queue} message:`, rawMessage, 'and error:', util.inspect(error));
                    }
                }, {noAck: true, consumerTag: consumerTag}).then((ok) => {
                    if (ok) {
                        self.log.info('subscribe ok', "exchange", self.exchange.name, "queue:", self.queue, 'patterns:', patterns, "consumerTag:", ok.consumerTag);
                        self.subscriptions.set(ok.consumerTag, {patterns, cb: onMessage});
                    } else {
                        self.log.error('subscribe failed', "exchange", self.exchange.name, "queue:", self.queue, 'patterns:', patterns, "consumerTag:", consumerTag);
                    }
                    onOk ? onOk() : null;
                });
            });
        } else {
            self.log.error('Failed to subscribe is not ready', "exchange", self.exchange.name, "queue:", self.queue, 'patterns:', patterns);
        }
    }

    async unsubscribe(patterns) {
        let self = this;
        patterns = patterns.concat();
        self.log.info('unsubscribe:', patterns);
        if (!(await self.isLoss(-1))) {
            const channel = self.bus.channel;
            let cancleTags = new Set();
            let ubindArr = []
            for (let index in patterns) {
                let pattern = patterns[index];
                let regx = self.translateToRegx(pattern);
                self.log.debug("translateToRegx:", pattern, "=>", regx);
                if (!regx) {
                    continue
                }
                self.subscriptions.forEach((sub, tag) => {
                    let hits = {};
                    let i = 0;
                    sub.patterns.forEach((oldPattern, index) => {
                        if (regx.test(oldPattern)) {
                            hits[index] = oldPattern;
                            i++;
                            ubindArr.push(new Promise(async (resolve) => {
                                try {
                                    await channel.unbindQueue(self.queue, self.exchange.name, oldPattern);
                                    self.log.info(`ubindQueue ${oldPattern} on queue:${self.queue} pattern:${oldPattern}`);
                                    resolve();
                                } catch (e) {
                                    self.log.error(`Failed to ubindQueue ${oldPattern} on queue:${self.queue}`, e);
                                    resolve();
                                }
                            }));
                        }
                    });

                    self.log.debug("pattern:", pattern, "hits:", hits, "hits.length:", i, "sub.patterns.length:", sub.patterns.length);
                    if (i >= sub.patterns.length) {
                        cancleTags.add(tag);
                    } else {
                        for (let i in hits) {
                            let index = sub.patterns.findIndex((p) => {
                                return p == hits[i]
                            });
                            if (index != -1) {
                                sub.patterns.splice(index, 1);
                            }
                        }
                    }
                });
            }

            if (ubindArr.length > 0) {
                await Promise.all(ubindArr);
            }
            ubindArr = [];
            if (cancleTags.size > 0) {
                cancleTags.forEach((consumerTag) => {
                    self.subscriptions.delete(consumerTag);
                    self.log.debug('del consumerTag:', consumerTag);
                    ubindArr.push(new Promise((resolve) => {
                        channel.cancel(consumerTag).then((ret) => {
                            self.log.info('cancel consumerTag:', consumerTag, "real.consumerTag:", consumerTag, "ret:", ret);
                            resolve();
                        }).catch((err) => {
                            self.log.error('Failed to cancel:', consumerTag);
                            resolve();
                        });
                    }));
                });
            }
            if (ubindArr.length > 0) {
                await Promise.all(ubindArr);
            }
            //keycoding add 2022-06-23 fix slave exit bug end
        } else {
            self.log.error(`Failed to unsubscribe is not ready`);
        }
    }

    async isLoss(count = 60) {
        let self = this;
        if (self.closed) {
            self.log.warn("is closed");
            return true;
        } else if (self.ready && !self.closed && self.bus.channel) {
            return false;
        } else {
            return new Promise(function (resolve, reject) {
                let interval = setInterval(function () {
                    count > 0 && count--;
                    if (self.closed) {
                        clearInterval(interval);
                        resolve(true);
                    } else if (self.ready && !self.closed && self.bus.channel && self.rebinded) {
                        clearInterval(interval);
                        resolve(false);
                    } else if (count === 0) {
                        clearInterval(interval);
                        resolve(true);
                    }
                }, 50);
            });
        }
    }

    close() {
        super.close();
        let self = this;
        let recycleArr = [];
        const channel = self.bus.channel;
        recycleArr.push(new Promise((resolve) => {
            if (!channel) {
                resolve();
            }
            channel.deleteQueue(self.queue).then((ret) => {
                self.log.info(`deleteQueue ${self.queue} success`, ret);
                resolve();
            }).catch((err) => {
                self.log.error(`deleteQueue ${self.queue} failed:`, err && err.message ? err.message : util.inspect(err));
                resolve();
            });
        }));
        return Promise.all(recycleArr).then(() => {
            self.subscriptions.clear();
        });
    }
}

class RpcClient extends MsgSenderReceiver {
    constructor(amqpCli) {
        super(amqpCli, RPC_EXC, getQueueName("client", amqpCli.idx));
        this.callMap = {};
        this.corrID = 0;
        this.log = logger.getLogger('RpcClient');
        this.subscriptions.set('rpcClient', {
            patterns: [this.queue],
            cb: this.receiveMsg.bind(this)
        });
    }

    receiveMsg(msg, routingKey) {
        let self = this;
        try {
            self.log.debug('New message received', msg);
            if (self.callMap[msg.corrID] !== undefined) {
                if (msg.data === 'error' || msg.data === 'timeout') self.log.error('Callback', msg.data, ' - ', msg.err);
                clearTimeout(self.callMap[msg.corrID].timer);
                self.callMap[msg.corrID].fn[msg.type].call({}, msg.data, msg.err);
                if (self.callMap[msg.corrID] !== undefined) {
                    delete self.callMap[msg.corrID];
                }
            } else {
                self.log.warn('Late rpc reply:', msg);
            }
        } catch (err) {
            self.log.error('Error processing response: ', util.inspect(err));
        }
    }

    async remoteCall(to, method, args, callbacks, timeout) {
        let self = this;
        let rpcTimeout = (global.faultAgentTimeOutMap && global.faultAgentTimeOutMap[to]) || timeout || TIMEOUT;
        self.log.debug('remoteCall, corrID:', self.corrID, 'to:', to, 'method:', method);
        if (self.ready && !self.closed) {
            const corrID = this.corrID++;
            self.callMap[corrID] = {};
            self.callMap[corrID].fn = callbacks || {
                callback: function () {
                }
            };
            self.callMap[corrID].timer = setTimeout(() => {
                if (self.callMap[corrID]) {
                    for (const i in self.callMap[corrID].fn) {
                        if (typeof self.callMap[corrID].fn[i] === 'function') {
                            self.log.debug(`remoteCall, corrID:, ${self.corrID}, to:, ${to}, method:, ${method}, args: ${JSON.stringify(args)}, timeout: ${rpcTimeout}`);
                            self.callMap[corrID].fn[i]('timeout');
                        }
                    }
                    delete self.callMap[corrID];
                }
            }, rpcTimeout);

            const content = {method, args, corrID, replyTo: self.queue};
            self.log.debug('Publish content:', JSON.stringify(content));
            try {
                self.publish(to, content)
            } catch (e) {
                self.log.error('Failed to publish:', util.inspect(e));
            }
        } else {
            self.log.error(`remoteCall, ready === ${self.ready}, to ${to}, method ${method}, args ${JSON.stringify(args)}`);
            for (const i in callbacks) {
                if (typeof callbacks[i] === 'function') {
                    callbacks[i]('error', 'rpc client is not ready');
                }
            }
        }
    }

    async remoteCast(to, method, args) {
        let self = this;
        if (self.ready && !self.closed) {
            const content = {method, args};
            try {
                self.publish(to, content)
            } catch (e) {
                self.log.error('Failed to publish:', e);
            }
        } else {
            self.log.error(`remoteCast, ready === ${self.ready}, to ${to}, method ${method}, args ${JSON.stringify(args)}`);
        }
    }

    makeRPC(to, method, args, on_ok, on_error, timeout) {
        return this.remoteCall(to, method, args, {
                callback: function (result, error_reason) {
                    if (result === 'error') {
                        typeof on_error === 'function' && on_error(error_reason, result);
                    } else if (result === 'timeout') {
                        typeof on_error === 'function' && on_error('Timeout to make rpc to ' + to + '.' + method, result);
                    } else {
                        typeof on_ok === 'function' && on_ok(result);
                    }
                }
            },
            timeout)
    }

    close() {
        let self = this;
        self.ready = false;
        self.closed = true;
        for (const i in self.callMap) {
            clearTimeout(self.callMap[i].timer);
        }
        self.callMap = {};
        return super.close();
    }
}

class RpcServer extends MsgSenderReceiver {
    constructor(amqpCli, id, methods) {
        super(amqpCli, RPC_EXC, id);
        this.methods = methods;
        this.log = logger.getLogger('RpcServer');
        this.subscriptions.set('rpcServer', {
            patterns: [this.queue],
            cb: this.receiveMsg.bind(this)
        });
    }

    receiveMsg(msg, routingKey) {
        let self = this;
        try {
            if (msg.replyTo && msg.corrID !== undefined) {
                msg.args.push((type, result, err) => {
                    const content = {
                        data: result,
                        corrID: msg.corrID,
                        type: type,
                        err: err,
                    };
                    self.publish(msg.replyTo, content);
                });
            }
            if (typeof self.methods[msg.method] === 'function') {
                self.methods[msg.method].apply(self.methods, msg.args);
            } else {
                self.log.warn('does not support this method:', msg.method);
                if (msg.replyTo && msg.corrID !== undefined) {
                    const content = {
                        data: 'error',
                        corrID: msg.corrID,
                        type: 'callback',
                        err: 'Not support method',
                    };
                    self.publish(msg.replyTo, content);
                }
            }
        } catch (error) {
            self.log.error('Error processing call: ', util.inspect(error));
        }
    }

    close() {
        let self = this;
        self.log.debug('close');
        return self.unsubscribe([self.queue]).then(()=>{
            self.ready = false;
            self.closed = true;
            self.subscriptions.clear();
        });
    }
}

class TopicParticipant extends MsgSenderReceiver {
    constructor(amqpCli, exchange = MONITOR_EXC, queue, options = Q_OPTION) {
        super(amqpCli, exchange, queue ? queue : getQueueName("topic", amqpCli.idx), options);
        this.log = logger.getLogger('TopicParticipant');
    }
}

class Monitor extends MsgSenderReceiver {
    constructor(amqpCli, onMessage) {
        super(amqpCli, MONITOR_EXC, getQueueName("monitor", amqpCli.idx));
        this.onMessage = onMessage;
        this.log = logger.getLogger('Monitor');
        this.subscriptions.set('monitor', {
            patterns: ['exit.#'],
            cb: this.receiveMsg.bind(this)
        });
    }

    receiveMsg(msg, routingKey) {
        let self = this;
        try {
            self.log.info('Received monitoring message:', msg);
            self.onMessage && self.onMessage(msg);
        } catch (error) {
            this.log.error('Error processing monitored message:', msg, 'and error:', util.inspect(error));
        }
    }
}

class MonitoringTarget extends MsgSender {
    constructor(amqpCli) {
        super(amqpCli, MONITOR_EXC);
        this.log = logger.getLogger('MonitoringTarget');
    }

    notify(reason, message) {
        try {
            this.publish(`exit.${reason}`, {reason, message});
        } catch (e) {
            this.log.warn('Failed to publish:', e);
        }
    }
}

class ConnDesc {
    constructor(host, amqpCli) {
        this.hostname = host;
        this.isConnected = false;
        this.isStart = false;
        this.reconnectLocked = false;
        this.conn = amqpCli.connection;
        this.ch = amqpCli.channel;
        this.amqpCli = amqpCli;
    }

    onStart() {
        this.isStart = true;
        log.info(`${this.amqpCli.idx} connect to ${this.hostname} start`);
    };

    async onConnected(conn) {
        log.info(`${this.amqpCli.idx} connect to ${this.hostname} ok`);
        this.isConnected = true;
        this.conn = conn;
    };

    async onChannelOk(ch) {
        log.info(`${this.amqpCli.idx} onChannelOk ${this.hostname} ok`);
        this.ch = ch;
    };

    async onSetupOk() {
        log.info(`${this.amqpCli.idx} onSetupOk ${this.hostname} ok`);
        this.reconnectLocked = false;
    };

    tryReconnect() {
        if (this.reconnectLocked) {
            return false;
        }
        this.reconnectLocked = true;
        return true;
    };

    wait(t) {
        return new Promise((resolve) => setTimeout(resolve, t));
    };

    async waitToReconnect() {
        await this.wait(5000);
    };

    async onConnectError(event, error) {
        log.fatal(`${this.amqpCli.idx} onConnectError ${this.hostname}, event: ${event}, error: ${util.inspect(error)}`);
        this.isStart = false;
        this.isConnected = false;
        try {
            this.ch && await this.ch.close();
        } catch (e) {
            log.warn(`${this.amqpCli.idx}, close channel for host ${this.hostname} catch error: ${util.inspect(e)}`);
        }
        try {
            this.conn && await this.conn.close();
        } catch (e) {
            log.warn(`${this.amqpCli.idx}, close connection for host ${this.hostname} catch error: ${util.inspect(e)}`);
        }
        this.ch = null;
        this.conn = null;
        this.reconnectLocked = false;
    };
};

var RECONNECT = true;

class AmqpCli {
    constructor() {
        this.idx = ++wrapIdx;
        this.options = null;
        this.connection = null;
        this.channel = null;
        this.rpcClient = null;
        this.rpcServer = null;
        this.monitor = null;
        this.monitoringTarget = null;
        this.msgSender = null;
        this.topicParticipants = new Map();
        this.connected = false;
        this.successCb = [];
        this.failureCb = [];
        this._errorHandler = this._errorHandler.bind(this);
        this._initAfterConnect = this._initAfterConnect.bind(this);
        this.balanceHosts = null;
        this.connectStatus = {};
        this.curHost = null;
    }

    connect(options, onOk, onFailure) {
        this.options = options;
        this.options.hostname = options.host;

        let connSt = null;
        if (!this.balanceHosts) {
            this.balanceHosts = this.options.host.split(',');
            this.balanceHosts.forEach((host) => {
                this.connectStatus[host] = new ConnDesc(host, this);
            });
            connSt = this._choseConnDesc();
        } else {
            connSt = this.connectStatus[this.curHost];
        }

        if (connSt.isConnected) {
            log.info(`${this.idx} connection of ${connSt.hostname} already connected`)
            return onOk();
        }

        this.successCb.push(onOk);
        this.failureCb.push(onFailure);

        if (fs.existsSync(cipher.astore)) {
            cipher.unlock(cipher.k, cipher.astore, (err, authConfig) => {
                if (!err) {
                    if (authConfig.rabbit) {
                        this.options.username = authConfig.rabbit.username;
                        this.options.password = authConfig.rabbit.password;
                    }
                } else {
                    log.error('Failed to get rabbitmq auth:', util.inspect(err));
                }
                this._connect(connSt);
            });
        } else {
            this._connect(connSt);
        }
    };

    _choseConnDesc(eConnInfo = null) {
        let host, i, k = 0;
        if (eConnInfo) {
            k = this.balanceHosts.findIndex((t) => t === eConnInfo.hostname);
            k = k + 1;
        } else {
            k = 0;
        }
        host = this.balanceHosts[k % this.balanceHosts.length];
        this.curHost = host;
        return this.connectStatus[this.curHost];
    };

    async _initAfterConnect(connSt) {
        try {
            log.debug(`${this.idx} _initAfterConnect ${connSt.hostname}`);
            let conn = connSt.conn;
            this.channel = await conn.createChannel();
            this.channel.on('error', (e) => {
                log.error(`${this.idx} Channel closed:, ${util.inspect(e)}`);
                connSt.onConnectError('function._initAfterConnect', e);
            });
            connSt.onChannelOk(this.channel);

            await (this.rpcClient && this.rpcClient.setup());
            await (this.rpcServer && this.rpcServer.setup());
            await (this.monitor && this.monitor.setup());
            await (this.monitoringTarget && this.monitoringTarget.setup());

            const setups = [];
            this.topicParticipants.forEach((tp) => {
                setups.push(tp.setup());
            });
            await Promise.all(setups);

            this.successCb.forEach((cb) => {
                cb();
            });
            this.successCb = [];
            connSt.onSetupOk();
        } catch (e) {
            log.error('Error after connect:', util.inspect(e));
            connSt.onConnectError('function._initAfterConnect', e);
        }
    };

    async _connect(connInfo) {
        try {
            let host = connInfo.hostname;
            if (connInfo.isStart) {
                log.info(`${this.idx} _connect to ${connInfo.hostname} is starting`);
                return;
            }
            connInfo.onStart();
            this.options.hostname = host;
            log.info(`${JSON.stringify(this.options)}`);
            let conn = await amqp.connect(this.options);
            conn.on('error', (err) => {
                return this._errorHandler(err, 'error', connInfo)
            });
            conn.on('close', (err) => {
                return this._errorHandler(err, 'close', connInfo)
            });
            conn.on('blocked', (err) => {
                log.error(`${this.idx} host ${connInfo.hostname} connection block, reason: ${util.inspect(err)}`)
            });
            this.connection = conn;
            this.connected = true;      //note: need to fix
            await connInfo.onConnected(conn);
            await this._initAfterConnect(connInfo);
        } catch (e) {
            this._errorHandler(e, 'function._connect', connInfo);
        }
    }

    async _errorHandler(err, event, connInfo) {
        this.connected = false;
        let connSt = connInfo || this.connectStatus[this.curHost];

        if (RECONNECT) {
            await connSt.onConnectError(event, err);
            this.channel = null;
            this.connection = null;
            let connSn = this._choseConnDesc(connSt);
            if (connSn.tryReconnect()) {
                await connSn.waitToReconnect();
                this._connect(connSn);
            } else {
                log.warn(`${this.idx} Ignore event ${event}, ${connSn.hostname} already is connecting`)
            }
        } else {
            this.failureCb.forEach((cb) => {
                cb(err)
            });
            this.close().catch((e) => {
                log.error(`${this.idx} Error during closing:`, util.inspect(e));
            });
        }
    }

    async asRpcClient(onOk, onFailure) {
        let self = this;
        if (!self.rpcClient) {
            self.rpcClient = new RpcClient(self);
        }
        await self.rpcClient.setup().then(() => {
            onOk(self.rpcClient);
        }).catch((e) => {
            onFailure(e);
        });
    }

    async asMsgSender(onOk, onFailure) {
        let self = this;
        if (!self.msgSender) {
            self.msgSender = new MsgSender(self);
        }
        await self.msgSender.setup().then(() => {
            onOk(self.msgSender);
        }).catch((e) => {
            onFailure(e);
        });
    };

    async asRpcServer(id, methods, onOk, onFailure) {
        let self = this;
        if (!self.rpcServer) {
            self.rpcServer = new RpcServer(self, id, methods);
        }
        await self.rpcServer.setup().then(() => {
            onOk(self.rpcServer);
        }).catch((e) => {
            onFailure(e);
        });
    }

    removeRpcServer() {
        let self = this;
        if (self.rpcServer) {
            return self.rpcServer.close().then(() => {
                self.rpcServer = undefined;
                return 'ok';
            }).catch(() => {
                return 'ok';
            });
        }
        return Promise.resolve("ok");
    }

    async asMonitor(messageReceiver, onOk, onFailure) {
        let self = this;
        if (!self.monitor) {
            self.monitor = new Monitor(self, messageReceiver);
        }

        await self.monitor.setup().then(() => {
            onOk(self.monitor);
        }).catch((e) => {
            onFailure(e);
        });
    }

    async asMonitoringTarget(onOk, onFailure) {
        let self = this;
        if (!self.monitoringTarget) {
            self.monitoringTarget = new MonitoringTarget(self);
        }
        await self.monitoringTarget.setup().then(() => {
            onOk(self.monitoringTarget);
        }).catch((e) => {
            onFailure(e);
        });
    }

    async asTopicParticipant(group, onOk, onFailure) {
        let self = this;
        let topicParticipant = null;
        if (!self.topicParticipants.has(group)) {
            topicParticipant = new TopicParticipant(self, Object.assign({}, MONITOR_EXC, {name: group}));
        }

        await topicParticipant.setup().then(() => {
            self.topicParticipants.set(group, topicParticipant);
            onOk(topicParticipant);
        }).catch((e) => {
            log.error("TopicParticipant restart setup", util.inspect(e));
            onFailure(e)
        });
    }

    async disconnect() {
        try {
            RECONNECT = false;
            await this.close();
            await this.connection.close();
        } catch (err) {
            log.error('Error closing AMQP connection:', err && err.message ? err.message :util.inspect(err));
        }
    }

    close() {
        const closingOps = [];
        if (this.msgSender) {
            closingOps.push(this.msgSender.close());
            this.msgSender = null;
        }
        if (this.rpcClient) {
            closingOps.push(this.rpcClient.close());
            this.rpcClient = null;
        }
        if (this.rpcServer) {
            closingOps.push(this.rpcServer.close());
            this.rpcServer = null;
        }
        if (this.monitor) {
            closingOps.push(this.monitor.close());
            this.monitor = null;
        }
        if (this.monitoringTarget) {
            closingOps.push(this.monitoringTarget.close());
            this.monitoringTarget = null;
        }
        this.topicParticipants.forEach((tp) => {
            closingOps.push(tp.close());
        });
        this.topicParticipants.clear();
        return Promise.all(closingOps);
    }
}

module.exports = function () {
  const amqpCli = new AmqpCli();
  return amqpCli;
};