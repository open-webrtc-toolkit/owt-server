/*global require, logger. setInterval, clearInterval, Buffer, exports*/
var Getopt = require('node-getopt');

var spawn = require('child_process').spawn;

var config = require('./../../etc/woogeen_config');

// Configuration default values
GLOBAL.config = config || {};
GLOBAL.config.erizoAgent = GLOBAL.config.erizoAgent || {};
GLOBAL.config.erizoAgent.maxProcesses = GLOBAL.config.erizoAgent.maxProcesses || 1;
GLOBAL.config.erizoAgent.prerunProcesses = GLOBAL.config.erizoAgent.prerunProcesses || 1;

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['M' , 'maxProcesses=ARG'           , 'Stun Server URL'],
  ['P' , 'prerunProcesses=ARG'        , 'Default video Bandwidth'],
  ['U' , 'my-purpose=ARG'             , 'Purpose of this agent'],
  ['h' , 'help'                       , 'display this help']
]);

var myId = '';
var myPurpose = 'webrtc';
var myState = 2;
var reuse = true;

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
            case 'my-purpose':
                if (value === 'webrtc'
                    || value === 'rtsp'
                    || value === 'file'
                    || value === 'audio'
                    || value === 'video') {
                    myPurpose = value;
                } else {
                    process.exit(0);
                }
                break;
            default:
                GLOBAL.config.erizoAgent[prop] = value;
                break;
        }
    }
}

// Load submodules with updated config
var logger = require('./../common/logger').logger;
var rpc = require('./../common/amqper');

// Logger
var log = logger.getLogger("ErizoAgent");

var INTERVAL_TIME_KEEPALIVE = GLOBAL.config.erizoAgent.interval_time_keepAlive;
var BINDED_INTERFACE_NAME = GLOBAL.config.erizoAgent.networkInterface;

GLOBAL.config.erizo.hardwareAccelerated = !!GLOBAL.config.erizo.hardwareAccelerated;

var SEARCH_INTERVAL = 5000;

var idle_erizos = [];

var erizos = [];

var processes = {};
var loads = {}; // {erizo_id: [RoomID]}

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

var os = require("os");
var getCPULoad = function (on_result) {
    on_result(os.loadavg()[0]);
};

var getGPULoad = function (on_result) {
    //FIXME: Here is just an roughly estimation of the GPU load. A formal way is needed to get the acurate load data.
    var maxVideoNodes = 32/*FIXME: hard coded. the max count of mixers or transcoders this agent can hold.*/;
    on_result((Object.keys(loads).length)/maxVideoNodes);
};

var receiveSpeed = 0, sendSpeed  = 0;
var queryNetworkInterval = undefined;

var startQueryNetwork = function (interf, period) {
    var firstTry = true;
    queryNetworkInterval = setInterval(function () {
        require('child_process').exec('ifstat ' + interf, function (err, stdout, stderr) {
            function string2MBytes (s) {
                if (s.endsWith('K')) {
                    return Number(s.slice(0, s.length -1)) / 1024;
                } else if (s.endsWith('M')) {
                    return Number(s.slice(0, s.length -1));
                } else if (s.endsWith('G')) {
                    return Number(s.slice(0, s.length -1)) * 1024;
                } else {
                    return Number(s) / (1024 * 1024);
                }
            }
            if (firstTry) {
                firstTry = false;
                return;
            }

            if (err) {
                log.error(stderr);
            } else {
                var pattern = new RegExp('^(' + interf + '\\s+.*)$', 'gm');
                var line = stdout.toString().match(pattern)[0].split(/\s+/g);
                receiveSpeed = string2MBytes(line[5]) * 8 / period;
                sendSpeed = string2MBytes(line[7]) * 8 / period;
           }
       });
    }, period * 1000);
};

var stopQueryNetwork = function () {
    queryNetworkInterval && clearInterval(queryNetworkInterval);
    receiveSpeed = 0;
    sendSpeed = 0;
};

var getNetworkLoad = function (on_result) {
    var maxReceiveBandwidth = 100, maxSendBandwidth = 100;/*FIXME: hard coded. the max r/s bandwidth.*/;
    on_result(Math.max(receiveSpeed / maxReceiveBandwidth, sendSpeed / maxSendBandwidth));
};

var diskUsage = 0;
var queryDiskInterval = undefined;
var startQueryDisk = function (drive, period) {
    queryDiskInterval = setInterval(function () {
       var total = 1, free = 0;
       require('child_process').exec("df -k '" + drive.replace(/'/g,"'\\''") + "'", function(err, stdout, stderr) {
            if (err) {
                log.error(stderr);
            } else {
                var lines = stdout.trim().split("\n");

                var str_disk_info = lines[lines.length - 1].replace( /[\s\n\r]+/g,' ');
                var disk_info = str_disk_info.split(' ');

                total = disk_info[1];
                free = disk_info[3];
                diskUsage = 1.0 - free / total;
            }
       });
    }, period * 1000);
};

var stopQueryDisk = function () {
    queryDiskInterval && clearInterval(queryDiskInterval);
    diskUsage = 0;
};

var getStorageLoad = function (on_result) {
    on_result(diskUsage);
};

var reportLoad = function() {
    var load = 0;

    var onResult = function (load) {
        var loadPercentage = Math.round(load * 100) / 100;
        log.debug('report load:', loadPercentage);
        rpc.callRpc('nuve', 'setInfo', {id: myId, load: loadPercentage}, {callback: function (msg) {}});
    };

    switch (myPurpose) {
        case 'webrtc':
        case 'rtsp':
            getNetworkLoad(onResult);
            break;
        case 'file':
            getStorageLoad(onResult);
            break;
        case 'audio':
            getCPULoad(onResult);
            break;
        case 'video':
            GLOBAL.config.erizo.hardwareAccelerated/*FIXME: should be double checked whether hardware acceleration is actually running*/ ? getGPULoad(onResult) : getCPULoad(onResult);
            break;
        default:
            break;
    }
};

var launchErizoJS = function() {
    console.log("Running process");
    var id = guid();
    var fs = require('fs');
    var out = fs.openSync('../../logs/erizo-' + id + '.log', 'a');
    var err = fs.openSync('../../logs/erizo-' + id + '.log', 'a');
    var erizoProcess = spawn('node', ['./../erizoJS/erizoJS.js', id, myPurpose], { detached: true, stdio: [ 'ignore', out, err ] });
    erizoProcess.unref();
    erizoProcess.on('close', function (code) {
        var index = idle_erizos.indexOf(id);
        var index2 = erizos.indexOf(id);
        if (index > -1) {
            idle_erizos.splice(index, 1);
            launchErizoJS();
        } else if (index2 > -1) {
            erizos.splice(index2, 1);
        }
        delete processes[id];
        delete loads[id];
        fillErizos();
    });
    processes[id] = erizoProcess;
    loads[id] = [];
    idle_erizos.push(id);
};

var dropErizoJS = function(erizo_id, callback) {
   if (processes.hasOwnProperty(erizo_id)) {
      var process = processes[erizo_id];
      process.kill();
      delete processes[erizo_id];
      delete loads[erizo_id];

      var index = erizos.indexOf(erizo_id);
      if (index !== -1) {
          erizos.splice(index, 1);
      }

      var idleIndex = idle_erizos.indexOf(erizo_id);
      if (idleIndex !== -1) {
          idle_erizos.splice(idleIndex, 1);
      }

      callback("callback", "ok");
   }
};

var fillErizos = function() {
    for (var i = idle_erizos.length; i < GLOBAL.config.erizoAgent.prerunProcesses; i++) {
        launchErizoJS();
    }
};

var api = {

    getErizoJS: function(room_id, callback) {
        try {
            for (var i in erizos) {
                var erizo_id = erizos[i];
                if (reuse && loads[erizo_id] !== undefined && loads[erizo_id].indexOf(room_id) !== -1) {
                    callback("callback", erizo_id);
                    return;
                }
            }

            for (var i in idle_erizos) {
                var erizo_id = idle_erizos[i];
                if (reuse && loads[erizo_id] !== undefined && loads[erizo_id].indexOf(room_id) !== -1) {
                    callback("callback", erizo_id);
                    return;
                }
            }

            var erizo_id = idle_erizos.shift();
            callback("callback", erizo_id);

            loads[erizo_id].push(room_id);
            erizos.push(erizo_id);

            if (reuse && ((erizos.length + idle_erizos.length + 1) >= GLOBAL.config.erizoAgent.maxProcesses)) {
                // We re-use Erizos
                var reused_erizo = erizos.shift();
                idle_erizos.push(reused_erizo);
            } else {
                // We launch more processes
                fillErizos();
            }
        } catch (error) {
            console.log("Error in ErizoAgent:", error);
        }
    },

    recycleErizoJS: function(id, room_id, callback) {
        try {
            if (loads[id]) {
                var i = loads[id].indexOf(room_id);
                if (i !== -1) {
                    loads[id].splice(i, 1);
                }

                if (loads[id].length === 0) {
                    dropErizoJS(id, callback);
                }
            }
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

        rpc.callRpc('nuve', 'addNewErizoAgent', controller, {callback: function (msg) {

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

                rpc.callRpc('nuve', 'keepAlive', myId, {"callback": function (result) {
                    if (result === 'whoareyou') {

                        // TODO: It should try to register again in Cloud Handler. But taking into account current rooms, users, ...
                        log.info('I don`t exist in cloudHandler. I`m going to be killed');
                        clearInterval(intervarId);
                        rpc.callRpc('nuve', 'killMe', privateIP, {callback: function () {}});
                    }
                }});

            }, INTERVAL_TIME_KEEPALIVE);

            callback("callback");

        }});
    };
    addEAToCloudHandler(5);
};

(function init_env() {
    'use strict';

    reuse = (myPurpose === 'audio' || myPurpose === 'video') ? false : true;

    switch (myPurpose) {
        case 'webrtc':
        case 'rtsp':
            startQueryNetwork(BINDED_INTERFACE_NAME || Object.keys(os.networkInterfaces())[0], 5/*S*/);
            break;
        case 'file':
            startQueryDisk('/tmp', 10/*S*/);
            break;
        default:
            break;
    }
})();

var reportInterval = undefined;
rpc.connect(function () {
    'use strict';
    rpc.setPublicRPC(api);
    log.info("Adding agent to cloudhandler, purpose:", myPurpose);

    addToCloudHandler(function () {
      var rpcID = 'ErizoAgent_' + myId;
      rpc.bind(rpcID, function callback () {
        log.info('ErizoAgent rpcID:', rpcID);
        reportInterval = setInterval(reportLoad, 5 * 1000);
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
    stopQueryNetwork();
    stopQueryDisk();
    reportInterval && clearInterval(reportInterval);
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
