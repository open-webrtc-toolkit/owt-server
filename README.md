## Open WebRTC Toolkit Media Server

The media server for OWT provides an efficient video conference and streaming service that is based on WebRTC. It scales a single WebRTC stream out to many endpoints. At the same time, it enables media analytics capabilities for media streams. It features:

- Distributed, scalable, and reliable SFU + MCU server
- High performance VP8, VP9, H.264, and HEVC real-time transcoding on Intel® Core™ and Intel® Xeon® processors
- Wide streaming protocols support including WebRTC, RTSP, RTMP, HLS, MPEG-DASH
- Efficient mixing of HD video streams to save bandwidth and power on mobile devices
- Intelligent Quality of Service (QoS) control mechanisms that adapt to different network environments
- Customer defined media analytics plugins to perform analytics on streams from MCU
- The usage scenarios for real-time media stream analytics including but not limited to movement/object detection

## How to install development dependencies

In the repository root, use one of following commands to install the dependencies.
- Interactive mode: `scripts/installDeps.sh`
- Non-interactive mode: `scripts/installDepsUnattended.sh`
In interactive mode, you need type "yes" to continue installation several times and in non-interactive, the installation continues automatically.

## How to build release package

### Requirements
The media server can be built on the following platforms:
1. Ubuntu 18.04
2. CentOS 7.6

### Instructions
In the root of the repository:
1. Build native components: `scripts/build.js -t all --check`.
2. Pack built components and js files: `scripts/pack.js -t all --install-module --app-path ${webrtc-javascript-sdk-sample-conference-dist}`.

The ${webrtc-javascript-sdk-sample-conference-dist} is built from owt-javascript-sdk, e.g. `~/owt-client-javascript/dist/sample/conference`, see https://github.com/open-webrtc-toolkit/owt-client-javascript for details.

If "--archive ${name}" option is appended to the pack command, a "Release-${name}.tgz" file will be generated in root folder. For other options, run the scripts with "--help" option.

## Quick Start
In the repository root, run the following commands to start the media server on a single machine:
1. `./bin/init-all.sh --deps`
2. `./bin/start-all.sh`
3. Open https://localhost:3004 to visit the web sample page. Due to the test certificate, you may need confirm this unsafe access.

## Where to find API documents
See [doc/servermd/Server.md](doc/servermd/Server.md) and [doc/servermd/RESTAPI.md](doc/servermd/RESTAPI.md).

## Build a Docker image with your app
Run the build_server.sh script located in docker/conference. It has one required flag, -p, which should contain the filepath of your app. Optional flags are -i for the final Docker image name, and -n
which will make the Docker build run with --no-cache. An example usecase:
```
./docker/conference/build_server.sh -p ~/my_app -i myapp_img
```

## How to contribute
We warmly welcome community contributions to Open WebRTC Toolkit Media Server repository. If you are willing to contribute your features and ideas to OWT, follow the process below:
- Make sure your patch will not break anything, including all the build and tests
- Submit a pull request onto https://github.com/open-webrtc-toolkit/owt-server/pulls
- Watch your patch for review comments, if any, until it is accepted and merged. The OWT project is licensed under Apache License, Version 2.0. By contributing to the project, you agree to the license and copyright terms therein and release your contributions under these terms.

## How to report issues
Use the "Issues" tab on Github.

## See Also
http://webrtc.intel.com

