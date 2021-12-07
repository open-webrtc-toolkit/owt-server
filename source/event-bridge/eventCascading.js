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

  /*setInterval(() => {
    console.log('IMOK');
  }, 1000);*/

  that.notify = function (participantId, msg, data) {
    if (controllers[msg.rpcId]) {
      if (msg.type === 'initialize') {
        log.info("Send initialize data to session:",msg.session, " and data:", data);
        if (quicStreams[msg.session][msg.stream]) {
          quicStreams[msg.session][msg.stream].send(JSON.stringify(data));
        } else {
          log.error("Quic stream abnormal");
        }
        
      } else {
        for (var item in controllers[msg.rpcId]) {
          log.info("Notify msg:", msg, " with data:", data, " from controller:", controller, " to stream:", item);
          item.send(JSON.stringify(data));
        }
      }
      return Promise.resolve('ok');
    } else {
      log.error("Quic connection abnormal");
      return Promise.reject('Quic connection abnormal');
    }
  }

  const createQuicStream = (controller, clientID, data) => {
    log.info("Create quic stream with controller:", controller, " clientID:", clientID, " and data:",data);
    var quicStream = clients[clientID].createBidirectionalStream();
    var streamID = quicStream.getId();
    if (!quicStreams[clientID]) {
      quicStreams[clientID] = {};
    }
    quicStreams[clientID][streamID] = {stream:quicStream, controller:controller};

    if (!controllers[controller]) {
      controllers[controller] = [];
    }
    controllers[controller].push(quicStream);
    var info = {
      type: 'bridge',
      room: data.room
    }

    var info = {
      type: 'ready'
    }
    quicStream.send(JSON.stringify(info));
    
    quicStream.onStreamData((msg) => {
      log.info("quic client stream get data:", msg);
      var event = JSON.parse(msg);
      if (event.type === 'ready') {
        var info = {
          type: 'bridge',
          room: data.room
        }
        quicStream.send(JSON.stringify(info));
        rpcReq.onCascadingConnected(controller, self_rpc_id, clientID, streamID);
      } else {
        rpcReq.handleCascadingEvents(controller, event.data);
      }
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
		      client.connect();
          clients[clientID] = client;
          client.id = clientID;
          cascadedRooms[data.room] = true;
          client.onConnection(() => {
		        log.info("Quic client connected");
            createQuicStream(controller.id, clientID, data);        
          });
        });
    } else {
      //A new different conference request between cascaded clusters
      if (!cascadedRooms[data.room]) {
        return rpcReq.getController(cluster_name, data.room)
                .then(function(controller) {
                  //Create a new quic stream for the new conference to cascading room events
                  createQuicStream(controller.id, clientID, data);
                });
      } else {
        log.debug('Cluster already cascaded');
        return Promise.resolve('ok');
      }
    }
  }

  that.start = function () {
    server = new quicCas.QuicTransportServer(port, cf, kf);

    server.start();
    server.onNewSession((session) => {
      var sessionId = session.getId();
      log.info("Server get new session:", sessionId);
      session.onNewStream((quicStream) => {
	    log.info("Server get new stream:", quicStream);
        var streamId = quicStream.getId();

        quicStream.onStreamData((msg) => {
          var event = JSON.parse(msg);
          log.info("Server get stream data:", event);
          if (event.type === 'bridge') {
            rpcReq.getController(cluster_name, event.room)
            .then(function(controller) {
              if (!quicStream[sessionId]) {
                quicStream[sessionId] = {};
              }
              quicStreams[sessionId][streamId] = {stream:quicStream, controller:controller.id};
              if (!controllers[controller.id]) {
                controllers[controller] = [];
              }
              controllers[controller.id].push(quicStream);
              rpcReq.onCascadingConnected(controller.id,self_rpc_id, sessionId, streamId);
            });
          } else if (event.type === 'ready') {
            var info = {
              type: 'ready'
            }
            quicStream.send(JSON.stringify(info));
          } else {
            rpcReq.handleCascadingEvents(quicStreams[sessionId][streamId].controller, event.data);
          }
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

