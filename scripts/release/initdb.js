#!/usr/bin/env node
'use strict';

var HOME = process.env.WOOGEEN_HOME;
if (!HOME) {
  throw 'WOOGEEN_HOME not found';
}
var dbURL = process.env.DB_URL;
if (!dbURL) {
  throw 'DB_URL not found';
}

var path = require('path');
var destConfigFile = path.join(HOME, 'etc/woogeen_config.js');
var configFile = destConfigFile;
var db = require('mongojs')(dbURL, ['services']);
var cipher = require('../common/cipher');

function prepareService (serviceName, next) {
  db.services.findOne({name: serviceName}, function cb (err, service) {
    if (err || !service) {
      var crypto = require('crypto');
      var key = crypto.pbkdf2Sync(crypto.randomBytes(64).toString('hex'), crypto.randomBytes(32).toString('hex'), 4000, 128, 'sha256').toString('base64');
      service = {name: serviceName, key: cipher.encrypt(cipher.k, key), encrypted: true, rooms: []};
      db.services.save(service, function cb (err, saved) {
        if (err) {
          console.log('mongodb: error in adding', serviceName);
          return db.close();
        }
        saved.key = key;
        next(saved);
      });
    } else {
      if (service.encrypted === true) {
        service.key = cipher.decrypt(cipher.k, service.key);
      }
      next(service);
    }
  });
}

prepareService('superService', function (service) {
  var fs = require('fs');
  var superServiceId = service._id+'';
  var superServiceKey = service.key;
  console.log('superServiceId:', superServiceId);
  console.log('superServiceKey:', superServiceKey);
  try {
    fs.statSync(configFile);
  } catch (e) {
    configFile = process.env.DEFAULT_CONFIG;
  }
  fs.readFile(configFile, 'utf8', function (err, data) {
    if (err) {
      return console.log(err);
    }
    data = data.replace(/config\.nuve\.dataBaseURL = '[^']*';/, 'config.nuve.dataBaseURL = \''+dbURL+'\';');
    data = data.replace(/config\.nuve\.superserviceID = '[^']*';/, 'config.nuve.superserviceID = \''+superServiceId+'\';');
    fs.writeFile(destConfigFile, data, 'utf8', function (err) {
      if (err) return console.log('Error in saving woogeen_config:', err);
    });
  });

  prepareService('sampleService', function (service) {
    var sampleServiceId = service._id+'';
    var sampleServiceKey = service.key;
    console.log('sampleServiceId:', sampleServiceId);
    console.log('sampleServiceKey:', sampleServiceKey);
    db.close();
    var sampleAppFile = path.join(HOME, 'extras/basic_example/basicServer.js');
    fs.readFile(sampleAppFile, 'utf8', function (err, data) {
      if (err) {
        return console.log(err);
      }
      data = data.replace(/N\.API\.init\('[^']*', '[^']*'/, 'N.API.init(\''+sampleServiceId+'\', \''+sampleServiceKey+'\'');
      fs.writeFile(sampleAppFile, data, 'utf8', function (err) {
         if (err) return console.log(err);
      });
    });
    var sampleAppFile2 = path.join(HOME, 'extras/rtsp-client.js');
    fs.readFile(sampleAppFile2, 'utf8', function (err, data) {
      if (err) {
        return console.log(err);
      }
      data = data.replace(/nuve\.init\('[^']*', '[^']*'/, 'nuve.init(\''+sampleServiceId+'\', \''+sampleServiceKey+'\'');
      fs.writeFile(sampleAppFile2, data, 'utf8', function (err) {
         if (err) return console.log(err);
      });
    });
  });
});