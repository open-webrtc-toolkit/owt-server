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

- `scripts/release/pack.sh --mcu --src-sample-path=${webrtc-javascript-sdk}/dist/samples/conference`

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
