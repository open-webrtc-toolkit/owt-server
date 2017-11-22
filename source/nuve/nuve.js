/*global require, __dirname, process, global*/
'use strict';

var log = require('./logger').logger.getLogger('Nuve');
var fs = require('fs');
var toml = require('toml');

try {
  global.config = toml.parse(fs.readFileSync('./nuve.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

var rpc = require('./rpc/rpc');

var express = require('express');
var bodyParser = require('body-parser');
var app = express();

var nuveAuthenticator = require('./auth/nuveAuthenticator');
var roomsResource = require('./resource/roomsResource');
var roomResource = require('./resource/roomResource');
var tokensResource = require('./resource/tokensResource');
var servicesResource = require('./resource/servicesResource');
var serviceResource = require('./resource/serviceResource');
var usersResource = require('./resource/usersResource');
var userResource = require('./resource/userResource');
//var clusterResource = require('./resource/clusterResource');

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
    extended: true
}));

app.set('view options', {
    layout: false
});

app.use(function(req, res, next) {
    try {
        decodeURIComponent(req.path)
    } catch(e) {
        log.warn('URI fail:', req.url);
        return res.status(404).send('URI Error');
    }
    next();
});

app.use(function(error, req, res, next) {
    if (error instanceof SyntaxError) {
        log.warn('SyntaxError:', error.message);
        res.status(404).send('JSON body SyntaxError');
    } else {
        next();
    }
});

app.use(function (req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE');
    res.header('Access-Control-Allow-Headers', 'origin, authorization, content-type');
    next();
});

app.options('*', function(req, res) {
    res.send(200);
});

// Only following paths need authentication.
var authPaths = ['/rooms*', '/services*', '/cluster*'];
app.get(authPaths, nuveAuthenticator.authenticate);
app.post(authPaths, nuveAuthenticator.authenticate);
app.delete(authPaths, nuveAuthenticator.authenticate);
app.put(authPaths, nuveAuthenticator.authenticate);

app.post('/rooms', roomsResource.createRoom);
app.get('/rooms', roomsResource.represent);

app.get('/rooms/:room', roomResource.represent);
app.delete('/rooms/:room', roomResource.deleteRoom);
app.put('/rooms/:room', roomResource.updateRoom);

app.post('/rooms/:room/tokens', tokensResource.create);
app.post('/rooms/:room/tokens/:type', tokensResource.create);

app.post('/services', servicesResource.create);
app.get('/services', servicesResource.represent);

app.get('/services/:service', serviceResource.represent);
app.delete('/services/:service', serviceResource.deleteService);

app.get('/rooms/:room/users', usersResource.getList);

app.get('/rooms/:room/users/:user', userResource.getUser);
app.delete('/rooms/:room/users/:user', userResource.deleteUser);

// app.get('/cluster/nodes', clusterResource.getNodes);
// app.get('/cluster/nodes/:node', clusterResource.getNode);
// app.get('/cluster/rooms', clusterResource.getRooms);
// app.get('/cluster/nodes/:node/config', clusterResource.getNodeConfig);

var nuveConfig = global.config.nuve || {};
// Mutiple process setup
var cluster = require("cluster");
var nuvePort = nuveConfig.port || 3000;
var numCPUs = nuveConfig.numberOfProcess || 1;

if (cluster.isMaster) {
    // Master Process

    // Save nuve key to database.
    // FIXME: we should check if nuve key already exists
    // in cache/database before generating a new one when
    // there are multiple machine running nuve.
    var dataAccess = require('./data_access');
    dataAccess.token.genKey();

    for (var i = 0; i < numCPUs; i++) {
        cluster.fork();
    }

    cluster.on('exit', function(worker, code, signal) {
        log.info(`Worker ${worker.process.pid} died`);
    });


    ['SIGINT', 'SIGTERM'].map(function (sig) {
    process.on(sig, function () {
            log.info('Master exiting on', sig);
            process.exit();
        });
    });

} else {
    // Worker Process
    rpc.connect(global.config.rabbit);

    if (nuveConfig.ssl === true) {
        var cipher = require('./cipher');
        var path = require('path');
        var keystore = path.resolve(path.dirname(nuveConfig.keystorePath), '.woogeen.keystore');
        cipher.unlock(cipher.k, keystore, function cb (err, passphrase) {
            if (!err) {
                try {
                    require('https').createServer({
                        pfx: require('fs').readFileSync(nuveConfig.keystorePath),
                        passphrase: passphrase
                    }, app).listen(nuvePort);
                } catch (e) {
                    err = e;
                }
            } else {
                log.warn('Failed to setup secured server:', err);
                return process.exit(1);
            }
        });
    } else {
        app.listen(nuvePort);
    }

    log.info(`Worker ${process.pid} started`);

    ['SIGINT', 'SIGTERM'].map(function (sig) {
        process.on(sig, function () {
            // This log won't be showed in nuve log,
            // because worker process disconnected.
            log.warn('Worker exiting on', sig);
            process.exit();
        });
    });

    process.on('exit', function () {
        rpc.disconnect();
    });
}
