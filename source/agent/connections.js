// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('../logger').logger;
// Logger
var log = logger.getLogger('Connections');

module.exports = function Connections () {
    var that = {},
        /*{ConnectionID: {type: 'webrtc' | 'avstream' | 'recording' | 'internal',
                          direction: 'in' | 'out',
                          audioFrom: ConnectionID | undefined,
                          videoFrom: ConnectionID | undefined,
                          connnection: WebRtcConnection | InternalOut | RTSPConnectionOut
                         }
          }
        */
        connections = {};

    var cutOffFrom = function (connectionId) {
        log.debug('remove subscriptions from connection:', connectionId);
        if (connections[connectionId] && connections[connectionId].direction === 'in') {
            for (var connection_id in connections) {
                if (connections[connection_id].direction === 'out') {
                    if (connections[connection_id].audioFrom === connectionId) {
                        log.debug('remove audio subscription:', connections[connection_id].audioFrom);
                        var dest = connections[connection_id].connection.receiver('audio');
                        if (dest) {
                            connections[connectionId].connection.removeDestination('audio', dest);
                        }
                        connections[connection_id].audioFrom = undefined;
                    }

                    if (connections[connection_id].videoFrom === connectionId) {
                        log.debug('remove video subscription:', connections[connection_id].videoFrom);
                        var dest = connections[connection_id].connection.receiver('video');
                        if (dest) {
                            connections[connectionId].connection.removeDestination('video', dest);
                        }
                        connections[connection_id].videoFrom = undefined;
                    }
                }
            }
        }
    };

    var cutOffTo = function (connectionId) {
        log.debug('remove subscription to connection:', connectionId);
        if (connections[connectionId] && connections[connectionId].direction === 'out') {
            var audioFrom = connections[connectionId].audioFrom,
                videoFrom = connections[connectionId].videoFrom;

            if (audioFrom && connections[audioFrom] && connections[audioFrom].direction === 'in') {
                log.debug('remove audio from:', audioFrom);
                var dest = connections[connectionId].connection.receiver('audio');
                connections[audioFrom].connection.removeDestination('audio', dest);
                connections[connectionId].audioFrom = undefined;
            }

            if (videoFrom && connections[videoFrom] && connections[videoFrom].direction === 'in') {
                log.debug('remove video from:', videoFrom);
                var dest = connections[connectionId].connection.receiver('video');
                connections[videoFrom].connection.removeDestination('video', dest);
                connections[connectionId].videoFrom = undefined;
            }
        }
    };

    that.addConnection =  function (connectionId, connectionType, connectionController, conn, direction) {
        log.debug('Add connection:', connectionId, connectionType, connectionController);
        if (connections[connectionId]) {
            log.error('Connection already exists:'+connectionId);
            return Promise.reject({type: 'failed', reason: 'Connection already exists:'+connectionId});
        }

        connections[connectionId] = {
            type: connectionType,
            direction: direction,
            audioFrom: undefined,
            videoFrom: undefined,
            connection: conn,
            controller: connectionController
        };

        return Promise.resolve('ok');
    };

    that.removeConnection = function (connectionId) {
        log.debug('Remove connection:', connectionId);
        if (connections[connectionId] !== undefined) {
            if (connections[connectionId].direction === 'in') {
                cutOffFrom(connectionId);
            } else {
                cutOffTo(connectionId);
            }
            delete connections[connectionId];
        } else {
            log.info('Connection does NOT exist:' + connectionId);
            return Promise.reject('Connection does NOT exist:' + connectionId);
        }

        return Promise.resolve('ok');
    };

    that.linkupConnection = function(connectionId, audioFrom, videoFrom, dataFrom) {
        log.debug('linkup, connectionId:', connectionId, ', audioFrom:', audioFrom, ', videoFrom:', videoFrom, ', dataFrom: ', dataFrom);
        if (!connectionId || !connections[connectionId]) {
            log.error('Subscription does not exist:' + connectionId);
            return Promise.reject('Subscription does not exist:' + connectionId);
        }

        const conn = connections[connectionId];

        for (const [name, from] of new Map([['audio', audioFrom], ['video', videoFrom], ['data', dataFrom]])) {
            if (from && !connections[from]) {
                log.error(name + ' stream does not exist:' + from);
                return Promise.reject({ type : 'failed', reason : name + ' stream does not exist:' + from });
            }
            if (from) {
                const dest = conn.connection.receiver(name);
                if (!dest) {
                    return Promise.reject({ type : 'failed', reason : 'Destination connection(' + name + ') is not ready' });
                }
                connections[from].connection.addDestination(name, dest);
                connections[connectionId][name + 'From'] = from;
            }
        }
        return Promise.resolve('ok');
    };

    that.cutoffConnection = function (connectionId) {
        log.debug('cutoff, connectionId:', connectionId);
        if (connections[connectionId]) {
            if (connections[connectionId].direction === 'in') {
                cutOffFrom(connectionId);
            } else {
                cutOffTo(connectionId);
            }
            return Promise.resolve('ok');
        } else {
            log.debug('Connection does NOT exist:' + connectionId);
            return Promise.reject('Connection does NOT exist:' + connectionId);
        }
    };

    that.getConnection = function (connectionId) {
        return connections[connectionId];
    };

    that.getIds = function () {
        return Object.keys(connections);
    };

    that.onFaultDetected = function (message) {
        if (message.purpose === 'conference') {
            for (var conn_id in connections) {
                if ((message.type === 'node' && message.id === connections[conn_id].controller) || (message.type === 'worker' && connections[conn_id].controller.startsWith(message.id))) {
                    log.error('Fault detected on controller (type:', message.type, 'id:', message.id, ') of connection:', conn_id , 'and remove it');
                    that.removeConnection(conn_id);
                }
            }
        }
    };

    return that;
};
