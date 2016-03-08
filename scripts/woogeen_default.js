var config = {};

/*********************************************************
 COMMON CONFIGURATION
 It's used by Nuve, ErizoController, ErizoAgent and ErizoJS
**********************************************************/
config.rabbit = {};
config.rabbit.host = 'localhost'; //default value: 'localhost'
config.rabbit.port = 5672; //default value: 5672

/*********************************************************
 CLOUD PROVIDER CONFIGURATION
 It's used by Nuve and ErizoController
**********************************************************/
config.cloudProvider = {};
config.cloudProvider.name = '';
//In Amazon Ec2 instances you can specify the zone host. By default is 'ec2.us-east-1a.amazonaws.com'
config.cloudProvider.host = '';
config.cloudProvider.accessKey = '';
config.cloudProvider.secretAccessKey = '';

/*********************************************************
 NUVE CONFIGURATION
**********************************************************/
config.nuve = {};
config.nuve.dataBaseURL = 'localhost/nuvedb'; // default value: 'localhost/nuvedb'
config.nuve.superserviceID = '_auto_generated_ID_'; // default value: ''
config.nuve.testErizoController = 'localhost:8080'; // default value: 'localhost:8080'
config.nuve.ssl = false; // default value: false
config.nuve.keystorePath = '../../cert/certificate.pfx';
// Timestamp check protects the nuve service from those massive requests with out of order timestamps, so that only sequential requests are accepted
config.nuve.timestampCheck = false; // default value: false
// Cloud Handler policies are in nuve/ch_policies/ folder
config.nuve.cloudHandlerPolicy = 'default_policy.js'; // default value: 'default_policy.js'


/*********************************************************
 ERIZO CONTROLLER CONFIGURATION
**********************************************************/
config.erizoController = {};

//Use undefined to run clients without Stun
config.erizoController.stunServerUrl = undefined; // default value: 'stun:stun.l.google.com:19302'

// Default and max video bandwidth parameters to be used by clients
config.erizoController.defaultVideoBW = 300; //default value: 300
config.erizoController.maxVideoBW = 4000; //max value: 4M

// Public erizoController IP for websockets (useful when behind NATs)
// Use '' to automatically get IP from the interface
config.erizoController.publicIP = ''; //default value: ''
// Use '' to use the public IP address instead of a hostname
config.erizoController.hostname = ''; //default value: ''
config.erizoController.port = 8080; //default value: 8080
// Use true if clients communicate with erizoController over SSL
config.erizoController.ssl = true; //default value: true
config.erizoController.keystorePath = '../../cert/certificate.pfx';

// Use the name of the inferface you want to bind to for websockets
// config.erizoController.networkInterface = 'eth1' // default value: undefined

//Use undefined to run clients without Turn
//url example: 'turn:ip_address:port_number?transport=tcp'
config.erizoController.turnServer = {}; // default value: undefined
config.erizoController.turnServer.url = ''; // default value: null
config.erizoController.turnServer.username = ''; // default value: null
config.erizoController.turnServer.password = ''; // default value: null

config.erizoController.warning_n_rooms = 15; // default value: 15
config.erizoController.limit_n_rooms = 20; // default value: 20
config.erizoController.interval_time_keepAlive = 1000; // default value: 1000

// Roles to be used by services
config.erizoController.roles =
{presenter: {publish: true, subscribe: true, record: true},
    viewer: {subscribe: true},
    viewerWithData: {subscribe: true, publish: {audio: false, video: false, screen: false, data: true}}}; // default value: {"presenter":{"publish": true, "subscribe":true, "record":true}, "viewer":{"subscribe":true}, "viewerWithData":{"subscribe":true, "publish":{"audio":false,"video":false,"screen":false,"data":true}}}

// If true, erizoController sends report to rabbitMQ queue "report_handler"
config.erizoController.report = {
    session_events: false, 		// Session level events -- default value: false
    connection_events: false, 	// Connection (ICE) level events -- default value: false
    rtcp_stats: false				// RTCP stats -- default value: false
};

// If undefined, the path will be /tmp/
config.erizoController.recording_path = undefined; // default value: undefined

/*********************************************************
 ERIZO AGENT CONFIGURATION
**********************************************************/
config.erizoAgent = {};

config.erizoAgent.interval_time_keepAlive = 1000; // default value: 1000
config.erizoAgent.interval_time_report = 5000; // default value: 5000

// Max processes that ErizoAgent can run
config.erizoAgent.maxProcesses = 13; // default value: 13
// Number of precesses that ErizoAgent runs when it starts. Always lower than or equals to maxProcesses.
config.erizoAgent.prerunProcesses = 2; // default value: 2

// Public erizoAgent IP for ICE candidates (useful when behind NATs)
// Use '' to automatically get IP from the interface
config.erizoAgent.publicIP = ''; //default value: ''
// Use the name of the inferface you want to bind for ICE candidates
// config.erizoAgent.externalNetworkInterface = 'eth1' // default value: undefined
// Use the name of the inferface you want to bind for cluster internal communication
// config.erizoAgent.internalNetworkInterface = 'eth0' // default value: undefined

/*********************************************************
 ERIZO JS CONFIGURATION
**********************************************************/
config.erizo = {};

//STUN server IP address and port to be used by the server.
//if '' is used, the address is discovered locally
config.erizo.stunserver = ''; // default value: ''
config.erizo.stunport = 0; // default value: 0

//note, this won't work with all versions of libnice. With 0 all the available ports are used
config.erizo.minport = 0; // default value: 0
config.erizo.maxport = 0; // default value: 0

// If true and the machine has the capability, the mixer will be accelerated by hardware graphic chips.
config.erizo.hardwareAccelerated = false;

// This configuration is only for software media engine. Hardware graphic acceleration provides H.264 by default.
// "true" means OpenH264 is deployed for H.264. Otherwise no support of H.264 in MCU.
config.erizo.openh264Enabled = false;

config.erizo.keystorePath = '../../cert/certificate.pfx';

// This configuration is only for RTSP muxing.
// e.g.: 'rtsp://localhost:1935/live/xxx.sdp'
config.erizo.rtsp = {};
config.erizo.rtsp.enabled = false;
config.erizo.rtsp.addr = 'localhost';
config.erizo.rtsp.port = 1935;
config.erizo.rtsp.application = '/live/'; // default application name, don't forget ending slash.
config.erizo.rtsp.username = 'webrtc';
config.erizo.rtsp.passwd = 'abc123';

/***** END *****/
// Following lines are always needed.
var module = module || {};
module.exports = config;
