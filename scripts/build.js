#!/usr/bin/env node
// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// building script
const fs = require('fs');
const os = require('os');
const path = require('path');
const { chdir, cwd } = process;
const { execSync } = require('child_process');
const { OptParser, exec } = require('./util');
const buildTargets = require('./build.json');

const optParser = new OptParser();
optParser.addOption('l', 'list', 'boolean', 'List avaliable build targets');
optParser.addOption('t', 'target', 'list', 'Specify target to build (Eg. build.js -t video-mixer-sw -t video-transcoder-sw)');
optParser.addOption('d', 'debug', 'boolean', 'Whether build debug addon (Please add debug-addon option in packing to pack debug addon)');
optParser.addOption('v', 'verbose', 'boolean', 'Whether use verbose level in building');
optParser.addOption('r', 'rebuild', 'boolean', 'Whether clean before build');
optParser.addOption('c', 'check', 'boolean', 'Whether check after build');
optParser.addOption('j', 'jobs', 'string', 'Number of concurrent build jobs');
optParser.addOption('f', 'features', 'list', 'Specify experinmental features to be enabled. Available features: quic.')

const options = optParser.parseArgs(process.argv);

const rootDir = path.join(path.dirname(module.filename), '..');
const depsDir = path.join(rootDir, 'build/libdeps/build');
const originCwd = cwd();

// Detect OS script
const osScript = path.join(rootDir, 'scripts/detectOS.sh');
const osType = execSync(`. ${osScript}`).toString().toLowerCase();
const msdkDir = '/opt/intel/mediasdk';

function getTargets() {
  var buildSet = new Set();
  const getIncludings = (includes) => {
    var childIncludes;
    for (const name of includes) {
      if (!buildTargets[name] || buildSet.has(name)) continue;

      if (buildTargets[name].include) {
        getIncludings(buildTargets[name].include);
      } else {
        buildSet.add(name);
      }
    }
  };

  getIncludings(options.target);
  return [...buildSet];
}

function listPrint() {
  console.log('Avaliable builds:');
  var outputs = [];
  for (const name in buildTargets) {
    let desc = '';
    if (buildTargets[name].description) {
      desc = '- ' + buildTargets[name].description;
    }
    outputs.push(`  ${name} ${desc}`);
  }
  outputs.sort();
  for (const line of outputs) {
    console.log(line);
  }
}

function constructBuildEnv() {
  var env = process.env;
  env['CFLAGS'] = '-fstack-protector -Wformat -Wformat-security';
  env['CXXFLAGS'] = env['CFLAGS'];
  env['LDFLAGS'] = '-z noexecstack -z relro';
  env['CORE_HOME'] = path.join(rootDir, 'source/core');
  env['PKG_CONFIG_PATH'] = path.join(depsDir, 'lib/pkgconfig') +
      ':' + path.join(depsDir, 'lib64/pkgconfig') +
      ':' + (env['PKG_CONFIG_PATH'] || '');
  usergcc = path.join(depsDir, 'bin/gcc');
  usergxx = path.join(depsDir, 'bin/g++');
  // Use user compiler if exists
  if (fs.existsSync(usergcc) && fs.existsSync(usergxx)) {
    env['CC'] = usergcc;
    env['CXX'] = usergxx;
  }

  if (options.debug) {
    env['OPTIMIZATION_LEVEL'] = '0';
  } else {
    env['OPTIMIZATION_LEVEL'] = '3';
    env['CFLAGS'] = env['CFLAGS'] + ' -D_FORTIFY_SOURCE=2';
    env['CXXFLAGS'] = env['CFLAGS'];
  }

  if (options.features && options.features.includes('quic')) {
    env['GYP_DEFINES'] += [' owt_enable_quic=1'];
  }

  console.log(env['PKG_CONFIG_PATH']);

  return env;
}

// Common build commands
var cpuCount = (Number(options.jobs) || os.cpus().length);
logLevel = options.verbose ? 'verbose' : 'error';
rebuildArgs = ['node-gyp', 'rebuild', `-j ${cpuCount}`, '--loglevel=' + logLevel];
configureArgs = ['node-gyp', 'configure', '--loglevel=' + logLevel];
buildArgs = ['node-gyp', 'build', `-j ${cpuCount}`, '--loglevel=' + logLevel];

if (options.debug) {
  rebuildArgs.push('--debug');
  configureArgs.push('--debug');
  buildArgs.push('--debug');
}

// Build single target
function buildTarget(name) {
  console.log('\x1b[32mBuilding addon\x1b[0m -', name);
  const target = buildTargets[name];
  if (!target.path) {
    console.log('\x1b[31mNo binding.gyp:', name, '\x1b[0m');
    return;
  }

  const buildPath = path.join(rootDir, target.path);
  var copyGyp = (target.gyp && target.gyp !== 'binding.gyp') ? true : false;

  chdir(buildPath);
  if (copyGyp) execSync(`cp ${target.gyp} binding.gyp`);

  var stdio = [null, process.stdout, process.stderr];
  if (options.rebuild) {
    execSync(rebuildArgs.join(' '), { stdio });
  } else {
    execSync(configureArgs.join(' '), { stdio });
    execSync(buildArgs.join(' '), { stdio });
  }

  if (copyGyp) execSync(`rm ${path.join(rootDir, target.path, 'binding.gyp')}`);
  console.log('\x1b[32mFinish addon\x1b[0m -', name);
}

if (options.list) {
  listPrint();
  process.exit(0);
}

if (!options.target || !options.target.length) {
  optParser.printHelp();
  process.exit(0);
}

constructBuildEnv();
var buildList = getTargets();
console.log('\x1b[32mFollowing targets will be built:\x1b[0m');
for (const name of buildList) {
  console.log('', name, '');
}
var works = buildList.map((name) => {
  if (name.indexOf('msdk') > 0 && !fs.existsSync(msdkDir)) {
    console.log(`\x1b[33mSkip: ${name} - MSDK not installed\x1b[0m`);
    return Promise.resolve();
  }
  return buildTarget(name);
});

Promise.all(works)
  .then(() => {
    if (options.check) {
      moduleTestScript = path.join(rootDir, 'scripts/module_test.js');
      runtimeAddonDir = path.join(rootDir, 'source/agent');
      console.log('* Checking modules...');
      let checkOutput = execSync(`node ${moduleTestScript} ${runtimeAddonDir}`).toString();
      console.log(checkOutput);
    }
  });
