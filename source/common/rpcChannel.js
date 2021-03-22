// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const RpcChannel = function(amqpClient) {
  const that = {};
  const amqp_client = amqpClient;

  that.makeRPC = function(node, method, args, timeout, onStatus) {
    return new Promise(function(resolve, reject) {
      const callbacks = {
        callback: function(result, error_reason) {
          if (result === 'error') {
            reject(error_reason ? error_reason : 'unknown reason');
          } else if (result === 'timeout') {
            reject('Timeout to make rpc to ' + node + '.' + method);
          } else {
            resolve(result);
          }
        },
      };

      if (onStatus) {
        callbacks.onStatus = function(status) {
          onStatus(status).catch(() => {});
        };
      }

      amqp_client.remoteCall(
          node,
          method,
          args,
          callbacks,
          timeout);
    });
  };

  return that;
};

module.exports = RpcChannel;

