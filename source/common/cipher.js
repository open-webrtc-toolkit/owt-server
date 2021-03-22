// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const crypto = require('crypto');
const algorithm = 'aes-256-ctr';
const stream = require('stream');
const fs = require('fs');
const zlib = require('zlib');
const iv = Buffer.from('3a613563c433a361a62a9efe4dd8462e', 'hex');

function encrypt(password, text) {
  const cipher = crypto.createCipheriv(algorithm, password, iv);
  let enc = cipher.update(text, 'utf8', 'hex');
  enc += cipher.final('hex');
  return enc;
}

function decrypt(password, text) {
  const decipher = crypto.createDecipheriv(algorithm, password, iv);
  let dec = decipher.update(text, 'hex', 'utf8');
  dec += decipher.final('utf8');
  return dec;
}

function lock(password, object, filename, cb) {
  const s = new stream.Readable();
  s._read = function noop() {};
  s.push(JSON.stringify(object));
  s.push(null);
  const out = fs.createWriteStream(filename);
  out.on('error', function(e) {
    cb(e);
  });
  out.on('finish', function() {
    cb(null);
  });
  s.pipe(zlib.createGzip())
      .pipe(crypto.createCipheriv(algorithm, password, iv))
      .pipe(out);
}

function unlock(password, filename, cb) {
  const s = fs.createReadStream(filename);
  s.on('error', function(e) {
    cb(e);
  });
  const unzip = zlib.createGunzip();
  let buf = '';
  unzip.on('data', function(chunk) {
    buf += chunk.toString();
  });
  unzip.on('end', function() {
    cb(null, JSON.parse(buf));
  });
  unzip.on('error', function(e) {
    cb(e);
  });
  s.pipe(crypto.createDecipheriv(algorithm, password, iv)).pipe(unzip);
}

module.exports = {
  encrypt: encrypt,
  decrypt: decrypt,
  // Replace k with your key generator
  k: Buffer.from(
      '3d84a1efc77268c98bc6ca2921eb35a6d82a40a38696d25fbb64ff1733cd2523',
      'hex'),
  astore: '.owt.authstore',
  kstore: '.owt.keystore',
  lock: lock,
  unlock: unlock,
};
