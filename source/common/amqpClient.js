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

class RpcClient {
  constructor(amqpCli) {
    this.bus = amqpCli;
    this.callMap = {};
    this.corrID = 0;
    this.replyQ = '';
    this.ready = false;
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
            log.warn('Late rpc reply:', message);
          }
        } catch (err) {
          log.error('Error processing response: ', err);
        }
      });
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
    for (const i in this.callMap) {
      clearTimeout(this.callMap[i].timer);
    }
    this.callMap = {};
    return this.bus.channel.cancel(this.consumerTag)
      .catch((err) => {
        log.error('Failed during close RpcClient:', this.replyQ);
      });
  }
}

class RpcServer {
  constructor(amqpCli, id, methods) {
    this.bus = amqpCli;
    this.requestQ = id;
    this.methods = methods;
    this.ready = false;
  }

  setup() {
    const channel = this.bus.channel;
    return channel.assertExchange(
        RPC_EXC.name, RPC_EXC.type, RPC_EXC.options).then(() => {
      return channel.assertQueue(this.requestQ, Q_OPTION);
    }).then((result) => {
      this.requestQ = result.queue;
      return channel.bindQueue(this.requestQ, RPC_EXC.name, this.requestQ);
    }).then(() => {
      return channel.consume(this.requestQ, (rawMessage) => {
        try {
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
          log.error('Error processing call: ', error);
          log.error('message:', rawMessage.content.toString());
        }
      });
    }).then((ok) => {
      log.debug('Setup rpc server ok:', this.requestQ);
      this.consumerTag = ok.consumerTag;
      this.ready = true;
    });
  }

  close() {
    this.ready = false;
    return this.bus.channel.cancel(this.consumerTag)
      .catch((err) => {
        log.error('Failed to during close RpcServer:', this.requestQ); 
      });
  }
}

class TopicParticipant {
  constructor(amqpCli, name) {
    this.bus = amqpCli;
    this.name = name;
    this.queue = '';
    this.onMessage = null;
    this.consumers = new Map();
    this.subscriptions = new Map(); // {patterns, cb}
    this.ready = false;
  }

  setup() {
    const channel = this.bus.channel;
    return channel.assertExchange(
        this.name, 'topic', MONITOR_EXC.options).then(() => {
      return channel.assertQueue('', {durable: false});
    }).then((result) => {
      this.queue = result.queue;
      this.ready = true;
      this.consumers.clear();
      this.subscriptions.forEach((sub, tag) => {
        this.subscribe(sub.patterns, sub.cb, () => {});
      });
    });
  }

  subscribe(patterns, onMessage, onOk) {
    log.debug('subscribe:', this.queue, patterns);
    const channel = this.bus.channel;
    if (this.queue && channel) {
      patterns.map((pattern) => {
        channel.bindQueue(this.queue, this.name, pattern)
          .then(() => log.debug('Follow topic [' + pattern + '] ok.'))
          .catch((err) => log.error('Failed to bind queue on topic'));
      });
      channel.consume(this.queue, (rawMessage) => {
        try {
          const msg = JSON.parse(rawMessage.content.toString());
          if (onMessage) {
            log.debug('Topic on message:', msg);
            onMessage(msg);
          }
        } catch (error) {
          log.error('Error processing topic message:', rawMessage, 'and error:', error);
        }
      }).then((ok) => {
        this.consumers.set(patterns.toString(), ok.consumerTag);
        this.subscriptions.set(ok.consumerTag, {patterns, cb: onMessage});
        onOk();
      });
    } else {
      this.ready = false;
    }
  }

  unsubscribe(patterns) {
    log.debug('unsubscribe:', patterns);
    const channel = this.bus.channel;
    if (this.queue && channel) {
      patterns.map((pattern) => {
        channel.unbindQueue(this.queue, this.name, pattern)
          .catch((err) => log.error('Failed to unbind queue on topic'));
        log.debug('Ignore topic [' + pattern + ']');
      });
      const consumerTag = this.consumers.get(patterns.toString());
      if (consumerTag) {
        this.consumers.delete(patterns.toString());
        this.subscriptions.delete(consumerTag);
        channel.cancel(consumerTag)
          .catch((err) => log.error('Failed to cancel:', consumerTag));
      }
    } else {
      this.ready = false;
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
    return this.bus.channel.deleteQueue(this.queue)
      .catch((err) => {
        log.error('Failed to destroy queue:', this.queue); 
      })
      .catch((err) => log.error('Failed to delete exchange:', this.name));
  }
}

class Monitor {
  constructor(amqpCli) {
    this.bus = amqpCli;
    this.queue = '';
    this.ready = false;
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
          log.error('Error processing monitored message:', msg, 'and error:', error);
        }
      });
    }).then((ok) => {
      this.consumerTag = ok.consumerTag;
      this.ready = true;
    });
  }

  setMsgReceiver(onMessage) {
    this.onMessage = onMessage;
  };

  close() {
    this.ready = false;
    return this.bus.channel.cancel(this.consumerTag)
      .catch((err) => {
        log.error('Failed to cancel consumer on queue:', this.queue); 
      })
      .catch((err) => log.error('Failed to delete exchange:', this.name));
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
    log.warn('Connecting error:', err);
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
      onOk(this.rpcServer);
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
      await this.connection.close();
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
