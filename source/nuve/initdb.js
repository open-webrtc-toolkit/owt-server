#!/usr/bin/env node
'use strict';

var dbURL = process.env.DB_URL;
if (!dbURL) {
  throw 'DB_URL not found';
}

var fs = require('fs');
var path = require('path');
var configFile = path.join(__dirname, 'nuve.toml');
try {
  fs.statSync(configFile);
} catch (e) {
  console.error('config file not found');
  process.exit(1);
}
var db = require('mongojs')(dbURL, ['services']);
var cipher = require('./cipher');

function prepareService (serviceName, next) {
  db.services.findOne({name: serviceName}, function cb (err, service) {
    if (err || !service) {
      var crypto = require('crypto');
      var key = crypto.pbkdf2Sync(crypto.randomBytes(64).toString('hex'), crypto.randomBytes(32).toString('hex'), 4000, 128, 'sha256').toString('base64');
      service = {name: serviceName, key: cipher.encrypt(cipher.k, key), encrypted: true, rooms: [], __v: 0};
      db.services.save(service, function cb (err, saved) {
        if (err) {
          console.log('mongodb: error in adding', serviceName);
          return db.close();
        }
        saved.key = key;
        next(saved);
      });
    } else {
      if (typeof service.__v !== 'number') {
        console.log(`The existed service "${serviceName}" is not in 4.0 format.`);
        console.log('Please use nuve/SchemaUpdate3to4.js to upgrade your database.');
        setTimeout(()=>db.close(), 1000);
        return;
      }
      if (service.encrypted === true) {
        service.key = cipher.decrypt(cipher.k, service.key);
      }
      next(service);
    }
  });
}

prepareService('superService', function (service) {
  var superServiceId = service._id+'';
  var superServiceKey = service.key;
  console.log('superServiceId:', superServiceId);
  console.log('superServiceKey:', superServiceKey);
  fs.readFile(configFile, 'utf8', function (err, data) {
    if (err) {
      return console.log(err);
    }
    data = data.replace(/\ndataBaseURL =[^\n]*\n/, '\ndataBaseURL = "'+dbURL+'"\n');
    data = data.replace(/\nsuperserviceID =[^\n]*\n/, '\nsuperserviceID = "'+superServiceId+'"\n');
    fs.writeFile(configFile, data, 'utf8', function (err) {
      if (err) return console.log('Error in saving configuration:', err);
    });
  });

  prepareService('sampleService', function (service) {
    var sampleServiceId = service._id+'';
    var sampleServiceKey = service.key;
    console.log('sampleServiceId:', sampleServiceId);
    console.log('sampleServiceKey:', sampleServiceKey);
    db.close();
    var sampleServiceFile = path.resolve(__dirname, '../extras/basic_example/samplertcservice.js');
    fs.readFile(sampleServiceFile, 'utf8', function (err, data) {
      if (err) {
        return console.log(err);
      }
      data = data.replace(/icsREST\.API\.init\('[^']*', '[^']*'/, 'icsREST.API.init(\''+sampleServiceId+'\', \''+sampleServiceKey+'\'');
      fs.writeFile(sampleServiceFile, data, 'utf8', function (err) {
         if (err) return console.log(err);
      });
    });
  });
});