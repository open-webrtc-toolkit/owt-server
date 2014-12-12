# WooGeen 2.0 Build & Package Brief Instruction

## Dependency

- on Ubuntu: `scripts/installUbuntuDeps.sh`
- on CentOS: `scripts/installCentOSDeps.sh`.


## MCU

### Build

- `scripts/build.sh --mcu --sdk`

### Package

- `scripts/release/pack.sh --mcu`

- run in package:

    - `cd dist`
    - `bin/init.sh`
    - `bin/start-all.sh`

## Gateway

### Build

- `scripts/build.sh --gateway --sdk`

### Package

- `scripts/release/pack.sh --gateway`

- run in package:

    - `cd dist`
    - `bin/start-all.sh`
