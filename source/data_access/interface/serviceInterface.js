/*global exports, require*/
'use strict';
var Service = require('./../model/serviceModel');

/*
 * Create Service.
 */
exports.create = function (config, callback) {
  var service = new Service(config);
  service.save(function (err, saved) {
    callback(err, saved);
  });
};

/*
 * List Service.
 */
exports.list = function (callback) {
  Service.find().populate('rooms').lean().exec(function (err, services) {
    callback(err, services);
  });
};

/*
 * Get Service.
 */
exports.get = function (serviceId, callback) {
  Service.findById(serviceId).populate('rooms').lean().exec(function (err, service) {
    callback(err, service);
  });
};

/*
 * Delete Service.
 */
exports.delete = function (serviceId, callback) {
  Service.remove({_id: serviceId}, function(err, ret) {
    if (ret.n === 0) serviceId = null;
    callback(err, serviceId);
  });
};
