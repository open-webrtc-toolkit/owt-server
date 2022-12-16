#!/usr/bin/env node

// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const fs = require('fs');
const MongoClient = require('mongodb').MongoClient;
const cipher = require('./cipher');

const connectOption = {
  useUnifiedTopology: true,
};

async function loadAuth() {
  return new Promise((resolve, reject) => {
    if (fs.existsSync(cipher.astore)) {
      cipher.unlock(cipher.k, cipher.astore, function cb (err, authConfig) {
        if (!err) {
          resolve(authConfig.mongo);
        } else {
          reject('Failed to get mongodb auth');
        }
      });
    }
    resolve();
  });
}

async function connect(dbURL, auth) {
  return new Promise((resolve, reject) => {
    if (auth && !dbURL.includes('@')) {
      dbURL = 'mongodb://' + auth.username
        + ':' + auth.password
        + '@' + dbURL.replace('mongodb://', '');
    } else if (dbURL.indexOf('mongodb://') !== 0) {
      dbURL = 'mongodb://' + dbURL;
    }
    MongoClient.connect(dbURL, connectOption, function(err, cli) {
      if (!err) {
        resolve(cli)
      } else {
        reject(err);
      }
    });
  });
}

// Default page size for readMany
const PAGE_SIZE = 100;

class StateStores {
  constructor(dbURL) {
    this.prepare = loadAuth().then((auth) => {
      return connect(dbURL, auth);
    }).then((client) => {
      this.client = client;
      this.db = client.db();
      return this.db;
    });
  }

  // CRUD operations
  async create(type, obj) {
    const db = await this.prepare;
    obj.id && (obj._id = obj.id);
    return new Promise((resolve, reject) => {
      db.collection(type).insertOne(obj, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result);
        }
      });
    });
  }
  async read(type, query) {
    const db = await this.prepare;
    query.id && (query._id = query.id);
    return new Promise((resolve, reject) => {
      db.collection(type).findOne(query, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result);
        }
      });
    });
  }
  // config = {start: number, pageSize: number}
  async readMany(type, query, config) {
    const db = await this.prepare;
    const count = await db.collection(type).countDocuments(query);
    const findConfig = {
      sort: {_id: 1},
      limit: PAGE_SIZE,
      skip: (config?.start || 0)
    };
    const cursor = db.collection(type).find(query, findConfig);
    const data = await cursor.toArray();
    return {
      total: count,
      start: findConfig.skip,
      data
    };
  }
  // update: {jsonPath: value}
  async update(type, filter, updates) {
    const db = await this.prepare;
    filter.id && (filter._id = filter.id);
    const mongoUpdate = {'$set': updates};
    for (const key in updates) {
      if (updates[key] === null) {
        mongoUpdate['$unset'] = mongoUpdate['$unset'] ?? {};
        mongoUpdate['$unset'][key] = '';
        delete updates[key];
      }
    }
    return new Promise((resolve, reject) => {
      db.collection(type).updateOne(filter, mongoUpdate, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result.modifiedCount > 0);
        }
      });
    });
  }
  async delete(type, filter) {
    const db = await this.prepare;
    filter.id && (filter._id = filter.id);
    return new Promise((resolve, reject) => {
      db.collection(type).deleteMany(filter, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result.deletedCount > 0);
        }
      });
    });
  }
  // Add value in set
  async setAdd(type, filter, updates) {
    const db = await this.prepare;
    filter.id && (filter._id = filter.id);
    const mongoUpdate = {'$addToSet': updates};
    return new Promise((resolve, reject) => {
      db.collection(type).updateOne(filter, mongoUpdate, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result.deletedCount > 0);
        }
      });
    });
  }
  // Remove value in set
  async setRemove(type, filter, updates) {
    const db = await this.prepare;
    filter.id && (filter._id = filter.id);
    const mongoUpdate = {'$pull': updates};
    return new Promise((resolve, reject) => {
      db.collection(type).updateOne(filter, mongoUpdate, (err, result) => {
        if (err) {
          reject(err);
        } else {
          resolve(result.deletedCount > 0);
        }
      });
    });
  }
  close() {
    if (this.client) {
      this.client.close();
    }
  }
}

class LocalState {
  static getKey(type, query) {
    if (!query.id) {
      throw new Error('Missing id');
    }
    return type + '.' + query.id;
  }

  constructor() {
    this.tsc = false;
    this.state = new Map();
  }
  async create(type, value) {
    const key = getKey(type, value);
    if (this.state.has(key)) {
      throw new Error('Duplicate key');
    }
    this.state.set(key, value);
    return value;
  }
  async read(type, query) {
    const key = getKey(type, query);
    return this.state.get(key);
  }
  async update(type, query, update) {
    const key = getKey(type, query);
    if (!this.state.has(key)) {
      return false;
    }
    const value = this.state.get(key);
    for (const prop in update) {
      value[prop] = update[prop];
    }
    this.state.set(key, value);
  }
  async delete(type, query) {
    const key = getKey(type, query);
    return this.state.delete(key);
  }
  async setAdd(type, filter, update) {
    const key = getKey(type, filter);
    const value = this.state.get(key);
    for (const prop in update) {
      if (Array.isArray(value[prop])) {
        if (value[prop].indexOf(update[prop]) < 0) {
          value[prop].push(update[prop]);
        }
      }
    }
    this.state.set(key, value);
  }
  async setRemove(type, filter, update) {
    const key = getKey(type, filter);
    const value = this.state.get(key);
    for (const prop in update) {
      if (Array.isArray(value[prop])) {
        const i = value[prop].indexOf(update[prop]);
        if (i >= 0) {
          value[prop].splice(i, 1);
        }
      }
    }
    this.state.set(key, value);
  }
  createTransaction() {
    throw new Error('Transaction not supported');
  }
}

exports.StateStores = StateStores;
exports.LocalState = LocalState;
