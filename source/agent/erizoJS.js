/*global require, GLOBAL, process*/
'use strict';
var Getopt = require('node-getopt');
var fs = require('fs');
var toml = require('toml');
var log = require('./logger').logger.getLogger('ErizoJS');

var config;
try {
  config = toml.parse(fs.readFileSync('./agent.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}


GLOBAL.config = config || {};
GLOBAL.config.webrtc = GLOBAL.config.webrtc || {};
GLOBAL.config.webrtc.stunserver = GLOBAL.config.webrtc.stunserver || '';
GLOBAL.config.webrtc.stunport = GLOBAL.config.webrtc.stunport || 0;
GLOBAL.config.webrtc.minport = GLOBAL.config.webrtc.minport || 0;
GLOBAL.config.webrtc.maxport = GLOBAL.config.webrtc.maxport || 0;
GLOBAL.config.webrtc.keystorePath = GLOBAL.config.webrtc.keystorePath || '';

GLOBAL.config.video = GLOBAL.config.video || {};
GLOBAL.config.video.hardwareAccelerated = !!GLOBAL.config.video.hardwareAccelerated;
GLOBAL.config.video.openh264Enabled = !!GLOBAL.config.video.openh264Enabled;
GLOBAL.config.video.yamiEnabled = !!GLOBAL.config.video.yamiEnabled;

GLOBAL.config.avatar = GLOBAL.config.avatar || {};

GLOBAL.config.audio = GLOBAL.config.audio || {};

GLOBAL.config.recording = GLOBAL.config.recording || {};
GLOBAL.config.recording.path = GLOBAL.config.recording.path || '/tmp';

GLOBAL.config.internal = GLOBAL.config.internal || {};
GLOBAL.config.internal.protocol = GLOBAL.config.internal.protocol || 'sctp';
GLOBAL.config.internal.minport = GLOBAL.config.internal.minport || 0;
GLOBAL.config.internal.maxport = GLOBAL.config.internal.maxport || 0;

// Parse command line arguments
var getopt = new Getopt([
  ['r' , 'rabbit-host=ARG'            , 'RabbitMQ Host'],
  ['g' , 'rabbit-port=ARG'            , 'RabbitMQ Port'],
  ['l' , 'logging-config-file=ARG'    , 'Logging Config File'],
  ['s' , 'stunserver=ARG'             , 'Stun Server hostname'],
  ['p' , 'stunport=ARG'               , 'Stun Server port'],
  ['m' , 'minport=ARG'                , 'Minimum port'],
  ['M' , 'maxport=ARG'                , 'Maximum port'],
  ['h' , 'help'                       , 'display this help']
]);

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
            default:
                GLOBAL.config.webrtc[prop] = value;
                break;
        }
    }
}

var rpc = require('./amqp_client')();

(function init_env() {
    if (GLOBAL.config.video.hardwareAccelerated) {
        // Query the hardware capability only if we want to try it.
        require('child_process').exec('vainfo', function (error, stdout, stderr) {
            if (error && error.code !== 0) {
                log.warn('vainfo check with exit code:', error.code);
                GLOBAL.config.video.hardwareAccelerated = false;
                return;
            }
            var info = stdout.toString() || stderr.toString();
            // Check hardware codec version
            GLOBAL.config.video.hardwareAccelerated = (info.indexOf('VA-API version') != -1);
        });
    }
})();

log.info('Connecting to rabbitMQ server...');

var controller;

rpc.connect(GLOBAL.config.rabbit, function () {
    rpc.asRpcClient(function(rpcClient) {
        var purpose = process.argv[3];
        var rpcID = process.argv[2];
        var clusterIP = process.argv[6];

        switch (purpose) {
        case 'session':
            controller = require('./session')(rpcClient, rpcID);
            break;
        case 'audio':
            controller = require('./audio')(rpcClient);
            break;
        case 'video':
            controller = require('./video')(rpcClient);
            break;
        case 'webrtc':
            controller = require('woogeen/webrtc/index')();
            break;
        case 'avstream':
            controller = require('woogeen/avstream/index')();
            break;
        case 'recording':
            controller = require('woogeen/recording/index')();
            break;
        case 'sip':
            controller = require('woogeen/sip/index')(rpcClient, {id:rpcID, addr:clusterIP});
            break;
        default:
            log.error('Ambiguous purpose:', purpose);
            process.send('ambiguous purpose');
            return;
        }

        controller.privateRegexp = new RegExp(process.argv[4], 'g');
        controller.publicIP = process.argv[5];
        controller.clusterIP = process.argv[6];
        controller.agentID = process.argv[7];
        controller.keepAlive = function (callback) {
            callback('callback', true);
        };

        rpc.asRpcServer(rpcID, controller, function(rpcServer) {
            log.info(rpcID + ' as rpc server ready');
            rpc.asMonitor(function (data) {
                if (data.reason === 'abnormal' || data.reason === 'error' || data.reason === 'quit') {
                    if (controller && typeof controller.onFaultDetected === 'function') {
                        controller.onFaultDetected(data.message);
                    }
                }
            }, function (monitor) {
                log.info(rpcID + ' as monitor ready');
                process.send('READY');
            }, function(reason) {
                process.send('ERROR');
                log.error(reason);
            });
        }, function(reason) {
            process.send('ERROR');
            log.error(reason);
        });
    }, function(reason) {
        process.send('ERROR');
        log.error(reason);
    });
}, function(reason) {
    process.send('ERROR');
    log.error('Node connect to rabbitMQ server failed, reason:', reason);
});

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        if (controller && typeof controller.close === 'function') {
            controller.close();
        }
        process.exit();
    });
});

['SIGHUP', 'SIGPIPE'].map(function (sig) {
    process.on(sig, function () {
        log.warn(sig, 'caught and ignored');
    });
});

process.on('exit', function () {
    rpc.disconnect();
});
