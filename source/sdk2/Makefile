mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(shell dirname $(mkfile_path))
version := $(shell grep "version" scripts/package.json | cut -d '"' -f 4)

all: build
	cp scripts/dist/woogeen.sdk-$(version).min.js ../extras/basic_example/public/woogeen.sdk.js
	cp scripts/dist/woogeen.sdk.ui-$(version).min.js ../extras/basic_example/public/woogeen.sdk.ui.js

build:
	@cd $(current_dir)/scripts; grunt

debug: build
	cp scripts/dist/woogeen.sdk-$(version).js ../extras/basic_example/public/woogeen.sdk.js
	cp scripts/dist/woogeen.sdk.ui-$(version).js ../extras/basic_example/public/woogeen.sdk.ui.js


dep:
	@cd $(current_dir)/scripts; mkdir -p node_modules && \
	sudo -E npm install --prefix . --development --loglevel error

clean:
	@cd $(current_dir) && rm -f ../extras/basic_example/public/woogeen.sdk*.js
