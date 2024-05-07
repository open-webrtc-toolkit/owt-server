// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var fs = require('fs');
var amqp = require('amqplib');
var log = require('./logger')
  .logger.getLogger('AmqpClient');
var cipher = require('./cipher');
var TIMEOUT = 2000;
var REMOVAL_TIMEOUT = 7 * 24 * 3600 * 1000;
var util = require('util');

const RPC_EXC = {
  name: 'owtRpc',
  type: 'direct',
  options: {
    autoDelete: true,
    durable: false,
  },
};

const MONITOR_EXC = {
  name: 'owtMonitoring',
  type: 'topic',
  options: {
    autoDelete: false,
    durable: false,
  },
};

const Q_OPTION = {
  durable: false,
  autoDelete: true,
};

var translateToRegx = function(regx) {
  if (!regx) {
    return;
  }

  let regxParts = regx.split('.');
  let regxStr = "";
  let len = regxParts.length;
  let jumpOut = false;
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
        jumpOut = true;
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

class RpcClient {
  constructor(amqpCli) {
    this.bus = amqpCli;
    this.callMap = {};
    this.corrID = 0;
    this.replyQ = '';
    this.ready = false;
    this.closed = false;
  }

  setup() {
    const channel = this.bus.channel;
    return channel.assertExchange(
        RPC_EXC.name, RPC_EXC.type, RPC_EXC.options).then(() => {
      return channel.assertQueue('', Q_OPTION);
    }).then((result) => {
      this.replyQ = result.queue;
      return channel.bindQueue(this.replyQ, RPC_EXC.name, this.replyQ);
    }).then(() => {
      if (this.closed) {
        return;
      }
      log.debug('Start consume', this.replyQ, RPC_EXC.name);
      return channel.consume(this.replyQ, (rawMessage) => {
        try {
          const msg = JSON.parse(rawMessage.content.toString());
          log.debug('New message received', msg);
          if (this.callMap[msg.corrID] !== undefined) {
            log.debug('Callback', msg.type, ' - ', msg.data);
            clearTimeout(this.callMap[msg.corrID].timer);
            this.callMap[msg.corrID].fn[msg.type].call({}, msg.data, msg.err);
            if (this.callMap[msg.corrID] !== undefined) {
              delete this.callMap[msg.corrID];
            }
          } else {
            log.warn('Late rpc reply:', msg);
          }
        } catch (err) {
          log.error('Error processing response: ', err);
        }
      }, {noAck: true});
    }).then((ok) => {
      this.consumerTag = ok.consumerTag;
      this.ready = true;
    });
  }

  remoteCall(to, method, args, callbacks, timeout) {
    log.debug('remoteCall, corrID:', this.corrID, 'to:', to, 'method:', method);
    const channel = this.bus.channel;
    if (this.ready && channel) {
      const corrID = this.corrID++;
      this.callMap[corrID] = {};
      this.callMap[corrID].fn = callbacks || {
        callback: function () {}
      };
      this.callMap[corrID].timer = setTimeout(() => {
        if (this.callMap[corrID]) {
          for (const i in this.callMap[corrID].fn) {
            if (typeof this.callMap[corrID].fn[i] === 'function') {
              this.callMap[corrID].fn[i]('timeout');
            }
          }
          delete this.callMap[corrID];
        }
      }, timeout || TIMEOUT);

      const content = JSON.stringify({
        method,
        args,
        corrID,
        replyTo: this.replyQ,
      });
      log.debug('Publish content:', content);
      try {
        channel.publish('', to, Buffer.from(content));
      } catch (e) {
        log.warn('Failed to publish:', e);
      }
    } else {
      this.ready = false;
      for (const i in callbacks) {
        if (typeof callbacks[i] === 'function') {
          callbacks[i]('error', 'rpc client is not ready');
        }
      }
    }
  }

  remoteCast(to, method, args) {
    const channel = this.bus.channel;
    if (this.ready && channel) {
      const content = JSON.stringify({
        method,
        args,
      });
      try {
        channel.publish(RPC_EXC.name, to, Buffer.from(content));
      } catch (e) {
        log.warn('Failed to publish:', e);
      }
    } else {
      this.ready = false;
    }
  }

  close() {
    log.debug('RpcClient close');
    this.closed = true;
    for (const i in this.callMap) {
      clearTimeout(this.callMap[i].timer);
    }
    this.callMap = {};
    if (this.consumerTag) {
      return this.bus.channel.cancel(this.consumerTag)
        .catch((err) => {
          log.error('Failed during close RpcClient:', this.replyQ);
        });
    }
  }
}

class RpcServer {
  constructor(amqpCli, id, methods) {
    this.bus = amqpCli;
    this.requestQ = id;
    this.methods = methods;
    this.ready = false;
    this.closed = false;
    this.consumerTag = undefined;
  }

  setup() {
    const tagTail = Math.floor(Math.random() * 1000000000)
    this.consumerTag = `${this.requestQ}.${tagTail}`

    const channel = this.bus.channel;
    return channel.assertExchange(
        RPC_EXC.name, RPC_EXC.type, RPC_EXC.options).then(() => {
      return channel.assertQueue(this.requestQ, Q_OPTION);
    }).then((result) => {
      this.requestQ = result.queue;
      return channel.bindQueue(this.requestQ, RPC_EXC.name, this.requestQ);
    }).then(() => {
      if (this.closed) {
        return;
      }
      return channel.consume(this.requestQ, (rawMessage) => {
        try {
          if (!rawMessage || !rawMessage.hasOwnProperty("content")) {
            log.error(`queue:${this.requestQ} error message received:`, rawMessage);
            return
          }
          const msg = JSON.parse(rawMessage.content.toString());
          log.debug('New message received', msg);
          msg.args = msg.args || [];
          if (msg.replyTo && msg.corrID !== undefined) {
            msg.args.push((type, result, err) => {
              const content = JSON.stringify({
                data: result,
                corrID: msg.corrID,
                type: type,
                err: err,
              });
              channel.publish(RPC_EXC.name, msg.replyTo, Buffer.from(content));
            });
          }
          if (typeof this.methods[msg.method] === 'function') {
            this.methods[msg.method].apply(this.methods, msg.args);
          } else {
            log.warn('RPC server does not support this method:', msg.method);
            if (msg.replyTo && msg.corrID !== undefined) {
              const content = JSON.stringify({
                data: 'error',
                corrID: msg.corrID,
                type: 'callback',
                err: 'Not support method',
              });
              channel.publish(RPC_EXC.name, msg.replyTo, Buffer.from(content));
            }
          }
        } catch (error) {
          this.log.error('Error processing call: ', util.inspect(error));
        }
      }, {noAck: true, consumerTag: this.consumerTag});
    }).then((ok) => {
      if (ok) {
        log.info('setup ok', "queue:", this.requestQ, "consumerTag:", ok.consumerTag);
        this.consumerTag = ok.consumerTag;
        this.ready = true;
      } else {
        log.error('setup failed', "queue:", this.requestQ, "consumerTag:", this.consumerTag);
      }
    });
  }

    close() {
        this.ready = false;
        this.closed = true;
        let closeSteps = [];
        if (this.consumerTag) {
            this.bus.channel.cancel(this.consumerTag)
                .then((ret) => {
                    log.info(`cancel ${this.consumerTag} success`, ret);
                })
                .catch((err) => {
                    log.error(`cancel ${this.consumerTag} failed`, util.inspect(err));
                })
        }
        return Promise.resolve();
    }
}

class TopicParticipant {
  constructor(amqpCli, name) {
    this.bus = amqpCli;
    this.name = name;
    this.queue = '';
    this.onMessage = null;
    this.subscriptions = new Map(); // {patterns, cb}
    this.ready = false;
  }

  setup() {
    const channel = this.bus.channel;
    return channel.assertExchange(
        this.name, 'topic', MONITOR_EXC.options).then(() => {
      return channel.assertQueue('', {exclusive: true, durable: false});
    }).then((result) => {
      log.debug('TopicQueue:', result.queue);
      this.queue = result.queue;
      this.ready = true;
      this.subscriptions.forEach((sub, tag) => {
        this.subscribe(sub.patterns, sub.cb, () => {});
      });
    });
  }

    async subscribe(patterns, onMessage, onOk) {
        let self = this;
        log.debug('subscribe:', this.queue, patterns);
        const channel = self.bus.channel;
        patterns = patterns.concat();
        //fix consumerTag repeat
        const tagTail = Math.floor(Math.random() * 1000000000);
        const consumerTag = `${patterns.toString()}.${tagTail}`;
        if (self.queue && channel) {
            let bindQueuePromise = patterns.map((pattern) => {
                return channel.bindQueue(self.queue, self.name, pattern)
                    .then(() => log.info(`exchange:${self.name},queue:${self.queue} Follow [${pattern}] ok.`))
                    .catch((err) => {
                        //queue might be delete
                        self.subscriptions.set(consumerTag, {patterns, cb: onMessage, onOk: onOk});
                        self.ready = false;
                        throw new Error(`Failed to bind queue:${self.queue} on exchange:${self.name} for pattern:${pattern}`);
                    });
            });
            Promise.all(bindQueuePromise).then(() => {
                channel.consume(self.queue, (rawMessage) => {
                    if (!rawMessage || !rawMessage.hasOwnProperty("content")) {
                        //queue might be delete
                        self.ready = false;
                        throw new Error(`subscribe failed patterns:${patterns}, exchange:${self.name}, queue:${self.queue} might be delete`);
                    }
                    try {
                        const msg = JSON.parse(rawMessage.content.toString());
                        const routingKey = rawMessage.fields.routingKey;
                        self.subscriptions.forEach((sub, tag) => {
                            let callback = undefined;
                            sub.patterns.forEach((pattern, index) => {
                                if (!callback) {
                                    let regx = translateToRegx(pattern);
                                    if (regx && regx.test(routingKey)) {
                                        callback = sub.cb;
                                    }
                                }
                            });

                            if (callback) {
                                log.debug(`exchange:${self.name},queue:${self.queue} on message:`, msg, rawMessage.fields.routingKey);
                                callback(msg, rawMessage.fields.routingKey);
                            }
                        });
                    } catch (error) {
                        log.error(`Error processing exchange:${self.name},queue:${self.queue} message:`, rawMessage, 'and error:', util.inspect(error));
                    }
                }, {noAck: true, consumerTag: consumerTag}).then((ok) => {
                    if (ok) {
                        log.info('subscribe ok', "exchange", self.name, "queue:", self.queue, 'patterns:', patterns, "consumerTag:", ok.consumerTag);
                        self.subscriptions.set(ok.consumerTag, {patterns, cb: onMessage});
                    } else {
                        log.error('subscribe failed', "exchange", self.name, "queue:", self.queue, 'patterns:', patterns, "consumerTag:", consumerTag);
                    }
                    onOk ? onOk() : null;
                });
            });
        } else {
            self.ready = false;
        }
    }

    async unsubscribe(patterns) {
        let self = this;
        patterns = patterns.concat();
        log.info('unsubscribe:', patterns);
        const channel = self.bus.channel;
        if (self.queue && channel) {
            let cancleTags = new Set();
            let ubindArr = []
            for (let index in patterns) {
                let pattern = patterns[index];
                let regx = translateToRegx(pattern);
                log.debug("translateToRegx:", pattern, "=>", regx);
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
                                    await channel.unbindQueue(self.queue, self.name, oldPattern);
                                    log.info(`ubindQueue ${oldPattern} on queue:${self.queue} pattern:${oldPattern}`);
                                    resolve();
                                } catch (e) {
                                    log.error(`Failed to ubindQueue ${oldPattern} on queue:${self.queue}`, util.inspect(e));
                                    resolve();
                                }
                            }));
                        }
                    });

                    log.debug("pattern:", pattern, "hits:", hits, "hits.length:", i, "sub.patterns.length:", sub.patterns.length);
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
                    log.debug('del consumerTag:', consumerTag);
                    ubindArr.push(new Promise((resolve) => {
                        channel.cancel(consumerTag).then((ret) => {
                            log.info('cancel consumerTag:', consumerTag, "real.consumerTag:", consumerTag, "ret:", ret);
                            resolve();
                        }).catch((err) => {
                            log.error('Failed to cancel:', consumerTag);
                            resolve();
                        });
                    }));
                });
            }
            if (ubindArr.length > 0) {
                await Promise.all(ubindArr);
            }
        }else{
            self.ready = false;
        }
    }

  publish(topic, data) {
    log.debug('Topic send:', topic, data, this.ready);
    const channel = this.bus.channel;
    if (this.ready && channel) {
      const content = JSON.stringify(data);
      log.debug('Publish:', this.name, topic);
      try {
        channel.publish(this.name, topic, Buffer.from(content));
      } catch (e) {
        log.warn('Failed to publish:', e);
      }
    } else {
      log.debug('TopicParticipant is not ready');
      this.ready = false;
    }
  }

  close() {
    this.ready = false;
    if (this.queue) {
      return this.bus.channel.deleteQueue(this.queue)
        .catch((err) => {
          log.error('Failed to destroy queue:', this.queue);
        });
    }
  }
}

class Monitor {
  constructor(amqpCli, onMessage) {
    this.bus = amqpCli;
    this.queue = '';
    this.ready = false;
    this.onMessage = onMessage;
  }

  setup() {
    const channel = this.bus.channel;
    return channel.assertExchange(
        MONITOR_EXC.name, MONITOR_EXC.type, MONITOR_EXC.options).then(() => {
      return channel.assertQueue('', Q_OPTION);
    }).then((result) => {
      this.queue = result.queue;
      return channel.bindQueue(this.queue, MONITOR_EXC.name, 'exit.#');
    }).then(() => {
      return channel.consume(this.queue, (rawMessage) => {
        try {
          const msg = JSON.parse(rawMessage.content.toString());
          log.debug('Received monitoring message:', msg);
          this.onMessage && this.onMessage(msg);
        } catch (error) {
          log.error('Error processing monitored message:', rawMessage, 'and error:', error);
        }
      }, {noAck: true});
    }).then((ok) => {
      this.consumerTag = ok.consumerTag;
      this.ready = true;
    });
  }

  close() {
    this.ready = false;
    if (this.consumerTag) {
      return this.bus.channel.cancel(this.consumerTag)
        .catch((err) => {
          log.error('Failed to cancel consumer on queue:', this.queue);
        });
    }
  };
}

class MonitoringTarget {
  constructor(amqpCli) {
    this.bus = amqpCli;
    this.ready = false;
  }

  setup() {
    return this.bus.channel.assertExchange(
        MONITOR_EXC.name, MONITOR_EXC.type, MONITOR_EXC.options)
      .then(() => {
        this.ready = true;
      });
  }

  notify(reason, message) {
    const channel = this.bus.channel;
    if (this.ready && channel) {
      const pattern = 'exit.' + reason;
      const content = JSON.stringify({reason, message});
      try {
        channel.publish(
            MONITOR_EXC.name, pattern, Buffer.from(content));
      } catch (e) {
        log.warn('Failed to publish:', e);
      }
    } else {
      this.ready = false;
    }
  }

  close() {
    this.ready = false;
    // return this.channel.deleteExchange(MONITOR_EXC.name, {ifUnused: true})
    //   .catch((err) => log.error('Failed to delete exchange:', this.name));
  }
}

const RECONNECT = true;
const RECONNECT_INTERVAL = 1000;

class AmqpCli {
  constructor() {
    this.options = null;
    this.connection = null;
    this.channel = null;
    this.rpcClient = null;
    this.rpcServer = null;
    this.monitor = null;
    this.monitoringTarget = null;
    this.topicParticipants = new Map();
    this.connected = false;
    this.successCb = () => {};
    this.failureCb = () => {};
    this.reconnectAttempts = 0;
    this._errorHandler = this._errorHandler.bind(this);
  }

  connect(options, onOk, onFailure) {
    this.options = options;
    this.options.hostname = options.host;
    this.successCb = onOk;
    this.failureCb = onFailure;
    if (fs.existsSync(cipher.astore)) {
      cipher.unlock(cipher.k, cipher.astore, (err, authConfig) => {
        if (!err) {
          if (authConfig.rabbit) {
            this.options.username = authConfig.rabbit.username;
            this.options.password = authConfig.rabbit.password;
          }
        } else {
          log.warn('Failed to get rabbitmq auth:', err);
        }
        this._connect();
      });
    } else {
      this._connect();
    }
  }

  _connect() {
    amqp.connect(this.options).then((conn) => {
      log.debug('Connect OK');
      conn.on('error', this._errorHandler);
      conn.on('close',this._errorHandler);
      this.connection = conn;
      return conn.createChannel();
    })
    .then((ch) => {
      this.channel = ch;
      this.channel.on('error', (e) => {
        log.warn('Channel closed:', e);
        this.channel = null;
      });
      if (!this.connected) {
        this.connected = true;
        this.successCb();
      } else {
        Promise.resolve().then(() => {
          return (this.rpcServer && this.rpcServer.setup());
        }).then(() => {
          return (this.rpcClient && this.rpcClient.setup());
        }).then(() => {
          return (this.monitor && this.monitor.setup());
        }).then(() => {
          return (this.monitoringTarget && this.monitoringTarget.setup());
        }).then(() => {
          const setups = [];
          this.topicParticipants.forEach((tp) => {
            setups.push(tp.setup());
          });
          return Promise.all(setups);
        }).catch((e) => {
          log.warn('Error after reconnect:', e);
        })
      }
    })
    .catch(this._errorHandler);
  }

  _errorHandler(err) {
    log.warn('Connecting error:', util.inspect(err));
    this.channel = null;
    if (RECONNECT) {
      setTimeout(() => {
        this._connect();
      }, RECONNECT_INTERVAL);
    } else {
      this.failureCb();
      this.close().catch((e) => {
        log.warn('Error during closing:', e);
      });
    }
  }

  asRpcClient(onOk, onFailure) {
    if (!this.rpcClient) {
      this.rpcClient = new RpcClient(this);
      this.rpcClient.setup()
        .then(() => onOk(this.rpcClient))
        .catch(onFailure);
    } else {
      log.warn('RpcClient already setup');
      onOk(this.rpcClient);
    }
  }

  asRpcServer(id, methods, onOk, onFailure) {
    if (!this.rpcServer) {
      this.rpcServer = new RpcServer(this, id, methods);
      this.rpcServer.setup()
        .then(() => onOk(this.rpcServer))
        .catch(onFailure);
    } else {
      log.warn('RpcServer already setup');
      onOk(this.rpcServer);
    }
  }

    removeRpcServer(){
        if (this.rpcServer) {
            let rpcServer = this.rpcServer;
            this.rpcServer = undefined;
            return rpcServer.close().then(()=>{
                return 'ok';
            }).catch(()=>{
                return 'ok';
            });
        }
        return Promise.resolve("ok");
    }

  asMonitor(messageReceiver, onOk, onFailure) {
    if (!this.monitor) {
      this.monitor = new Monitor(this, messageReceiver);
      this.monitor.setup()
        .then(() => onOk(this.monitor))
        .catch(onFailure);
    } else {
      log.warn('Monitor already setup');
      onOk(this.rpcServer);
    }
  }

  asMonitoringTarget(onOk, onFailure) {
    if (!this.monitoringTarget) {
      this.monitoringTarget =
        new MonitoringTarget(this);
      this.monitoringTarget.setup()
        .then(() => onOk(this.monitoringTarget))
        .catch(onFailure);
    } else {
      log.warn('MonitoringTarget already setup');
      onOk(this.rpcServer);
    }
  }

  asTopicParticipant(group, onOk, onFailure) {
    if (!this.topicParticipants.has(group)) {
      const topicParticipant =
        new TopicParticipant(this, group);
      topicParticipant.setup()
        .then(() => {
          this.topicParticipants.set(group, topicParticipant);
          onOk(topicParticipant);
        })
        .catch(onFailure);
    }
     else {
      log.warn('TopicParticipant already setup for:', group);
      onOk(this.topicParticipants.get(group));
    }
  }

  async disconnect() {
    try {
      await this.close();
      if (this.connection) {
        await this.connection.close();
        this.connection = null;
      }
    } catch (err) {
      log.warn('Error closing AMQP connection:', err);
    }
  }

  close() {
    const closingOps = [];
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
