/*global require, logger. setInterval, clearInterval, Buffer, exports*/
var Getopt = require('node-getopt');

var spawn = require('child_process').spawn;

var config = require('./../../etc/woogeen_config');

// Configuration default values
GLOBAL.config = config || {};
GLOBAL.config.erizoAgent = GLOBAL.config.erizoAgent || {};
GLOBAL.config.erizoAgent.maxProcesses = GLOBAL.config.erizoAgent.maxProcesses || 1;
GLOBAL.config.erizoAgent.prerunProcesses = GLOBAL.config.erizoAgent.prerunProcesses === undefined ? 1 : GLOBAL.config.erizoAgent.prerunProcesses;

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['M' , 'maxProcesses=ARG'           , 'Stun Server URL'],
  ['P' , 'prerunProcesses=ARG'        , 'Default video Bandwidth'],
  ['h' , 'help'                       , 'display this help']
]);

var myId = '';
var myPurpose = 'general-use';
var myState = 2;

var opt = getopt.parse(process.argv.slice(2));

for (var prop in opt.options) {
    if (opt.options.hasOwnProperty(prop)) {
        var value = opt.options[prop];
        switch (prop) {
            case "help":
                getopt.showHelp();
                process.exit(0);
                break;
            case "rabbit-host":
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.host = value;
                break;
            case "rabbit-port":
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.port = value;
                break;
            case "logging-config-file":
                GLOBAL.config.logger = GLOBAL.config.logger || {};
                GLOBAL.config.logger.config_file = value;
                break;
            case 'my-id':
                myId = value;
                break;
            default:
                GLOBAL.config.erizoAgent[prop] = value;
                break;
        }
    }
}

// Load submodules with updated config
var logger = require('./../common/logger').logger;
var amqper = require('./../common/amqper');

// Logger
var log = logger.getLogger("ErizoAgent");

var INTERVAL_TIME_KEEPALIVE = GLOBAL.config.erizoAgent.interval_time_keepAlive;
var BINDED_INTERFACE_NAME = GLOBAL.config.erizoAgent.networkInterface;

var childs = [];

var SEARCH_INTERVAL = 5000;

var idle_erizos = [];

var erizos = [];

var processes = {};

var guid = (function() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
               .toString(16)
               .substring(1);
  }
  return function() {
    return s4() + s4() + '-' + s4() + '-' + s4() + '-' +
           s4() + '-' + s4() + s4() + s4();
  };
})();

var saveChild = function(id) {
    childs.push(id);
};

var removeChild = function(id) {
    childs.push(id);
};

var launchErizoJS = function() {
    console.log("Running process");
    var id = guid();
    var fs = require('fs');
    var out = fs.openSync('../../logs/erizo-' + id + '.log', 'a');
    var err = fs.openSync('../../logs/erizo-' + id + '.log', 'a');
    var erizoProcess = spawn('node', ['./../erizoJS/erizoJS.js', id], { detached: true, stdio: [ 'ignore', out, err ] });
    erizoProcess.unref();
    erizoProcess.on('close', function (code) {
        var index = idle_erizos.indexOf(id);
        var index2 = erizos.indexOf(id);
        if (index > -1) {
            idle_erizos.splice(index, 1);
        } else if (index2 > -1) {
            erizos.splice(index2, 1);
        }
        delete processes[id];
        fillErizos();
    });
    processes[id] = erizoProcess;
    idle_erizos.push(id);
};

var dropErizoJS = function(erizo_id, callback) {
    if (processes.hasOwnProperty(erizo_id)) {
        var process = processes[erizo_id];
        process.kill();
        delete processes[erizo_id];

        var index = erizos.indexOf(erizo_id);
        if (index !== -1) {
            erizos.splice(index, 1);
        }

        var idleIndex = idle_erizos.indexOf(erizo_id);
        if (idleIndex !== -1) {
            idle_erizos.splice(idleIndex, 1);
        }
        return callback('callback', 'ok');
    }
    callback('callback', 'not found');
};

var fillErizos = function() {
    if (erizos.length + idle_erizos.length < GLOBAL.config.erizoAgent.maxProcesses) {
        if (idle_erizos.length < GLOBAL.config.erizoAgent.prerunProcesses) {
            launchErizoJS();
            fillErizos();
        }
    }
};

var getErizo = function () {

    var erizo_id = idle_erizos.shift();

    if (!erizo_id) {
        if (erizos.length < GLOBAL.config.erizoAgent.maxProcesses) {
            launchErizoJS();
            return getErizo();
        } else {
            erizo_id = erizos.shift();
        }
    }

    return erizo_id;
};

var api = {
    createErizoJS: function(callback) {
        try {
            var erizo_id = getErizo();
            callback("callback", erizo_id);

            erizos.push(erizo_id);
            fillErizos();

        } catch (error) {
            console.log("Error in ErizoAgent:", error);
        }
    },
    deleteErizoJS: function(id, callback) {
        try {
            dropErizoJS(id, callback);
        } catch(err) {
            log.error("Error stopping ErizoJS");
        }
    }
};

var privateIP;

var addToCloudHandler = function (callback) {
    "use strict";

    var interfaces = require('os').networkInterfaces(),
        addresses = [],
        k,
        k2,
        address;


    for (k in interfaces) {
        if (interfaces.hasOwnProperty(k)) {
            for (k2 in interfaces[k]) {
                if (interfaces[k].hasOwnProperty(k2)) {
                    address = interfaces[k][k2];
                    if (address.family === 'IPv4' && !address.internal) {
                        if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
                            addresses.push(address.address);
                        }
                    }
                }
            }
        }
    }

    privateIP = addresses[0];


    var addEAToCloudHandler = function(attempt) {
        if (attempt <= 0) {
            return;
        }

        var controller = {
            ip: privateIP,
            purpose: myPurpose
        };

        amqper.callRpc('nuve', 'addNewErizoAgent', controller, {callback: function (msg) {

            if (msg === 'timeout') {
                log.info('CloudHandler does not respond');

                // We'll try it more!
                setTimeout(function() {
                    attempt = attempt - 1;
                    addEAToCloudHandler(attempt);
                }, 3000);
                return;
            }
            if (msg == 'error') {
                log.info('Error in communication with cloudProvider');
            }

            privateIP = msg.privateIP;
            myId = msg.id;
            myState = 2;

            var intervarId = setInterval(function () {

                amqper.callRpc('nuve', 'keepAlive', myId, {"callback": function (result) {
                    if (result === 'whoareyou') {

                        // TODO: It should try to register again in Cloud Handler. But taking into account current rooms, users, ...
                        log.info('I don`t exist in cloudHandler. I`m going to be killed');
                        clearInterval(intervarId);
                        amqper.callRpc('nuve', 'killMe', privateIP, {callback: function () {}});
                    }
                }});

            }, INTERVAL_TIME_KEEPALIVE);

            callback("callback");

        }});
    };
    addEAToCloudHandler(5);
};

amqper.connect(function () {
    'use strict';
    amqper.setPublicRPC(api);
    log.info("Adding agent to cloudhandler, purpose:", myPurpose);
    addToCloudHandler(function () {
      var rpcID = 'ErizoAgent_' + myId;
      amqper.bind(rpcID, function callback () {
        log.info('ErizoAgent rpcID:', rpcID);
      });
      fillErizos();
    });
});

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        process.exit();
    });
});

process.on('exit', function () {
    Object.keys(processes).map(function (k) {
        dropErizoJS(k, function (unused, status) {
            log.info('Terminate ErizoJS', k, status);
        });
    });
});

/*
setInterval(function() {
    var search = spawn("ps", ['-aef']);

    search.stdout.on('data', function(data) {

    });

    search.on('close', function (code) {

    });
}, SEARCH_INTERVAL);
*/
