// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

// Logger
const log = require('./logger').logger.getLogger('StreamService');

const fs = require('fs');
const { randomUUID } = require('crypto');

const toml = require('toml');
const protobuf = require('protobufjs');
const createChannel = require('./rpcChannel');
const createRequest = require('./rpcRequest');
const amqper = require('./amqpClient')();
const starter = require('./rpcStarter');

const {RtcController} = require('./rtcController');
const {StreamingController} = require('./streamingController');
const {VideoController} = require('./videoController');
const {AudioController} = require('./audioController');
const {AnalyticsController} = require('./analyticsController');

const {Publication, Subscription, Processor} = require('./session');

const PROTO_FILE = "service.proto";

let config;
try {
    config = toml.parse(fs.readFileSync('./service.toml')) || {};
} catch (e) {
    log.warn('Failed to parse config:', e);
    process.exit(1);
}

const rpcID = config.service.name || 'stream-service';
const defaultPreference = {
  audio: {optional: [{codec: 'opus'}]},
  video: {optional: [{codec: 'vp8'}, {codec: 'h264'}]},
};

function RTCUser(id, domain, portal) {
  this.id = id;
  this.domain = domain;
  this.portal = portal;
  this.pubs = new Set();
  this.subs = new Set();
}

function Node(id, agent, type) {
  this.id = id;
  this.agent = agent;
  this.type = type;
  this.pubs = new Set();
  this.subs = new Set();
}

Node.prototype.isEmpty = function() {
  return this.pubs.size === 0 && this.subs.size === 0;
}

Node.prototype.plain = function() {
  const plain = Object.assign({}, this);
  plain.pubs = [...this.pubs];
  plain.subs = [...this.subs];
  return plain;
}

function RPCInterface(rpcClient, protoRoot) {
  const that = {};

  const rpcChannel = createChannel(rpcClient);
  const rpcReq = createRequest(rpcChannel);

  // string => [userId]
  const domainUsers = new Map();
  // WebRTC participants from portal. Id => RTCUser
  const participants = new Map();
  // All published streams
  const publications = new Map();
  // All subscriptions
  const subscriptions = new Map();
  // Addresses for node. id => {ip, port}
  const nodeAddress = new Map();
  // Addresses for track. id => {ip, port}
  const trackAddress = new Map();
    // All subscribers to track. id => Set {SubscriptionId}
  const trackSubscribers = new Map();
  // Listeners. domain => Set {listenerId}
  const listeners = new Map();
  // Locality node info. id => {agent, node, type, pubs, subs}
  const nodes = new Map();
  // All processors
  const processors = new Map();
  // Proto requests
  const PublishRequest = protoRoot.lookupType('PublishRequest');
  const SubscribeRequest = protoRoot.lookupType('SubscribeRequest');
  const AddProcessorRequest = protoRoot.lookupType('AddProcessorRequest');

  const matchObject = function (subset, obj) {
    if (typeof subset !== 'object') {
      return true;
    }
    let match = true;
    for (const key in subset) {
      if (subset[key] !== obj[key]) {
        match = false;
        break;
      }
    }
    return match;
  };

  const sendNotification = function (endpoints, event, data) {
    const domain = endpoints.domain || participants.get(endpoints.to)?.domain;
    if (listeners.get(domain)) {
      for (const id of listeners.get(domain)) {
        rpcChannel.makeRPC(id, 'onNotification', [endpoints, event, data])
          .catch((e) => log.debug('onNotification error:', e));
      }
    } else {
      log.warn('Unknown endpoints for notification:', endpoints);
    }
  };

  const getInternalAddress = function (localityNode) {
    return rpcChannel.makeRPC(localityNode, 'getInternalAddress', []);
  };

  const linkSubscription = function (subscription) {
    // Linkup
    log.debug('linkSubscription:', JSON.stringify(subscription));
    const links = [];
    for (const type in subscription.sink) {
      subscription.sink[type].forEach((track) => {
        if (publications.has(track.from) && !trackAddress.has(track.from)) {
          const prevFrom = track.from;
          track.from = publications.get(track.from).source[type][0].id;
          log.debug('Update subscription from:', prevFrom, '->', track.from);
        }

        const linkAddr = trackAddress.get(track.from);
        const fromInfo = {};
        if (linkAddr) {
          fromInfo[type] = linkAddr;
        } else {
          log.debug('Failed to get link address for:', track.from);
          return;
        }
        trackSubscribers.get(track.from).add(subscription.id);
        // rpc linkup
        const subNode = subscription.locality.node;
        log.debug('linkup:', subNode, track.id, fromInfo);
        rpcChannel.makeRPC(subNode, 'linkup', [track.id, fromInfo])
          .then((ok) => {
            log.debug('linkup ok');
          })
          .catch((reason) => {
            log.warn('linkup failed, reason:', reason);
          });
      });
    }
  };

  const cutSubscription = function (subscription) {
    // cutoff
    log.debug('cutSubscription:', JSON.stringify(subscription));
    const links = [];
    for (const type in subscription.sink) {
      subscription.sink[type].forEach((track) => {
        // rpc cutoff
        const subNode = subscription.locality.node;
        log.debug('cutoff:', subNode, track.id, track.from);
        rpcChannel.makeRPC(subNode, 'cutoff', [track.from])
          .then((ok) => {
            log.debug('cutoff ok');
          })
          .catch((reason) => {
            log.warn('cutoff failed, reason:', reason);
          });
      });
    }
  };

  const addPublication = function (publication) {
    log.debug('addPublication:', publication.id);
    publications.set(publication.id, publication);

    const locality = publication.locality;
    const owner = publication.info.owner;
    const user = participants.get(owner);
    if (user) {
      user.pubs.add(publication.id);
    } else if (publication.type === 'webrtc') {
      log.debug('Owner not found for publication:', publication.id);
    }
    if (locality.node) {
      const nodeId = locality.node;
      if (!nodes.has(nodeId)) {
        nodes.set(nodeId, new Node(nodeId, locality.agent, publication.type));
      }
      nodes.get(nodeId).pubs.add(publication.id);
    }
    const notification = {
      id: publication.id,
      status: 'add',
      data: publication.toSignalingFormat(),
    };
    sendNotification({domain: publication.domain}, 'stream', notification);

    if (!nodeAddress.has(locality.node)) {
      getInternalAddress(locality.node).then((addr) => {
        nodeAddress.set(locality.node, addr);
        for (const type in publication.source) {
          publication.source[type].forEach((track) => {
            trackAddress.set(track.id, Object.assign({id: track.id}, addr));
            log.debug('Track address:', track.id, trackAddress.get(track.id));
            trackSubscribers.set(track.id, new Set());
          });
        }
      }).catch((e) => {
        log.warn('Failed to get internal address:', e);
      });
    } else {
      const addr = nodeAddress.get(locality.node);
      for (const type in publication.source) {
        publication.source[type].forEach((track) => {
          trackAddress.set(track.id, Object.assign({id: track.id}, addr));
          trackSubscribers.set(track.id, new Set());
        });
      }
    }
  };

  const removePublication = function (id) {
    log.debug('removePublication:', id);
    const publication = publications.get(id);
    if (publication) {
      const owner = publication.info.owner;
      const user = participants.get(owner);
      const domain = publication.domain;
      const node = nodes.get(publication.locality.node);
      if (user) {
        user.pubs.delete(id);
      }
      if (node) {
        node.pubs.delete(id);
        if (node.isEmpty()) {
          nodes.delete(node.id);
        }
      }
      // clean source track data
      for (const type in publication.source) {
        publication.source[type].forEach((track) => {
          const relatedSubs = trackSubscribers.get(track.id);
          log.debug('Clean publication track:', track.id, relatedSubs);
          trackSubscribers.delete(track.id);
          // TODO: cut all source track in related subscriptions
          for (const subId of relatedSubs) {
            const sub = subscriptions.get(subId);
            if (sub) {
              sub.removeSource(track.type, track.id);
            }
          }
        });
      }
      publications.delete(id);
      sendNotification({domain}, 'stream', {id: id, status: 'remove'});
    }
  };

  const updatePublication = function (updates) {
    if (!publications.has(updates.id)) {
      log.warn('Update invalid publication:', updates.id);
      return;
    }
    const publication = publications.get(updates.id);
    updates.tracks.forEach((track) => {
      publication.removeSource(track.type, track.id);
      publication.addSource(
        track.type, track.id, track.format, track.parameters);
    });
    const notification = {
      id: publication.id,
      status: 'update',
      data: {field: '.', value: publication.toSignalingFormat()},
    };
    sendNotification({domain: publication.domain}, 'stream', notification);
  };

  const addSubscription = function (subscription) {
    log.debug('addSubscription:', subscription.id);
    subscriptions.set(subscription.id, subscription);
    const owner = subscription.info.owner;
    const user = participants.get(owner);
    const locality = subscription.locality;
    if (user) {
      user.subs.add(subscription.id);
    } else if (subscription.type === 'webrtc') {
      log.warn('Owner not found for subscription:', subscription.id);
    }
    if (locality.node) {
      const nodeId = locality.node;
      if (!nodes.has(nodeId)) {
        nodes.set(nodeId, new Node(nodeId, locality.agent, subscription.type));
      }
      nodes.get(nodeId).subs.add(subscription.id);
    }

    // Link
    linkSubscription(subscription);
  };

  const removeSubscription = function (id) {
    const subscription = subscriptions.get(id);
    if (subscription) {
      const owner = subscription.info.owner;
      const user = participants.get(owner);
      const node = nodes.get(subscription.locality.node);
      // cut off
      cutSubscription(subscription);
      if (user) {
        user.subs.delete(id);
      }
      if (node) {
        node.subs.delete(id);
        if (node.isEmpty()) {
          nodes.delete(node.id);
        }
      }
      subscriptions.delete(id);
    }
  };

  const controller = new RtcController(null, rpcReq, rpcID, config.cluster.name);
  controller.on('signaling', (owner, name, data) => {
    log.debug('On signaling:', owner, name, data);
    sendNotification({to: owner}, name, data);
  });

  that.onRTCSignaling = function (id, name, data, callback) {
    log.debug('signal:', name, data);
    if (name === 'join') {
      participants.set(data.id, new RTCUser(data.id, data.domain, data.portal));
      const joinResponse = {
        permission: {},
        room: {
          id: '',
          participants:[],
          streams:[],
        },
      };
      for (const [id, ppt] of participants) {
        if (data.domain === ppt.domain) {
          joinResponse.room.participants.push(ppt);
        }
      }
      for (const [id, pub] of publications) {
        if (data.domain === pub.domain) {
          joinResponse.room.streams.push(pub.toSignalingFormat());
        }
      }
      callback('callback', joinResponse);
      return;
    } else if (name === 'leave') {
      participants.delete(data.id);
      controller.terminateByOwner(data.id)
        .catch((e) => log.warn('terminate by owner exception:', e));
      callback('callback', 'ok');
      return;
    }

    // Fill default control data
    let controlData = data.control ?? {};
    controlData.formatPreference = controlData.formatPreference ??
      data?.media?.tracks?.map((track) => {
        return defaultPreference[track.type];
      });
    controlData.owner = controlData.owner ?? id;
    controlData.origin = controlData.origin ??
      {
        isp:'isp',
        region:'region',
      };
    controlData.domain = controlData.domain ?? participants.get(id)?.domain;

    return controller.handleRTCSignaling(name, data, controlData)
      .then((result) => {
        const ret = result ? result : 'ok';
        callback('callback', ret);
      }).catch((e) => {
        callback('callback', 'error', e);
      });
  };

  that.onSessionProgress = function (id, name, data) {
    log.debug('progress:', id, name, data);
    if (publications.has(id) || subscriptions.has(id)) {
      const session = publications.get(id) || subscriptions.get(id);
      controllers[session.type].onSessionProgress(id, name, data);
    } else { //
      log.warn('Unknown SessionProgress:', id, name, data);
    }
  };

  // WebRTC progress interface start
  that.onTransportProgress = function (transportId, status) {
    controller.onTransportProgress(transportId, status);
  };
  that.onTrackUpdate = function (transportId, info) {
    controller.onTrackUpdate(transportId, info);
  };
  that.onMediaUpdate = function (trackId, direction, mediaUpdate) {
    controller.onSessionProgress(trackId, 'onMediaUpdate', mediaUpdate);
  };
  // WebRTC progress interface end

  const controllers = {};
  controllers['webrtc'] = controller;
  controllers['streaming'] =
      new StreamingController(rpcReq, rpcID, config.cluster.name);
  controllers['recording'] =
      new StreamingController(rpcReq, rpcID, config.cluster.name);
  controllers['video'] =
      new VideoController(rpcChannel, rpcID, config.cluster.name);
  controllers['audio'] =
      new AudioController(rpcChannel, rpcID, config.cluster.name);
  controllers['analytics'] =
      new AnalyticsController(rpcChannel, rpcID, config.cluster.name);

  for (const type in controllers) {
    const ctrl = controllers[type];
    ctrl.on('session-established', async (id, session) => {
      log.debug('onSessionEstablished', id, session);
      if (session.source) {
        // Add publication
        addPublication(session);
      } else if (session.sink) {
        subscriptions.set(id, session);
        // Link
        linkSubscription(session);
      } else {
        log.warn('Unknown session:', id, session);
        // May remove session
      }
    });
    ctrl.on('session-aborted', async (id, reason) => {
      try {
        if (publications.has(id)) {
          removePublication(id);
        } else if (subscriptions.has(id)) {
          removeSubscription(id);
        }
      } catch (e) {
        log.warn('Error during session-aborted:', e.stack || e);
      }
    });
  }

  that.publish = function(pubConfig, callback) {
    log.debug('publish:', pubConfig.type, pubConfig);
    /*
    const errMsg = PublishRequest.verify(pubConfig);
    if (errMsg) {
      log.warn('Invalid publish format:', errMsg);
      callback('callback', 'error', err && err.message);
      return;
    }
    */

    const type = pubConfig.type;
    if (controllers[type]) {
      pubConfig.id = pubConfig.id || randomUUID().replace(/-/g, '');
      pubConfig.info = {};
      return controllers[type].createSession('in', pubConfig)
        .then((id) => {
          publications.set(id, new Publication(id, type, pubConfig.info));
          callback('callback', {id});
        }).catch((err) => {
          log.warn('Failed to publish:', err, err.stack, err.message);
          callback('callback', 'error', err && err.message);
        });
    } else {
      throw new Error('Invalid type:' + type);
    }
  };

  that.unpublish = function(pubId, callback) {
    log.debug('unpublish:', pubId);
    if (publications.has(pubId)) {
      const type = publications.get(pubId).type;
      return controllers[type].removeSession(pubId, 'in', 'Parcipant terminate')
        .then(() => {
          removePublication(pubId);
          callback('callback', 'ok');
        }).catch((err) => {
          log.warn('Failed to unpublish:', err, err.stack);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Publication not found');
    }
  };

  that.getPublications = function(filter, callback) {
    log.debug('getPublications:', filter, callback);
    const ret = [];
    for (const [id, publication] of publications) {
      if (matchObject(filter, publication)) {
        ret.push(publication);
      }
    }
    callback('callback', ret);
  };

  that.subscribe = function(subConfig, callback) {
    log.debug('subscribe:', subConfig.type, subConfig);
    const type = subConfig.type;
    if (controllers[type]) {
      subConfig.id = subConfig.id || randomUUID().replace(/-/g, '');
      subConfig.info = {};
      return controllers[type].createSession('out', subConfig)
        .then((id) => {
          subscriptions.set(id, new Subscription(id, type, subConfig.info));
          callback('callback', {id});
        }).catch((err) => {
          log.warn('Failed to subscribe:', err, err.stack, err.message);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };

  that.unsubscribe = function(subId, callback) {
    log.debug('unsubscribe:', subId);
    if (subscriptions.has(subId)) {
      const type = subscriptions.get(subId).type;
      return controllers[type].removeSession(subId, 'out', 'Parcipant terminate')
        .then(() => {
          removeSubscription(subId);
          callback('callback', 'ok');
        }).catch((err) => {
          log.warn('Failed to unsubscribe:', err, err.stack);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Subscription not found');
    }
  };

  that.getSubscriptions = function(filter, callback) {
    log.debug('getSubscriptions:', filter);
    const ret = [];
    for (const [id, subscription] of subscriptions) {
      if (matchObject(filter, subscription)) {
        ret.push(subscription);
      }
    }
    callback('callback', ret);
  };

  that.getNodes = function(filter, callback) {
    log.debug('getNodes:', filter);
    const ret = [];
    for (const [id, node] of nodes) {
      if (matchObject(filter, node)) {
        ret.push(node.plain());
      }
    }
    callback('callback', ret);
  }

  that.addNotificationListener = function (domain, listenerId, callback) {
    log.debug('addNotificationListener:', domain, listenerId);
    if (typeof domain === 'string') {
      if (!listeners.has(domain)) {
        listeners.set(domain, new Set());
      }
      listeners.get(domain).add(listenerId);
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Invalid domain');
    }
  }

  that.removeNotificationListener = function (domain, listenerId, callback) {
    log.debug('removeNotificationListener:', domain, listenerId);
    if (typeof domain === 'string') {
      if (listeners.has(domain)) {
        listeners.get(domain).delete(listenerId);
      }
      callback('callback', 'ok');
    } else {
      callback('callback', 'error', 'Invalid domain');
    }
  }

  that.addProcessor = function (procConfig, callback) {
    log.debug('addProcessor:', procConfig.type, procConfig);
    const type = procConfig.type;
    if (controllers[type] && controllers[type].addProcessor) {
      procConfig.id = procConfig.id || randomUUID().replace(/-/g, '');
      procConfig.info = {};
      return controllers[type].addProcessor(procConfig)
        .then((proc) => {
          proc.controllerType = type;
          processors.set(procConfig.id, proc);
          callback('callback', proc);
        }).catch((err) => {
          log.warn('Failed to addProcessor:', err, err.stack, err.message);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };

  that.removeProcessor = function (processorId, callback) {
    log.debug('removeProcessor:', processorId);
    if (processors.has(processorId)) {
      const type = processors.get(processorId).controllerType;
      return controllers[type].removeProcessor(processorId)
        .then((ok) => {
          processors.delete(processorId);
          callback('callback', 'ok');
        }).catch((err) => {
          log.warn('Failed to removeProcessor:', err, err.stack, err.message);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Processor not found');
    }
  };

  that.getProcessors = function (filter, callback) {
    log.debug('getProcessors:', filter);
    const ret = [];
    for (const [id, processor] of processors) {
      if (matchObject(filter, processor)) {
        ret.push(processor);
      }
    }
    callback('callback', ret);
  };

  that.close = function () {
    controller.destroy();
    for (const type in controllers) {
      controllers[type].destroy();
    }
  };

  return that;
}


let serviceRPC = null;

protobuf.load(PROTO_FILE, function(err, root) {
  if (err) {
    log.warn('Failed to parse proto:', err.stack);
    return;
  }

  amqper.connect(config.rabbit, function connectOk() {
    starter.startClient(amqper)
      .then((rpcClient) => {
        serviceRPC = RPCInterface(rpcClient, root);
        return starter.startServer(amqper, rpcID, serviceRPC);
      })
      .then((rpcServer) => {
        log.info(rpcID + ' as rpc server ready');
        return starter.startMonitor(amqper, function onData(data) {
          log.debug('Monitor data:', data);
        });
      })
      .catch((err) => {
        log.error('Failed to start RPC:', err, err.stack);
        process.exit();
      });
  }, function connectError(reason) {
    log.error(rpcID + ' start rpc failed, reason:', reason, reason.stack);
    process.exit();
  });
});

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, async function () {
        log.warn('Exiting on', sig);
        serviceRPC && serviceRPC.close();
        try {
            await amqper.disconnect();
        } catch(e) {
            log.warn('Exiting e:', e);
        }
        process.exit();
    });
});

process.on('exit', function () {
    log.info('Process exit');
});

process.on('unhandledRejection', (reason) => {
    log.info('Reason: ' + reason);
    log.info('Reason s:', reason.stack);
});

process.on('SIGUSR2', function() {
    logger.reconfigure();
});

/*
module.exports = RPCInterface;
*/