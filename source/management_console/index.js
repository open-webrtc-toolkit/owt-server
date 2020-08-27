// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var fs = require('fs');
var toml = require('toml');
var url = require('url');

var config;
try {
  config = toml.parse(fs.readFileSync('./management_console.toml'));
} catch (e) {
  console.log('Parsing config error on line ' + e.line + ', column ' + e.column + ': ' + e.message);
  process.exit(1);
}

// Default manage-console port, api port, keystorePath.
var port = config.console.port || 3300;
var apiHost = config.api.host || 'http://localhost:3000';
var keystorePath = config.console.keystorePath || './cert/certificate.pfx';

var express = require('express');
var app = express();

app.use('/console', express.static(__dirname + '/public'));

var httpProxy = require('http-proxy');
var proxyOption = {};

if (url.parse(apiHost).protocol === 'https:' && !config.api.secure) {
  proxyOption.secure = false;
}

var proxy = httpProxy.createProxyServer(proxyOption);
proxy.on('error', function(e) {
  console.log(e);
});

// Only following paths allowed.
var allowedPaths = ['/v1/rooms', '/services'];
var isAllowed = function (path) {
  var i;
  for (i = 0; i < allowedPaths.length; i++) {
    if (path.indexOf(allowedPaths[i]) === 0)
      return true;
  }
  return false;
};

app.use(function (req, res) {
  //proxyRequest(req, res);
  if (isAllowed(req.path)) {
    try {
      proxy.web(req, res, {target: apiHost});
    } catch (e) {
      res.status(400).send('Invalid path');
    }
  } else {
    res.status(404).send('Resource not found');
  }
});

if (config.console.ssl) {
  var path = require('path');
  var cipher = require('./cipher');
  var keystore = path.resolve(path.dirname(keystorePath), cipher.kstore);
  cipher.unlock(cipher.k, keystore, function cb (err, passphrase) {
    if (!err) {
      try {
        require('https').createServer({
          pfx: require('fs').readFileSync(keystorePath),
          passphrase: passphrase
        }, app).listen(port);

        console.log('Start management-console HTTPS server');
      } catch (e) {
        err = e;
      }
    } else {
      console.warn('Failed to setup secured server:', err);
      return process.exit(1);
    }
  });
} else {
  app.listen(port);
  console.log('Start management-console HTTP server');
}
