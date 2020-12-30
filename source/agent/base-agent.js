/**
 * Base class for Internal Agents
 */
'use strict';

const log = require('../logger').logger.getLogger('BaseAgent');
const Connections = require('./connections');
const InternalConnectionFactory = require('./InternalConnectionFactory');

const config = global.config;

class BaseAgent {
  constructor(name, config) {
    this.name = name;
    this.clusterIp = config.clusterIp;
    this.agentId = config.agentId;
    this.rpcId = config.rpcId;
    this.networkInterfaces = config.networkInterfaces;

    this.connections = Connections();
    this.internalConnFactory = InternalConnectionFactory();
  }

  createExternalConnection(connectionId, direction, externalOpt) {
    log.error("createExternalConnection not implemented.");
    return Promise.reject('External not implemented');
  }

  /**
   *@function createInternalConnection
   *@param {string} connectionId
   *@param {string} direction
   *@param {object} internalOpt { ip: string(), minport: number(), maxport: number() }
   */
  createInternalConnection(connectionId, direction, internalOpt) {
    internalOpt.minport = global.config.internal.minport;
    internalOpt.maxport = global.config.internal.maxport;
    const portInfo = this.internalConnFactory.create(connectionId, direction, internalOpt);
    // Create internal connection always success
    return Promise.resolve({ip: global.config.internal.ip_address, port: portInfo});
  }

  /**
   *@function destroyInternalConnection
   *@param {string} connectionId
   *@param {string} direction
   */
  destroyInternalConnection(connectionId, direction) {
    this.internalConnFactory.destroy(connectionId, direction);
    return Promise.resolve();
  }

  _setupConnection(connectionId, connectionType, direction, options) {
    if (this.connections.getConnection(connectionId)) {
      return Promise.reject('Connection already exists:'+connectionId);
    }

    var conn = null;
    if (connectionType === 'internal') {
        conn = this.internalConnFactory.fetch(connectionId, direction);
        if (conn)
          conn.connect(options);
    } else if (connectionType === this.name) {
        conn = this.createExternalConnection(connectionId, direction, options);
    } else {
        log.error('Connection type invalid:' + connectionType);
    }

    if (!conn) {
        log.error('Create connection failed', connectionId, connectionType);
        return Promise.reject('Create Connection failed');
    }

    return this.connections
      .addConnection(connectionId, connectionType, options.controller, conn, direction);
  }

  _destroyConnection(connectionId, direction) {
    var conn = this.connections.getConnection(connectionId);
    return this.connections.removeConnection(connectionId)
      .then(function(ok) {
        if (conn && conn.type === 'internal') {
          this.internalConnFactory.destroy(connectionId, direction);
        } else if (conn) {
          conn.connection.close();
        }
      });
  }

  /**
    *@function publish
    *@param {string} connectionId
    *@param {string} connectionType
    *@param {object} options
    */
  publish(connectionId, connectionType, options) {
      log.debug('publish, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
      return this._setupConnection(connectionId, connectionType, 'in', options);
  }

  /**
    *@function unpublish
    *@param {string} connectionId
    */
  unpublish(connectionId) {
      log.debug('unpublish, connectionId:', connectionId);
      return this._destroyConnection(connectionId, 'in');
  }

  /**
    *@function subscribe
    *@param {string} connectionId
    *@param {string} connectionType
    *@param {object} options
    */
  subscribe(connectionId, connectionType, options) {
    log.debug('subscribe, connectionId:', connectionId, 'connectionType:', connectionType, 'options:', options);
    return this._setupConnection(connectionId, connectionType, 'out', options);
  }

  /**
    *@function unsubscribe
    *@param {string} connectionId
    */
  unsubscribe(connectionId) {
    log.debug('unsubscribe, connectionId:', connectionId);
    return this._destroyConnection(connectionId, 'out');
  }

  /**
    *@function linkup
    *@param {string} connectionId
    *@param {string} audioFrom
    *@param {string} videoFrom
    *@param {string} dataFrom
    */
  linkup(connectionId, audioFrom, videoFrom, dataFrom) {
    log.debug('linkup, connectionId:', connectionId);
    return this.connections.linkupConnection(connectionId, audioFrom, videoFrom, dataFrom);
  }

  /**
    *@function cutoff
    *@param {string} connectionId
    */
  cutoff(connectionId) {
    log.debug('cutoff, connectionId:', connectionId);
    return this.connections.cutoffConnection(connectionId);
  }

  cleanup() {
    log.debug('cleanup called');
    var cleans = connections.getIds().map((connId) => {
      var conn = connections.getConnection(connId);
      return connections.removeConnection(connId)
        .then(function(ok) {
          if (conn && conn.type === 'internal') {
            this.internalConnFactory.destroy(connId, direction);
          } else if (conn) {
            conn.connection.close();
          }
      });
    });
    return Promise.all(cleans);
  }

}

module.exports = BaseAgent;
