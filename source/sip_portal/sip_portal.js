/*global require, process*/
'use strict';

var fs = require('fs');
var toml = require('toml');
var amqper = require('./amqper');
var makeRPC = require('./makeRPC').makeRPC;
var log = require('./logger').logger.getLogger('SipPortal');
var sipErizoHelper = require('./sipErizoHelper');

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

var rebuildErizo = function(erizo_id) {
  //TODO: clean the old erizo and rebuild a new one to resume the biz.
};

var sipErizoHelper = sipErizoHelper({cluster : config.cluster.name,
                                     amqper : amqper,
                                     on_broken : function (erizo_id) {
        rebuildErizo(erizo_id);
    }
});

var api = {
	//define rpc calls to receive sip info change from nuve
    handleSipUpdate : function(update) {
        // Argument update: {type: (create|update|delete), room_id:room-id, sipInfo:updated-sipInfo}
        log.info("receivied api called, args", update);
        var room_id = update.room_id;
        var sipInfo = update.sipInfo;
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

var erizos = {};

function initSipRooms() {
    log.info('Start to get SIP rooms');
    amqper.callRpc('nuve', 'getRoomsWithSIP', null, {
        callback: function (rooms) {
            // get sip rooms here
            // Argument rooms: [{room_id:room-id, sipInfo:room's-sipInfo}...]
            log.debug('SIP rooms', rooms);
            for (var index in rooms) {
                var room_id = rooms[index].room_id;
                var sipInfo = rooms[index].sipInfo;
                createSipConnectivity(room_id, sipInfo.sipServer, sipInfo.username, sipInfo.password);
            }
        }
    });
}

function createSipConnectivity(room_id, sip_server, sip_user, sip_passwd) {
    sipErizoHelper.allocateSipErizo(room_id, function(erizo) {
        log.info('allocateSipErizo', erizo);
        makeRPC(
            amqper,
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
                sipErizoHelper.deallocateSipErizo(erizo.id);
            });
  }, function(error_reson) {
      log.error("Allocate sip Erizo fail: ", error_reson);
  });
}

function deleteSipConnectivity(room_id) {
    var erizo_id = erizos[room_id];
    if (erizo_id) {
        log.info('deallocateSipErizo', erizo_id);
        makeRPC(
            amqper,
            erizo_id,
            'clean');

        sipErizoHelper.deallocateSipErizo(erizo_id);
        delete erizos[room_id];
    }
}

function startup () {
    amqper.connect(config.rabbit, function () {
        amqper.bind('sip-portal', function () {
            log.info('sip-portal up!');
            amqper.setPublicRPC(api);
            initSipRooms();
        });
    });
}

startup();
