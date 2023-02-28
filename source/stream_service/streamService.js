// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

// Logger
const log = require('./logger').logger.getLogger('StreamService');

const fs = require('fs');
const { randomUUID } = require('crypto');

const toml = require('toml');
const createChannel = require('./rpcChannel');
const createRequest = require('./rpcRequest');
const AmqpCient = require('./amqpClient');
const starter = require('./rpcStarter');
const {DomainHandler, RemoteDomainHandler} = require('./domainHandler');
const {StateStores} = require('./stateStores');
const State = require('./stateTypes');
const {ServiceScheduler} = require('./scheduler');
const controllerConfig = require('./controllers.json');

let config;
try {
  config = toml.parse(fs.readFileSync('./service.toml')) || {};
} catch (e) {
  log.warn('Failed to parse config:', e);
  process.exit(1);
}

const rpcID = config.service.name ||
  'streamservice-' + randomUUID().replace(/-/g, '');
const schedulerID = config.scheduler &&
  (config.scheduler.name || 'stream-service');
const startScheduler = config.scheduler?.start || false;
const CONTROL_AGENT = config.service.control_agent || null;
const DB_URL = config.mongo?.dataBaseURL || 'localhost/owtstreams';
const ADMIN = 'admin';
const DEFAULT_DOMAIN = '';

// stream mesh engine
function streamEngine(rpcClient) {
  // RPC API
  const that = {};
  const rpcChannel = createChannel(rpcClient);
  const rpcReq = createRequest(rpcChannel);
  // Ongoing publications
  const publishings = new Map();
  // Ongoing subscriptions
  const subscribings = new Map();
  // Addresses for node. id => {ip, port}
  const nodeAddress = new Map();
  // Domain => Domain controller
  const domainHandlers = new Map();
  // Controllers for different types of streams. Type => controller
  const controllers = {};
  // Shared state for streams
  const stateStores = new StateStores(DB_URL);
  // Schedulers
  const scheduler = startScheduler ?
    new ServiceScheduler(rpcChannel, stateStores) : null;

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
      const domainState = new State.Domain(domain, null);
      try {
        await stateStores.create('domains', domainState.plain());
        // Add default domain user
        await addParticipant(domain, {domain});
      } catch (e) {
        log.info('Failed to save domain state:', domain, e?.message);
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
      stateStores.read('participants', {id: endpoints.to})
        .then((user) => {
          const portal = user?.portal;
          if (portal) {
            return rpcReq.sendMsg(portal, endpoints.to, event, data);
          } else {
            log.debug('Participant not exist:', endpoints.to);
          }
        }).catch((e) => {
          log.debug('Failed to send message to portal:', e?.message);
        });
    } else {
      stateStores.read('domains', {id: endpoints.domain})
        .then((dm) => {
          log.debug('domain:', dm);
          const excludes = [];
          if (endpoints.from) {
            excludes.push(endpoints.from);
          }
          for (const portal of dm.portals) {
            const controller = schedulerID || rpcID;
            rpcReq.broadcast(portal, controller, excludes, event, data)
              .catch((e) => {
                log.debug('Failed to broadcast to portal:', e?.message);
              });
          }
        }).catch((e) => {
          log.debug('Failed to get domain portals:', e?.message);
        });
    }
  };

  // Participant control
  const addParticipant = async function (id, config) {
    const user = new State.User(id, config.domain, config.portal);
    user.type = config.portal ? 'portal' : 'manage';
    user.notifying = config.notifying ?? false;
    await stateStores.create('participants', user.plain());
    if (config.portal) {
      const filter = {id: config.domain};
      const updates = {'portals': config.portal};
      await stateStores.setAdd('domains', filter, updates);
    }
    if (id && user.notifying) {
      const notification = {
        action: 'join',
        data: {id: id, user: config.user, role: config.role},
      };
      const endpoints = {from: id, domain: user.domain};
      notifyPortal(endpoints, 'participant', notification);
    }
  };
  const removeParticipant = async function (id) {
    const user = await stateStores.read('participants', {id});
    // Remove in state
    const removed = await stateStores.delete('participants', {id});
    if (user && removed) {
      // Clean related resources
      const subClean = [];
      const pubClean = [];
      const reason = 'Participant leave'
      for (const subId of user.subs) {
        const prom = stateStores.read('subscriptions', {id: subId})
        .then((sub) => {
          if (sub) {
            return stopSession(subId, sub.type, 'out', reason);
          }
        });
        subClean.push(prom);
      }
      for (const pubId of user.pubs) {
        const prom = stateStores.read('publications', {id: pubId})
        .then((pub) => {
          if (pub) {
            return stopSession(pubId, pub.type, 'in', reason);
          }
        });
        pubClean.push(prom);
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
      if (id && user.notifying) {
        const notification = {
          action: 'leave',
          data: id,
        };
        const endpoints = {
          from: id,
          domain: user.domain
        };
        notifyPortal(endpoints, 'participant', notification);
      }
      return user;
    }
    return null;
  };

  // Save worker node states
  const saveWorkerNode = async function (locality, type) {
    const node = new State.WorkerNode(
      locality.node, locality.agent, type);
    try {
      await stateStores.create('nodes', node.plain());
    } catch (e) {
      log.debug('Failed to create node:', e?.message);
    }
    return node;
  };

  // Get internal stream address for node
  const getInternalAddress = async function (nodeId) {
    if (nodeAddress.has(nodeId)) {
      return nodeAddress.get(nodeId);
    }
    const plainNode = await stateStores.read('nodes', {id: nodeId});
    if (!plainNode) {
      throw new Error('No node for internal address');
    }
    if (plainNode.streamAddr) {
      nodeAddress.set(nodeId, plainNode.streamAddr);
      return plainNode.streamAddr;
    }
    const addr = await rpcChannel.makeRPC(
      nodeId, 'getInternalAddress', []);
    await stateStores.update('nodes', {id: nodeId}, {streamAddr: addr});
    nodeAddress.set(nodeId, addr);
    return addr;
  };

  // Stop publication or subscription
  const stopSession = async function(id, type, direction, reason) {
    if (!controllers[type]) {
      log.warn('Invalid type in stopSession:', type);
      return;
    }
    if (direction === 'in') {
      publishings.get(id)._aborted = true;
    } else if (direction === 'out') {
      subscribings.get(id)._aborted = true;
    }
    return controllers[type].removeSession(id, direction, reason);
  };

  // Link subscription tracks to their subscribed source
  const linkSubscription = async function (subscription) {
    // Linkup
    log.debug('linkSubscription:', subscription.id);
    // SubTrack => SubSource {audio, video, data}
    const links = new Map();
    const updatePros = [];
    for (const type in subscription.sink) {
      for (const track of subscription.sink[type]) {
        if (!track.from) {
          continue;
        }
        // Convert from publicationId to trackId if needed
        const prom = stateStores.read('publications', {id: track.from})
        .then(async (pub) => {
          if (pub) {
            if (!pub.source[type].length === 0) {
              throw new Error(`No ${type}: ${track.from} to subscribe}`);
            }
            const prevFrom = track.from;
            track.from = pub.source[type][0].id;
            log.debug('Update subscription from:', prevFrom, '->', track.from);
          }
          const srcTrack = await stateStores.read(
              'sourceTracks', {id: track.from});
          if (!srcTrack?.address) {
            throw new Error(`Failed to get link address for ${track.from}`);
          }
          const linkAddr = Object.assign({id: track.from}, srcTrack.address);
          const fromInfo = links.get(track.id) || {};
          if (fromInfo[type]) {
            log.warn('Overwrite link address:', fromInfo[type], linkAddr);
          }
          fromInfo[type] = linkAddr;
          links.set(track.id, fromInfo);
          // Update state
          const subAdd = {'linkedSubs': subscription.id};
          return stateStores.setAdd('sourceTracks', {id: srcTrack.id}, subAdd);
        });
        updatePros.push(prom);
      }
    }
    await Promise.all(updatePros);
    const linkPros = [];
    for (const [trackId, fromInfo] of links) {
      // RPC linkup
      const subNode = subscription.locality.node;
      log.debug('linkup:', subNode, trackId, fromInfo);
      linkPros.push(
        rpcChannel.makeRPC(subNode, 'linkup', [trackId, fromInfo]));
    }
    return Promise.all(linkPros);
  };

  // Cut subscription tracks from source stream
  const cutSubscription = async function (subscription) {
    // cutoff
    log.debug('cutSubscription:', JSON.stringify(subscription));
    const cutPromises = [];
    for (const type in subscription.sink) {
      subscription.sink[type].forEach((track) => {
        if (!track.from) {
          return;
        }
        // Update state
        const subRemove = {'linkedSubs': subscription.id};
        const p1 = stateStores.setRemove(
            'sourceTracks', {id: track.from}, subRemove)
          .catch((e) => {
            log.debug('Failed to update source track:', e?.message);
          });
        cutPromises.push(p1);
        // RPC cutoff
        const subNode = subscription.locality.node;
        log.debug('Cutoff:', subNode, track.id, track.from);
        const p2 = rpcChannel.makeRPC(subNode, 'cutoff', [track.id])
          .catch((reason) => {
            log.debug('Cutoff failed, reason:', reason);
          });
        cutPromises.push(p2);
      });
    }
    return Promise.all(cutPromises);
  };

  const addPublication = async function (publication) {
    log.debug('addPublication:', publication.id);
    if (!publishings.has(publication.id)) {
      log.error('No publishing to add!');
      return;
    }
    const locality = publication.locality;
    const owner = publishings.get(publication.id).participant;
    const user = await stateStores.read('participants', {id: owner});
    if (!user) {
      log.debug('Participant leave during publish:', publication.id);
      throw new Error('Participant leave');
    }
    if (!(locality?.node)) {
      log.debug('No locaility for publication:', publication.id);
      throw new Error('Locality missing');
    }
    // Get internal address for publish tracks
    const node = await saveWorkerNode(locality, publication.type);
    const addr = await getInternalAddress(locality.node);
    const trackMap = new Map();
    for (const type in publication.source) {
      publication.source[type].forEach((track) => {
        const srcTrack = new State.SourceTrack(track.id, publication.id);
        srcTrack.address = addr;
        log.debug('Track address:', track.id, addr);
        trackMap.set(srcTrack.id, srcTrack);
      });
    }
    // Update in participant, node, publication, states
    const pubAdd = {'pubs': publication.id};
    await stateStores.setAdd('participants', {id: owner}, pubAdd);
    await stateStores.setAdd('nodes', {id: node.id}, pubAdd);
    // Save publication tracks
    for (const [, srcTrack] of trackMap) {
      await stateStores.create('sourceTracks', srcTrack.plain());
    }
    await stateStores.create('publications', publication);
    // Send notification
    if (!publication.info?.hidden) {
      const notification = {
        id: publication.id,
        status: 'add',
        data: publication.toSignalingFormat(),
      };
      notifyPortal({domain: publication.domain}, 'stream', notification);
    }
    const status = {
      type: 'publication',
      status: 'add',
      data: publication
    };
    notifyDomainHandler(publication.domain, status);
    // Check if publication aborted during setup
    if (publishings.get(publication.id)?._aborted) {
      log.debug('Publication aborted during setup:', publication.id);
      removePublication(publication.id);
    }
  };

  const removePublication = function (id) {
    log.debug('removePublication:', id);
    stateStores.read('publications', {id}).then(async (publication) => {
      if (!publication) {
        log.debug('No publication:', id);
        return;
      }
      // Delete pub state first
      const removed = await stateStores.delete('publications', {id});
      if (!removed) {
        log.debug('Publication already removed:', id);
        return;
      }
      const owner = publication.info.owner;
      const nodeId = publication.locality.node;
      const domain = publication.domain;
      // clean source track data
      const removedTracks = new Map();
      for (const type in publication.source) {
        for (const track of publication.source[type]) {
          removedTracks.set(track.id, track);
        }
      }
      for (const [id, track] of removedTracks) {
        stateStores.read('sourceTracks', {id: track.id}).then((srcTrack) => {
          log.debug('Clean track:', track.id, srcTrack.linkedSubs);
          // Remove all related subscriptions
          const reason = 'Source stream loss';
          for (const subId of srcTrack.linkedSubs) {
            stateStores.read('subscriptions', {id: subId}).then((sub) => {
              return stopSession(subId, sub.type, 'out', reason);
            }).catch((e) => {
              log.debug('Failed to remove related sub:', subId);
            });
          }
        }).catch((e) => {
          log.debug('Failed to remove source track:', track.id);
        });
      }
      // Update states
      const stateErrorHandler = (e) => {
        log.debug('Clean state error for removePublication:', e?.message);
      };
      stateStores.setRemove('participants', {id: owner}, {'pubs': id})
        .catch(stateErrorHandler);
      stateStores.setRemove('nodes', {id: nodeId}, {'pubs': id})
        .catch(stateErrorHandler);
      stateStores.delete('sourceTracks', {parent: id})
        .catch(stateErrorHandler);
      // Notify portal & controller
      if (!publication.info?.hidden) {
        notifyPortal({domain}, 'stream', {id: id, status: 'remove'});
      }
      const status = {
        type: 'publication',
        status: 'remove',
        data: {id}
      };
      notifyDomainHandler(domain, status);
      publishings.delete(id);
    }).catch((e) => log.debug('removePublication:', e?.message));
  };

  const updatePublication = function (updates) {
    log.debug('updatePublication:', updates);
    if (!updates.id) {
      log.warn('Updates missing id:', updates);
      return;
    }
    stateStores.read('publications', {id: updates.id})
    .then(async (publication) => {
      if (!publication) {
        log.debug('No publication to update:', updates.id);
        return;
      }
      const pubUpdate = {source: updates.source};
      await stateStores.update('publications', {id: updates.id}, pubUpdate);
      publication.source = updates.source;
      const pub = State.Publication.from(publication);
      const notification = {
        id: publication.id,
        status: 'update',
        data: {field: '.', value: pub.toSignalingFormat()},
      };
      notifyPortal({domain: publication.domain}, 'stream', notification);
      const status = {
        type: 'publication',
        status: 'update',
        data: publication
      };
      notifyDomainHandler(publication.domain, status);
    }).catch((e) => log.debug('updatePublication:', e?.message));
  };

  const addSubscription = async function (subscription) {
    log.debug('addSubscription:', subscription.id);
    if (!subscribings.has(subscription.id)) {
      log.error('No subscribing to add!');
      return;
    }
    const owner = subscribings.get(subscription.id).participant;
    const locality = subscription.locality;
    const user = await stateStores.read('participants', {id: owner});
    if (!user) {
      log.debug('Participant leave during subscribe:', subscription.id);
      throw new Error('Participant leave');
    }
    if (!(locality?.node)) {
      log.debug('No locaility for subscription:', subscription.id);
      throw new Error('Locality missing');
    }
    await saveWorkerNode(locality, subscription.type);
    try {
      await linkSubscription(subscription);
    } catch (e) {
      log.debug('Subscription link failed:', e);
      await cutSubscription(subscription);
      throw new Error('Link failed');
    }
    // Update subscription related states
    const subAdd = {'subs': subscription.id};
    await stateStores.setAdd('participants', {id: owner}, subAdd);
    await stateStores.setAdd('nodes', {id: locality.node}, subAdd);
    await stateStores.create('subscriptions', subscription);
    // Send notification
    const status = {
      type: 'subscription',
      status: 'add',
      data: subscription
    };
    notifyDomainHandler(subscription.domain, status);
    // Check if publication aborted during setup
    if (subscribings.get(subscription.id)?._aborted) {
      log.debug('Subscription aborted during setup:', subscription.id);
      removeSubscription(subscription.id); 
    }
  };

  const removeSubscription = function (id) {
    log.debug('removeSubscription:', id);
    stateStores.read('subscriptions', {id}).then(async (subscription) => {
      if (!subscription) {
        log.debug('No subscription:', id);
        return;
      }
      // Delete sub state first
      const removed = await stateStores.delete('subscriptions', {id});
      if (!removed) {
        log.debug('Subscription already removed:', id);
        return;
      }
      const owner = subscription.info.owner;
      const nodeId = subscription.locality.node;
      // Cut off
      cutSubscription(subscription)
        .catch((e) => log.debug('Cut failed:', e));
      // Update states
      const stateErrorHandler = (e) => {
        log.debug('Clean state error removeSubscription:', e?.message);
      };
      stateStores.setRemove('participants', {id: owner}, {'subs': id})
        .catch(stateErrorHandler);
      stateStores.setRemove('nodes', {id: nodeId}, {'subs': id})
        .catch(stateErrorHandler);
      // Notify controller
      const status = {
        type: 'subscription',
        status: 'remove',
        data: {id}
      };
      notifyDomainHandler(subscription.domain, status);
      subscribings.delete(id);
    });
  };

  const updateSubscription = function (updates) {
    log.debug('updateSubscription:', updates);
    if (!updates.id) {
      log.warn('Updates missing id:', updates);
      return;
    }
    const sub = subscribings.get(updates.id);
    if (sub._updating) {
      sub._nextUpdate = updates;
      return;
    }
    sub._updating = true;
    stateStores.read('subscriptions', {id: updates.id})
    .then(async (subscription) => {
      if (!subscription) {
        log.debug('No subscription to update:', updates.id);
        return;
      }
      await cutSubscription(subscription);
      subscription.sink = updates.sink;
      await linkSubscription(subscription);
      const subUpdate = {sink: updates.sink};
      const updated = await stateStores.update(
        'subscriptions', {id: updates.id}, subUpdate);
      if (!updated) {
        await cutSubscription(subscription);
        return;
      }
      const status = {
        type: 'subscription',
        status: 'update',
        data: subscription
      };
      notifyDomainHandler(subscription.domain, status);
      // Process pending update if there is
      const sub = subscribings.get(updates.id);
      if (sub) {
        sub._updating = null;
        if (sub._nextUpdate) {
          const nextUpdate = sub._nextUpdate;
          sub._nextUpdate = null;
          updateSubscription(nextUpdate);
        }
      }
    }).catch((e) => log.debug('updateSubscription:', e?.message));
  };

  // Initialize service
  (function initialize() {
    // Save service state
    stateStores.create('streamEngineNodes', {id: rpcID})
      .catch((e) => log.debug('Failed to save service state:', e));
    // Initialize controllers
    const ctrlId = rpcID;
    rpcChannel.selfId = ctrlId;
    rpcChannel.clusterId = config.cluster.name;
    // Load controller from JSON config
    for (const type in controllerConfig) {
      const config = controllerConfig[type];
      log.debug('Controller config:', type, config);
      const ControllerClass = require(config.requirePath)[config.className];
      controllers[type] = new ControllerClass(rpcChannel);
    }

    for (const type in controllers) {
      const ctrl = controllers[type];
      // Add event handlers for controllers
      ctrl.on('session-established', (id, session) => {
        log.debug('onSessionEstablished', id, session);
        if (!session.id || !controllers[session?.type]) {
          log.warn('Invalid format for established session');
          return;
        }
        if (session.source) {
          // Add publication
          addPublication(session).catch((e) => {
            stopSession(session.id, session.type, 'in', e?.message)
              .catch((e) => {
                log.debug('Failed to remove publication:', session.id);
              });
          });
        } else if (session.sink) {
          // Add subscription
          addSubscription(session).catch((e) => {
            stopSession(session.id, session.type, 'out', e?.message)
              .catch((e) => {
                log.debug('Failed to remove subscription:', session.id);
              });
          });
        } else {
          log.warn('Unknown established session:', id, session);
        }
      });
      ctrl.on('session-aborted', (id, data) => {
        log.debug('onSessionAborted', id, data);
        if (publishings.has(id)) {
          publishings.get(id)._aborted = true;
          removePublication(id);
        } else if (subscribings.has(id)) {
          subscribings.get(id)._aborted = true;
          removeSubscription(id);
        } else {
          log.warn('Unknown aborted session:', id);
        }
      });
      ctrl.on('session-updated', (id, update) => {
        log.debug('onSessionUpdated:', id, update);
        if (publishings.has(id)) {
          updatePublication(update.data);
        } else if (subscribings.has(id)) {
          updateSubscription(update.data);
        }
      });
      ctrl.on('signaling', (owner, name, data) => {
        log.debug('On signaling:', owner, name, data);
        notifyPortal({to: owner}, name, data);
      });
      // Add RPC interface for receiving progress from worker node
      if (ctrl.progressListeners) {
        log.debug('Add progress listeners RPC for type:', type);
        const listeners = ctrl.progressListeners();
        for (const method in listeners) {
          that[method] = listeners[method];
        }
      }
    }

    // RPC interface for receiving session progress from worker node
    that.onSessionProgress = function (id, name, data) {
      log.debug('progress:', id, name, data);
      if (publishings.has(id) || subscribings.has(id)) {
        // Ongoing session
        const req = publishings.get(id) || subscribings.get(id);
        controllers[req.type].onSessionProgress(id, name, data);
      } else if (controllers[data?.type]) {
        controllers[data?.type].onSessionProgress(id, name, data);
      } else { //
        log.warn('Unknown SessionProgress:', id, name, data);
      }
    };
  })();

  // Interface for portal signaling
  that.onSessionSignaling = function (req, callback) {
    log.debug('onSessionSignaling:', req);
    const type = req.type || 'webrtc';
    controllers[type].onClientTransportSignaling(req.id, req.signaling)
      .then(() => {
        callback('callback', 'ok');
      })
      .catch((e) => {
        callback('callback', 'error', e.message ? e.message : e);
      });
  };

  // Interface for user control
  that.join = function (req, callback) {
    log.debug('join:', req);
    getDomainHandler(req.domain)
      .then(async (handler) => {
        const ret = await handler.join(req);
        await addParticipant(ret.participant.id, ret.participant);
        // Build join response
        const response = {room: {id: req.domain}};
        const ppts = await stateStores.readMany(
            'participants', {domain: req.domain});
        const pubs = await stateStores.readMany(
            'publications', {domain: req.domain});
        response.room.participants = ppts.data.filter((ppt) => {
          return ppt.notifying;
        });
        response.room.streams = pubs.data.filter((plainPub) => {
          return !plainPub.info?.hidden;
        }).map((plainPub) => {
          const pub = State.Publication.from(plainPub);
          return pub.toSignalingFormat();
        }).concat(ret.streams);
        callback('callback', response);
      }).catch((err) => {
        log.info('join:', err, err.stack);
        callback('callback', 'error', err && err.message);
      });
  };
  that.leave = function (req, callback) {
    log.debug('leave:', req);
    removeParticipant(req.id)
      .then(async (user) => {
        if (user) {
          const handler = await getDomainHandler(user.domain);
          await handler.leave(req);
          callback('callback', 'ok');
        } else {
          callback('callback', 'error', 'Invalid participant leave');
        }
      }).catch((err) => {
        callback('callback', 'error', err && err.message);
      });
  };
  that.getParticipants = function (filter, callback) {
    log.debug('getParticipants:', filter, callback);
    const query = filter?.query || {};
    stateStores.readMany('participants', query).then((ret) => {
      callback('callback', ret);
    }).catch((e) => {
      log.debug('Get participants error:', e, e?.stack);
      callback('callback', 'error', e?.message);
    });
  };
  // Interfaces for publication
  that.publish = function(req, callback) {
    log.debug('publish:', req.type, req);
    const type = req.type;
    if (controllers[type]) {
      stateStores.read('participants', {id: req.participant})
      .then(async (user) => {
        if (!user) {
          throw new Error('Particitpant not joined');
        }
        req.domain = user.domain;
        const handler = await getDomainHandler(user.domain);
        const ret = await handler.publish(req);
        const id = await controllers[type].createSession('in', ret);
        if (publishings.has(ret.id)) {
          throw new Error(`Ongoing publication ${ret.id}`);
        }
        publishings.set(ret.id, ret);
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to publish:', err?.stack, err?.message);
        callback('callback', 'error', err?.message);
      });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };
  that.unpublish = function(req, callback) {
    log.debug('unpublish:', req.id);
    const pubId = req.id;
    stateStores.read('publications', {id: pubId})
    .then(async (pub) => {
      if (!pub) {
        throw new Error('Publication not found');
      }
      const reason = 'Participant terminate';
      await stopSession(pubId, pub.type, 'in', reason);
      callback('callback', 'ok');
    }).catch((err) => {
      log.warn('Failed to unpublish:', err, err.stack);
      callback('callback', 'error', err && err.message);
    });

  };
  that.getPublications = function(filter, callback) {
    log.debug('getPublications:', filter, callback);
    const query = filter?.query || {};
    stateStores.readMany('publications', query).then((ret) => {
      callback('callback', ret);
    }).catch((e) => {
      log.debug('Get publications error:', e, e?.stack);
      callback('callback', 'error', e?.message);
    });
  };
  that.streamControl = function (req, callback) {
    log.debug('streamControl:', req.type, req);
    const type = req.type;
    if (controllers[type]) {
      stateStores.read('participants', {id: req.participant})
      .then(async (user) => {
        if (!user) {
          throw new Error('Particitpant not joined');
        }
        req.domain = user.domain;
        const handler = await getDomainHandler(user.domain);
        const ret = await handler.streamControl(req);
        const id = await controllers[type].controlSession('in', ret);
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to streamControl:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };
  // Interfaces for subscription
  that.subscribe = function(req, callback) {
    log.debug('subscribe:', req.type, req);
    const type = req.type;
    if (controllers[type]) {
      stateStores.read('participants', {id: req.participant})
      .then(async (user) => {
        if (!user) {
          throw new Error('Particitpant not joined');
        }
        req.domain = user.domain;
        const handler = await getDomainHandler(user.domain);
        const ret = await handler.subscribe(req);
        if (subscribings.has(ret.id)) {
          throw new Error(`Ongoing subscription ${ret.id}`);
        }
        subscribings.set(ret.id, ret);
        const id = await controllers[type].createSession('out', ret);
        if (id !== ret.id) {
          subscribings.delete(ret.id);
          subscribings.set(id, ret);
        }
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
    stateStores.read('subscriptions', {id: subId})
    .then(async (sub) => {
      if (!sub) {
        throw new Error('Subscription not found');
      }
      const reason = 'Participant terminate';
      await stopSession(subId, sub.type, 'out', reason);
      callback('callback', 'ok');
    }).catch((err) => {
      log.warn('Failed to unsubscribe:', err, err.stack);
      callback('callback', 'error', err && err.message);
    });
  };
  that.getSubscriptions = function(filter, callback) {
    log.debug('getSubscriptions:', filter);
    const query = filter?.query || {};
    stateStores.readMany('subscriptions', query).then((ret) => {
      callback('callback', ret);
    }).catch((e) => {
      log.debug('Get subscriptions error:', e, e?.stack);
      callback('callback', 'error', e?.message);
    });
  };
  that.subscriptionControl = function (req, callback) {
    log.debug('subscriptionControl:', req.type, req);
    const type = req.type;
    if (controllers[type]) {
      stateStores.read('participants', {id: req.participant})
      .then(async (user) => {
        if (!user) {
          throw new Error('Particitpant not joined');
        }
        req.domain = user.domain;
        const handler = await getDomainHandler(user.domain);
        const ret = await handler.subscriptionControl(req);
        const id = await controllers[type].controlSession('out', ret);
        callback('callback', {id});
      }).catch((err) => {
        log.warn('Failed to subscriptionControl:', err, err.stack, err.message);
        callback('callback', 'error', err && err.message);
      });
    } else {
      callback('callback', 'error', 'Invalid type:' + type);
    }
  };
  // Get worker nodes in use
  that.getNodes = function(filter, callback) {
    log.debug('getNodes:', filter);
    const query = filter?.query || {};
    stateStores.readMany('nodes', query).then((ret) => {
      callback('callback', ret);
    }).catch((e) => {
      log.debug('Get nodes error:', e, e?.stack);
      callback('callback', 'error', e?.message);
    });
  }
  // Interfaces for processor
  that.addProcessor = function (req, callback) {
    log.debug('addProcessor:', req.type, req);
    const type = req.type;
    if (controllers[type] && controllers[type].addProcessor) {
      stateStores.read('participants', {id: req.participant})
      .then(async (user) => {
        if (!user) {
          throw new Error('Particitpant not joined');
        }
        req.domain = user.domain;
        const handler = await getDomainHandler(user.domain);
        const ret = await handler.addProcessor(req);
        const proc = await controllers[type].addProcessor(ret)
        proc.controllerType = type;
        await stateStores.create('processors', proc);
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
    const procId = req.id;
    stateStores.read('processors', {id: procId})
    .then(async (proc) => {
      if (!proc) {
        throw new Error('Processor not found');
      }
      const removed = await stateStores.delete('processors', {id: procId});
      if (removed) {
        await controllers[proc.type].removeProcessor(procId);
      }
      callback('callback', 'ok');
    }).catch((err) => {
      log.warn('Failed to removeProcessor:', err, err.stack, err.message);
      callback('callback', 'error', err && err.message);
    });
  };
  that.getProcessors = function (filter, callback) {
    log.debug('getProcessors:', filter);
    const query = filter?.query || {};
    stateStores.readMany('processors', query).then((ret) => {
      callback('callback', ret);
    }).catch((e) => {
      log.debug('Get processors error:', e, e?.stack);
      callback('callback', 'error', e?.message);
    });
  };
  // Send notifications to portal
  that.notifyPortal = function (req, callback) {
    notifyPortal(req.endpoints, req.event, req.data);
    callback('callback', 'ok');
  };

  that.close = async function () {
    for (const type in controllers) {
      controllers[type].destroy();
    }
    try {
      log.debug('clean:', rpcID);
      await stateStores.delete('streamEngineNodes', {id: rpcID});
      await stateStores.delete('scheduleMaps', {node: rpcID});
      const others = await stateStores.readMany('streamEngineNodes', {});
      if (others.total === 0) {
        // Clean all states
        await stateStores.delete('participants', {});
        await stateStores.delete('domains', {});
        await stateStores.delete('nodes', {});
        await stateStores.delete('publications', {});
        await stateStores.delete('subscriptions', {});
        await stateStores.delete('sourceTracks', {});
        await stateStores.delete('processors', {});
      }
    } catch (e) {
      log.debug('Clean state stores:', e);
    }
    stateStores.close();
  };

  return {
    API: that, // RPC API for stream mesh engine
    scheduler: scheduler // Scheduler for stream mesh engine
  };
}

let streamMeshEngine = null;

const amqper = AmqpCient();
amqper.connect(config.rabbit, function connectOk() {
  // Share amqp connection
  const schedulerAmqp = AmqpCient();
  Object.assign(schedulerAmqp, amqper);
  starter.startClient(amqper)
    .then(async (rpcClient) => {
      streamMeshEngine = streamEngine(rpcClient);
      await starter.startServer(amqper, rpcID, streamMeshEngine.API);
      log.info(rpcID + ' as rpc server ready');
      if (startScheduler) {
        await starter.startServer(schedulerAmqp, schedulerID,
          streamMeshEngine.scheduler.getAPI());
        log.info(schedulerID + ' as rpc server ready');
      }
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

async function terminator() {
  log.debug('terminate!');
  try {
    if (streamMeshEngine) {
      await streamMeshEngine.API.close();
    }
    await amqper.disconnect();
  } catch(e) {
    log.warn('AMQP disconnect failed:', e?.message);
  }
  process.exit();
}

['SIGINT', 'SIGTERM'].map(function (sig) {
  process.on(sig, terminator);
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
