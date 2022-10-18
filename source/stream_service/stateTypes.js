#!/usr/bin/env node

// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

// Domain and its controller node
class Domain {
  constructor(id, node) {
    this.id = id;
    this.node = node; // Worker node of this domain
    this.portals = new Set(); // Portals related with domain
  }
  plain() {
    const plain = Object.assign({}, this);
    plain.portals = [...this.portals];
    return plain;
  }
  static from(obj) {
    const dm = new Domain();
    Object.assign(dm, obj);
    dm.portals = new Set(obj.portals)
    return dm;
  }
}

// Owner of stream sessions
class User {
  constructor(id, domain, portal) {
    this.id = id;
    this.domain = domain;
    this.portal = portal;
    this.pubs = new Set();
    this.subs = new Set();
    this.type = null;
    this.notifying = false; // Notify others join/leave
  }
  plain() {
    const plain = Object.assign({}, this);
    plain.pubs = [...this.pubs];
    plain.subs = [...this.subs];
    return plain;
  }
  static from(obj) {
    const user = new User();
    Object.assign(user, obj);
    user.pubs = new Set(obj.pubs)
    user.subs = new Set(obj.subs)
    return user;
  }
}

// Worker node for stream sessions
class WorkerNode {
  constructor(id, agent, type) {
    this.id = id;
    this.agent = agent;
    this.type = type;
    this.pubs = new Set();
    this.subs = new Set();
    this.procs = new Set();
    this.address = null;
  }
  isEmpty() {
    return this.pubs.size === 0 && this.subs.size === 0;
  }
  plain() {
    const plain = Object.assign({}, this);
    plain.pubs = [...this.pubs];
    plain.subs = [...this.subs];
    plain.procs = [...this.procs];
    return plain;
  }
  static from(obj) {
    const node = new WorkerNode();
    Object.assign(node, obj);
    node.pubs = new Set(obj.pubs)
    node.subs = new Set(obj.subs)
    node.procs = new Set(obj.procs)
    return node;
  }
}

// Stream publication
class Publication {
  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.source = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }
  addSource(trackType, trackId, format, parameters) {
    if (this.source[trackType]) {
      this.source[trackType].push({id: trackId, format, parameters});
      return true;
    } else {
      return false;
    }
  }
  removeSource(trackType, trackId) {
    if (this.source[trackType]) {
      const idx = this.source[trackType]
        .findIndex((track) => (track.id === trackId));
      if (idx >= 0) {
        this.source[trackType].splice(idx, 1);
        return true;
      }
    }
    return false;
  };
  toSignalingFormat() {
    const tracks = [];
    const optional = {
      format: [],
      parameters: {
        resolution: [],
        bitrate: [],
        framerate: [],
        keyFrameInterval: [],
      }
    };
    this.source.audio.forEach((track) => {
        tracks.push(Object.assign({type: 'audio', optional}, track));
    });
    this.source.video.forEach((track) => {
        tracks.push(Object.assign({type: 'video', optional}, track));
    });
    return {
      id: this.id,
      type: "forward",
      media: {
        tracks,
      },
      data: false,
      info: {
        owner: this.info?.owner,
        type: this.type,
        inViews: [],
        attributes: null,
      }
    };
  }
  plain() {
    const plain = Object.assign({}, this);
    return plain;
  }
  static from(obj) {
    const pub = new Publication();
    Object.assign(pub, obj);
    return pub;
  }
}

// Stream subscription
class Subscription {
  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.sink = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }
  plain() {
    const plain = Object.assign({}, this);
    return plain;
  }
  static from(obj) {
    const sub = new Subscription();
    Object.assign(sub, obj);
    return sub;
  }
}

// Stream processor
class Processor {
  constructor(id, type, info) {
    this.id = id;
    this.type = type;
    this.info = info;
    this.inputs = {
      audio: [],
      video: [],
      data: [],
    };
    this.outputs = {
      audio: [],
      video: [],
      data: [],
    };
    this.locality = null;
    this.domain = '';
  }
  plain() {
    const plain = Object.assign({}, this);
    return plain;
  }
  static from(obj) {
    const pub = new Processor();
    Object.assign(pub, obj);
    return pub;
  }
}

// Source track info of publication
class SourceTrack {
  constructor(id, parent) {
    this.id = id;
    this.parent = parent;
    this.linkedSubs = new Set(); // Set {SubscriptionId}
    this.address = null; // Same as WokerNode's
  }
  plain() {
    const plain = Object.assign({}, this);
    plain.linkedSubs = [...this.linkedSubs];
    return plain;
  }
  static from(obj) {
    const trk = new SourceTrack();
    Object.assign(trk, obj);
    trk.linkedSubs = new Set(obj.linkedSubs);
    return trk;
  }
}

exports.Domain = Domain;
exports.User = User;
exports.WorkerNode = WorkerNode;
exports.Publication = Publication;
exports.Subscription = Subscription;
exports.Processor = Processor;
exports.SourceTrack = SourceTrack;
