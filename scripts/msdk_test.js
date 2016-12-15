#!/usr/bin/env node
'use strict';

var path = require('path');
var module = process.argv[2];
try {
  require(module);
  console.log('[PASS]', path.basename(module));
} catch (e) {
  console.log('[FAIL]', path.basename(module), e);
}
