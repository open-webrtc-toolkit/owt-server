/* global exports, require */
'use strict';
var cloudHandler = require('../cloudHandler');
var superService = require('./../mdb/dataBase').superService;

function authorized (req, callback) {
  var authData = req.authData || {};

  if (authData.service === undefined) {
      callback(false);
      return;
  }

  var serviceId = authData.service._id + '';
  callback(serviceId === superService);
}

exports.getNodes = function (req, res) {
  authorized(req, function (ok) {
    if (ok) {
      return cloudHandler.getPortals(function (portals) {
        res.send(portals);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};

exports.getNode = function (req, res) {
  var node = req.params.node;
  authorized(req, function (ok) {
    if (ok) {
      return cloudHandler.getPortal(node, function (attr) {
        res.send(attr);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};

exports.getNodeConfig = function (req, res) {
  var node = req.params.node;
  authorized(req, function (ok) {
    if (ok) {
      return cloudHandler.getEcConfig(node, function (config) {
        res.send(config);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};

exports.getRooms = function (req, res) {
  authorized(req, function (ok) {
    if (ok) {
      return cloudHandler.getHostedRooms(function (rooms) {
        res.send(rooms);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};
