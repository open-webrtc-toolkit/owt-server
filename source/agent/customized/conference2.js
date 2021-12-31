// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const log = require('./logger').logger.getLogger('SimpleController');

const createChannel = require('./rpcChannel');
const amqper = require('./amqpClient')();

const fs = require('fs');
const toml = require('toml');

const STREAM_LAYER = 'stream-service';

const defaultPreference = {
  audio: {optional: [{codec: 'opus'}]},
  video: {optional: [{codec: 'vp8'}, {codec: 'h264'}]},
};

class ControllerInterface {

  onRTCSignaling(clientId, requestName, requestData) {
    log.warn('onPortalSignaling is not implemented');
  }

  onNotification(endpoints, event, data) {
    log.warn('onStreamEvent is not implemented');
  }
}

class SimpleController extends ControllerInterface {

  constructor(rpcId, rpcClient) {
    super();
    this.rpcChannel = createChannel(rpcClient);
    this.rpcId = rpcId;
    this.domain = null;
    // id => JoinData
    this.clients = new Map();
    // id => count
    this.portals = new Map();
    // id => stream
    this.streams = new Map();
  }

  onRTCSignaling(clientId, requestName, requestData, callback) {
    log.debug('onRTCSignaling:', clientId, requestName, requestData);
    if (requestName === 'join') {
      this.clients.set(clientId, requestData);
      const count = (this.portals.get(requestData.portal) || 0);
      this.portals.set(requestData.portal, count + 1);
      if (!this.domain) {
        this.domain = requestData.domain;
        // RPC
        this.rpcChannel.makeRPC(STREAM_LAYER, 'addNotificationListener', [this.domain, this.rpcId])
          .catch((e) => log.debug('addNotificationListener error:', e));
      }
    } else if (requestName === 'leave') {
      const portal = this.clients.get(clientId).portal;
      this.clients.delete(clientId);
      const count = (this.portals.get(portal) || 0);
      if (count === 1) {
        this.portals.delete(portal);
      }
    } else if (requestName === 'publish') {
      // Initialize control info
      const formatPreference = requestData.media.tracks.map((track) => {
        return defaultPreference[track.type];
      });
      const controlData = {
        owner: clientId,
        origin: {isp:'isp', region:'region'},
        formatPreference,
        domain: this.domain,
      };
      requestData.control = controlData;
    } else if (requestName === 'subscribe') {
      // Initialize control info
      const formatPreference = requestData.media.tracks.map((track) => {
        const trackPreference = defaultPreference[track.type];
        const sourceStream = this.streams.get(track.from);
        if (sourceStream) {
          const fromTrack = sourceStream.media.tracks.find((t) => {
            return t.type === track.type;
          });
          if (fromTrack) {
            trackPreference.preferred = fromTrack.format;
          }
        }
        return trackPreference;
      });
      const controlData = {
        owner: clientId,
        origin: {isp:'isp', region:'region'},
        formatPreference,
      };
      requestData.control = controlData;
    }

    // To stream service
    this.rpcChannel.makeRPC(STREAM_LAYER, 'onRTCSignaling', [clientId, requestName, requestData])
      .then((ret) => {
        callback('callback', ret);
      })
      .catch((e) => log.debug('onRTCSignaling error:', e));
  }

  // endpoints = {from, to}
  onNotification(endpoints, name, data) {
    log.debug('onNotification:', endpoints, name, data);
    // Redirect to portals
    if (endpoints.to) {
      // Send to specific user
      const portal = this.clients.get(endpoints.to)?.portal;
      if (portal) {
        this.rpcChannel.makeRPC(portal, 'onNotification', [endpoints, name, data])
          .then((ret) => {
            callback('callback', ret);
          })
          .catch((e) => log.debug('onNotification error:', e));
      }
    } else {
      // Broadcast
      if (name === 'stream') {
        // Process stream events
        if (data.status === 'add') {
          this.streams.set(data.id, data.data);
        } else if (data.status === 'remove') {
          this.streams.delete(data.id);
        } else if (data.status === 'update') {

        }
      }
      this.portals.forEach((count, portal) => {
        this.rpcChannel.makeRPC(portal, 'onNotification', [endpoints, name, data])
          .then((ret) => {
            callback('callback', ret);
          })
          .catch((e) => log.debug('onNotification error:', e));
      })
    }
  }

  close() {
    // RPC
    this.rpcChannel.makeRPC(STREAM_LAYER, 'removeNotificationListener', [this.domain, this.rpcId])
      .catch((e) => log.debug('removeNotificationListener error:', e));
  }

}

function RPCInterface(rpcClient, rpcID) {
  return new SimpleController(rpcID, rpcClient);
}

module.exports = RPCInterface;
