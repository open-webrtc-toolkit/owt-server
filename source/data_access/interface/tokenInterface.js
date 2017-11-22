/*global exports, require, Buffer*/
'use strict';
var tokenRegistry = require('./../mdb/tokenRegistry');
var dataBase = require('./../mdb/dataBase');

/*
 * Create a token.
 */
exports.create = function (token, callback) {
  tokenRegistry.addToken(token, function (id) {
    callback(id);
  });
};

/*
 * Delete a token.
 */
exports.delete = function (tokenId) {
  var db = dataBase.db;
  var expireDate = new Date(new Date().getTime() - 1000 * 60 * 3);

  return new Promise((resolve, reject) => {
    db.tokens.findOne({_id: db.ObjectId(tokenId)}, function (errFind, token) {
      db.tokens.remove({
        $or: [
          {_id: db.ObjectId(tokenId)},
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
  dataBase.saveKey();
};

/*
 * Get token key
 */
exports.key = function () {
  return dataBase.getKey();
};
