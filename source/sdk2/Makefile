mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(shell dirname $(mkfile_path))

all:
	@cd $(current_dir)/scripts; grunt
	cp scripts/dist/woogeen.sdk-2.0.0.min.js ../extras/basic_example/public/woogeen.sdk.js
	cp scripts/dist/woogeen.sdk.ui-2.0.0.min.js ../extras/basic_example/public/woogeen.sdk.ui.js

dep:
	@cd $(current_dir)/scripts; mkdir -p node_modules && \
 	npm install --prefix . --development --loglevel error

clean:
	@cd $(current_dir) && rm -f ../extras/basic_example/public/woogeen.sdk*.js
