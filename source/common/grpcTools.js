// Copyright (C) <2022> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const protobuf = require('protobufjs');

const config = require('./protoConfig.json');

const notificationTypes = {
  'onMediaUpdate': 'owt.MediaUpdateData',
  'onTrackUpdate': 'owt.TrackUpdateData',
  'onTransportProgress': 'owt.TransportProgressData',
  'onAudioActiveness': 'owt.AudioActivenessData',
  'onSessionProgress': 'owt.SessionProgressData',
  'onVideoLayoutChange': 'owt.VideoLayoutChangeData',
  'onStreamAdded': 'owt.StreamAddedData',
  'onStreamRemoved': 'owt.StreamAddedData',
};

// Return promise with (port).
function startServer(type, serviceObj, serverPort = 0) {
  if (!config[type]) {
    return Promise.reject(new Error('Invalid proto type:' + type));
  }
  const protoPath = config[type].file;
  const packageName = config[type].package;
  const serviceName = config[type].service;
  // console.log('path:', protoPath, 'pkg:', packageName, serviceName);
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
        '0.0.0.0:' + serverPort,
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
  // console.log('path:', protoPath, 'pkg:', packageName, serviceName, address);
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

function packNotification(notification) {
  const protoFile = config[notification.type].file;
  const dataType = notificationTypes[notification.name];
  const root = protobuf.loadSync(protoFile);
  const DataType = root.lookupType(dataType);
  if (!DataType) {
    console.log('No notification data type', dataType, protoFile);
    return null;
  }
  const Any = root.lookupType("protobuf.Any");
  const dataMessage = DataType.create(notification.data);
  // Pack to Any
  const anyMessage = Any.create({
    type_url: dataType,
    value: DataType.encode(dataMessage).finish()
  });
  return {
    type: notification.type,
    name: notification.name,
    data: anyMessage
  };
}

function unpackNotification(notification) {
  const protoFile = config[notification.type].file;
  const dataType = notification.data.type_url;
  const root = protobuf.loadSync(protoFile);
  const DataType = root.lookupType(dataType);
  if (!DataType) {
    console.log('No notification data type', dataType, protoFile);
    return null;
  }
  const dataMessage = DataType.decode(notification.data.value);
  return {
    type: notification.type,
    name: notification.name,
    data: DataType.toObject(dataMessage, {enums: String})
  };
}

module.exports = {
  startServer,
  startClient,
  packOption,
  unpackOption,
  packNotification,
  unpackNotification,
};
