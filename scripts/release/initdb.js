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
var mongojs = require('mongojs');
var db = mongojs.connect(dbURL, ['services']);

function prepareService (serviceName, next) {
  db.services.findOne({name: serviceName}, function cb (err, service) {
    if (err || !service) {
      var key = require('crypto').randomBytes(64).toString('hex');
      service = {name: serviceName, key: key, rooms: []};
      db.services.save(service, function cb (err, saved) {
        if (err) {
          console.log('mongodb: error in adding', serviceName);
          return db.close();
        }
        next(saved);
      });
    } else {
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
  });
});