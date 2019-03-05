// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('Nuve');
var fs = require('fs');
var toml = require('toml');

try {
  global.config = toml.parse(fs.readFileSync('./nuve.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

var e = require('./errors');
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
var participantsResource = require('./resource/participantsResource');
var streamsResource = require('./resource/streamsResource');
var streamingOutsResource = require('./resource/streamingOutsResource');
var recordingsResource = require('./resource/recordingsResource');
var sipcallsResource = require('./resource/sipcallsResource');

// parse application/x-www-form-urlencoded
app.use(bodyParser.urlencoded({ extended: true }))
// parse application/json
app.use(bodyParser.json())

// for CORS
app.use(function (req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE, PATCH');
  res.header('Access-Control-Allow-Headers', 'origin, authorization, content-type');
  next();
});
app.options('*', function(req, res) {
  res.send(200);
});

// Only following paths need authentication.
var authPaths = ['/rooms*', '/v1/rooms*', '/services*', '/cluster*'];
app.get(authPaths, nuveAuthenticator.authenticate);
app.post(authPaths, nuveAuthenticator.authenticate);
app.delete(authPaths, nuveAuthenticator.authenticate);
app.put(authPaths, nuveAuthenticator.authenticate);
app.patch(authPaths, nuveAuthenticator.authenticate);

app.post('/services', servicesResource.create);
app.get('/services', servicesResource.represent);

app.get('/services/:service', serviceResource.represent);
app.delete('/services/:service', serviceResource.deleteService);

////////////////////////////////////////////////////////////////////////////////////////////
// v1 interface begin
// /////////////////////////////////////////////////////////////////////////////////////////

//Room management
app.post('/v1/rooms', roomsResource.createRoom); //FIXME: The definition of 'options' needs to be refined.
app.get('/v1/rooms', roomsResource.represent); //FIXME: The list result needs to be simplified.
app.get('/v1/rooms/:room', roomResource.represent); //FIXME: The detailed format of a complete room configuration data object needs to be refined.
app.delete('/v1/rooms/:room', roomResource.deleteRoom);
app.put('/v1/rooms/:room', roomResource.updateRoom);
app.patch('/v1/rooms/:room', (req, res, next) => next(new e.AppError('Not implemented'))); //FIXME: To be implemented.

//Room checker for all the sub-resources
app.use('/v1/rooms/:room/*', roomResource.validate);

//Participant management
app.get('/v1/rooms/:room/participants', participantsResource.getList);
app.get('/v1/rooms/:room/participants/:participant', participantsResource.get);
app.patch('/v1/rooms/:room/participants/:participant', participantsResource.patch);
app.delete('/v1/rooms/:room/participants/:participant', participantsResource.delete);

//Stream(including external streaming-in) management
app.get('/v1/rooms/:room/streams', streamsResource.getList);
app.get('/v1/rooms/:room/streams/:stream', streamsResource.get);
app.patch('/v1/rooms/:room/streams/:stream', streamsResource.patch);
app.delete('/v1/rooms/:room/streams/:stream', streamsResource.delete);
app.post('/v1/rooms/:room/streaming-ins', streamsResource.addStreamingIn);//FIXME: Validation on body.type === 'streaming' is needed.
app.delete('/v1/rooms/:room/streaming-ins/:stream', streamsResource.delete);

//External streaming-out management
app.get('/v1/rooms/:room/streaming-outs', streamingOutsResource.getList);
app.post('/v1/rooms/:room/streaming-outs', streamingOutsResource.add);//FIXME: Validation on body.type === 'streaming' is needed.
app.patch('/v1/rooms/:room/streaming-outs/:id', streamingOutsResource.patch);
app.delete('/v1/rooms/:room/streaming-outs/:id', streamingOutsResource.delete);

//Server side recording management
app.get('/v1/rooms/:room/recordings', recordingsResource.getList);
app.post('/v1/rooms/:room/recordings', recordingsResource.add);//FIXME: Validation on body.type === 'recording' is needed.
app.patch('/v1/rooms/:room/recordings/:id', recordingsResource.patch);
app.delete('/v1/rooms/:room/recordings/:id', recordingsResource.delete);

//Sip call management
app.get('/v1/rooms/:room/sipcalls', sipcallsResource.getList);
app.post('/v1/rooms/:room/sipcalls', sipcallsResource.add);
app.patch('/v1/rooms/:room/sipcalls/:id', sipcallsResource.patch);
app.delete('/v1/rooms/:room/sipcalls/:id', sipcallsResource.delete);

//Create token.
app.post('/v1/rooms/:room/tokens', tokensResource.create);

////////////////////////////////////////////////////////////////////////////////////////////
// v1 interface end
// /////////////////////////////////////////////////////////////////////////////////////////

// for path not match
app.use('*', function(req, res, next) {
  next(new e.NotFoundError());
});

// error handling
app.use(function(err, req, res, next) {
  log.debug(err);
  if (!(err instanceof e.AppError)) {
    if (err instanceof SyntaxError) {
      err = new e.BadRequestError('Failed to parse JSON body');
    } else {
      log.warn(err);
      err = new e.AppError(err.name + ' ' + err.message);
    }
  }
  res.status(err.status).send(err.data);
});


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

    process.on('SIGUSR2', function() {
        logger.reconfigure();
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

    process.on('SIGUSR2', function() {
        logger.reconfigure();
    });
}
