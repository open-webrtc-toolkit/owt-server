/* global exports, require */
'use strict';
var cloudHandler = require('../cloudHandler');
var superService = require('./../mdb/dataBase').superService;

function authorized (callback) {
  var service = require('./../auth/nuveAuthenticator').service;
  service._id = service._id + '';
  callback(service._id === superService);
}

exports.getNodes = function (req, res) {
  authorized(function (ok) {
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
  authorized(function (ok) {
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
  authorized(function (ok) {
    if (ok) {
      return cloudHandler.getEcConfig(node, function (config) {
        res.send(config);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};

exports.getRooms = function (req, res) {
  authorized(function (ok) {
    if (ok) {
      return cloudHandler.getHostedRooms(function (rooms) {
        res.send(rooms);
      });
    }
    return res.status(401).send('Service not authorized for this action');
  });
};
