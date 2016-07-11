/* global require */
'use strict';

var RpcChannel = function(amqpClient) {
  var that = {};
  var amqp_client = amqpClient;

  that.makeRPC = function (node, method, args, onStatus) {
    return new Promise(function(resolve, reject) {
      var callbacks = {
        callback: function (result, error_reason) {
          if (result === 'error') {
            reject(error_reason ? error_reason : 'unknown reason');
          } else if (result === 'timeout') {
            reject('Timeout to make rpc to ' + node + '.' + method);
          } else {
            resolve(result);
          }
         }
      };
      onStatus && (callbacks.onStatus = onStatus);

      amqp_client.callRpc(
        node,
        method,
        args,
        callbacks
      );
    });
  };

  return that;
}

module.exports = RpcChannel;

