# Steps to run OWT server with QUIC enabled.

## Build and Run

### Build Chromium
1. Get Chromium code. QUIC implementation depends on [QUICHE](https://quiche.googlesource.com/quiche.git). Please download the entire Chromium code according to [Chromium Linux build instruction](https://chromium.googlesource.com/chromium/src/+/master/docs/linux_build_instructions.md). The version we are using is 35a38537d3dfbc276f813c9201c674ae241ff4ae.
1. Apply [patches](../../scripts/patches) to Chromium. If you don't want to build Chromium with GCC, the first patch may not neccessary. But other depencies are built by GCC, I'm not sure if there would be any issue during link stage.
1. Build target net_quic. `ninja -C <dir> net_quic`.

### Build OWT server

It basically follows the steps in [README.md](../../README.md#how-to-build-release-package). The change we made is to replace webrtc agent with a webrtc-quic agent.

1. Install dependencies `scripts/installDeps.sh`.
1. Build conference server with the scripts [here](../../scripts/build.js), `scripts/build.js -t mcu`. If you just want to build the QUIC module, please run `scripts/build.js -t webrtc-quic`. You may use this command after making some changes to quic module. Please build webrtc-quic without OpenSSL, see section OpenSSL and BoringSSL below.
1. Generate server package by `scripts/pack.js -f --sample-path <Path to JavaScript package>`.

### Run OWT server
Please follow [Quick Start](https://github.com/open-webrtc-toolkit/owt-server#quick-start) section to start conference server.

## OpenSSL and BoringSSL

As node.js depends on OpenSSL while QUICHE depends on BoringSSL, there will be macro redefinitions and symbol conflicts during building and linking. In order to resolve this issue, we'll split webrtc-quic agent into two shared libraries. Before this, we need a workaround to make it work.

### A workaround

1. After you install node.js and node-gyp. Rename `~/.node-gyp/<version>/include/node/openssl` to a different name. So OpenSSL headers will not be included.
1. Run webrtc-agent with a special `node`. You may find a `snode` in the [shared folder](\\kona.sh.intel.com\WebRTC\VolumetricStreaming). OpenSSL is not included in snode.
1. Build webrtc-quic with this workaround enabled, and build all other modules without this workaround.

You may use [nvm](https://github.com/nvm-sh/nvm) to quickly switch between different versions of Node.js. For example, you may install Node.js v8.15 and v8.16 through `nvm` on the same machine. Performaning the workaround for v8.15 will not impact 8.16.