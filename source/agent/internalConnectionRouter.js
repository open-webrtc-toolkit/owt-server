// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/**
 * Router class for internal connections between agents
 */
'use strict';

const config = global.config;

const log = require('../logger').logger.getLogger('InternalConnectionRouter');
const Connections = require('./connections');
const internalIO = require('../internalIO/build/Release/internalIO');

const {InternalServer, InternalClient} = internalIO;

const setSecurePromise = new Promise(function (resolve) {
  try {
    const cipher = require('../cipher');
    cipher.unlock(cipher.k, cipher.astore, function cb (err, authConfig) {
      if (!err) {
        if (authConfig.internalPass) {
          log.debug('Secure enabled for internal IO');
          internalIO.setPassphrase(authConfig.internalPass);
        }
      } else {
        log.debug('Unlock error:', err);
      }
      resolve();
    });
  } catch (e) {
    log.debug('No secure for internal IO');
    resolve();
  }
});


/*
 * Wrapper class for addon FrameDestination object
 * Manage linked sources
 */
class StreamDestination {
  /*
   * id: string
   * conn: object {
   *   receiver: [Function],
   * }
   */
  constructor(id, conn) {
    this.id = id;
    this.conn = conn;
    this.srcs = {
      'audio': new Map(), // id => StreamSource
      'video': new Map(), // id => StreamSource
      'data': new Map(), // id => StreamSource
    };
  }

  /*
   * Cutoff all the linked StreamSource
   */
  clearSources() {
    for (const [track, srcMap] of this.srcs) {
      for (const [/*srcId*/, src] of srcMap) {
        src.removeDestination(track, this);
      }
      srcMap.clear();
    }
  }

  _addSource(track, src) {
    if (this.srcs[track] && !this.srcs[track].has(src.id)) {
      this.srcs[track].set(src.id, src);
    }
  }

  _removeSource(track, src) {
    if (this.srcs[track] && this.srcs[track].has(src.id)) {
      this.srcs[track].delete(src.id);
    }
  }
}

/*
 * Wrapper class for addon FrameSource object
 * Manage linked destinations
 */
class StreamSource {
  /*
   * id: string
   * conn: object { // Addon object wrapper
   *   addDestination: [Function],
   *   removeDestination: [Function]
   * }
   */
  constructor(id, conn) {
    this.id = id;
    this.conn = conn;
    this.dests = {
      'audio': new Map(), // id => StreamSink
      'video': new Map(), // id => StreamSink
      'data': new Map(), // id => StreamSink
    };
  }

  addDestination(track, sink) {
    if (!this.dests[track].has(sink.id)) {
      this.dests[track].set(sink.id, sink);
      let isNanObject = false;
      if (sink instanceof QuicTransportStreamPipeline) {
          isNanObject = true;
      }
      this.conn.addDestination(track, sink.conn, isNanObject);
      sink._addSource(track, this);
    }
  }

  removeDestination(track, sink) {
    if (this.dests[track].has(sink.id)) {
      this.dests[track].delete(sink.id);
      this.conn.removeDestination(track, sink.conn);
      sink._removeSource(track, this);
    }
  }

  clearDestinations() {
    for (const [track, destMap] of this.dests) {
      for (const [/*destId*/, sink] of destMap) {
        this.conn.removeDestination(track, sink.conn);
        sink._removeSource(track, this);
      }
      destMap.clear();
    }
  }
}

/*
 * Router for internal connection,
 * represents a router which can establish internal connections
 * with remote node for local stream(source)/subscription(destination).
 */
class InternalConnectionRouter {
  /*
   * @param {string} protocol Protocol for internal connection (tcp/quic)
   * @param {number} minport Internal server listening min port
   * @param {number} maxport Internal server listening max port
   */
  constructor({protocol, minport, maxport}) {
    this.protocol = protocol;
    this.connections = Connections();

    this.internalServer = {
      addSource: () => log.warn('Server is not initialized'),
      removeSource: () => log.warn('Server is not initialized'),
      internalPort: 0,
    };
    setSecurePromise.then(() => {
      this.internalServer = new InternalServer(
        protocol, minport, maxport, (a, b) => {
          log.debug('server stat:', a, b);
      });
      this.internalPort = this.internalServer.getListeningPort();
    });
    this.remoteStreams = new Map(); // id => Set {string}
  }

  /*
   * @param {string} id ID for source connection (stream)
   * @param {string} type Type description for connection
   * @param {FrameSource} source Wrapper class for FrameSource
   */
  addLocalSource(id, type, source) {
    const isNativeSource = (type === 'quic' || type === 'mediabridge');
    this.internalServer.addSource(id, source, isNativeSource);
    return this.connections.addConnection(id, type, '', source, 'in');
  }

  /*
   * @param {string} id ID for destination connection (subscription)
   * @param {string} type Type description for connection
   * @param {FrameSource} source Wrapper class for FrameDestination
   */
  addLocalDestination(id, type, dest) {
    log.debug('addLocalDestination:', id, type);
    return this.connections.addConnection(id, type, '', dest, 'out');
  }

  // Remove a connection with ID (either source or destination)
  removeConnection(id) {
    log.debug('removeConnection:', id);
    const conn = this.connections.getConnection(id);
    if (conn) {
      if (conn.direction === 'in') {
        return this.removeLocalSource(id);
      } else if (conn.direction === 'out') {
        return this.removeLocalDestination(id);
      } else {
        return Promise.reject('Unexpected direction '+conn.direction);
      }
    } else {
      return Promise.reject('Cannot find connection.');
    }
  }

  removeLocalSource(id) {
    log.debug('removeLocalSource:', id);
    this.internalServer.removeSource(id);
    return this.connections.removeConnection(id);
  }

  removeLocalDestination(id) {
    log.debug('removeLocalDestination:', id);
    return this.connections.removeConnection(id);
  }

  hasConnection(id) {
    return !!this.connections.getConnection(id);
  }

  getConnection(id) {
    const connObj = this.connections.getConnection(id);
    return connObj ? connObj.connection : null;
  }

  /*
   * Get or create an internal connection for remote source(stream)
   * @param {string} id
   * @param {string} ip
   * @param {number} port
   * @param {function} onStat(stat) Callback for internal connection
   */
  getOrCreateRemoteSource({id, ip, port}, onStat) {
    if (this.connections.getConnection(id)) {
      let conn = this.connections.getConnection(id).connection;
      return conn;
    } else if (!this.remoteStreams.has(id) && ip && port) {
      log.debug('RemoteSource created:', id, ip, port);
      let conn = new InternalClient(id, this.protocol, ip, port, onStat);
      conn.receiver = () => conn;
      this.connections.addConnection(id, 'internal', '', conn, 'in');
      this.remoteStreams.set(id, new Set());
      return conn;
    } else {
      log.info('Failed to get remote source:', id, ip, port);
      return null;
    }
  }

  // Destroy an internal connection for remote source with ID
  destroyRemoteSource(id) {
    if (this.remoteStreams.has(id)) {
      this.connections.cutoffConnection(id);
      this.connections.removeConnection(id);
      this.remoteStreams.delete(id);
    }
  }

  /*
   * This function linkup localDestination with ID `dstId` with `from`,
   * `from` can either be localSource or remoteSource.
   * SourceInfo = {id: 'string', ip: 'string', port: 'number'}
   * from = {audio: SourceInfo, video: SourceInfo, data: SourceInfo}
   */
  linkup(dstId, from) {
    log.debug('linkup:', dstId, from);
    let audioFrom, videoFrom, dataFrom;
    for(let [type, stream] of Object.entries(from)) {
      if (!stream.id) {
        continue;
      }
      if (!this.connections.getConnection(stream.id)) {
        this.getOrCreateRemoteSource(stream, (stat) => {
          log.debug('Remote source stat:', stream.id, stat);
          if (stat === 'disconnected') {
            this.destroyRemoteSource(stream.id);
          }
        });
        if (!this.remoteStreams.has(stream.id)) {
          log.warn('Remote stream never added:', stream.id);
          return Promise.reject('Invalid remote from:' + type + ', ' + stream.id);
        }
        // Save destination mapping for remote stream
        // TODO: destroy remote streams when needed
        this.remoteStreams.get(stream.id).add(dstId);
      }
    }
    if (from.audio) audioFrom = from.audio.id;
    if (from.video) videoFrom = from.video.id;
    if (from.data) dataFrom = from.data.id;

    return this.connections.linkupConnection(
      dstId, audioFrom, videoFrom, dataFrom);
  }

  cutoff(id) {
    // TODO: update remoteStreams map
    return this.connections.cutoffConnection(id);
  }

  clear() {
    const connIds = this.connections.getIds();
    for (const id of connIds) {
      const conn = this.connections.getConnection(id);
      if (conn.type === 'internal') {
        this.destroyRemoteSource(id);
      } else {
        this.removeConnection(id);
      }
    }
  }

  onFaultDetected(message)
  {
      log.error('Internal connection router detected error ' + JSON.stringify(message));
  }

}

exports.InternalConnectionRouter = InternalConnectionRouter;
