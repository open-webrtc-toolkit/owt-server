// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const redis = require('redis')
const bluebird = require('bluebird')
const log = require('./logger').logger.getLogger('Redis');

// Using promises
bluebird.promisifyAll(redis)

module.exports.create = function (messageChannel, roomId, selfRpcId, onSubMessage) {
  var that = {},
      room_id, selfRpcId, callback, redisClient, pubClient, subClient;

  var storeConnectState = false;
  var pubConnectState = false;

  var retry_strategy = function(options) {
    if (options.error && options.error.code === "ECONNREFUSED") {
      // End reconnecting on a specific error and flush all commands with
      // a individual error
      log.error('The server refused the connection');
      return 1000;
    }
    if (options.total_retry_time > 1000 * 60 * 60) {
      // End reconnecting after a specific timeout and flush all commands
      // with a individual error
      log.error('Retry time exhausted');
      return 1000;
    }
    if (options.attempt > 10000) {
      // End reconnecting with built in error
      return 1000;
    }
    // reconnect after
    return Math.min(options.attempt * 1000, 3000);
  };

  if (messageChannel) {
    room_id = roomId;
    selfRpcId = selfRpcId;
    callback = onSubMessage;
    redisClient = redis.createClient({retry_strategy}),
    pubClient = redis.createClient({retry_strategy}),
    subClient = redis.createClient({retry_strategy});
  } else {
    redisClient = redis.createClient({retry_strategy});
  }

  var addItem = function (key, id, data) {
    if (!storeConnectState) {
      log.error('not add item to redis for redis disconnected');
      return undefined;
    } else {
      return redisClient.hsetAsync(key, id, JSON.stringify(data))
              .then(
                  () => log.debug("Item:", key, " id:", id, " added"),
                  err => log.error("addItem with error:", err)
              )
      }
  };

  var delItem = function (key, id) {
    if (!storeConnectState) {
      log.error('not delete item in redis for redis disconnected');
      return undefined;
    } else {
      return redisClient.hdelAsync(key, id)
                .then(
                    res => log.debug("Item:", key, " id:", id, " removed with:", res),
                    err => log.error("delItem with error:", err)
                )
    }
  };

  that.initRedisClient = function (messageChannel, roomId, selfRpcId, onSubMessage) {
    if (messageChannel) {
      room_id = roomId;
      selfRpcId = selfRpcId;
      callback = onSubMessage;
      redisClient = redis.createClient(),
      pubClient = redis.createClient(),
      subClient = redis.createClient();
    } else {
      redisClient = redis.createClient();
    }
  }

  that.delByKey = function (key) {
    log.info("delete key from redis:", key);
    if (!storeConnectState) {
        log.error('not delete key from redis for redis disconnected');
        return undefined;
      } else {
        return redisClient.delAsync(key)
          .then(
            res => log.info("Item:", key, " removed with:", res),
            err => log.error("delItem with error:", err)
          );
    }
  };

  that.getItem = function (key, id) {
    return new Promise((resolve, reject) => {
      if (!storeConnectState) {
        resolve(null);
      } else {
        redisClient.hgetAsync(key, id)
          .then(user => {
            resolve(JSON.parse(user));
          }, error => {
              reject(error);
          });
      }
    })
  };

  that.getItems = function (key) {
    return new Promise((resolve, reject) => {
      if (!storeConnectState) {
        reject('getItems failed for redis disconnected');
      } else {
        redisClient.hgetallAsync(key)
                .then(users => {
                  const userList = {}
                  for (let user in users) {
                    var item = JSON.parse(users[user]);
                    userList[user] = item;
                  }
                  resolve(userList);
                }, error => {
                    reject(error);
                });
      }
    });
  };

  that.subscribeChannel = function (channel) {
    subClient.subscribe(channel, function(e) {
      log.debug("Subscribed channel:", channel, " with:", e);
    });
  }

  var publishMsg = function (channel, type, module, id, data) {
    var message = {
      type: type,
      module: module,
      id: id,
      self: selfRpcId,
      data: data
    };
    pubConnectState && pubClient.publish(channel, JSON.stringify(message), function(e) {
      log.debug("Published message:", message, " to channel:", channel, " with e:", e);
    });

  }

  that.updateToRedis = function (type, module, name, id, data) {
    log.debug("update to redis type:", type, " module:", module, " name:", name, " id:", id, " data", data);
    var key = room_id + name;
    if (type === 'add') {
      addItem(key, id, data);
      publishMsg(key, type, module, id, data);
    } else {
      delItem(key, id);
      publishMsg(key, type, module, id);
    }
  }

  that.saveList = function (key, value) {
    return new Promise((resolve, reject) => {
      if (!storeConnectState) {
        reject('saveList failed for redis disconnected');
      } else {
        redisClient.lpush(key, value, function(err, data) {
          if (err) {
            log.error("lpush key:", key, "with value:", data, " error", err);
          }
          resolve(data);
        });
      }
    });
  }

  that.getList = (key) => {
    return new Promise((resolve, reject) => {
      if (!storeConnectState) {
        log.error("getList failed for redis disconnected");
        resolve([]);
      } else {
        redisClient.lrange(key, 0, -1, function(err, data) {
          if (err) {
            log.error("lpush key:", key, "with value:", data, " error", err);
          }
          resolve(data);
        });
      }
    });
  }

  that.deleteList = (key, value) => {
    return new Promise((resolve, reject) => {
      if (!storeConnectState) {
        reject('deleteList failed for redis disconnected');
      } else {
        redisClient.lrem(key, 0,  value, function(err, data) {
          if (err) {
            log.error("lrem key:", key, "with value:", data, " error", err);
          }
          resolve(data);
        });
      }
    });
  }

  that.cleanRoomdata = (roomid) => {
    var key = roomid + "*";
    if (!storeConnectState) {
      log.error('cleanRoomdata failed for redis disconnected');
    } else {
      redisClient.keys(key, function(err, rows) {
        log.info("rows are:", rows);
        if (rows.length > 0) {
          redisClient.del(rows);
        }
      });
    }
  }

  if (subClient) {
    subClient.on("message", function (channel, message) {
      callback(channel, message);
    });

    subClient.on("error", function (err) {
    log.error("redis sub client connection error:", err);
  });
  }

  if (pubClient) {
    pubClient.on("ready", function () {
      pubConnectState = true;
      log.info("redis pub client connection ready");
    });

    pubClient.on("end", function (channel, message) {
      pubConnectState = false;
      log.error("redis pub client connection to server ended");
    });

    pubClient.on("error", function (err) {
    log.error("redis pub client connection error:", err);
  });
  }

  redisClient.on("connect", function () {
    log.info("redis client connected");
  });

  redisClient.on("ready", function () {
    storeConnectState = true;
    log.info("redis client connection ready");
  });

  redisClient.on("reconnecting", function () {
     log.info("Try to reconnect to redis server");
  });

  redisClient.on("error", function (err) {
    log.error("redis client connection error:", err);
  });

  redisClient.on("end", function (channel, message) {
    storeConnectState = false;
    log.error("redis client connection to server ended");
  });

  return that;
};

