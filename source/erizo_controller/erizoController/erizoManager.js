/*global require, setInterval, module*/
'use strict';

var logger = require('./../common/logger').logger;
var makeRPC = require('./../common/makeRPC').makeRPC;

// Logger
var log = logger.getLogger('ErizoManager');

module.exports = function (spec) {

    var that = {},
         /*{ErizoId: {agent: AgentId, purpose: 'webrtc' | 'sip' | 'rtsp' | 'file' | 'audio&mixing' | 'video&mixing'}*/
        erizos = {};

    var amqper = spec.amqper,
        room_id = spec.room_id,
        on_erizo_broken = spec.on_broken;

    var callbackFor = function(erizo_id) {
        return function(ok) {
            if (ok !== true && erizos[erizo_id] !== undefined) {
                log.info('ErizoJS[', erizo_id, '] check alive timeout!');
                on_erizo_broken(erizo_id, erizos[erizo_id].purpose);
                makeRPC(amqper,
                        'ErizoAgent_' + erizos[erizo_id].agent,
                        'recycleErizoJS',
                        [erizo_id],
                        function(){
                            delete erizos[erizo_id];
                        },
                        function(){
                            delete erizos[erizo_id];
                        });
            }
        };
    };

    var KEEPALIVE_INTERVAL = 12*1000;
    var sendKeepAlive = function() {
        for (var erizo_id in erizos) {
            makeRPC(amqper,
                    'ErizoJS_' + erizo_id,
                    'keepAlive',
                    [],
                    callbackFor(erizo_id),
                    callbackFor(erizo_id));
        }
    };

    setInterval(sendKeepAlive, KEEPALIVE_INTERVAL);

    var getAgent = function (purpose, for_whom, on_ok, on_failed) {
        makeRPC(amqper,
                'nuve',
                'getErizoAgent',
                {purpose: purpose, for_whom: for_whom},
                function (result) {
                    on_ok({id: result.id, addr: result.ip});
                },
                on_failed);
        //setTimeout(function () {on_ok({id: (purpose === 'mixing' ? exports.mixAgentId : exports.accessAgentId), addr: 'localhost'});}, 10);
    };

    that.allocateErizo = function (purpose, for_whom, on_ok, on_failed) {
        getAgent(purpose, for_whom, function(result) {
            makeRPC(amqper,
                    'ErizoAgent_' + result.id,
                    'getErizoJS',
                    [room_id],
                    function(erizo_id) {
                        erizos[erizo_id] = {agent: result.id, purpose: purpose};
                        on_ok({id: erizo_id, addr: result.addr});
                    },
                    on_failed);
        }, function () {
            on_failed('Failed to get accessing agent.');
        });
    };

    that.deallocateErizo = function (erizo_id) {
        if (erizos[erizo_id] !== undefined) {
            makeRPC(amqper,
                    'ErizoAgent_' + erizos[erizo_id].agent,
                    'recycleErizoJS',
                    [erizo_id, room_id],
                    function() {
                        delete erizos[erizo_id];
                    },
                    function() {
                        delete erizos[erizo_id];
                    });
        }
    };

    return that;
};
