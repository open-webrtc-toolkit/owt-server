mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(shell dirname $(mkfile_path))

all:
	@cd $(current_dir)/scripts; grunt
	#cd ../; rm -f woogeen.sdk.js; ln -s scripts/dist/woogeen.sdk-2.0.0.js woogeen.sdk.js
	cp scripts/dist/woogeen.sdk-2.0.0.min.js ../extras/basic_example/public/woogeen.sdk.js
	cp scripts/dist/woogeen.sdk.ui-2.0.0.min.js ../extras/basic_example/public/woogeen.sdk.ui.js

clean:
	@cd $(current_dir) && rm -f woogeen.sdk.js
