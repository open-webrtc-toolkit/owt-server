/*global require, process*/
'use strict';

var fs = require('fs');
var toml = require('toml');
var amqper = require('./amqp_client')();
var rpcClient;
var makeRPC = require('./makeRPC').makeRPC;
var log = require('./logger').logger.getLogger('SipPortal');
var sipErizoHelper = require('./sipErizoHelper');
var helper;
var config;
try {
  config = toml.parse(fs.readFileSync('./sip_portal.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

config.rabbit = config.rabbit || {};
config.rabbit.host = config.rabbit.host || 'localhost';
config.rabbit.port = config.rabbit.port || 5672;

config.cluster = config.cluster || {};
config.cluster.name = config.cluster.name || 'woogeen-cluster';

// Allocate retry interval when allocate sip agent failed
var AllocateInterval = 5000;

var erizos = {};
// Keep rooms after initialization for re-assign agent
var roomInfo = {};

var api = {
	//define rpc calls to receive sip info change from nuve
    handleSipUpdate : function(update) {
        // Argument update: {type: (create|update|delete), room_id:room-id, sipInfo:updated-sipInfo}
        log.info("receivied api called, args", update);
        var room_id = update.room_id;
        var sipInfo = update.sipInfo;
        // Update the room sip info
        roomInfo[room_id] = sipInfo;
        if (update.type == 'delete') {
            delete roomInfo[room_id];
        }

        if (update.type === 'create') {
            createSipConnectivity(room_id, sipInfo.sipServer, sipInfo.username, sipInfo.password);
        } else if (update.type === 'update') {
            deleteSipConnectivity(room_id);
            createSipConnectivity(room_id, sipInfo.sipServer, sipInfo.username, sipInfo.password);
        } else if (update.type === 'delete') {
            deleteSipConnectivity(room_id);
        }
    }
};

var rebuildErizo = function(erizo_id) {
    // Rebuild a new one to resume the biz.
    for (var roomId in erizos) {
        if (erizos[roomId] === erizo_id) {
            // Get failed room id
            log.info('SIP agent failed, try to re-assign an agent work for room:', roomId);
            delete erizos[roomId];

            var sipInfo = roomInfo[roomId];
            if (sipInfo) {
                createSipConnectivity(roomId, sipInfo.sipServer, sipInfo.username, sipInfo.password);
            }
            break;
        }
    }
};

function initSipRooms() {
    log.info('Start to get SIP rooms');

    if (!helper) {
        helper = sipErizoHelper({
            cluster : config.cluster.name,
            rpcClient : rpcClient,
            on_broken : function (erizo_id) {
                rebuildErizo(erizo_id);
            }
        });
    }

    // Why not access mongodb directly here?
    rpcClient.remoteCall('nuve', 'getRoomsWithSIP', null, {
        callback: function (rooms) {
            // Get sip rooms here
            if (!Array.isArray(rooms)) {
                log.warn('Get sip rooms from nuve failed:', rooms);
                setTimeout(function() {
                    log.info('Try to re-get sip rooms from nuve.');
                    initSipRooms();
                }, 5000);
                return;
            }

            // Argument rooms: [{room_id:room-id, sipInfo:room's-sipInfo}...]
            log.debug('SIP rooms', rooms);
            for (var index in rooms) {
                var room_id = rooms[index].room_id;
                var sipInfo = rooms[index].sipInfo;
                // Save the room sip info
                roomInfo[room_id] = sipInfo;
                createSipConnectivity(room_id, sipInfo.sipServer, sipInfo.username, sipInfo.password);
            }
        }
    });
}

function createSipConnectivity(room_id, sip_server, sip_user, sip_passwd) {
    helper.allocateSipErizo({session: room_id, consumer: room_id}, function(erizo) {
        log.info('allocateSipErizo', erizo);
        makeRPC(
            rpcClient,
            erizo.id,
            'init',
            [{
                room_id: room_id,
                sip_server: sip_server,
                sip_user: sip_user,
                sip_passwd: sip_passwd
            }],
            function(result) {
                log.info("Sip node init successfully.");
                erizos[room_id] = erizo.id;
            }, function(reason) {
                log.error("Init sip node fail, try to de-allocate it.");
                helper.deallocateSipErizo(erizo.id);
            });
  }, function(error_reson) {
        log.error("Allocate sip Erizo fail: ", error_reson);
        log.info("Try to allocate after", AllocateInterval / 1000, "s.");
        setTimeout(function() {
            createSipConnectivity(room_id, sip_server, sip_user, sip_passwd);
        }, AllocateInterval);
  });
}

function deleteSipConnectivity(room_id) {
    var erizo_id = erizos[room_id];
    if (erizo_id) {
        log.info('deallocateSipErizo', erizo_id);
        makeRPC(
            rpcClient,
            erizo_id,
            'clean');

        helper.deallocateSipErizo(erizo_id);
        delete erizos[room_id];
    }
}

function startup () {
    amqper.connect(config.rabbit, function () {
        amqper.asRpcClient(function(rpcClnt) {
            rpcClient = rpcClnt;
            amqper.asRpcServer('sip-portal', api, function(rpcSvr) {
                log.info('sip-portal up!');
                initSipRooms();
            }, function(reason) {
                log.error('initializing sip-portal as rpc server failed, reason:', reason);
                process.exit();
            });
        }, function(reason) {
            log.error('initializing sip-portal as rpc server failed, reason:', reason);
            process.exit();
        });
    }, function(reason) {
        log.error('Sip-portal connect to rabbitMQ server failed, reason:', reason);
        process.exit();
    });
}

startup();
