/*global require, module, GLOBAL*/
'use strict';

var logger = require('./logger').logger;
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
                        connections[connectionId].connection.removeDestination('audio', dest);
                        connections[connection_id].audioFrom = undefined;
                    }

                    if (connections[connection_id].videoFrom === connectionId) {
                        log.debug('remove video subscription:', connections[connection_id].videoFrom);
                        var dest = connections[connection_id].connection.receiver('video');
                        connections[connectionId].connection.removeDestination('video', dest);
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
                connections[connectionId].audioFrom = undefined;
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


    that.linkupConnection = function (connectionId, audioFrom, videoFrom) {
        log.debug('linkup, connectionId:', connectionId, ', audioFrom:', audioFrom, ', videoFrom:', videoFrom);
        if (!connectionId || !connections[connectionId]) {
            log.error('Subscription does not exist:' + connectionId);
            return Promise.reject('Subscription does not exist:' + connectionId);
        }

        if (audioFrom && connections[audioFrom] === undefined) {
            log.error('Audio stream does not exist:' + audioFrom);
            return Promise.reject({type: 'failed', reason: 'Audio stream does not exist:' + audioFrom});
        }

        if (videoFrom && connections[videoFrom] === undefined) {
            log.error('Video stream does not exist:' + videoFrom);
            return Promise.reject({type: 'failed', reason: 'Video stream does not exist:' + videoFrom});
        }

        var conn = connections[connectionId];

        if (audioFrom) {
            var dest = conn.connection.receiver('audio');
            connections[audioFrom].connection.addDestination('audio', dest);
            connections[connectionId].audioFrom = audioFrom;
        }

        if (videoFrom) {
            var dest = conn.connection.receiver('video');
            connections[videoFrom].connection.addDestination('video', dest);
            connections[connectionId].videoFrom = videoFrom;
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
            log.info('Connection does NOT exist:' + connectionId);
            return Promise.reject('Connection does NOT exist:' + connectionId);
        }
    };

    that.getConnection = function (connectionId) {
        return connections[connectionId];
    };

    that.onFaultDetected = function (message) {
        if (message.purpose === 'session' || message.purpose === 'portal') {
            for (var conn_id in connections) {
                if ((message.purpose === 'session' &&
                     connections[conn_id].type === 'internal' &&
                     ((message.type === 'node' && message.id === connections[conn_id].controller) || (message.type === 'worker' && connections[conn_id].controller.startsWith(message.id)))) ||
                    (message.purpose === 'portal' &&
                     message.type === 'worker' &&
                     connections[conn_id].controller === message.id)){
                    log.error('Fault detected on controller (type:', message.type, 'id:', message.id, ') of connection:', conn_id , 'and remove it');
                    that.removeConnection(conn_id);
                }
            }
        }
    };

    return that;
};
