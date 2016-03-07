/*global require, GLOBAL, process*/
'use strict';
var Getopt = require('node-getopt');
var config = require('./../../etc/woogeen_config');

GLOBAL.config = config || {};
GLOBAL.config.erizo = GLOBAL.config.erizo || {};
GLOBAL.config.erizo.stunserver = GLOBAL.config.erizo.stunserver || '';
GLOBAL.config.erizo.stunport = GLOBAL.config.erizo.stunport || 0;
GLOBAL.config.erizo.minport = GLOBAL.config.erizo.minport || 0;
GLOBAL.config.erizo.maxport = GLOBAL.config.erizo.maxport || 0;
GLOBAL.config.erizo.keystorePath = GLOBAL.config.erizo.keystorePath || '';

GLOBAL.config.erizo.hardwareAccelerated = !!GLOBAL.config.erizo.hardwareAccelerated;
GLOBAL.config.erizo.openh264Enabled = !!GLOBAL.config.erizo.openh264Enabled;

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
            case "logging-config-file":
                GLOBAL.config.logger = GLOBAL.config.logger || {};
                GLOBAL.config.logger.config_file = value;
                break;
            default:
                GLOBAL.config.erizo[prop] = value;
                break;
        }
    }
}

// Load submodules with updated config
var logger = require('../../common/logger').logger;
var rpc = require('../../common/amqper');

// Logger
var log = logger.getLogger("ErizoJS");

(function init_env() {
    if (GLOBAL.config.erizo.hardwareAccelerated) {
        // Query the hardware capability only if we want to try it.
        require('child_process').exec('vainfo', function (err, stdout) {
            var info = '';
            if (err) {
                info = err.toString();
            } else if(stdout.length > 0) {
                info = stdout.toString();
            }
            // Check whether hardware codec should be used for this room
            GLOBAL.config.erizo.hardwareAccelerated = (info.indexOf('VA-API version 0.35.0') != -1) ||
                                                        (info.indexOf('VA-API version: 0.35') != -1);
        });
    }
})();

rpc.connect(function () {
    try {
        var rpcID = process.argv[2];
        var purpose = process.argv[3];

        var controller;
        switch (purpose) {
        case 'audio':
            controller = require('./audio').AudioNode();
            break;
        case 'video':
            controller = require('./video').VideoNode();
            break;
        case 'webrtc':
        case 'rtsp':
        case 'file':
            controller = require('./access').AccessNode();
            break;
        default:
            log.error('Ambiguous purpose to start ErizoJS');
            return;
        }

        controller.privateRegexp = new RegExp(process.argv[4], 'g');
        controller.publicIP = process.argv[5];

        rpc.setPublicRPC(controller);

        controller.keepAlive = function(callback) {
            callback('callback', true);
        };

        log.info('ID: ErizoJS_' + rpcID);

        rpc.bind('ErizoJS_' + rpcID, function() {
            log.info("ErizoJS started");
        });
    } catch (err) {
        log.error(err);
    }
});
/*
var sdp = '{"messageType":"OFFER","sdp":"v=0\\r\\no=- 6915254541026829363 2 IN IP4 127.0.0.1\\r\\ns=-\\r\\nt=0 0\\r\\na=group:BUNDLE audio video\\r\\na=msid-semantic: WMS UnArvhkmFB7DVXMqb7f4dcEdgmaRQ2vQbVEO\\r\\nm=audio 54748 RTP/SAVPF 111 103 104 0 8 106 105 13 126\\r\\nc=IN IP4 192.168.56.1\\r\\na=rtcp:54748 IN IP4 192.168.56.1\\r\\na=candidate:2999745851 1 udp 2113937151 192.168.56.1 54748 typ host generation 0\\r\\na=candidate:2999745851 2 udp 2113937151 192.168.56.1 54748 typ host generation 0\\r\\na=candidate:3633925102 1 udp 2113937151 138.4.4.168 54749 typ host generation 0\\r\\na=candidate:3633925102 2 udp 2113937151 138.4.4.168 54749 typ host generation 0\\r\\na=candidate:4233069003 1 tcp 1509957375 192.168.56.1 0 typ host generation 0\\r\\na=candidate:4233069003 2 tcp 1509957375 192.168.56.1 0 typ host generation 0\\r\\na=candidate:2518333214 1 tcp 1509957375 138.4.4.168 0 typ host generation 0\\r\\na=candidate:2518333214 2 tcp 1509957375 138.4.4.168 0 typ host generation 0\\r\\na=ice-ufrag:A8dCYvqNBi3AR8mA\\r\\na=ice-pwd:vFqVsVfVRRPSZLS+mRUGsJ6K\\r\\na=ice-options:google-ice\\r\\na=fingerprint:sha-256 72:D5:1E:A0:CD:71:C9:D8:65:99:BD:BB:82:E7:A5:AE:BE:DF:23:4C:0F:D8:8C:26:97:9E:5A:CA:56:00:57:47\\r\\na=setup:actpass\\r\\na=mid:audio\\r\\na=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\\r\\na=sendrecv\\r\\na=rtcp-mux\\r\\na=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:Ps33PIN5iNIn/5DCRR0KnQr/vvRMmMB/ziY/kyN/\\r\\na=rtpmap:111 opus/48000/2\\r\\na=fmtp:111 minptime=10\\r\\na=rtpmap:103 ISAC/16000\\r\\na=rtpmap:104 ISAC/32000\\r\\na=rtpmap:0 PCMU/8000\\r\\na=rtpmap:8 PCMA/8000\\r\\na=rtpmap:106 CN/32000\\r\\na=rtpmap:105 CN/16000\\r\\na=rtpmap:13 CN/8000\\r\\na=rtpmap:126 telephone-event/8000\\r\\na=maxptime:60\\r\\na=ssrc:3839633812 cname:8GSPxlTIqeSLyJtS\\r\\na=ssrc:3839633812 msid:UnArvhkmFB7DVXMqb7f4dcEdgmaRQ2vQbVEO 876be926-357a-4342-b3fb-03db9870cc93\\r\\na=ssrc:3839633812 mslabel:UnArvhkmFB7DVXMqb7f4dcEdgmaRQ2vQbVEO\\r\\na=ssrc:3839633812 label:876be926-357a-4342-b3fb-03db9870cc93\\r\\nm=video 54748 RTP/SAVPF 100 116 117\\r\\nc=IN IP4 192.168.56.1\\r\\na=rtcp:54748 IN IP4 192.168.56.1\\r\\na=candidate:2999745851 1 udp 2113937151 192.168.56.1 54748 typ host generation 0\\r\\na=candidate:2999745851 2 udp 2113937151 192.168.56.1 54748 typ host generation 0\\r\\na=candidate:3633925102 1 udp 2113937151 138.4.4.168 54749 typ host generation 0\\r\\na=candidate:3633925102 2 udp 2113937151 138.4.4.168 54749 typ host generation 0\\r\\na=candidate:4233069003 1 tcp 1509957375 192.168.56.1 0 typ host generation 0\\r\\na=candidate:4233069003 2 tcp 1509957375 192.168.56.1 0 typ host generation 0\\r\\na=candidate:2518333214 1 tcp 1509957375 138.4.4.168 0 typ host generation 0\\r\\na=candidate:2518333214 2 tcp 1509957375 138.4.4.168 0 typ host generation 0\\r\\na=ice-ufrag:A8dCYvqNBi3AR8mA\\r\\na=ice-pwd:vFqVsVfVRRPSZLS+mRUGsJ6K\\r\\na=ice-options:google-ice\\r\\na=fingerprint:sha-256 72:D5:1E:A0:CD:71:C9:D8:65:99:BD:BB:82:E7:A5:AE:BE:DF:23:4C:0F:D8:8C:26:97:9E:5A:CA:56:00:57:47\\r\\na=setup:actpass\\r\\na=mid:video\\r\\na=extmap:2 urn:ietf:params:rtp-hdrext:toffset\\r\\na=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\\r\\na=sendrecv\\r\\nb=AS:300\\r\\na=rtcp-mux\\r\\na=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:Ps33PIN5iNIn/5DCRR0KnQr/vvRMmMB/ziY/kyN/\\r\\na=rtpmap:100 VP8/90000\\r\\na=rtcp-fb:100 ccm fir\\r\\na=rtcp-fb:100 nack\\r\\na=rtcp-fb:100 goog-remb\\r\\na=rtpmap:116 red/90000\\r\\na=rtpmap:117 ulpfec/90000\\r\\na=ssrc:2014607583 cname:8GSPxlTIqeSLyJtS\\r\\na=ssrc:2014607583 msid:UnArvhkmFB7DVXMqb7f4dcEdgmaRQ2vQbVEO f396aae1-913c-4801-8e51-f194ff0e51fa\\r\\na=ssrc:2014607583 mslabel:UnArvhkmFB7DVXMqb7f4dcEdgmaRQ2vQbVEO\\r\\na=ssrc:2014607583 label:f396aae1-913c-4801-8e51-f194ff0e51fa\\r\\n","offererSessionId":104,"seq":1,"tiebreaker":120058340}';
controller.RoomController().addPublisher("1", sdp, function(type, answer) {
    console.log(answer);
});
*/

['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
        log.warn('Exiting on', sig);
        process.exit();
    });
});

['SIGHUP', 'SIGPIPE'].map(function (sig) {
    process.on(sig, function () {
        log.warn(sig, 'caught and ignored');
    });
});
