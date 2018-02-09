/*global exports, require, Buffer*/
'use strict';
var mongoose = require('mongoose');
var Token = require('./../model/tokenModel');
var Key = require('./../model/keyModel');

/*
 * Create a token.
 */
exports.create = function (token, callback) {
  Token.create(token, function (err, result) {
    if (err) {
      console.error(err);
      return callback(null);
    }
    callback(result._id);
  });
};

/*
 * Delete a token.
 */
exports.delete = function (tokenId) {
  var expireDate = new Date(new Date().getTime() - 1000 * 60 * 3);

  return new Promise((resolve, reject) => {
    Token.findById(tokenId, function (errFind, token) {
      Token.remove({
        $or: [
          {_id: tokenId},
          {creationDate: {$lt: expireDate}}
        ]},
        function (errRemove, remove) {
          if (errFind || !token) {
            console.log('err:', errFind || 'WrongToken');
            reject(errFind);
          } else {
            if (token.creationDate < expireDate) {
                reject({message:'Expired'});
            } else {
                resolve(token);
            }
          }
        });
    });
  });
};

/*
 * Generate token key
 */
exports.genKey = function (callback) {
  var key = require('crypto').randomBytes(64).toString('hex');
  var newOne = new Key({ key: key });
  Key.findOneAndUpdate({ _id: 0 }, newOne, { upsert: true }, function (err, saved) {
    if (err) {
      console.log('Save nuveKey error:', err);
    }
  });
};

/*
 * Get token key
 */
exports.key = function () {
  return new Promise((resolve, reject) => {
    Key.findById(0, function (err, result) {
      if (err || !result) {
        reject(err);
      } else {
        resolve(result.key);
      }
    });
  });
};
