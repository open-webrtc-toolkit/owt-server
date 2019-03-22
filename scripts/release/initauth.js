#!/usr/bin/env node
// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

const path = require('path');

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

const saveAuth = (obj, filename) => {
  cipher.lock(cipher.k, obj, filename, (err) => {
    process.stdout.write(err || 'done!\n');
  });
};

question(`(${authBase}) Enter username of rabbitmq: `)
  .then((username) => {
    question(`(${authBase}) Enter password of rabbitmq: `)
      .then((password) => {
        readline.close();
        saveAuth({ username, password }, authStore);
      });
    mutableStdout.muted = true;
  });
