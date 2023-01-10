// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var fs = require('fs');
var toml = require('toml');
var url = require('url');
const crypto = require('crypto');

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

const session = require('express-session');
const {v4: uuidv4} = require('uuid');

const options = {
  name: 'owtserver',
  genid: function(req) {
    return uuidv4();
  },
  resave: true,
  saveUninitialized: true,
  cookie: {
    httpOnly: true,
    secure: config.console.ssl,
    sameSite: true,
    maxAge: 600000
  }
}
Object.assign(options, {secret: crypto.randomBytes(32).toString('hex')});

app.use(session(options));

app.use((req, res, next) => {
  res.set('Cache-Control', 'no-store, no-cache');
  next();
});

var bodyParser = require('body-parser');
// parse application/x-www-form-urlencoded
app.use('/login', bodyParser.urlencoded({ extended: true }));
// parse application/json
app.use('/login', bodyParser.json());

var httpProxy = require('http-proxy');
var proxyOption = {};

if (url.parse(apiHost).protocol === 'https:' && !config.api.secure) {
  proxyOption.secure = false;
}

var proxy = httpProxy.createProxyServer(proxyOption);
proxy.on('error', function(e) {
  console.log(e);
});

function calculateSignature(toSign, key) {
  const hash = crypto.createHmac("sha256", key).update(toSign);
  const hex = hash.digest('hex');
  return Buffer.from(hex).toString('base64');
}

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

app.post('/login', function(req, res) {
  req.session.serviceid = req.body.id;
  req.session.servicekey = req.body.key;
  res.status(200).send('Successfully logged in');
});

app.get('/login', function(req, res) {
    if (req.session.serviceid) {
      let header = req.headers.authorization;
      const timestamp = new Date().getTime();
      const cnounce = Math.floor(Math.random() * 99999);
      const toSign = timestamp + ',' + cnounce;
      header += ',mauth_serviceid=';
      header += req.session.serviceid;
      header += ',mauth_cnonce=';
      header += cnounce;
      header += ',mauth_timestamp=';
      header += timestamp;
      header += ',mauth_signature=';
      header += calculateSignature(toSign, req.session.servicekey);
      req.headers.authorization = header;
      req.url = '/services/' + req.session.serviceid;
      proxy.web(req, res, {target: apiHost});
    } else {
      res.status(401).send('No session existed');
    }
});

app.get('/logout', function(req, res) {
  req.session.destroy(function(err) {
    res.clearCookie('secretname', {path: '/'}).status(200).send('Successfully logged out');
  });
})

app.use(function (req, res) {
  res.header('Strict-Transport-Security', 'max-age=1024000; includeSubDomain');
  res.header('X-Content-Type-Options', 'nosniff');
  if (isAllowed(req.path)) {
    try {
      let header = req.headers.authorization;
      const timestamp = new Date().getTime();
      const cnounce = Math.floor(Math.random() * 99999);
      const toSign = timestamp + ',' + cnounce;
      header += ',mauth_serviceid=';
      header += req.session.serviceid;
      header += ',mauth_cnonce=';
      header += cnounce;
      header += ',mauth_timestamp=';
      header += timestamp;
      header += ',mauth_signature=';
      header += calculateSignature(toSign, req.session.servicekey);
      req.headers.authorization = header;
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
      log.warn('Failed to setup secured server:', err);
      return process.exit(1);
    }
  });
} else {
  app.listen(port);
  console.log('Start management-console HTTP server');
}
