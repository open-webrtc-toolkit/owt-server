/* global require, module */
'use strict';

var crypto = require('crypto');
var algorithm = 'aes-256-ctr';
var stream = require('stream');
var fs = require('fs');
var zlib = require('zlib');

function encrypt (password, text){
  var cipher = crypto.createCipher(algorithm, password);
  var enc = cipher.update(text, 'utf8', 'hex');
  enc += cipher.final('hex');
  return enc;
}
 
function decrypt (password, text){
  var decipher = crypto.createDecipher(algorithm, password);
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
  s.pipe(zlib.createGzip()).pipe(crypto.createCipher(algorithm, password)).pipe(out);
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
  s.pipe(crypto.createDecipher(algorithm, password)).pipe(unzip);
}

module.exports = {
  encrypt: encrypt,
  decrypt: decrypt,
  k: crypto.pbkdf2Sync('woogeen', 'mcu', 4000, 128, 'sha256'),
  lock: lock,
  unlock: unlock
};
