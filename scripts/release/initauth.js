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

const question = (input) => new Promise((resolve) => {
  readline.question(input, (answer) => {
    resolve(answer);
  });
});

const saveAuth = (obj, filename) => new Promise((resolve, reject) => {
  const lock = (obj) => {
    cipher.lock(cipher.k, obj, filename, (err) => {
      if (!err) {
        process.stdout.write(err || 'done!\n');
        resolve();
      } else {
        reject(err);
      }
    });
  };
  if (fs.existsSync(filename)) {
    cipher.unlock(cipher.k, filename, (err, res) => {
      if (!err) {
        res = Object.assign(res, obj);
        lock(res);
      } else {
        reject(err);
      }
    });
  } else {
    lock(obj);
  }
});

const updateRabbit = (cleanup) => new Promise((resolve, reject) => {
  if (cleanup) {
    console.log('Cleanup RabbitMQ account...');
    saveAuth({rabbit: null}, authStore)
      .then(resolve)
      .catch(reject);
    return;
  }
  question('Update RabbitMQ account?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        resolve();
        return;
      }
      question(`(${authBase}) Enter username of rabbitmq: `)
        .then((username) => {
          question(`(${authBase}) Enter password of rabbitmq: `)
            .then((password) => {
              mutableStdout.muted = false;
              saveAuth({ rabbit: { username, password } }, authStore)
                .then(resolve)
                .catch(reject);
            });
          mutableStdout.muted = true;
        });
    });
});

const updateMongo = (cleanup) => new Promise((resolve, reject) => {
  if (cleanup) {
    console.log('Cleanup MongoDB account...');
    saveAuth({mongo: null}, authStore)
      .then(resolve)
      .catch(reject);
    return;
  }
  question('Update MongoDB account?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        resolve();
        return;
      }
      question(`(${authBase}) Enter username of mongodb: `)
        .then((username) => {
          question(`(${authBase}) Enter password of mongodb: `)
            .then((password) => {
              mutableStdout.muted = false;
              saveAuth({ mongo: { username, password } }, authStore)
                .then(resolve)
                .catch(reject);
            });
          mutableStdout.muted = true;
        });
    });
});

const updateInternal = (cleanup) => new Promise((resolve, reject) => {
  if (cleanup) {
    console.log('Cleanup internal passphrase...');
    saveAuth({internalPass: null}, authStore)
      .then(resolve)
      .catch(reject);
    return;
  }
  question('Update internal passphrase?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        resolve();
        return;
      }
      question(`(${authBase}) Enter internal passphrase: `)
        .then((passphrase) => {
          mutableStdout.muted = false;
          saveAuth({ internalPass: passphrase }, authStore)
            .then(resolve)
            .catch(reject);
        });
      mutableStdout.muted = true;
    });
});

const updateGKeyPass = (cleanup) => new Promise((resolve, reject) => {
  if (cleanup) {
    console.log('Cleanup gRPC TLS key...');
    saveAuth({grpc: null}, authStore)
      .then(resolve)
      .catch(reject);
    return;
  }
  question('Update passphrase for gRPC TLS key?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer !== 'y' && answer !== 'yes') {
        resolve();
        return;
      }
      question(`(${authBase}) Enter passphrase of server key: `)
        .then((serverPass) => {
          mutableStdout.muted = false;
          question(`(${authBase}) Enter passphrase of client key: `)
            .then((clientPass) => {
              mutableStdout.muted = false;
              saveAuth({ grpc: { serverPass, clientPass } }, authStore)
                .then(resolve)
                .catch(reject);
            });
          mutableStdout.muted = true;
        });
      mutableStdout.muted = true;
    });
});

const generateServiceProtectionKey = (cleanup) => new Promise((resolve, reject) => {
  if (cleanup) {
    console.log('Cleanup service protection key...');
    saveAuth({spk: null}, authStore)
      .then(resolve)
      .catch(reject);
    return;
  }
  question('Generate service protection key?[yes/no]')
    .then((answer) => {
      answer = answer.toLowerCase();
      if (answer === 'y' || answer === 'yes') {
        const spk = require('crypto').randomBytes(64).toString('hex');
        saveAuth({spk: spk}, authStore)
          .then(() => {
            console.log(`Service protection key generated: ${spk}`);
            resolve();
          })
          .catch(reject);
      } else {
        resolve();
      }
    });
});

const printUsage = () => {
  let usage = 'Usage:\n';
  usage += '  --rabbitmq  Update RabbitMQ account(default)\n';
  usage += '  --mongodb   Update MongoDB account(default)\n';
  usage += '  --internal  Update internal TLS key passphrase\n';
  usage += '  --grpc      Update gRPC TLS key passphrase\n';
  usage += '  --spk       Generate service protection key\n';
  usage += '  --cleanup   Clean up selected credentials\n';
  console.log(usage);
}
const options = {};
const parseArgs = () => {
  if (process.argv.includes('--help')) {
    printUsage();
    process.exit(0);
  }
  if (process.argv.includes('--rabbitmq')) {
    options.rabbit = true;
  }
  if (process.argv.includes('--mongodb')) {
    options.mongo = true;
  }
  if (process.argv.includes('--internal')) {
    options.internal = true;
  }
  if (process.argv.includes('--grpc')) {
    options.grpc = true;
  }
  if (process.argv.includes('--spk')) {
    options.spk = true;
  }
  if (process.argv.includes('--cleanup')) {
    options.cleanup = true;
  }
  let selectedUpdate = Object.keys(options).length;
  if (options.cleanup) {
    selectedUpdate -= 1;
  }
  if (selectedUpdate === 0) {
    options.rabbit = true;
    options.mongo = true;
  }
  return Promise.resolve();
}

parseArgs()
  .then(() => {
    if (options.rabbit) {
      return updateRabbit(options.cleanup);
    }
  })
  .then(() => {
    if (options.mongo) {
      return updateMongo(options.cleanup);
    }
  })
  .then(() => {
    if (options.internal) {
      return updateInternal(options.cleanup);
    }
  })
  .then(() => {
    if (options.grpc) {
      return updateGKeyPass(options.cleanup);
    }
  })
  .then(() => {
    if (options.spk) {
      return generateServiceProtectionKey(options.cleanup);
    }
  })
  .finally(() => readline.close());
