// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var fs = require('fs');
var toml = require('toml');
var url = require('url');
const CryptoJS = require('crypto-js');

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

app.use(session({
  secret: 'veryimportantsecret',
  name: 'secretname',
  genid: function(req) {
    return uuidv4();
  },
  cookie: {
    httpOnly: true,
    secure: true,
    sameSite: true,
    maxAge: 600000,
    resave: true,
    saveUninitialized: true
  }
}));

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

var encodeBase64 = (function () {
  var END_OF_INPUT = -1;

  var base64Chars = new Array(
      'A','B','C','D','E','F','G','H',
      'I','J','K','L','M','N','O','P',
      'Q','R','S','T','U','V','W','X',
      'Y','Z','a','b','c','d','e','f',
      'g','h','i','j','k','l','m','n',
      'o','p','q','r','s','t','u','v',
      'w','x','y','z','0','1','2','3',
      '4','5','6','7','8','9','+','/'
  );

  var base64Str;
  var base64Count;

  function readBase64 () {
      if (!base64Str) return END_OF_INPUT;
      if (base64Count >= base64Str.length) return END_OF_INPUT;
      var c = base64Str.charCodeAt(base64Count) & 0xff;
      base64Count++;
      return c;
  }

  return function encodeBase64 (str) {
      base64Str = str;
      base64Count = 0;
      var result = '';
      var inBuffer = new Array(3);
      var done = false;
      while (!done && (inBuffer[0] = readBase64()) != END_OF_INPUT){
          inBuffer[1] = readBase64();
          inBuffer[2] = readBase64();
          result += (base64Chars[ inBuffer[0] >> 2 ]);
          if (inBuffer[1] != END_OF_INPUT){
              result += (base64Chars [(( inBuffer[0] << 4 ) & 0x30) | (inBuffer[1] >> 4) ]);
              if (inBuffer[2] != END_OF_INPUT){
                  result += (base64Chars [((inBuffer[1] << 2) & 0x3c) | (inBuffer[2] >> 6) ]);
                  result += (base64Chars [inBuffer[2] & 0x3F]);
              } else {
                  result += (base64Chars [((inBuffer[1] << 2) & 0x3c)]);
                  result += ('=');
                  done = true;
              }
          } else {
              result += (base64Chars [(( inBuffer[0] << 4 ) & 0x30)]);
              result += ('=');
              result += ('=');
              done = true;
          }
      }
      return result;
  };
}());

function calculateSignature(toSign, key) {
  var hash = CryptoJS.HmacSHA256(toSign, key);
  var hex = hash.toString(CryptoJS.enc.Hex);
  return encodeBase64(hex);
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
  //proxyRequest(req, res);
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
