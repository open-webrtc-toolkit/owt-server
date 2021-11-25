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
   *     session: sessionId,
   *     stream: streamId
   * }}
   */
  var server_controllers = {};
  var client_controllers = {};
  var server, clients = {};
  var quicStreams = {};

  // Key is participantId, value is token ID.
  const quicTransportIds = new Map();

  /*setInterval(() => {
    console.log('IMOK');
  }, 1000);*/

  that.notify = function (participantId, msg, data) {
    if (server_controllers[msg.rpcId]) {
      if (msg.type === 'initialize') {
        log.info("Send initialize data to session:",msg.session, " stream:",msg.stream, " and data:", data);
        server.send(msg.session, msg.stream, JSON.stringify(data));
      } else {
        server_controllers[msg.rpcId].forEach(function(item) {
          log.info("server send msg:", msg, " with data:", data, " from controller:", controller);
          server.send(item.session, item.stream, JSON.stringify(data));
        })
      }
      return Promise.resolve('ok');
    } else if(client_controllers[msg.rpcId]) {
      if (msg.type === 'initialize') {
        log.info("Send initialize data to session:",msg, " and data:", data);
        client_controllers[msg.rpcId][msg.client].client.send(msg.session, msg.stream, JSON.stringify(data));
      } else {
        for (var client in client_controllers[msg.rpcId]) {
          var session = client_controllers[msg.rpcId][client].session;
          var stream = client_controllers[msg.rpcId][client].stream;
          log.info("Client send notify msg:", msg, " with data:", data);
          client_controllers[msg.rpcId][client].client.send(session, stream, JSON.stringify(data));
        }
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
	      log.info("Client get controller:", controller);
        var client = new quicCas.QuicTransportClient(data.evIP, data.evPort);
        clients[clientID] = true;
        client.id = clientID;
        client.onNewStreamEvent((msg) => {
          var info = JSON.parse(msg);
          log.info("Get new stream:", info);
          client.controller = controller;
          client.initialize = false;
          var quicinfo = {session: info.sessionId, stream: info.streamId, client: client};
          if (!client_controllers[controller]) {
            client_controllers[controller] = {};
          }

          if (!client_controllers[controller][clientID]) {
            client_controllers[controller][clientID] = {};
          }
          client_controllers[controller][clientID] = quicinfo;
          client.send(info.sessionId, info.streamId, data.room);
        });
        
        client.onRoomEvent((msg) => {
          var event = JSON.parse(msg);
          log.debug('Get room cascaded event:', event);
          if (!client.initialize) {
            client.initialize = true;
            rpcReq.onCascadingConnected(client.controller,self_rpc_id, client_controllers[client.controller][client.id].sessionId, client_controllers[client.controller][client.id].streamId, client.id);
          }

          rpcReq.handleCascadingEvents(client.controller, event);

        });
      });
    } else {
      log.debug('Cluster already cascaded');
      return Promise.resolve('ok');
    }
  }

  that.start = function () {
    server = new quicCas.QuicTransportServer(port, cf, kf);

    server.onNewStreamEvent((msg) => {
      var event = JSON.parse(msg);
      log.info("Server get new stream:", event);
      //setTimeout(() => {
	    rpcReq.getController(cluster_name, event.room)
        .then(function(controller) {
          log.debug('got controller:', controller);
          if (!quicStreams[event.sessionId]) {
            quicStreams[event.sessionId] = {};
          }

          if (!quicStreams[event.sessionId][event.streamId]) {
            quicStreams[event.sessionId][event.streamId] = {};
          }
          quicStreams[event.sessionId][event.streamId].controller = controller;
          var quicinfo = {session: event.sessionId, stream: event.streamId};
          if (!server_controllers[controller]) {
            server_controllers[controller] = [];
          }
          server_controllers[controller].push(quicinfo);
          rpcReq.onCascadingConnected(controller,self_rpc_id, event.sessionId, event.streamId);
	});
        //})}, 0);
    });

    server.onRoomEvent((msg) => {
      var event = JSON.parse(msg);
      log.info('Server get cascaded event:', event);
      log.info('quicStreams:', quicStreams);

      //setTimeout(() => {
      rpcReq.handleCascadingEvents(quicStreams[event.sessionId][event.streamId].controller, event.data);
      //}, 0);
    });

  }
/*
    that.start = function () {
      server = new QuicTransportServer(port, cf, kf);
      server.start();
      var test1 = inspector.dump();
      log.info("Dump event before starting quic server:", JSON.stringify(test1));


      server.on('newstream', async (event) => {
        log.info("Server get new stream:", event);
        var test2 = inspector.dump();
        log.info("Dump event when getting new stream:", JSON.stringify(test2));

        var controller = await rpcReq.getController(cluster_name, event.room);

        log.debug('got controller:', controller);
        if (!quicStreams[event.sessionId]) {
          quicStreams[event.sessionId] = {};
        }

        if (!quicStreams[event.sessionId][event.streamId]) {
          quicStreams[event.sessionId][event.streamId] = {};
        }
        quicStreams[event.sessionId][event.streamId].controller = controller;
        var quicinfo = {session: event.sessionId, stream: event.streamId};
        if (!server_controllers[controller]) {
          server_controllers[controller] = {};
        }
        server_controllers[controller] = quicinfo;
        await rpcReq.onCascadingConnected(controller,self_rpc_id, event.sessionId, event.streamId);

      });

      server.on('roomevents', async (event) => {
        log.debug('Get cascaded event:', event);

        await rpcReq.handleCascadingEvents(quicStreams[event.sessionId][event.streamId].controller, event.data);

      });

  }
*/
  that.stop = function () {
    log.info("stop event bridge");
  }


  return that;
};


module.exports = EventCascading;

