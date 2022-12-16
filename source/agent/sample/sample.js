// Sample Agent Server

'use strict';

const log = require('./logger').logger.getLogger('SampleNode');

/*
 * {rpcClient} is the object used send RPC request.
 * {rpcID} is the rpcID for this RPC server.
 * {parentID} is the rpcID for its parent process.
 * {clusterWorkerIP} is the IP saved in cluster.
 */
function RPCInterface(rpcClient, rpcID, parentID, clusterWorkerIP) {
  log.info('Sample RPC:', rpcID, parentID, clusterWorkerIP);
  const that = {};
  let receiver = null;

  that.process = function (data, callback) {
    log.info('Process:', data);
    callback('callback', 'ok');
  };

  return that;
}

module.exports = RPCInterface;
