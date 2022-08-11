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

// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

// A REST service for creating token for testing.

/*global require, __dirname, console, process*/
'use strict';

var express = require('express'),
  spdy = require('spdy'),
  bodyParser = require('body-parser'),
  errorhandler = require('errorhandler'),
  morgan = require('morgan'),
  fs = require('fs'),
  https = require('https'),
  icsREST = require('./rest');

var app = express();

// app.configure ya no existe
app.use(errorhandler());
app.use(morgan('dev'));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
  extended: true
}));
app.disable('x-powered-by');

app.use(function(req, res, next) {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'POST, GET, PUT, PATCH, OPTIONS, DELETE');
  res.header('Access-Control-Allow-Headers', 'origin, content-type');
  res.header('Strict-Transport-Security', 'max-age=1024000; includeSubDomain');
  res.header('X-Content-Type-Options', 'nosniff');
  if (req.method == 'OPTIONS') {
    res.send(200);
  } else {
    next();
  }
});

icsREST.API.init('_service_ID_', '_service_KEY_', 'https://localhost:3000/', false);

var sampleRoom;
var pageOption = { page: 1, per_page: 100 };
(function initSampleRoom () {
  icsREST.API.getRooms(pageOption, function(rooms) {
    console.log(rooms.length + ' rooms in this service.');
    for (var i = 0; i < rooms.length; i++) {
      if (sampleRoom === undefined && rooms[i].name === 'sampleRoom') {
        sampleRoom = rooms[i]._id;
        console.log('sampleRoom Id:', sampleRoom);
      }
      if (sampleRoom !== undefined) {
        break;
      }
    }
    var tryCreate = function(room, callback) {
      var options = {
        views: [],
        transcoding: {audio: false, video: false}
      };
      icsREST.API.createRoom(room.name, options, function(roomId) {
        console.log('Created room:', roomId._id);
        callback(roomId._id);
      }, function(status, err) {
        console.log('Error in creating room:', err, '[Retry]');
        setTimeout(function() {
          tryCreate(room, options, callback);
        }, 100);
      }, room);
    };

    var room;
    if (!sampleRoom) {
      room = {
        name: 'sampleRoom'
      };
      tryCreate(room, function(Id) {
        sampleRoom = Id;
        console.log('sampleRoom Id:', sampleRoom);
      });
    }
  }, function(stCode, msg) {
    console.log('getRooms failed(', stCode, '):', msg);
  });
})();

app.post('/tokens', function(req, res) {
  'use strict';
  var room = req.body.room || sampleRoom,
    user = req.body.user,
    role = req.body.role;

  //Note: The actual *ISP* and *region* information should be retrieved from the *req* object and filled in the following 'preference' data.
  var preference = {isp: 'isp', region: 'region'};
  icsREST.API.createToken(room, user, role, preference, function(token) {
    res.send(token);
  }, function(err) {
    res.status(401).send(err);
  });
});

spdy.createServer({
  spdy: {
    plain: true
  }
}, app).listen(3001, (err) => {
  if (err) {
    console.log('Failed to setup plain server, ', err);
    return process.exit(1);
  }
});

var cipher = require('./cipher');
cipher.unlock(cipher.k, 'cert/.woogeen.keystore', function cb(err, obj) {
  if (!err) {
    spdy.createServer({
      pfx: fs.readFileSync('cert/certificate.pfx'),
      passphrase: obj.sample
    }, app).listen(3004, (error) => {
      if (error) {
        console.log('Failed to setup secured server: ', error);
        return process.exit(1);
      }
    });
  }
  if (err) {
    console.error('Failed to setup secured server:', err);
    return process.exit();
  }
});
