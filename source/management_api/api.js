// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';

var logger = require('./logger').logger;
var log = logger.getLogger('ManagementServer');
var fs = require('fs');
var toml = require('toml');
var amqper = require('./amqpClient')();

try {
  global.config = toml.parse(fs.readFileSync('./management_api.toml'));
} catch (e) {
  log.error('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

var e = require('./errors');
var rpc = require('./rpc/rpc');
var cluster_name = ((global.config || {}).cluster || {}).name || 'owt-cluster';

var express = require('express');
var bodyParser = require('body-parser');
var app = express();

var serverAuthenticator = require('./auth/serverAuthenticator');
var servicesResource = require('./resource/servicesResource');
var serviceResource = require('./resource/serviceResource');
var cipher = require('./cipher');

// parse application/x-www-form-urlencoded
app.use(bodyParser.urlencoded({ extended: true }))
// parse application/json
app.use(bodyParser.json())

// for CORS
app.use(function (req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE, PATCH');
  res.header('Access-Control-Allow-Headers', 'origin, authorization, content-type');
  res.header('Strict-Transport-Security', 'max-age=1024000; includeSubDomain');
  res.header('X-Content-Type-Options', 'nosniff');
  next();
});
app.options('*', function(req, res) {
  res.send(200);
});

// Only following paths need authentication.
var authPaths = ['/v1/rooms*', '/v1.1/rooms*', '/services*', '/cluster*'];
app.get(authPaths, serverAuthenticator.authenticate);
app.post(authPaths, serverAuthenticator.authenticate);
app.delete(authPaths, serverAuthenticator.authenticate);
app.put(authPaths, serverAuthenticator.authenticate);
app.patch(authPaths, serverAuthenticator.authenticate);

app.post('/services', servicesResource.create);
app.get('/services', servicesResource.represent);

app.get('/services/:service', serviceResource.represent);
app.delete('/services/:service', serviceResource.deleteService);

// API for version 1.0.
var routerV1 = require('./resource/v1');
app.use('/v1', routerV1);

// API for version 1.1.
var routerV1_1 = require('./resource/v1.1');
app.use('/v1.1', routerV1_1);

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


var serverConfig = global.config.server || {};
// Mutiple process setup
var cluster = require("cluster");
var serverPort = serverConfig.port || 3000;
var numCPUs = serverConfig.numberOfProcess || 1;

var ip_address;
(function getPublicIP() {
  var BINDED_INTERFACE_NAME = serverConfig.networkInterface;
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
          if (address.family === 'IPv6' && !address.internal) {
            if (k === BINDED_INTERFACE_NAME || !BINDED_INTERFACE_NAME) {
              addresses.push('[' + address.address + ']');
            }
          }
        }
      }
    }
  }

  if (serverConfig.hostname === '' || serverConfig.hostname === undefined) {
    if (serverConfig.ip_address === '' || serverConfig.ip_address === undefined){
      ip_address = addresses[0];
    } else {
      ip_address = serverConfig.ip_address;
    }
  } else {
    ip_address = serverConfig.hostname;
  }

})();

var url;
if (serverConfig.ssl === false) {
    url = 'http://' + ip_address + ':' + serverPort;
} else {
    url = 'https://' + ip_address + ':' + serverPort;
}

if (cluster.isMaster) {
    // Master Process

    // Save server key to database.
    // FIXME: we should check if server key already exists
    // in cache/database before generating a new one when
    // there are multiple machine running server.
    var dataAccess = require('./data_access');
    dataAccess.token.genKey();

    var registerInfo = undefined;

    amqper.connect(global.config.rabbit, function () {
        amqper.asRpcClient(function(rpcCli) {
            var keepTrying = true;
            var trySendInfo = function(attempts) {
                if (attempts <= 0) {
                    log.info("Send register info to cluster manager timeout");
                    return
                }
                log.info("Send restful info to cluster manager:", registerInfo);
                rpcCli.remoteCall(cluster_name, 'registerInfo', [registerInfo], { callback : function(result) {
                    if (result === 'timeout') {
                        if (keepTrying) {
                            log.info('Faild to register restful server info, keep trying.');
                            setTimeout(function() { trySendInfo(attempts - (result === 'timeout' ? 4 : 1)); }, 1000);
                        }
                    } else {
                        log.info('Send register info to cluster manager succeed');
                        keepTrying = false;
                    }
                } }, 1000);
            }

            if (registerInfo != undefined) {
                trySendInfo(5);
            } else {
                setTimeout(function() { trySendInfo(5); }, 1000);
            }
        }, function(reason) {
            log.error('Initializing as rpc client failed, reason:', reason);
            process.exit();
      });
    }, function(reason) {
        log.error('Connect to rabbitMQ server failed, reason:', reason);
        process.exit();
    });

    dataAccess.service.list(function (err, sers) {
        if (err) {
            log.warn('Failed to get service:', err.message);
        } else {
            var serviceToCloud = sers.filter((t) => {return t.name === serverConfig.servicename;});
            log.info('Representing service ', serviceToCloud);
            var key = serviceToCloud[0].key;
            if (serviceToCloud[0].encrypted === true) {
                key = cipher.decrypt(cipher.k, key);
            }

            registerInfo = {
                resturl: url,
                servicekey: key,
                serviceid: serviceToCloud[0]._id
            }
        }
    });

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
    // Load spk
    const aconfig = cipher.unlockSync(cipher.k, cipher.astore);
    global.config.spk = aconfig.spk ? Buffer.from(aconfig.spk, 'hex') : cipher.dk;

    if (serverConfig.ssl === true) {
        var cipher = require('./cipher');
        var path = require('path');
        var keystore = path.resolve(path.dirname(serverConfig.keystorePath), cipher.kstore);
        cipher.unlock(cipher.k, keystore, function cb (err, passphrase) {
            if (!err) {
                try {
                  require('https').createServer({
                      pfx: require('fs').readFileSync(serverConfig.keystorePath),
                      passphrase: passphrase
                  }, app).listen(serverPort);
                } catch (e) {
                  log.warn('Failed to start secured server:', e);
                  return process.exit(1);
                }
            } else {
                log.warn('Failed to setup secured server:', err);
                return process.exit(1);
            }
        });
    } else {
        app.listen(serverPort);
    }

    log.info(`Worker ${process.pid} started`);

    ['SIGINT', 'SIGTERM'].map(function (sig) {
        process.on(sig, async function () {
            // This log won't be showed in server log,
            // because worker process disconnected.
            log.warn('Worker exiting on', sig);
            await rpc.disconnect();
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
