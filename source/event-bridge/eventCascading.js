// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('EventCascading');
var quicCas = require('./quicCascading/build/Release/quicCascading.node');
const inspector = require('event-loop-inspector')();

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

  /*
   * {
   *    sessionId: {
   *       session : quicSessionObject,
   *       id      : quicSessionId,
   *       target  : targetCluster,
   *       streams : {
   *         streamId: {
   *           stream: quicStreamObject,
   *           controller: conferenceRPCId
   *         }
   *       }
   *     }
   * }
   */
  var server, sessions = {};

  /*setInterval(() => {
    console.log('IMOK');
  }, 1000);*/

  that.notify = function (participantId, msg, data) {
    if (controllers[msg.rpcId]) {
      if (msg.type === 'initialize') {
        log.info("Send initialize data to session:",msg.session, " and data:", data);
        if (sessions[msg.session].streams[msg.stream]) {
          sessions[msg.session].streams[msg.stream].stream.send(JSON.stringify(data));
        } else {
          log.error("Quic stream abnormal");
        }
        
      } else {
        for (var item in controllers[msg.rpcId]) {
          log.info("Notify msg:", msg, " with data:", data, " to stream:", item, " controllers:", controllers);
          controllers[msg.rpcId][item].send(JSON.stringify(data));
        }
      }
      return Promise.resolve('ok');
    } else {
      log.error("Quic connection abnormal");
      return Promise.reject('Quic connection abnormal');
    }
  }

  that.destroyRoom = function (data) {
    //Close related quic stream if room is destroyed
    for (var item in controllers[msg.rpcId]) {
      log.info("destroyRoom with data:", data);
      controllers[msg.rpcId][item].close();
      delete controllers[msg.rpcId][item];
    }

    cascadedRooms[data.room] = false;
  }

  const createQuicStream = (controller, clientID, data) => {
    log.info("Create quic stream with controller:", controller, " clientID:", clientID, " and data:",data);
    var quicStream = sessions[clientID].session.createBidirectionalStream();
    var streamID = quicStream.getId();

    if (!sessions[clientID].streams) {
      sessions[clientID].streams = {};
    }

    if (!sessions[clientID].streams[streamID]) {
      sessions[clientID].streams[streamID] = {};
    }
    sessions[clientID].streams[streamID].stream = quicStream;
    sessions[clientID].streams[streamID].controller = controller;

    if (!controllers[controller]) {
      controllers[controller] = [];
    }
    controllers[controller].push(quicStream);
    var info = {
      type: 'bridge',
      room: data.room
    }

    var info = {
      type: 'ready',
      target: data.selfCluster
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
      } else {
        if (event.type === 'initialize') {
          rpcReq.onCascadingConnected(controller, self_rpc_id, clientID, streamID);
        } 
        rpcReq.handleCascadingEvents(controller, self_rpc_id, data.targetCluster, event);
      }
    })
  }

  that.startCascading = function (data, on_ok, on_error) {
    var clientID = data.evIP.replace(/\./g, '-') + '-' + data.evPort;
    log.info("startEventCascading with data:", data, " clientID:", clientID);

    //A new conference request between uncascaded clusters
    if(!sessions[clientID]) {
      return rpcReq.getController(cluster_name, data.room)
        .then(function(controller) {
  	      log.info("Client get controller:", controller);
          sessions[clientID] = {};
          var client = new quicCas.QuicTransportClient(data.evIP, data.evPort);
		      client.connect();
          sessions[clientID].session = client;
          sessions[clientID].target = data.targetCluster;
          cascadedRooms[data.room] = true;
          client.onConnection(() => {
		        log.info("Quic client connected");
            on_ok('ok');
            sessions[clientID].id = client.getId();
            createQuicStream(controller, clientID, data);
          });

          client.onClosedStream((closedStreamId) => {
            log.info("client stream:", closedStreamId, " is closed");
            var event = {
              type: 'onCascadingDisconnected'
            }
            rpcReq.handleCascadingEvents(controller, self_rpc_id, data.targetCluster, event);
            delete sessions[clientID].streams[closedStreamId];
          })

          client.onConnectionFailed(() => {
            log.info("Quic client failed to connect");
            delete sessions[clientID]
            on_error("Event bridge quic connection failed");
          })

          client.onConnectionClosed((sessionId) => {
            log.info("Quic client:", sessionId, " connection closed");
            on_error("Event bridge quic connection closed");
          })
        });
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

    server.start();
    server.onNewSession((session) => {
      var sessionId = session.getId();
      if (!sessions[sessionId]) {
        sessions[sessionId] = {};
      }
      sessions[sessionId].session = session;

      log.info("Server get new session:", sessionId);
      session.onNewStream((quicStream) => {
	    log.info("Server get new stream:", quicStream);
        var streamId = quicStream.getId();
        if (!sessions[sessionId].streams) {
          sessions[sessionId].streams = {};
        }

        if (!sessions[sessionId].streams[streamId]) {
          sessions[sessionId].streams[streamId] = {};
        }
        sessions[sessionId].streams[streamId].stream = quicStream;
        log.info("quicStreams:", sessions);
        quicStream.onStreamData((msg) => {
          var event = JSON.parse(msg);
          log.info("Server get stream data:", event);
          if (event.type === 'bridge') {
            rpcReq.getController(cluster_name, event.room)
            .then(function(controller) {

              sessions[sessionId].streams[streamId].controller = controller;

              if (!controllers[controller]) {
                controllers[controller] = [];
              }
              controllers[controller].push(quicStream);
              rpcReq.onCascadingConnected(controller, self_rpc_id, sessionId, streamId);
            });
          } else if (event.type === 'ready') {
            var info = {
              type: 'ready'
            }
            sessions[sessionId].target = event.target;
            quicStream.send(JSON.stringify(info));
          } else {
            rpcReq.handleCascadingEvents(sessions[sessionId].streams[streamId].controller, self_rpc_id, sessions[sessionId].target, event);
          }
        });
      })

      session.onClosedStream((closedStreamId) => {
        log.info("server stream:", closedStreamId, " is closed");
          var event = {
            type: 'onCascadingDisconnected'
          }

          rpcReq.handleCascadingEvents(sessions[sessionId].streams[closedStreamId].controller, self_rpc_id, sessions[sessionId].target, event);
          delete sessions[sessionId].streams[closedStreamId];
      })
    });

    server.onClosedSession((sessionId) => {
      log.info("Session:", sessionId, " in server is closed");
    })
  }

  that.stop = function () {
    log.info("stop event bridge");
    for (var item in sessions) {
      sessions[item].session.close();
    }
    server.stop();
  }


  return that;
};


module.exports = EventCascading;

