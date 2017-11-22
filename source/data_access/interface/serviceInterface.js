/*global exports, require*/
'use strict';
var serviceRegistry = require('./../mdb/serviceRegistry');
var db = require('./../mdb/dataBase').db;

/*
 * Create Service.
 */
exports.create = function (service, callback) {
  serviceRegistry.addService(service, function (result) {
    callback(result);
  });
};

/*
 * List Service.
 */
exports.list = function (callback) {
  serviceRegistry.getList(function (list) {
        callback(list);
    });
};

/*
 * Get Service.
 */
exports.get = function (serviceId, callback) {
  serviceRegistry.getService(serviceId, function (service) {
    if (service) {
      callback(service);
    } else {
      callback(null);
    }
  });
};

/*
 * Delete Service.
 */
exports.delete = function (serviceId, callback) {
  serviceRegistry.removeService(serviceId);
  callback(serviceId);
};
