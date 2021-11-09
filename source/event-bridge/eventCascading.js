// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('EventCascading');
var quicCas = require('./quicCascading/build/Release/quicCascading.node');
var quicStreams, cascadedClusters;

var cf = 'leaf_cert.pem';
var kf = 'leaf_cert.pkcs8';

var EventCascading = function(spec, rpcReq) {
  var that = {},
    port = spec.port,
    cluster_name = spec.clusterName,
    self_rpc_id = spec.selfRpcId;

  /*
   * {conferenRpcId: {
   *     session: sessionId,
   *     stream: streamId
   * }}
   */
  var server_controllers = {};
  var client_controllers = {};
  var server, clients = {};

  // Key is participantId, value is token ID.
  const quicTransportIds = new Map();
  
  that.notify = function (participantId, event, data) {
    if (server_controllers[data.rpcId]) {
      for (var controller in server_controllers[data.rpcId]) {
        data.sessionId = server_controllers[data.rpcId].session;
        data.streamId = server_controllers[data.rpcId].stream;
        server.send(controllers[data.rpcId].session, controllers[data.rpcId].stream, JSON.stringify(data));

      }
      return Promise.resolve('ok');
    } else if(client_controllers[data.rpcId]) {
      for (var controller in client_controllers[data.rpcId]) {
        data.sessionId = client_controllers[data.rpcId].session;
        data.streamId = client_controllers[data.rpcId].stream;
        client_controllers[controller].client.send(client_controllers[data.rpcId].session, client_controllers[data.rpcId].stream, JSON.stringify(data));

      }
      return Promise.resolve('ok');
    } else {
      log.error("Quic connection abnormal");
      return Promise.reject('Quic connection abnormal');
    }
  }

  that.startEventCascading = function (data) {
    var clientID = data.evIP.replace(/\./g, '-') + '-' + data.evPort;
    log.info("startEventCascading with data:", data, " clientID:", clientID);
    if(!clients[clientID]) {
      return rpcReq.getController(cluster_name, data.room)
                .then(function(controller) {
      var client = new quicCas.out(data.evIP, data.evPort);
      client.addEventListener('newstream', function (msg) {
        var info = JSON.parse(msg);
        log.info("Get new stream:", info);
        client.controller = controller;
        var quicinfo = {session: info.sessionId, stream: info.streamId, client: client};
        if (!client_controllers[controller]) {
          client_controllers[controller] = [];
        }
        client_controllers[controller].push(quicinfo);
        client.send(info.sessionId, info.streamId, data.room);
        rpcReq.onCascadingConnected(controller,self_rpc_id);
      });
      
      client.addEventListener('roomevents', function (msg) {
        var event = JSON.parse(msg);
        log.debug('Get cascaded event:', event);

        rpcReq.handleCascadingEvents(client.controller, event);

      });
    })
    } else {
      log.debug('Cluster already cascaded');
      return Promise.resolve('ok');
    }
  }

  that.start = function () {
    server = new quicCas.in(port, cf, kf);

    server.addEventListener('newstream', function (msg) {
      var event = JSON.parse(msg);
      log.info("Server get new stream:", event);
      rpcReq.getController(cluster_name, event.room)
        .then(function(controller) {
          log.debug('got controller:', controller);
          quicStreams[event.sessionId][event.streamId].controller = controller;
          var quicinfo = {session: event.sessionId, stream: event.streamId};
          server_controllers[controller].push(quicinfo);
        })
    });

    server.addEventListener('roomevents', function (msg) {
      var event = JSON.parse(msg);
      log.debug('Get cascaded event:', event);

      rpcReq.handleCascadingEvents(quicStreams[event.sessionId][event.streamId].controller, event);

    });

    return Promise.resolve('ok');
  }

  that.stop = function () {
    log.info("stop event bridge");
  }


  return that;
};


module.exports = EventCascading;

