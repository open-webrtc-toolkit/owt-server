// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

function startClient(rpc) {
  return new Promise(function (resolve, reject) {
    rpc.asRpcClient(resolve, reject);
  });
}

function startServer(rpc, id, api) {
  return new Promise(function (resolve, reject) {
    rpc.asRpcServer(id, api, resolve, reject);
  });
}

function startMonitor(rpc, dataHandler) {
  return new Promise(function (resolve, reject) {
    rpc.asMonitor(dataHandler, resolve, reject);
  });
}

module.exports = {
  startClient,
  startServer,
  startMonitor,
};
