// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var crypto = require('crypto');
var algorithm = 'aes-256-ctr';
var stream = require('stream');
var fs = require('fs');
var zlib = require('zlib');
var iv = Buffer.from('3a613563c433a361a62a9efe4dd8462e', 'hex');

function encrypt (password, text){
  var cipher = crypto.createCipheriv(algorithm, password, iv);
  var enc = cipher.update(text, 'utf8', 'hex');
  enc += cipher.final('hex');
  return enc;
}
 
function decrypt (password, text){
  var decipher = crypto.createDecipheriv(algorithm, password, iv);
  var dec = decipher.update(text, 'hex', 'utf8');
  dec += decipher.final('utf8');
  return dec;
}

function lock (password, object, filename, cb) {
  var s = new stream.Readable();
  s._read = function noop() {};
  s.push(JSON.stringify(object));
  s.push(null);
  var out = fs.createWriteStream(filename);
  out.on('error', function (e) {
    cb(e);
  });
  out.on('finish', function () {
    cb(null);
  });
  s.pipe(zlib.createGzip()).pipe(crypto.createCipheriv(algorithm, password, iv)).pipe(out);
}

function unlock (password, filename, cb) {
  var s = fs.createReadStream(filename);
  s.on('error', function (e) {
    cb(e);
  });
  var unzip = zlib.createGunzip();
  var buf = '';
  unzip.on('data', function (chunk) {
    buf += chunk.toString();
  });
  unzip.on('end', function () {
    cb(null, JSON.parse(buf));
  });
  unzip.on('error', function (e) {
    cb(e);
  });
  s.pipe(crypto.createDecipheriv(algorithm, password, iv)).pipe(unzip);
}

function lockSync (password, object, filename) {
  const buf = Buffer.from(JSON.stringify(object), 'utf8');
  const data = zlib.gzipSync(buf);
  const cipher = crypto.createCipheriv(algorithm, password, iv);
  let enc = cipher.update(data);
  let enc2 = cipher.final();
  enc = Buffer.concat([enc, enc2], enc.length + enc2.length) ;
  fs.writeFileSync(filename, enc);
}

function unlockSync (password, filename) {
  const buf = fs.readFileSync(filename);
  // const dec = decrypt(password, fs.readFileSync(filename, {encoding: 'hex'}));
  const decipher = crypto.createDecipheriv(algorithm, password, iv);
  let dec = decipher.update(buf);
  let dec2 = decipher.final();
  dec = Buffer.concat([dec, dec2], dec.length + dec2.length);
  const data = zlib.gunzipSync(dec);
  return JSON.parse(data.toString());
}

module.exports = {
  encrypt: encrypt,
  decrypt: decrypt,
  // Replace k with your key generator
  k: Buffer.from('3d84a1efc77268c98bc6ca2921eb35a6d82a40a38696d25fbb64ff1733cd2523', 'hex'),
  astore: '.owt.authstore',
  kstore: '.owt.keystore',
  lock: lock,
  unlock: unlock,
  lockSync: lockSync,
  unlockSync: unlockSync,
};
