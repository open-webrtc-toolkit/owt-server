// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('EventCascading');
var quicCas = require('./quicCascading/build/Release/quicCascading.node');
var server, client, quicStreams, cascadedClusters;

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
  var controllers = {};
  var server, client;

  // Key is participantId, value is token ID.
  const quicTransportIds = new Map();
  
  that.notify = function (participantId, event, data) {
    if (controllers[data.rpcId]) {
      for (var controller in controllers[data.rpcId]) {
        data.sessionId = controllers[data.rpcId].session;
        data.streamId = controllers[data.rpcId].stream;
        if (controller.server) {
          server.send(controllers[data.rpcId].session, controllers[data.rpcId].stream, JSON.stringify(data));
        } else {
          client.send(controllers[data.rpcId].session, controllers[data.rpcId].stream, JSON.stringify(data));
        }
      }
      return Promise.resolve('ok');
    } else {
      log.error("Quic connection abnormal");
      return Promise.reject('Quic connection abnormal');
    }
  }

  that.startEventCascading = function (data) {
    client = new quicCas.out(data.ip, data.port);
    client.addEventListener('newstream', function (info) {
      log.info("Get new stream:", info);
      return rpcReq.getController(cluster_name, data.selfroom)
          .then(function(controller) {
            log.debug('got controller:', controller);
            quicStreams[info.sessionId][info.streamId].controller = controller;
            var quicinfo = {session: info.sessionId, stream: info.streamId, server: false};
            controllers[controller].push(quicinfo);
            client.send(info.sessionId, info.streamId, data.room);
            return Promise.resolve('ok');
          })
    });
    
    client.addEventListener('roomevents', function (msg) {
      var event = JSON.parse(msg);
      log.debug('Get cascaded event:', event);

      return rpcReq.handleCascadingEvents(quicStreams[event.sessionId][event.streamId].controller, event);

    });
  }

  that.start = function () {
    server = new quicCas.in(port, cf, kf);

    server.addEventListener('newstream', function (msg) {
      var event = JSON.parse(msg);
      log.info("Server get new stream:", info);
      return rpcReq.getController(cluster_name, event.room)
          .then(function(controller) {
            log.debug('got controller:', controller);
            quicStreams[event.sessionId][event.streamId].controller = controller;
            var quicinfo = {session: event.sessionId, stream: event.streamId, server: true};
            controllers[controller].push(quicinfo);
            return Promise.resolve('ok');
          })
    });

    server.addEventListener('roomevents', function (msg) {
      var event = JSON.parse(msg);
      log.debug('Get cascaded event:', event);

      return rpcReq.handleCascadingEvents(quicStreams[event.sessionId][event.streamId].controller, event);

    });

    return Promise.resolve('ok');
  }

  that.stop = function () {
    log.info("stop event bridge");
  }


  return that;
};


module.exports = EventCascading;

