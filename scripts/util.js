// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// utils for packing

/*
 *@class OptParser
 *@desc Parser for command line arguments.
 */
class OptParser {
  /*
   *@constructor
   */
  constructor() {
    this.shortMap = {};
    this.longMap = {};
  }

  /*
   *@function addOption
   *@param {string} all arguments, 'boolean' | 'string' | 'list' for valueType
   *@return {boolean} whether succeed
   *@example op.addOption('t', 'target', 'list', 'pack target')
   */
  addOption(shortName, longName, valueType, description) {
    if (!['boolean', 'string', 'list'].includes(valueType)) return false;

    var newOpt = { type: valueType, desc: description, name: longName };
    this.shortMap[shortName] = newOpt;
    this.longMap[longName] = newOpt;

    return true;
  }

  /*
   *@function printHelp
   */
  printHelp() {
    var name, opt, val;
    var slen = 0, llen = 0;
    for (name in this.shortMap) {
      opt = this.shortMap[name];
      if (name.length > slen) slen = name.length;
      if (opt.name.length > llen) llen = opt.name.length;
    }

    var genStrN = (n) => {
      var ret = '';
      while (n > 0) {
        ret += ' ';
        n--;
      }
      return ret;
    };
    var padding;
    for (name in this.shortMap) {
      opt = this.shortMap[name];
      padding = slen + llen - name.length - opt.name.length;
      console.log(`-${name},--${opt.name} ${genStrN(padding)} ${opt.desc || ''}`);
    }
  }

  /*
   *@function parseArgs
   *@param {array} args
   *@return {object} parsed result
   *@example op.parseArgs(['-t', 'portal'])
   */
  parseArgs(args) {
    var i = 0;
    var arg, opt;
    var ret = {};
    while (i < args.length) {
      arg = args[i];
      i++;

      let k = 0;
      while (arg[k] === '-') k++;

      let argName = arg.substr(k);
      if (k === 1) {
        opt = this.shortMap[argName];
      } else if (k === 2) {
        opt = this.longMap[argName];
      }

      if (!opt) continue;

      if (opt.type === 'boolean') {
        ret[opt.name] = true;
      } else if (opt.type === 'string') {
        if (i >= args.length || args[i][0] === '-') {
          console.log(`\x1b[33mWarning: [${arg}] Missing argument\x1b[0m`);
          continue;
        }
        ret[opt.name] = args[i];
        i++;
      } else if (opt.type === 'list' && i < args.length) {
        if (!ret[opt.name]) ret[opt.name] = [];
        ret[opt.name].push(args[i]);
        i++;
      }
    }

    return ret;
  }
}

const { exec } = require('child_process');

/*
 *@function wrapExec
 *@desc Promise Wrapper of child_process exec
 *@param same as child_process exec
 */
function wrapExec(...args) {
  return new Promise((resolve, reject) => {
    exec(...args, (error, stdout, stderr) => {
      //if (stdout) console.log(stdout);
      if (error) {
        if (stderr.trim()) {
          console.log('\x1b[31mCommandFailed:', args[0], '\x1b[0m');
          console.error(stderr.trim());
        }
        reject(error, stderr);
      } else {
        resolve(stdout, stderr);
      }
    });
  });
}

exports.OptParser = OptParser;
exports.exec = wrapExec;