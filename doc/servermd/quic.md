# Steps to run OWT server with QUIC enabled

QUIC support is working in progress. It is enabled in QUIC addon only. It is not integrated into any agent so far.

### Build OWT QUIC SDK

OWT QUIC SDK is a Chromium based project. However, it is not publicly available right now. Please follow internal instructions in QUIC SDK repo to build it. 

### Build and try OWT server QUIC addon

QUIC addon provides WebTransport and P2P QUIC style JavaScript APIs for node.js developers.

1. Install dependencies `scripts/installDeps.sh`.
1. Run `./build.js -t quic` in `scripts` directory to build QUIC addon.
1. You are able to run tests for QUIC addon, and try a connection between the addon and browser.


QUIC tests could be found in `source/agent/addons/quic/test`. If you want to try how it works with browser, please run `Create an RTCQuicTransport and listen` case of `RTCQuicTransport.js`. You need to get the client side JavaScript code from [here](https://github.com/jianjunz/owt-client-javascript/tree/quicsample/src/samples/quic) and run it in a browser supports P2P QUIC API.