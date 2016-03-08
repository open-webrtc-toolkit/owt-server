/*global require, setInterval, clearInterval, GLOBAL, process*/
'use strict';
var Getopt = require('node-getopt');
var spawn = require('child_process').spawn;
var config = require('../etc/woogeen_config');

// Configuration default values
GLOBAL.config = config || {};
GLOBAL.config.erizoAgent = GLOBAL.config.erizoAgent || {};
GLOBAL.config.erizoAgent.maxProcesses = GLOBAL.config.erizoAgent.maxProcesses || 1;
GLOBAL.config.erizoAgent.prerunProcesses = GLOBAL.config.erizoAgent.prerunProcesses || 1;
GLOBAL.config.erizoAgent.publicIP = GLOBAL.config.erizoAgent.publicIP || '';

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
            case 'help':
                getopt.showHelp();
                process.exit(0);
                break;
            case 'rabbit-host':
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.host = value;
                break;
            case 'rabbit-port':
                GLOBAL.config.rabbit = GLOBAL.config.rabbit || {};
                GLOBAL.config.rabbit.port = value;
                break;
            case 'my-purpose':
                if (value === 'webrtc' ||
                    value === 'rtsp' ||
                    value === 'file' ||
                    value === 'audio' ||
                    value === 'video') {
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
var logger = require('./logger').logger;
var clusterWorker = require('./clusterWorker');
var rpc = require('./amqper');

var worker;

// Logger
var log = logger.getLogger('ErizoAgent');

const INTERVAL_TIME_REPORT = GLOBAL.config.erizoAgent.interval_time_report;
var EXTERNAL_INTERFACE_NAME = GLOBAL.config.erizoAgent.externalNetworkInterface;
var INTERNAL_INTERFACE_NAME = GLOBAL.config.erizoAgent.internalNetworkInterface;

GLOBAL.config.erizo.hardwareAccelerated = !!GLOBAL.config.erizo.hardwareAccelerated;

var idle_erizos = [];
var erizos = [];
var processes = {};
var tasks = {}; // {erizo_id: [RoomID]}

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

var os = require('os');
var getCPULoad = function (on_result) {
    on_result(os.loadavg()[0] / os.cpus().length);
};

var getGPULoad = function (on_result) {
    //FIXME: Here is just an roughly estimation of the GPU load. A formal way is needed to get the acurate load data.
    var maxVideoNodes = 32/*FIXME: hard coded. the max count of mixers or transcoders this agent can hold.*/;
    on_result((Object.keys(tasks).length)/maxVideoNodes);
};

var receiveSpeed = 0, sendSpeed  = 0;
var nqChild;

var startQueryNetwork = function (interf, period) {
    var foundIntf = undefined,
        sendUnit = 'K',
        receiveUnit = 'K';

    var nqChild = require('child_process').exec('bwm-ng -u bytes -t ' + period + ' -I ' + interf, function (err, stdout, stderr) {
        if (err) {
            log.error('error:', err)
        }
    });

    nqChild.stdout.on('data', function (data) {
        var segs = data.toString().split('\u001b');

        if (foundIntf === undefined) {
            var candidates = segs.filter(function (s) {return s.endsWith(interf + ':');});
            if (candidates.length > 0) {
                foundIntf = candidates[0].split(';')[0] + ';';
                log.warn('foundIntf:', foundIntf);
            } else {
                log.info('Not found interface yet.');
                return;
            }
        }

        var calcSpeed = function (base, unit) {
            var mul = unit === 'M' ? 1 : (unit === 'K' ? 0.001 : (unit === 'G' ? 1000 : 0));
            return base * mul;
        };

        segs.filter(function (s) { return s.startsWith(foundIntf) && !s.endsWith(interf + ':');}).forEach(function (line) {
            var exp = new RegExp('\\' + foundIntf + '(\\d+)H\\s*(\\d+\\.?\\d*)\\s?([KMG])?.*$');
            var result = line.match(exp);
            if (result === null) {
               log.info('Unrecorgnized line:', line);
               return;
            }
            var pos = Number(result[1]),
                value = Number(result[2]),
                unit = result[3];
            if (pos < 36) {
                receiveUnit = unit ? unit : receiveUnit;
                receiveSpeed = calcSpeed(value, receiveUnit) || receiveSpeed;
                log.debug('receive speed:', receiveSpeed);
            } else if (pos < 57) {
                sendUnit = unit ? unit : sendUnit;
                sendSpeed = calcSpeed(value, sendUnit) || sendSpeed;
                log.debug('send speed:', sendSpeed);
            }
        });
    });
};

var stopQueryNetwork = function () {
    nqChild && nqChild.kill('SIGINT');
    nqChild = undefined;
    receiveSpeed = 0;
    sendSpeed = 0;
};

var getNetworkLoad = function (on_result) {
    var maxReceiveBandwidth = 100, maxSendBandwidth = 100; /*FIXME: hard coded. the max r/s bandwidth.*/
    on_result(Math.max(receiveSpeed / maxReceiveBandwidth, sendSpeed / maxSendBandwidth));
};

var diskUsage = 0;
var queryDiskInterval;
var startQueryDisk = function (drive, period) {
    queryDiskInterval = setInterval(function () {
       var total = 1, free = 0;
       require('child_process').exec("df -k '" + drive.replace(/'/g,"'\\''") + "'", function(err, stdout, stderr) {
            if (err) {
                log.error(stderr);
            } else {
                var lines = stdout.trim().split('\n');

                var str_disk_info = lines[lines.length - 1].replace( /[\s\n\r]+/g,' ');
                var disk_info = str_disk_info.split(' ');

                total = disk_info[1];
                free = disk_info[3];
                diskUsage = 1.0 - free / total;
            }
       });
    }, period);
};

var stopQueryDisk = function () {
    queryDiskInterval && clearInterval(queryDiskInterval);
    diskUsage = 0;
};

var getStorageLoad = function (on_result) {
    on_result(diskUsage);
};

var reportLoad = function() {
    var onResult = function (load) {
        var loadPercentage = Math.round(load * 100) / 100;
        log.debug('report load:', loadPercentage);
        rpc.callRpc(GLOBAL.config.cluster_name || 'woogeenCluster', 'reportLoad', [myId, loadPercentage], {callback: function () {}});
    };
    var report;
    switch (myPurpose) {
        case 'webrtc':
        case 'rtsp':
            report = getNetworkLoad;
            break;
        case 'file':
            report = getStorageLoad;
            break;
        case 'audio':
            report = getCPULoad;
            break;
        case 'video':
            /*FIXME: should be double checked whether hardware acceleration is actually running*/
            report = (GLOBAL.config.erizo.hardwareAccelerated ? getGPULoad : getCPULoad);
            break;
        default:
            return;
    }
    report(onResult);
};

var launchErizoJS = function() {
    log.info('Running process');
    var id = guid();
    var fs = require('fs');
    var out = fs.openSync('../logs/erizo-' + id + '.log', 'a');
    var err = fs.openSync('../logs/erizo-' + id + '.log', 'a');
    var erizoProcess = spawn('node', ['./erizoJS.js', id, myPurpose, privateIP, publicIP], { detached: true, stdio: [ 'ignore', out, err ] });
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
        delete tasks[id];
        log.info(id, 'exit with', code);
        fillErizos();
    });
    processes[id] = erizoProcess;
    tasks[id] = [];
    idle_erizos.push(id);
};

var dropErizoJS = function(erizo_id, callback) {
   if (processes.hasOwnProperty(erizo_id)) {
      var process = processes[erizo_id];
      process.kill();
      delete processes[erizo_id];
      delete tasks[erizo_id];

      var index = erizos.indexOf(erizo_id);
      if (index !== -1) {
          erizos.splice(index, 1);
      }

      var idleIndex = idle_erizos.indexOf(erizo_id);
      if (idleIndex !== -1) {
          idle_erizos.splice(idleIndex, 1);
      }

      callback('callback', 'ok');
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
            var i, erizo_id;
            for (i in erizos) {
                erizo_id = erizos[i];
                if (reuse && tasks[erizo_id] !== undefined && tasks[erizo_id].indexOf(room_id) !== -1) {
                    callback('callback', erizo_id);
                    worker && worker.addTask(room_id);
                    return;
                }
            }

            for (i in idle_erizos) {
                erizo_id = idle_erizos[i];
                if (reuse && tasks[erizo_id] !== undefined && tasks[erizo_id].indexOf(room_id) !== -1) {
                    callback('callback', erizo_id);
                    worker && worker.addTask(room_id);
                    return;
                }
            }

            erizo_id = idle_erizos.shift();
            worker && worker.addTask(room_id);
            callback('callback', erizo_id);

            tasks[erizo_id].push(room_id);
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
            log.error('Error in ErizoAgent:', error);
        }
    },

    recycleErizoJS: function(id, room_id, callback) {
        try {
            if (tasks[id]) {
                var i = tasks[id].indexOf(room_id);
                if (i !== -1) {
                    tasks[id].splice(i, 1);
                }

                if (tasks[id].length === 0) {
                    dropErizoJS(id, callback);
                }
            }

            var stillInUse = false;
            for (var eid in tasks) {
                if (tasks[eid].indexOf(room_id) !== -1) {
                    stillInUse = true;
                    break;
                }
            }
            !stillInUse && worker && worker.removeTask(room_id);
        } catch(err) {
            log.error('Error stopping ErizoJS');
        }
    }
};

var privateIP, publicIP, clusterIP;
(function collectIPs () {
    var interfaces = require('os').networkInterfaces(),
        externalAddresses = [],
        internalAddresses = [],
        k,
        k2,
        address;

    for (k in interfaces) {
        if (interfaces.hasOwnProperty(k)) {
            for (k2 in interfaces[k]) {
                if (interfaces[k].hasOwnProperty(k2)) {
                    address = interfaces[k][k2];
                    if (address.family === 'IPv4' && !address.internal) {
                        if (k === EXTERNAL_INTERFACE_NAME || !EXTERNAL_INTERFACE_NAME) {
                            externalAddresses.push(address.address);
                        }
                        if (k === INTERNAL_INTERFACE_NAME || !INTERNAL_INTERFACE_NAME) {
                            internalAddresses.push(address.address);
                        }
                    }
                }
            }
        }
    }

    privateIP = externalAddresses[0];

    if (GLOBAL.config.erizoAgent.publicIP === '' || GLOBAL.config.erizoAgent.publicIP === undefined){
        publicIP = externalAddresses[0];
    } else {
        publicIP = GLOBAL.config.erizoAgent.publicIP;
    }

    clusterIP = internalAddresses[0];
})();

var reportInterval;
var joinCluster = function (on_ok) {
    var joinOK = function (id) {
        myId = id;
        myState = 2;
        log.info(myPurpose, 'agent join cluster ok.');
        on_ok(id);
        reportInterval = setInterval(reportLoad, INTERVAL_TIME_REPORT);
    };

    var joinFailed = function (reason) {
        log.error(myPurpose, 'agent join cluster failed. reason:', reason);
        worker && worker.quit();
    };

    var loss = function () {
        log.info(myPurpose, 'agent lost.');
    };

    var recovery = function () {
        log.info(myPurpose, 'agent recovered.');
    };

    var spec = {amqper: rpc,
                purpose: myPurpose,
                clusterName: GLOBAL.config.erizoAgent.cluster_name || 'woogeenCluster',
                joinRery: GLOBAL.config.erizoAgent.join_cluster_retry || 5,
                joinPeriod: GLOBAL.config.erizoAgent.join_cluster_period || 3000,
                recoveryPeriod: GLOBAL.config.erizoAgent.recover_cluster_period || 1000,
                keepAlivePeriod: GLOBAL.config.erizoAgent.interval_time_keepAlive || 1000,
                info: {ip: clusterIP,
                       purpose: myPurpose,
                       state: 2,
                       max_load: GLOBAL.config.erizoAgent.max_load || 0.85
                      },
                onJoinOK: joinOK,
                onJoinFailed: joinFailed,
                onLoss: loss,
                onRecovery: recovery
               };

    worker = clusterWorker(spec);
};

(function init_env() {
    reuse = (myPurpose === 'audio' || myPurpose === 'video') ? false : true;

    switch (myPurpose) {
        case 'webrtc':
        case 'rtsp':
            var internalNetworkInterface = INTERNAL_INTERFACE_NAME || Object.keys(os.networkInterfaces())[0];
            startQueryNetwork(internalNetworkInterface, 5 * 1000);
            if (EXTERNAL_INTERFACE_NAME && EXTERNAL_INTERFACE_NAME !== internalNetworkInterface) {
                startQueryNetwork(EXTERNAL_INTERFACE_NAME, 5 * 1000);
            }
            break;
        case 'file':
            startQueryDisk('/tmp', 10 * 1000);
            break;
        default:
            break;
    }
})();

rpc.connect(function () {
    rpc.setPublicRPC(api);
    log.info('Adding agent to cloudhandler, purpose:', myPurpose);

    joinCluster(function (rpcID) {
      rpc.bind(rpcID, function () {
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
    stopQueryNetwork();
    stopQueryDisk();
    worker && worker.quit();
    reportInterval && clearInterval(reportInterval);
    Object.keys(processes).map(function (k) {
        dropErizoJS(k, function (unused, status) {
            log.info('Terminate ErizoJS', k, status);
        });
    });
});
