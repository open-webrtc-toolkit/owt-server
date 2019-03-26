#!/usr/bin/env node
// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

const path = require('path');
const fs = require('fs');

const cipher = require('./cipher');
const authStore = path.resolve(__dirname, cipher.astore);
const authBase = path.basename(path.dirname(authStore));
const { Writable } = require('stream');

const mutableStdout = new Writable({
  write(chunk, encoding, callback) {
    if (!this.muted) process.stdout.write(chunk, encoding);
    callback();
  },
});

const readline = require('readline').createInterface({
  input: process.stdin,
  output: mutableStdout,
  terminal: true,
});

readline.on('SIGINT', () => {
  readline.close();
});

const question = input => new Promise((resolve) => {
  readline.question(input, (answer) => {
    resolve(answer);
  });
});

const saveAuth = (obj, filename, cb) => {
  const lock = (obj) => {
    cipher.lock(cipher.k, obj, filename, (err) => {
      process.stdout.write(err || 'done!\n');
      cb(err);
    });
  };
  if (fs.existsSync(filename)) {
    cipher.unlock(cipher.k, filename, (err, res) => {
      if (!err) {
        res = Object.assign(res, obj);
        lock(res);
      } else {
        cb(err);
      }
    });
  } else {
    lock(obj);
  }
};

const updateRabbit = (cb) => {
  question('Update RabbitMQ account?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        cb();
        return;
      }
      question(`(${authBase}) Enter username of rabbitmq: `)
        .then((username) => {
          question(`(${authBase}) Enter password of rabbitmq: `)
            .then((password) => {
              mutableStdout.muted = false;
              saveAuth({ rabbit: { username, password } }, authStore, cb);
            });
          mutableStdout.muted = true;
        });
    });
};

const updateMongo = (cb) => {
  question('Update MongoDB account?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        cb();
        return;
      }
      question(`(${authBase}) Enter username of mongodb: `)
        .then((username) => {
          question(`(${authBase}) Enter password of mongodb: `)
            .then((password) => {
              mutableStdout.muted = false;
              saveAuth({ mongo: { username, password } }, authStore, cb);
            });
          mutableStdout.muted = true;
        });
    });
};

updateRabbit(() => {
  updateMongo(()=> readline.close())
});
