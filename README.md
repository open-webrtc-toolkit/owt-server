# WooGeen 2.0 Build & Package Brief Instruction

## Dependency

- on Ubuntu: `scripts/installUbuntuDeps.sh`
- on CentOS: `scripts/installCentOSDeps.sh`.


## MCU

### Build

- without msdk: `scripts/build.sh --mcu --sdk`
- with msdk: `scripts/build.sh --mcu-hardware --sdk`
- both: `scripts/build.sh --mcu-all --sdk`

### Package

- `scripts/release/pack.sh --mcu`

- run in package:

    - `cd dist`
    - `bin/init.sh` for software; `bin/init.sh --hardware` for hardware.
    - `bin/start-all.sh`

## Gateway

### Build

- `scripts/build.sh --gateway --sdk`

### Package

- `scripts/release/pack.sh --gateway`

- run in package:

    - `cd dist`
    - `bin/start-all.sh`
