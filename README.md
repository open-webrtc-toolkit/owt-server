# WooGeen 2.0 Build & Package Brief Instruction

## Dependency

- Interactive mode: `scripts/installDeps.sh --disable-nonfree`
- Non-interactive mode: `scripts/installDepsUnattended.sh --disable-nonfree`.


## MCU

### Build

- without msdk: `scripts/build.sh --mcu`
- with msdk: `scripts/build.sh --mcu-hardware`
- both: `scripts/build.sh --mcu-all`

### Package

- release: `scripts/pack.js --full --sample-path ${webrtc-javascript-sdk}/dist/samples/conference`
- release alias: `scripts/pack.js --target all --repack --install-module --sample-path ${webrtc-javascript-sdk}/dist/samples/conference`
- without repack, node modules and sample: `scripts/pack.js --target (or -t) all`
- debug: `scripts/pack.js --target all --debug`
- pack specified target: `scripts/pack.js --target ${target-name} [ --target ${target-name} ]`
- use help option for details: `scripts/pack.js --help`

### Run

    - `cd dist`
    - follow sections 2.3.6 and 2.3.7 from user guide doc/servermd/Server.md

## Gateway

### Build

- `scripts/build.sh --gateway --sdk`

### Package

- `scripts/release/pack.sh --gateway`

- run in package:

    - `cd dist`
    - `bin/start-all.sh`
