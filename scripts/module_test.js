#!/usr/bin/env node
'use strict';

var path = require('path');
if (!process.env.MODULE_TEST) {
  process.env.MODULE_TEST = true;
  process.env.LD_LIBRARY_PATH = [
    path.resolve(__dirname, '../build/libdeps/build/lib'),
    path.resolve(__dirname, '../third_party/openh264'),
    process.env.LD_LIBRARY_PATH || '',
  ].join(':');
  require('child_process').fork(process.argv[1], process.argv.slice(2));
} else {
  var dirname = process.argv[2] || __dirname+'/../source';
  require('child_process').exec('find '+dirname+' -path */build/Release/obj.target -prune -o -type f -name *.node -print', function (error, stdout) {
    if (error !== null) {
      console.log('[ERROR]', error);
      return;
    }
    var addons = stdout.toString();
    if (addons.length > 0) {
      return addons.split('\n').filter(function (p) {
        return p.length > 0;
      }).map(function (module) {
        try {
          module = path.resolve(process.cwd(), module);
          require(module);
          console.log('[PASS]', path.basename(module));
        } catch (e) {
          console.log('[FAIL]', path.basename(module), e);
        }
      });
    }
    console.log('[ERROR]', 'addons not found');
  });
}
