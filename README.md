## Open WebRTC Toolkit Media Server

The media server for OWT provides an efficient video conference and streaming service that is based on WebRTC. It scales a single WebRTC stream out to many endpoints. At the same time, it enables media analytics capabilities for media streams. It features:

- Distributed, scalable, and reliable SFU + MCU server
- High performance VP8, VP9, H.264, and HEVC real-time transcoding on Intel® Core™ and Intel® Xeon® processors
- Wide streaming protocols support including WebRTC, RTSP, RTMP, HLS, MPEG-DASH
- Efficient mixing of HD video streams to save bandwidth and power on mobile devices
- Intelligent Quality of Service (QoS) control mechanisms that adapt to different network environments
- Customer defined media analytics plugins to perform analytics on streams from MCU
- The usage scenarios for real-time media stream analytics including but not limited to movement/object detection

## How to install develop dependency

- Interactive mode: `scripts/installDeps.sh`
- Non-interactive mode: `scripts/installDepsUnattended.sh`

## How to build release package
1. Build native components: `scripts/build.js -t all --check`
2. Pack built components and js files: `scripts/pack.js -t all --install-module --sample-path ${webrtc-javascript-sdk-sample-conference-dist}`.
3. For other build & pack options, run the scripts above with "--help".

## Where to find API documents
See "doc/servermd/Server.md" and "doc/servermd/RESTAPI.md".

## How to contribute
We warmly welcome community contributions to Open WebRTC Toolkit Media Server repository. If you are willing to contribute your features and ideas to OWT, follow the process below:
- Make sure your patch will not break anything, including all the build and tests
- Submit a pull request onto https://github.com/open-webrtc-toolkit/owt-server/pulls
- Watch your patch for review comments if any, until it is accepted and merged
OWT project is licensed under Apache License, Version 2.0. By contributing to the project, you agree to the license and copyright terms therein and release your contributions under these terms.

## How to report issues
Use the "Issues" tab on Github.

## See Also
http://webrtc.intel.com
