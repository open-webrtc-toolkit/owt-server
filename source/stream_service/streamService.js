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
const {DomainHandler, RemoteDomainHandler} = require('./domainHandler');

const PROTO_FILE = "service.proto";

let config;
try {
    config = toml.parse(fs.readFileSync('./service.toml')) || {};
} catch (e) {
    log.warn('Failed to parse config:', e);
    process.exit(1);
}

const rpcID = config.service.name || 'stream-service';
const CONTROL_AGENT = config.service.control_agent || null;
const DEFAULT_DOMAIN = '';

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

// RPC interface for stream mesh engine
function RPCInterface(rpcClient, protoRoot) {
  const that = {};

  const rpcChannel = createChannel(rpcClient);
  const rpcReq = createRequest(rpcChannel);

  // WebRTC participants from portal. Id => RTCUser
  const participants = new Map();
  // All completed publications
  const publications = new Map();
  // All completed subscriptions
  const subscriptions = new Map();
  // Ongoing publications
  const publishings = new Map();
  // Ongoing subscriptions
  const subscribings = new Map();
  // Addresses for node. id => {ip, port}
  const nodeAddress = new Map();
  // Addresses for track. id => {ip, port}
  const trackAddress = new Map();
    // All subscribers to track. id => Set {SubscriptionId}
  const trackSubscribers = new Map();
  // Locality node info. id => {agent, node, type, pubs, subs}
  const nodes = new Map();
  // All processors
  const processors = new Map();
  // Domain => Domain controller RPC ID
  const domainHandlers = new Map();
  // Portals
  const portals = new Set();
  // Controller for webrtc streams
  let rtcCtrl = null;
  // Controllers for different types of streams. Type => controller
  const controllers = {};

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

  // Get request handler for specified domain
  const getDomainHandler = async function (domain) {
    if (!domain) {
      domain = DEFAULT_DOMAIN;
    }
    if (!domainHandlers.has(domain)) {
      if (CONTROL_AGENT) {
        // Schedule control agent
        const taskConfig = {room: domain, task: domain};
        log.debug(`getWorkerNode ${config.cluster.name}, ${taskConfig}`);
        const locality = await rpcReq.getWorkerNode(
          config.cluster.name, CONTROL_AGENT, taskConfig, {origin: null});
        domainHandlers.set(domain,
          new RemoteDomainHandler(locality, rpcChannel));
      } else {
        domainHandlers.set(domain, new DomainHandler());
      }
    }
    return domainHandlers.get(domain);
  };

  // Notify request handler for specified domain
  const notifyDomainHandler = function (domain, data) {
    getDomainHandler(domain)
      .then((handler) => {
        handler.onStatus(data);
      }).catch((e) => {
        log.warn('Notify domain error:', e);
      });
  };

  // Notify related portal
  const notifyPortal = function (endpoints, event, data) {
    log.debug('notifyPortal:', endpoints, event, data);
    if (endpoints.to) {
      const portal = participants.get(endpoints.to)?.portal;
      if (portal) {
        rpcReq.sendMsg(portal, endpoints.to, event, data);
      } else {
        log.debug('Participant not exist:', endpoints.to);
      }
    } else {
      const excludes = [];
      if (endpoints.from) {
        excludes.push(endpoints.from);
      }
      for (const portal of portals) {
        rpcReq.broadcast(portal, rpcID, excludes, event, data);
      }
    }
  };

  const addParticipant = function (id, config) {
    const user = new RTCUser(id, config.domain, config.portal);
    user.type = config.portal ? 'portal' : 'manage';
    participants.set(id, user);
    if (config.portal) {
      portals.add(config.portal);
    }
    if (id) {
      const notification = {
        action: 'join',
        data: {id: id, user: config.user, role: config.role},
      };
      notifyPortal({from: id}, 'participant', notification);
    }
  };

  const removeParticipant = function (id) {
    if (participants.has(id)) {
      const user = participants.get(id);
      participants.delete(id);
      // Clean related resources
      const subClean = [];
      const pubClean = [];
      const reason = 'Participant leave'
      for (const subId of user.subs) {
        if (subscriptions.has(subId)) {
          const type = subscriptions.get(subId).type;
          subClean.push(
            controllers[type].removeSession(subId, 'out', reason));
        } else {
          log.debug('Non-existent subscription in leave:', subId);
        }
      }
      for (const pubId of user.pubs) {
        if (publications.has(pubId)) {
          const type = publications.get(pubId).type;
          pubClean.push(
            controllers[type].removeSession(pubId, 'in', reason));
        } else {
          log.debug('Non-existent publication in leave:', pubId);
        }
      }
      // Clean subscriptions before publications
      Promise.all(subClean)
        .catch((e) => {
          log.debug('Error during subscriptions clean up:', e);
        })
        .then(() => {
          return Promise.all(pubClean);
        })
        .catch((e) => {
          log.debug('Error during publications clean up:', e);
        });
      const notification = {
        action: 'leave',
        data: id,
      };
      notifyPortal({from: id}, 'participant', notification);
      return user;
    }
    return null;
  };

  // Get internal stream address for node
  const getInternalAddress = async function (localityNode) {
    if (nodeAddress.has(localityNode)) {
      return nodeAddress.get(localityNode);
    }
    const addr = await rpcChannel.makeRPC(
      localityNode, 'getInternalAddress', [])
    nodeAddress.set(localityNode, addr);
    return addr;
  };

  // Link subscription tracks to their subscribed source
  const linkSubscription = async function (subscription) {
    // Linkup
    log.debug('linkSubscription:', JSON.stringify(subscription));
    // SubTrack => SubSource {audio, video, data}
    const links = new Map();
    for (const type in subscription.sink) {
      for (const track of subscription.sink[type]) {
        // Convert from publicationId to trackId
        if (publications.has(track.from) && !trackAddress.has(track.from)) {
          const prevFrom = track.from;
          track.from = publications.get(track.from).source[type][0].id;
          log.debug('Update subscription from:', prevFrom, '->', track.from);
        }
        const linkAddr = trackAddress.get(track.from);
        const fromInfo = links.get(track.id) || {};
        if (linkAddr) {
          if (fromInfo[type]) {
            log.warn('Conflict source for subscription track:', track.from);
            continue;
          }
          fromInfo[type] = linkAddr;
        } else {
          log.warn('Failed to get link address for:', track.from);
          continue;
        }
        links.set(track.id, fromInfo);
        trackSubscribers.get(track.from).add(subscription.id);
      }
    }

    const linkPros = [];
    for (const [trackId, fromInfo] of links) {
      // rpc linkup
      const subNode = subscription.locality.node;
      log.debug('linkup:', subNode, trackId, fromInfo);
      linkPros.push(
        rpcChannel.makeRPC(subNode, 'linkup', [trackId, fromInfo]));
    }
    return Promise.all(linkPros);
  };

  // Cut subscription tracks from source stream
  const cutSubscription = function (subscription) {
    // cutoff
    log.debug('cutSubscription:', JSON.stringify(subscription));
    const links = [];
    for (const type in subscription.sink) {
      subscription.sink[type].forEach((track) => {
        // rpc cutoff
        const subNode = subscription.locality.node;
        log.warn('cutoff:', subNode, track.id, track.from);
        rpcChannel.makeRPC(subNode, 'cutoff', [track.id])
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
    if (!publishings.has(publication.id)) {
      log.error('No publishing to add!');
      return;
    }
    const locality = publication.locality;
    const owner = publication.info.owner;
    const user = participants.get(owner);
    if (!user || !(locality?.node)) {
      let reason = '';
      if (!user) {
        log.debug('Participant leave during publish:', publication.id);
        reason = 'Participant leave';
      } else {
        log.debug('No locaility for publication:', publication.id);
        reason = 'Locality missing';
      }
      publishings.delete(publication.id);
      controllers[publication.type]
        .removeSession(publication.id, 'in', reason)
        .catch((e) => {
          log.debug('Failed to remove publication:', publication.id);
        });
      return;
    }

    getInternalAddress(locality.node).then((addr) => {
      for (const type in publication.source) {
        publication.source[type].forEach((track) => {
          trackAddress.set(track.id, Object.assign({id: track.id}, addr));
          log.debug('Track address:', track.id, trackAddress.get(track.id));
          trackSubscribers.set(track.id, new Set());
        });
      }
      // Update in participant & node
      user.pubs.add(publication.id);
      if (!nodes.has(locality.node)) {
        nodes.set(locality.node,
          new Node(locality.node, locality.agent, publication.type));
      }
      nodes.get(locality.node).pubs.add(publication.id);
      // Publication completed
      publishings.delete(publication.id);
      publications.set(publication.id, publication);
      // Send notification
      const notification = {
        id: publication.id,
        status: 'add',
        data: publication.toSignalingFormat(),
      };
      notifyPortal({domain: publication.domain}, 'stream', notification);
      const status = {
        type: 'publication',
        status: 'add',
        data: publication
      };
      notifyDomainHandler(publication.domain, status);
    }).catch((e) => {
      publishings.delete(publication.id);
      controllers[publication.type]
        .removeSession(publication.id, 'in', 'No internal address')
        .catch((e) => {
          log.debug('Failed to remove publication:', publication.id);
        });
    });
  };

  const removePublication = function (id) {
    log.debug('removePublication:', id);
    const publication = publications.get(id);
    if (publication) {
      const owner = publication.info.owner;
      const user = participants.get(owner || null);
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
          if (!relatedSubs) {
            // Same track has been cleaned
            return;
          }
          log.debug('Clean publication track:', track.id, relatedSubs);
          trackSubscribers.delete(track.id);
          // Remove all related subscriptions
          for (const subId of relatedSubs) {
            if (subscriptions.has(subId)) {
              const subType = subscriptions.get(subId).type;
              controllers[subType].removeSession(subId, 'Source stream loss');
            }
          }
        });
      }
      publications.delete(id);
      notifyPortal({domain}, 'stream', {id: id, status: 'remove'});
      const status = {
        type: 'publication',
        status: 'remove',
        data: {id}
      };
      notifyDomainHandler(domain, status);
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
    notifyPortal({domain: publication.domain}, 'stream', notification);
    const status = {
      type: 'publication',
      status: 'update',
      data: publication
    };
    notifyDomainHandler(publication.domain, status);
  };

  const addSubscription = function (subscription) {
    log.debug('addSubscription:', subscription.id);
    subscriptions.set(subscription.id, subscription);
    const owner = subscription.info.owner;
    const user = participants.get(owner);
    const locality = subscription.locality;
    if (!user || !(locality?.node)) {
      let reason = '';
      if (!user) {
        log.debug('Participant leave during subscribe:', subscription.id);
        reason = 'Participant leave';
      } else {
        log.debug('No locaility for subscription:', subscription.id);
        reason = 'Locality missing';
      }
      subscribings.delete(subscription.id);
      controllers[subscription.type]
        .removeSession(subscription.id, 'out', reason)
        .catch((e) => {
          log.debug('Failed to remove subscription:', subscription.id);
        });
      return;
    }
    linkSubscription(subscription).then(() => {
      // Update participant & node
      user.subs.add(subscription.id);
      if (!nodes.has(locality.node)) {
        nodes.set(locality.node,
          new Node(locality.node, locality.agent, subscription.type));
      }
      nodes.get(locality.node).subs.add(subscription.id);
      // Subscription completed
      subscribings.delete(subscription.id);
      subscriptions.set(subscription.id, subscription);
      // Send notification
      const status = {
        type: 'subscription',
        status: 'add',
        data: subscription
      };
      notifyDomainHandler(subscription.domain, status);
    }).catch((e) => {
      log.debug('Subscription link failed:', e);
      cutSubscription(subscription);
      subscribings.delete(subscription.id);
      controllers[subscription.type]
        .removeSession(subscription.id, 'out', 'Link failed')
        .catch((e) => {
          log.debug('Failed to remove subscription:', subscription.id);
        });
    })
  };

  const removeSubscription = function (id) {
    const subscription = subscriptions.get(id);
    if (subscription) {
      const owner = subscription.info.owner;
      const user = participants.get(owner || null);
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
      const status = {
        type: 'subscription',
        status: 'remove',
        data: {id}
      };
      notifyDomainHandler(subscription.domain, status);
    }
  };

  // Initialize service
  (function initialize() {
    // Default participant for admin operations
    addParticipant(null, {domain: DEFAULT_DOMAIN});
    // Initialize controllers
    rtcCtrl = new RtcController(null, rpcReq, rpcID, config.cluster.name);
    rtcCtrl.on('signaling', (owner, name, data) => {
      log.warn('On signaling:', owner, name, data);
      notifyPortal({to: owner}, name, data);
    });

    controllers['webrtc'] = rtcCtrl;
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
          // Add subscription
          addSubscription(session);
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
      ctrl.on('session-updated', (id, update) => {
        if (publications.has(id)) {
          updatePublication(update.data);
        } else if (subscriptions.has(id)) {
          //updateSubscription
        }
      });
    }
  })();

  that.onSessionProgress = function (id, name, data) {
    log.debug('progress:', id, name, data);
    if (publishings.has(id) || subscribings.has(id)) {
      // Ongoing session
      const req = publishings.get(id) || subscribings.get(id);
      controllers[req.type].onSessionProgress(id, name, data);
    } else if (publications.has(id) || subscriptions.has(id)) {
      // Completed session
      const session = publications.get(id) || subscriptions.get(id);
      controllers[session.type].onSessionProgress(id, name, data);
    } else { //
      log.warn('Unknown SessionProgress:', id, name, data);
    }
  };

  // Interface for user control
  that.join = function (req, callback) {
    log.debug('join:', req);
    getDomainHandler(req.domain)
      .then((handler) => {
        return handler.join(req);
      }).then((ret) => {
        addParticipant(req.id, req);
        callback('callback', ret);
      }).catch((err) => {
        callback('callback', 'error', err && err.message);
      });
  };
  that.leave = function (req, callback) {
    log.debug('leave:', req);
    const user = removeParticipant(req.id)
    if (user) {
      getDomainHandler(user.domain)
        .then((handler) => {
          return handler.leave(req);
        }).then(() => {
          callback('callback', 'ok');
        }).catch((err) => {
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Invalid participant leave');
    }
  };

  // WebRTC progress interface start
  that.onSessionSignaling = function (id, signaling, callback) {
    rtcCtrl.onClientTransportSignaling(id, signaling)
      .then(() => {
        callback('callback', 'ok');
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };
  that.onTransportProgress = function (transportId, status) {
    rtcCtrl.onTransportProgress(transportId, status);
  };
  that.onTrackUpdate = function (transportId, info) {
    rtcCtrl.onTrackUpdate(transportId, info);
  };
  that.onMediaUpdate = function (trackId, direction, mediaUpdate) {
    rtcCtrl.onSessionProgress(trackId, 'onMediaUpdate', mediaUpdate);
  };
  // WebRTC progress interface end

  that.publish = function(req, callback) {
    log.debug('publish:', req.type, req);
    const type = req.type;
    const user = participants.get(req.participant || null);
    if (!user) {
      callback('callback', 'error', 'Participant not joined');
      return;
    }
    if (controllers[type]) {
      getDomainHandler(user.domain)
      .then((handler) => {
        return handler.publish(req);
      }).then((ret) => {
        req.id = ret.id;
        req.info = ret.info;
        publishings.set(req.id, req);
        return controllers[type].createSession('in', req)
      }).then((id) => {
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to publish:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      throw new Error('Invalid type:' + type);
    }
  };
  that.unpublish = function(req, callback) {
    log.debug('unpublish:', req.id);
    const pubId = req.id;
    if (publications.has(pubId)) {
      const type = publications.get(pubId).type;
      return controllers[type].removeSession(pubId, 'in', 'Participant terminate')
        .then(() => {
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
  // participantId, streamId, command
  that.streamControl = function (req, callback) {
    log.debug('streamControl:', req.type, req);
    const type = req.type;
    const user = participants.get(req.participant || null);
    if (!user) {
      callback('callback', 'error', 'Participant not joined');
      return;
    }
    if (controllers[type]) {
      getDomainHandler(user.domain)
      .then((handler) => {
        return handler.streamControl(req);
      }).then((ret) => {
        return controllers[type].controlSession('in', ret)
      }).then((id) => {
        // publications.set(id, new Publication(id, type, pubConfig.info));
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to streamControl:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      throw new Error('Invalid type:' + type);
    }
  };

  that.subscribe = function(req, callback) {
    log.debug('subscribe:', req.type, req);
    const type = req.type;
    const user = participants.get(req.participant || null);
    if (!user) {
      callback('callback', 'error', 'Participant not joined');
      return;
    }
    if (controllers[type]) {
      getDomainHandler(user.domain)
      .then((handler) => {
        return handler.subscribe(req);
      }).then((ret) => {
        req.id = ret.id;
        req.info = ret.info;
        subscribings.set(req.id, req);
        return controllers[type].createSession('out', req);
      }).then((id) => {
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to subscribe:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };
  that.unsubscribe = function(req, callback) {
    log.debug('unsubscribe:', req.id);
    const subId = req.id;
    if (subscriptions.has(subId)) {
      const type = subscriptions.get(subId).type;
      return controllers[type].removeSession(subId, 'out', 'Participant terminate')
        .then(() => {
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
  //participantId, subscriptionId, command
  that.subscriptionControl = function (config, callback) {
    log.debug('subscriptionControl:', req.type, req);
    const type = req.type;
    const user = participants.get(req.participant || null);
    if (!user) {
      callback('callback', 'error', 'Participant not joined');
      return;
    }
    if (controllers[type]) {
      getDomainHandler(user.domain)
      .then((handler) => {
        return handler.subscriptionControl(req);
      }).then((ret) => {
        return controllers[type].controlSession('out', ret)
      }).then((id) => {
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to subscriptionControl:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      throw new Error('Invalid type:' + type);
    }
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

  that.addProcessor = function (req, callback) {
    log.debug('addProcessor:', req.type, req);
    const type = req.type;
    if (controllers[type] && controllers[type].addProcessor) {
      req.id = req.id || randomUUID().replace(/-/g, '');
      req.info = {};
      return controllers[type].addProcessor(req)
        .then((proc) => {
          proc.controllerType = type;
          processors.set(req.id, proc);
          callback('callback', proc);
        }).catch((err) => {
          log.warn('Failed to addProcessor:', err, err.stack, err.message);
          callback('callback', 'error', err && err.message);
        });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };

  that.removeProcessor = function (req, callback) {
    log.debug('removeProcessor:', req.id);
    const processorId = req.id;
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
    rtcCtrl.destroy();
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