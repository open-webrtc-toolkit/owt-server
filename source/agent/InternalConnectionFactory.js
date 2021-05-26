// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var internalIO = require('../internalIO/build/Release/internalIO');
var SctpIn = internalIO.SctpIn;
var SctpOut = internalIO.SctpOut;
var InternalIn = internalIO.In;
var InternalOut = internalIO.Out;

var logger = require('../logger').logger;
// Logger
var log = logger.getLogger('InternalConnectionFactory');

var quicIO;
try {
    quicIO = require('../quicIO/build/Release/quicIO.node');
} catch (e) {
    log.info('QUIC is not enabled for internal IO');
}

var cf = 'leaf_cert.pem';
var kf = 'leaf_cert.pkcs8';
var cipher;
try {
    cipher = require('../cipher');
    cipher.unlock(cipher.k, cipher.astore, function cb (err, authConfig) {
        if (!err) {
            if (authConfig.internalPass) {
                internalIO.setPassphrase(authConfig.internalPass);
            }
        } else {
            log.debug('Unlock error:', err);
        }
    });
} catch (e) {
    log.info('Failed to set secure for internal IO');
}

// Wrapper object for sctp-connection and tcp/udp-connection
function InConnection(prot, minport, maxport, ticket) {
    var conn = null;
    var protocol = "quic";

    switch (prot) {
        case 'tcp':
        case 'udp':
            protocol = prot;
            conn = new InternalIn(prot, minport, maxport, ticket);
            break;
        case 'quic':
            conn = new quicIO.in(cf, kf);
            break;
        default:
            prot = "sctp";
            conn = new SctpIn();
            break;
    }

    // Connect to another connection's listening address
    conn.connect = function(connectOpt) {
        if (protocol === "sctp") {
            Object.getPrototypeOf(conn).connect.call(conn, connectOpt.ip, connectOpt.port.udp, connectOpt.port.sctp);
        }
        // TCP/UDP in-connection never connect
    };

    // Cover the close method with null
    conn.close = null;

    // Hide the destroy method to outside
    conn.destroy = function() {
        Object.getPrototypeOf(conn).close.call(conn);
    };

    return conn;
}

// Wrapper object for sctp-connection and tcp/udp-connection
function OutConnection(prot, minport, maxport, ticket) {
    var that = {};
    var conn = null;
    var protocol = "quic";

    switch (prot) {
        case 'tcp':
        case 'udp':
        case 'quic':
            protocol = prot;
            break;
        default:
            protocol = "sctp";
            conn = new SctpOut();
            break;
    }

    // Get the listening port info
    that.getListeningPort = function() {
        if (protocol === "sctp") {
            return conn.getListeningPort();
        }
        // TCP/UDP out-connection never listen
        return 0;
    };

    // Connect to another connection's listening address
    that.connect = function(connectOpt) {
        if (protocol === "sctp") {
            conn.connect(connectOpt.ip, connectOpt.port.udp, connectOpt.port.sctp);
        } else if (protocol === 'quic') {
            conn = new quicIO.out(connectOpt.ip, connectOpt.port);
        } else {
            conn = new InternalOut(protocol, connectOpt.ip, connectOpt.port, ticket);
        }
    };

    that.receiver = function() {
        return conn;
    };

    // Hide the destroy method to outside
    that.destroy = function() {
        if (conn) {
            conn.close();
        }
    };

    return that;
}

module.exports = function() {
    var preparedSet = {};
    var that = {};

    // Create the connection and return the port info
    that.create = function (connId, direction, internalOpt) {
        // Get internal connection's arguments
        var prot = internalOpt.protocol;
        var minport = internalOpt.minport || 0;
        var maxport = internalOpt.maxport || 0;
        var ticket = internalOpt.ticket;

        if (preparedSet[connId]) {
            log.warn('Internal Connection already prepared:', connId);
            // FIXME: Correct work flow should not reach here, when a connection
            // is in use, it should not be created again. we should ensure the
            // right call sequence in upper layer.
            return preparedSet[connId].connection.getListeningPort();
        }
        var conn = (direction === 'in')
            ? InConnection(prot, minport, maxport, ticket)
            : OutConnection(prot, minport, maxport, ticket);

        preparedSet[connId] = {connection: conn, direction: direction};
        return conn.getListeningPort();
    };

    // Return the created connections
    that.fetch = function (connId, direction) {
        if (!preparedSet[connId]) {
            log.error('Internal Connection not created for fetch:', connId);
            return null;
        }
        if (preparedSet[connId].direction != direction) {
            log.error('Created connection direction not match for fetch:', connId);
            return null;
        }

        return preparedSet[connId].connection;
    };

    // Destroy the connection created
    that.destroy = function (connId, direction) {
        if (!preparedSet[connId]) {
            log.warn('Internal Connection not created for destroy:', connId);
            return false;
        }
        if (preparedSet[connId].direction != direction) {
            log.error('Created connection direction not match for destroy:', connId);
            return false;
        }

        preparedSet[connId].connection.destroy();
        delete preparedSet[connId];
    };

    return that;
};
