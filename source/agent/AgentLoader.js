// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/**
 * Module Loader for Agents
 * Should not be packed into node
 */
'use strict';

const Module = require('module');
const path = require('path');
const debug = process.env.NODE_DEBUG_ERIZO;

var loadPrefix = '';
try {
  loadPrefix = (Module._load('./loader.json').lib || '');
} catch (e) {
  // No loader.json
}

function checkError(e) {
  if (e.code !== 'MODULE_NOT_FOUND') {
    console.log(e);
  }
}

/*
 * Agent-Module Load Orders
 * Debug:
 * A. For path like 'my-module'
 *  1. require('my-module')
 *  2. require(`./${node-folder}/my-module`)
 * B. For path like './my-module'
 *  1. require('./my-module')
 *  2. require(`./${node-folder}/my-module`)
 *
 * Release:
 * A. For path like 'my-module'
 *  1. require('my-module')
 *  2. Module._load('./my-module')
 * B. For path like './my-module'
 *  1. Module._load('./my-module')
 *  2. require('my-module')
 *  3. require('my-module/index')
 *  4. require('${node-folder}/my-module')
 * Will fail if all attempts fail
 */
module.exports = function (loadPath) {
  var ret;
  if (!debug) {
    // Loader in non-debug mode
    try {
      // Directly
      ret = require(loadPath);
      return ret;
    } catch (e) {
      checkError(e);
    }
    // Try node folder
    return require('./' + path.join(loadPrefix, loadPath));
  }

  // Loader in debug mode
  var normalPath = path.normalize(loadPath);
  if (normalPath === loadPath) {
    try {
      // Require native
      ret = require(loadPath);
      return ret;
    } catch (e) {
      checkError(e);
    }
    // Require deps like 'amqp'
    ret = Module._load('./' + loadPath);
    return ret;
  } else {
    try {
      // Require js file
      ret = Module._load(loadPath);
      return ret;
    } catch (e) {
      checkError(e);
    }
    try {
      // Require native
      ret = require(normalPath);
      return ret;
    } catch (e) {
      checkError(e);
    }
    try {
      // Require native index
      ret = require(path.join(normalPath, 'index'));
      return ret;
    } catch (e) {
      checkError(e);
    }
    // Require native with load prefix
    ret = require(path.join(loadPrefix, normalPath));
    return ret;
  }
};
