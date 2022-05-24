// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const protobuf = require('protobufjs');

const config = require('./protoConfig.json');

// Return promise with (server, port).
function startServer(type, serviceObj) {
  if (!config[type]) {
    return Promise.reject(new Error('Invalid proto type:' + type));
  }
  const protoPath = config[type].file;
  const packageName = config[type].package;
  const serviceName = config[type].service;
  console.log('path:', protoPath, 'pkg:', packageName, serviceName);
  const packageDefinition = protoLoader.loadSync(
    protoPath,
    {keepCase: true,
     longs: String,
     enums: String,
     defaults: true,
     oneofs: true
    });
  const pkg = grpc.loadPackageDefinition(packageDefinition)[packageName];
  const service = pkg[serviceName].service;

  const server = new grpc.Server();
  server.addService(service, serviceObj);

  // Start server.
  return new Promise((resolve, reject) => {
    server.bindAsync(
        '0.0.0.0:0',
        grpc.ServerCredentials.createInsecure(),
        (err, port) => {
      if (err) {
        reject(err);
      } else {
        server.start();
        resolve(port);
      }
    });
  });
}

// Return client
function startClient(type, address) {
  if (!config[type]) {
    return Promise.reject(new Error('Invalid proto type:' + type));
  }
  const protoPath = config[type].file;
  const packageName = config[type].package;
  const serviceName = config[type].service;
  console.log('path:', protoPath, 'pkg:', packageName, serviceName, address);
  const packageDefinition = protoLoader.loadSync(
    protoPath,
    {keepCase: true,
     longs: String,
     enums: String,
     defaults: true,
     oneofs: true
    });
  const pkg = grpc.loadPackageDefinition(packageDefinition)[packageName];
  const client = new pkg[serviceName](
      address, grpc.credentials.createInsecure());
  return client;
}

function packOption(type, option) {
  const protoFile = config[type].file;
  const root = protobuf.loadSync(protoFile);
  const RequestOption = root.lookupType('owt.RequestOption');
  if (!RequestOption) {
    console.log('No owt.RequestOption in ', protoFile);
    return null;
  }
  const Any = root.lookupType("protobuf.Any");
  const optionMessage = RequestOption.create(option);
  // Pack to Any
  const anyMessage = Any.create({
    type_url: 'owt.RequestOption',
    value: RequestOption.encode(optionMessage).finish()
  });
  return anyMessage;
}

function unpackOption(type, packedOption) {
  const protoFile = config[type].file;
  const root = protobuf.loadSync(protoFile);
  const RequestOption = root.lookupType('owt.RequestOption');
  if (!RequestOption) {
    console.log('No owt.RequestOption in ', protoFile);
    return null;
  }
  if (packedOption.type_url !== 'owt.RequestOption') {
    console.log('Type url is not owt.RequestOption');
    return null;
  }
  const optionMessage = RequestOption.decode(packedOption.value);
  return RequestOption.toObject(optionMessage, {enums: String});
}

module.exports = {
  startServer,
  startClient,
  packOption,
  unpackOption,
};
