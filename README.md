## Open WebRTC Toolkit Media Server

Open WebRTC Toolkit Media Server...

## How to install develop dependency

- Interactive mode: `scripts/installDeps.sh --disable-nonfree`
- Non-interactive mode: `scripts/installDepsUnattended.sh --disable-nonfree`.

## How to build release package
1. Build native components: `scripts/build.js -t mcu --check`
2. Pack built components and js files: `scripts/pack.js -t all --install-module --sample-path ${webrtc-javascript-sdk-sample-conference-dist`.
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
