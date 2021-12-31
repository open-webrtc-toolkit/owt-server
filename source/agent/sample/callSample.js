// Call to Sample Agent

'use strict';

const amqper = require('./amqpClient')();
const RPCChannel = require('./rpcChannel');

const rabbitConfig = {host: 'localhost', port: 5672};

if (process.argv.length < 3) {
  console.error('Usage: node callSample.js [RPC_ID]');
  process.exit();
}

const rpcId = process.argv[2];

function cleanAndExit() {
  amqper.disconnect()
    .then(() => {
      process.exit();
    }).catch((e) => {
      console.error('Error during disconnect:', e);
    });
}

amqper.connect(rabbitConfig, function connectOk() {
  amqper.asRpcClient(function clientOk(rpcClient) {
    const rpcChannel = RPCChannel(rpcClient);
    // Make RPC to sample agent
    rpcChannel.makeRPC(rpcId, 'process', ['Test data!'])
      .then((ok) => {
        console.info('Call RPC success!');
        cleanAndExit();
      })
      .catch((e) => console.info('Call RPC failed:', e));

  }, function clientFail(reason) {
    console.error('Setup client failed, reason:', reason.stack);
    cleanAndExit();
  });
}, function connectError(reason) {
  console.error('Connect failed, reason:', reason.stack);
  cleanAndExit();
});
