// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('EventCascading');
var quicCas = require('./quicCascading/build/Release/quicCascading.node');
const inspector = require('event-loop-inspector')();
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
   *     clientID/sessionId : { 
   *     stream: quicStreamObject
   * }}}
   */
  var controllers = {};
  var cascadedRooms = {};
  var client_controllers = {};
  var server, clients = {};
  var quicStreams = {};

  // Key is participantId, value is token ID.
  const quicTransportIds = new Map();

  /*setInterval(() => {
    console.log('IMOK');
  }, 1000);*/

  that.notify = function (participantId, msg, data) {
    if (controllers[msg.rpcId]) {
      if (msg.type === 'initialize') {
        log.info("Send initialize data to session:",msg.session, " and data:", data);
        controllers[msg.session].sendData(JSON.stringify(data));
      } else {
        for (var item in controllers[msg.rpcId]) {
          log.info("Notify msg:", msg, " with data:", data, " from controller:", controller, " to stream:", item);
          item.sendData(JSON.stringify(data));
        }
      }
      return Promise.resolve('ok');
    } else {
      log.error("Quic connection abnormal");
      return Promise.reject('Quic connection abnormal');
    }
  }

  const createQuicStream = (controller, clientID, data) => {
    var quicStream = clients[clientID].createBidirectionalStream();
    controllers[clientID] = quicStream;
    var info = {
      type: 'bridge',
      room: data.room
    }
    quicStream.sendData(info);
    quicStream.initialized = false;
    quicStream.onStreamData((msg) => {
      var event = JSON.parse(msg);
      if (!quicStream.initialized) {
        quicStream.initialized = true;
        rpcReq.onCascadingConnected(controller, self_rpc_id, clientID);
      }
      rpcReq.handleCascadingEvents(controller, event.data);
    })
  }

  that.startEventCascading = function (data) {
    var clientID = data.evIP.replace(/\./g, '-') + '-' + data.evPort;
    log.info("startEventCascading with data:", data, " clientID:", clientID);

    //A new conference request between uncascaded clusters
    if(!clients[clientID]) {
      return rpcReq.getController(cluster_name, data.room)
              .then(function(controller) {
        	      log.info("Client get controller:", controller);
                var client = new quicCas.QuicTransportClient(data.evIP, data.evPort);
                clients[clientID] = client;
                client.id = clientID;
                cascadedRooms[data.room] = true;
                client.onConnected(() => {
                  createQuicStream(controller, clientID, data);        
        });
      }
    } else {
      //A new different conference request between cascaded clusters
      if (!cascadedRooms[data.room]) {
        return rpcReq.getController(cluster_name, data.room)
                .then(function(controller) {
                  //Create a new quic stream for the new conference to cascading room events
                  createQuicStream(controller, clientID, data);
                });
      } else {
        log.debug('Cluster already cascaded');
        return Promise.resolve('ok');
      }
    }
  }

  that.start = function () {
    server = new quicCas.QuicTransportServer(port, cf, kf);

    server.onNewSession((session) => {
      var sessionId = session.idGetter();
      session.onNewStream((quicStream) => {
        controllers[sessionId] = quicStream;
        rpcReq.onCascadingConnected(controller,self_rpc_id, sessionId);
        quicStream.onStreamData((msg) => {
          var event = JSON.parse(msg);
          rpcReq.handleCascadingEvents(controller, event.data);
        });
      })
    });
  }

  that.stop = function () {
    log.info("stop event bridge");
  }


  return that;
};


module.exports = EventCascading;

