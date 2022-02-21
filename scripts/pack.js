#!/usr/bin/env node

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// packing script
const fs = require('fs');
const path = require('path');
const { chdir, cwd } = process;
const { execSync } = require('child_process');
const { OptParser, exec } = require('./util');

const optParser = new OptParser();
optParser.addOption('l', 'list', 'boolean', 'List avaliable pack targets');
optParser.addOption('t', 'target', 'list', 'Specify target to pack (Eg. pack.js -t portal -t webrtc-agent)');
optParser.addOption('i', 'install-module', 'boolean', 'Whether install node module after packing (Eg. pack.js -t portal -i)');
optParser.addOption('r', 'repack', 'boolean', 'Whether clean dist before pack (Eg. pack.js -t portal -r)');
optParser.addOption('e', 'encrypt', 'boolean', 'Whether encrypt during pack (Eg. pack.js -t portal -e)');
optParser.addOption('d', 'debug', 'boolean', '(Disabled)');
optParser.addOption('o', 'addon-debug', 'boolean', 'Whether pack debug addon (Eg. pack.js -t webrtc-agent -o)');
optParser.addOption('f', 'full', 'boolean', 'Whether perform a full pack (--full is the equalivation of pack.js -t all -r -i). Experimental features are not included, please include them explicitly with -t.');
optParser.addOption('p', 'app-path', 'string', 'Specify app path (Eg. pack.js -t all --app-path ${appPath})');
optParser.addOption('a', 'archive', 'string', 'Specify archive name (Eg. pack.js -t all -a ${archiveName})');
optParser.addOption('n', 'node-module-path', 'string', 'Specify shared-node-module directory');
optParser.addOption('c', 'copy-module-path', 'string', 'Specify copy node modules directory');
optParser.addOption('b', 'binary', 'boolean', 'Pack binary');
optParser.addOption('np', 'no-pseudo', 'boolean', 'Whether use pseudo library');
optParser.addOption('wf', 'with-ffmpeg', 'boolean', 'Whether pack ffmpeg library');
optParser.addOption('h', 'help', 'boolean', 'Show help');
optParser.addOption('li', 'lint', 'boolean', 'Whether lint code with eslint');

const options = optParser.parseArgs(process.argv);

const rootDir = path.join(path.dirname(module.filename), '..');
const distDir = path.join(rootDir, options.debug ? 'dist-debug' : 'dist');
const depsDir = path.join(rootDir, 'build/libdeps/build');
const originCwd = cwd();

// Detect OS script
const osScript = path.join(rootDir, 'scripts/detectOS.sh');
const osType = execSync(`bash ${osScript}`).toString().toLowerCase();

const experimentalTargets = ['quic-agent'];

var allTargets = [];

if (options.full) {
  if (!options.target) {
    options.target = [];
  }
  options.target.push('all');
  options.repack = true;
  options['install-module'] = true;
}

if (options.help || Object.keys(options).length === 0) {
  optParser.printHelp();
  process.exit(0);
}

if (options.lint) {
  // Check lint deps
  const lintDeps = ['eslint'];
  console.log('Checking lint dependencies...');
  const npmRoot = execSync(`npm root -g`).toString().trim();
  const missingLintDeps = lintDeps.filter((dep) => {
    return !fs.existsSync(path.join(npmRoot, dep));
  });

  if (missingLintDeps.length === 0) {
    console.log('Lint dependencies OK.');
  } else {
    for (const dep of missingLintDeps) {
      console.log('Installing eslint');
      execSync(`npm install eslint --global --save-dev`);
    }
  }
}

var npmInstallOption = '';
if (process.getuid && process.getuid() === 0) {
  // Running as root
  console.log('\x1b[33mWarning: running as root\x1b[0m');
  npmInstallOption += ' --unsafe-perm';
}
if (options.archive) {
  npmInstallOption += ' --production';
}

if (options.encrypt) {
  const encryptDeps = [
    'uglify-es',
  ];

  // Check encrypt deps
  console.log('Checking encrypt dependencies...');
  const npmRoot = execSync(`npm root -g`).toString().trim();
  const missingDeps = encryptDeps.filter((dep) => {
    return !fs.existsSync(path.join(npmRoot, dep));
  });

  if (missingDeps.length === 0) {
    console.log('Encrypt dependencies OK.');
  } else {
    for (const dep of missingDeps) {
      execSync(`npm install -g --save-dev ${dep}`, { stdio: 'inherit' });
    }
  }
}

if (options.binary) {
  try {
    execSync('npm list -g pkg@4.2.5');
  } catch (e) {
    execSync('npm install -g pkg@4.2.5');
  }
  options['install-module'] = true;
}

function getTargets() {
  return exec(`find ${rootDir}/source -type f -name "dist.json"`)
  .then((packs) => {
    var lines = packs.split('\n');
    var line, target;
    var targets = [];
    for (line of lines) {
      if (line) targets.push({ rules: require(line), path: line });
    }
    allTargets = targets;
    return targets;
  }).catch((e) => {
    console.warn('Fail to get targets:', e.message);
    return [];
  });
}

function getPackList(targets) {
  if (options.list) {
    // List targets
    console.log('---------------------');
    console.log('Avalible Targets Are:');
    console.log('---------------------');
    for (const target of targets) {
      let targetTitle = `\x1b[32m${target.rules.name}\x1b[0m`;
      if (experimentalTargets.includes(target.rules.name)) {
        targetTitle += ' (experimental feature)'
      }
      console.log(targetTitle);
      console.log(' ->', target.path);
    }
    process.exit(0);
  }

  if (!options.target) {
    throw new Error('No target specified');
  }

  var packList = targets.filter((element) => {
    // Experimental features are not included by default.
    if (options.target.includes('all') && !experimentalTargets.includes(element.rules.name)) return true;
    return options.target.includes(element.rules.name);
  });
  if (packList.length === 0) {
    throw new Error('No available target to pack');
  }
  return packList;
}

function cleanIfRepack(targets) {
  if (options.repack) {
    return exec(`rm -rf ${distDir}`)
      .then((ret) => {
        return targets;
      });
  }
  return targets;
}

function processTargets(targets) {
  return exec(`mkdir -p ${distDir}`)
    .then((mkdir) => {
      packings = [];
      for (const target of targets) {
        packings.push(packTarget(target));
      }
      return Promise.all(packings)
        .then(() => {
          return targets;
        });
    });
}

function packTarget(target) {
  if (!target.rules.dest) {
    throw new Error(`${target.path}: No "dest" field`);
  }
  const packDist = path.join(distDir, target.rules.dest);
  // Change to dist directory
  chdir(distDir);
  return exec(`mkdir -p ${packDist}`)
    .then((mkdir) => {
      return packCommon(target);
    })
    .then((commonPacked) => {
      return packAddon(target);
    })
    .then((addonPacked) => {
      //generateStart(target);
      if (options.binary) {
        let pkg = require(path.join(packDist, 'package.json')).pkg;
        if (pkg) {
          chdir(packDist);
          if (fs.existsSync('erizoJS.js')) {
            execSync(`pkg erizoJS.js -t ${pkg.targets}`);
          }
          if (fs.existsSync('initdb.js')) {
            execSync(`pkg initdb.js -t ${pkg.targets}`);
          }
          if (fs.existsSync('initcert.js')) {
            execSync(`pkg initcert.js -t ${pkg.targets}`);
          }
          execSync('pkg .');
          execSync('rm -rf auth');
          execSync('rm -rf data_access');
          execSync('rm -rf resource');
          execSync('rm -rf rpc');
          execSync('rm -rf package.json');
          execSync('rm -rf dist.json');
          if (!target.rules.node) {
            execSync('rm -rf node_modules');
          }
        }
      }
      if (options.encrypt)
        return encrypt(target);
    });
}

function packCommon(target) {
  if (!target.rules.common) return;
  const common = target.rules.common;
  const debugItems = target.rules.debug;
  const packSrc = path.dirname(target.path);
  const packDist = path.join(distDir, target.rules.dest);

  console.log('\x1b[32mPack common\x1b[0m -' , target.rules.name);
  console.log(packSrc, '->', packDist);

  if (options.debug && debugItems) {
    // Pack debug files as common
    if (debugItems.files) {
      common.files = (common.files || []).concat(debugItems.files);
    }
    if (debugItems.folders) {
      common.folders = Object.assign((common.folders || {}), debugItems.folders);
    }
  }
  if (common.files) {
    // Copy common files
    for (const file of common.files) {
      const filePath = path.join(packSrc, file);
      const extname = path.extname(filePath);
      if (options.lint && extname === '.js') {
        try {
          execSync(`eslint -c ${rootDir}/source/.eslintrc.json ${filePath}`);
        } catch(error) {
          console.error(error.stdout.toString());
        }
      }
      execSync(`cp -a ${filePath} ${packDist}`);
    }
  }
  if (common.folders) {
    // Copy common folders
    for (const folder in common.folders) {
      let folderDist = path.join(packDist, folder);
      execSync(`mkdir -p ${folderDist}`);
      for (const file of common.folders[folder]) {
        // Copy in-folder files or folders
        let filePath = path.join(packSrc, file);
        execSync(`cp -a ${filePath} ${folderDist}`);
      }
    }
  }

  var npmInstall;
  if (common.package) {
    const filePath = path.join(packSrc, common.package);
    execSync(`cp ${filePath} ${packDist}`);

    if (options['copy-module-path']) {
      const copySrc = path.join(options['copy-module-path'], target.rules.dest);
      execSync(`cp -a ${copySrc}/node_modules ${packDist}`);
      npmInstall = 'ok';
    } else if (options['install-module']) {
      chdir(distDir);
      // Install node modules
      var installDist = packDist;
      if (options['node-module-path']) {
        // Use a shared node module directory
        installDist = options['node-module-path'];
        execSync(`mkdir -p ${installDist}`);
        execSync(`cp ${filePath} ${installDist}`);
        chdir(installDist);
        execSync('npm install', { stdio: [null, process.stdout, process.stderr] });
        const packageJson = path.basename(filePath);
        execSync(`mv ${packageJson} ${packageJson}.${target.rules.name}`);
      } else {
        chdir(installDist);
        npmInstall = exec('npm install' + npmInstallOption)
          .then((stdout, stderr) => {
            stdout && console.log(stdout);
            stderr && console.log(stderr);
          });
      }
    }
  }
  console.log(target.rules.name, '- Pack common finished.');
  return npmInstall;
}

function packAddon(target) {
  if (!target.rules.natives) return;
  const natives = target.rules.natives;
  const packSrc = path.dirname(target.path);
  const packDist = path.join(distDir, target.rules.dest);
  const libDist = path.join(packDist, natives.libdist);

  console.log('\x1b[32mPack addon\x1b[0m -', target.rules.name);
  var packLibs = [];
  execSync(`rm -rf ${libDist}`);
  execSync(`mkdir -p ${libDist}`);

  for (const addon of natives.addons) {
    let buildType = options['addon-debug'] ? 'Debug' : 'Release';
    // Debug or Release addon, all pack to 'Release' folder
    let buildPath = `${path.basename(addon.folder)}/build/Release`;
    let addonDist = path.join(packDist, buildPath);
    let addonPath = path.join(packSrc, addon.folder, `build/${buildType}/${addon.name}.node`);

    if (!fs.existsSync(addonPath)) {
      console.log(`\x1b[33mWarning: ${addonPath} not exist\x1b[0m`);
      continue;
    }
    execSync(`rm -rf ${addonDist}`);
    execSync(`mkdir -p ${addonDist}`);
    execSync(`cp ${addonPath} ${addonDist}`)
    console.log(addonPath, '->', addonDist);

    // Get addon lib files
    let packLib = getAddonLibs(path.join(addonDist, `${addon.name}.node`))
      .then((libs) => {
        for (let libsrc of libs) {
          execSync(`cp -Lv ${libsrc} ${libDist}`, { stdio: 'inherit' });
        }
      });
    packLibs.push(packLib);
  }
  return Promise.all(packLibs)
    .then((libsOk) => {
      // Replace openh264 if needed
      let libOpenh264 = path.join(libDist, 'libopenh264.so.4');
      if (options['archive'] && fs.existsSync(libOpenh264) && !options['no-pseudo']) {
        let dummyOpenh264 = path.join(rootDir, 'third_party/openh264/pseudo-openh264.so');
        execSync(`cp -av ${dummyOpenh264} ${libOpenh264}`);
      }

      let libSvtHevcEnc = path.join(libDist, 'libSvtHevcEnc.so.1');
      if (options['archive'] && fs.existsSync(libSvtHevcEnc) && !options['no-pseudo']) {
        let dummySvtHevcEnc = path.join(rootDir, 'third_party/SVT-HEVC/pseudo-svtHevcEnc.so');
        execSync(`cp -av ${dummySvtHevcEnc} ${libSvtHevcEnc}`);
      }

      if (target.rules.name.indexOf('video') === 0
        || target.rules.name.indexOf('analytics') === 0) {
        if (fs.existsSync('/opt/intel/mediasdk')) {
          let dst_bin = path.join(packDist, 'bin');
          let dst_lib = path.join(packDist, 'lib');
          if (!fs.existsSync(dst_bin)) {
            execSync(`mkdir -p ${dst_bin}`);
          }

          execSync(`find /opt/intel/mediasdk -type f -name metrics_monitor  | xargs -I '{}' cp -av '{}' ${dst_bin}`);
          execSync(`find /opt/intel/mediasdk -type f -name libcttmetrics.so | xargs -I '{}' cp -av '{}' ${dst_lib}`);
        }
      }

      console.log(target.rules.name, '- Pack addon finished.');
    });
}

var checkPros = {};
function getAddonLibs(addonPath) {
  //console.log('\x1b[32mPack libs\x1b[0m', path.basename(addonPath));

  var env = {};
  for (const key in process.env) {
    env[key] = process.env[key];
  }
  env['LD_LIBRARY_PATH'] = (env['LD_LIBRARY_PATH'] || '');
  env['LD_LIBRARY_PATH'] = path.join(depsDir, 'lib') +
    ':' + path.join(rootDir, 'third_party/openh264') +
    ':' + path.join(rootDir, 'third_party/quic-lib/dist/lib') +
    ':' + env['LD_LIBRARY_PATH'];

  return exec(`ldd ${addonPath} | grep '=>' | awk '{print $3}' | sort | uniq | grep -v "^("`, { env })
    .then((stdout, stderr) => {
      var checks = [];
      for (let line of stdout.trim().split('\n')) {
        if (checkPros[line]) {
          checks.push(checkPros[line]);
          continue;
        }
        checkPros[line] = exec(`[ -s ${line} ]`)
          .then((ok) => {
            // Remove those have glibc or libselinux, libc6
            if (osType.includes('ubuntu')) {
              return exec(`dpkg -S ${line} 2>\/dev\/null`, { env })
                .then((out, err) => {
                  if (out.includes('libc6') || out.includes('libselinux')) return '';
                  return line;
                }).catch((e) => line);
            } else if (osType.includes('centos')) {
              return exec(`rpm -qf ${line} 2>\/dev\/null`, { env })
                .then((out, err) => {
                  if (out.includes('glibc')) return '';
                  return line;
                }).catch((e) => line);
            }
          })
          .catch((e) => {
            // give more detail when ldd returns somelib.so => not found
            if (!line.startsWith('/')) {
              return exec(`ldd ${addonPath} | grep '=>' | grep -v '=> /'`).then(stdout => {
                e.message = `library dependency not found for\n  ${addonPath}:\n${stdout}` +
                  'Something failed to build. Try nvm use v8.15.0 and rerun build.js.';
                throw e;
              });
            }
            throw e;
          });
        checks.push(checkPros[line]);
      }
      return Promise.all(checks);
    })
    .then((results) => {
      return results.filter(filterLib).filter(isLibAllowed);
    });
}

function isLibAllowed(libSrc) {
  if (!libSrc)
    return false;

  const whiteList = [
    'rtcadapter',
    'libssl.so.1.1',
    'libcrypto.so.1.1',
    'libnice',
    'libSvtHevcEnc',
    'libusrsctp',
    'libopenh264',
    'libre',
    'sipLib',
    'librawquic'
  ];
  if (!options['archive'] || options['with-ffmpeg']) {
    whiteList.push('libsrt');
    whiteList.push('libav');
    whiteList.push('libsw');
    if (osType.includes('centos') || (osType.includes('ubuntu') && osType.includes('20.04'))) {
      whiteList.push('libboost');
    }
  }

  const libName = path.basename(libSrc);

  var found = false;
  for (let i in whiteList) {
    if (libName.indexOf(whiteList[i]) === 0) {
      found = true;
      break;
    }
  }

  return found;
}

function filterLib(libSrc) {
  if (!libSrc) return false;

  //console.log('require:', libSrc);
  const libName = path.basename(libSrc);

  // Remove mfx/libva/libdrm
  if (libName.indexOf('libmfx') === 0) return false;
  if (libName.indexOf('libva') === 0) return false;
  if (libName.indexOf('libdrm') === 0) return false;

  // Remove libfdk-aac
  if (libName.indexOf('libfdk-aac') === 0 && options['archive']) return false;
  // Remove libav/ffmpeg if aac
  if (libName.indexOf('libav') === 0 || libName.indexOf('libsw') === 0) {
    let output = execSync(`ldd ${libSrc}`).toString();
    if (options['archive'] && output.indexOf('libfdk-aac') >= 0) {
      return false;
    }
  }
  // Remove glib
  if (libName.indexOf('libgio-2.0') === 0) return false;
  if (libName.indexOf('libglib-2.0') === 0) return false;
  if (libName.indexOf('libgmodule-2.0') === 0) return false;
  if (libName.indexOf('libgobject-2.0') === 0) return false;
  if (libName.indexOf('libgthread-2.0') === 0) return false;
  // Remove pcre
  if (libName.indexOf('libpcre') === 0) return false;
  if (libName.indexOf('libbz2') === 0) return false;
  // Remove ldap
  if (libName.indexOf('libldap') === 0) return false;
  return true;
}

function generateStart(target) {
  // Generate a start.sh
  if (!target.rules.start) return;
  const packDist = path.join(distDir, target.rules.dest);
  const stdout = `../logs/owt-${target.rules.name}.stdout`;
  const pid = `../logs/owt-${target.rules.name}.pid`;
  const command = 'nohup nice -n 0 ' + target.rules.start +
    ' > "${stdout}" 2>&1 </dev/null & echo $! > ${pid}\n' +
    'sleep 1; [[ -f ${stdout} ]] && head "$stdout"';

  chdir(distDir);
  var content = '';
  if (target.rules.node) {
    content  += 'LD_LIBRARY_PATH=' + target.rules.node.libpath + ' \\\n';
  }
  if (options['node-module-path']) {
    content  += 'NODE_PATH=' + path.resolve(options['node-module-path']) + ' \\\n';
  }
  content += 'stdout=' + stdout + '\n';
  content += 'pid=' + pid + '\n';
  content += command;

  fs.writeFileSync(`${packDist}/start.sh`, content);
}

function encrypt(target) {
  console.log('\x1b[32mEncrypt\x1b[0m -', target.rules.name);

  const packDist = path.join(distDir, target.rules.dest);
  const nodeEnv = execSync(`npm root -g`).toString().trim();
  const oldEnv = process.env['NODE_PATH'];
  process.env['NODE_PATH'] = nodeEnv;
  var env = process.env;
  // Encrypt js
  return exec(
    `find ${packDist} -path "${packDist}/public" -prune -o ` +
    `-path "*/node_modules" -prune -o -type f -name "*.js" -print`
  )
  .then((stdout, stderr) => {
    if (stdout.length === 0) {
      return Promise.resolve();
    }
    var jsJobs = stdout.trim().split('\n').map((line) => {
      return exec(`uglifyjs ${line} -o ${line} -c -m`, { env });
    });
    return Promise.all(jsJobs);
  })
  .then(() => {
    // Encrypt Libs
    if (!target.rules.natives) return;
    const natives = target.rules.natives;
    const libDist = path.join(packDist, natives.libdist);
    var soList = execSync(`find ${libDist} -type f -print`).toString();
    var soJobs = soList.trim().split('\n').map((soFile) => {
      if (!soFile) return;
      return exec(`strip -s ${soFile}`);
    });
    return Promise.all(soJobs);
  })
  .then(() => {
    // Encrypt Node
    var nodeList = execSync(`find ${packDist} -type f -name "*.node"`).toString();
    var nodeJobs = nodeList.trim().split('\n').map((nodeFile) => {
      if (!nodeFile) return;
      return exec(`strip -s ${nodeFile}`);
    });
    return Promise.all(nodeJobs);
  });
}

function packScripts() {
  console.log('\x1b[32mPack scripts\x1b[0m');

  const binDir = path.join(distDir, 'bin');
  execSync(`rm -rf ${binDir}`);
  execSync(`mkdir -p ${binDir}`);

  execSync(`cp -a ${rootDir}/scripts/detectOS.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/init-all.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/install_node.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/init-rabbitmq.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/init-mongodb.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/package.mcu.json ${distDir}/package.json`);
  execSync(`cp -a ${rootDir}/third_party/NOTICE ${distDir}`);
  execSync(`cp -a ${rootDir}/third_party/ThirdpartyLicenses.txt ${distDir}`);
  if (options.binary) {
    execSync(`cp -a ${rootDir}/scripts/release/daemon-bin.sh ${binDir}/daemon.sh`);
  } else {
    execSync(`cp -a ${rootDir}/scripts/release/daemon-mcu.sh ${binDir}/daemon.sh`);
  }

  const startScript = `${binDir}/start-all.sh`;
  const stopScript = `${binDir}/stop-all.sh`;
  const restartScript = `${binDir}/restart-all.sh`;
  const baseContent = fs.readFileSync(`${rootDir}/scripts/release/launch-base.sh`);
  fs.writeFileSync(startScript, baseContent);
  fs.writeFileSync(stopScript, baseContent);
  fs.writeFileSync(restartScript, baseContent);

  var startCommands = '';
  var stopCommands = '';
  var scriptItems = [];
  scriptItems.push('management-api');
  scriptItems.push('cluster-manager');
  for (let i = 0; i < allTargets.length; i++) {
    let name = allTargets[i].rules.name;
    if (scriptItems.indexOf(name) < 0) scriptItems.push(name);
  }
  scriptItems.push('app');
  scriptItems.forEach((m) => {
    if (!options.target.includes(m) && experimentalTargets.includes(m)) {
      return;
    }
    startCommands += '${bin}/daemon.sh start ' + m + ' $1\n';
    stopCommands += '${bin}/daemon.sh stop ' + m + '\n';
  });

  fs.appendFileSync(startScript, startCommands);
  fs.appendFileSync(stopScript, stopCommands);
  fs.appendFileSync(restartScript, stopCommands);
  fs.appendFileSync(restartScript, startCommands);

  execSync(`chmod +x ${binDir}/\*.sh`);
}

function packApps() {
  if (!options['app-path']) return;
  chdir(originCwd);
  var appPath = options['app-path'];
  if (!fs.existsSync(appPath)) {
    console.log(`\x1b[31mError: ${appPath} does not exist\x1b[0m`);
    return;
  }
  execSync(`rm -rf ${distDir}/apps`);
  execSync(`mkdir -p ${distDir}/apps`);
  console.log('\x1b[32mApps folder created in :', distDir, '\x1b[0m');
  execSync(`cp -a ${appPath} ${distDir}/apps/current_app`);

  // Look in the app's package.json to see what file to use for main.js
  var jsonTXT = execSync(`cat ${distDir}/apps/current_app/package.json`);
  var appJSON = JSON.parse(jsonTXT)["main"];

  if (!appJSON === undefined) {
    console.log("\x1b[31mError: No main js file for the app\x1b[0m");
    return;
  } else {
    // Make a soft link to the main JS file node.js should call
    if (appJSON !== 'main.js')
      execSync(`ln -srf ${distDir}/apps/current_app/${appJSON} ${distDir}/apps/current_app/main.js`);
  }
  const certScript = `${distDir}/apps/current_app/initcert.js`;
  if (fs.existsSync(certScript))
    execSync(`chmod +x ${certScript}`);

  if (options['install-module']) {
    chdir(`${distDir}/apps/current_app`);
    execSync('npm install' + npmInstallOption);
  }
}

function archive() {
  if (!options['archive']) return;
  const myVersion = options['archive'];

  chdir(rootDir);
  execSync(`rm -f Release-${myVersion}.tgz`);
  execSync(`mv ${distDir} Release-${myVersion}`);
  execSync(`tar --numeric-owner -czf "Release-${myVersion}.tgz" Release-${myVersion}/`);
  execSync(`mv Release-${myVersion} ${distDir}`);
  console.log(`\x1b[32mRelease-${myVersion}.tgz generated in ${rootDir}.\x1b[0m`);
}

getTargets()
  .then(getPackList)
  .then(cleanIfRepack)
  .then(processTargets)
  .then(packScripts)
  .then(packApps)
  .then(archive)
  .then(() => {
    console.log('\x1b[32mWork finished in directory:', distDir, '\x1b[0m');
  })

.catch((err) => {
  console.error('\x1b[31mERROR:', err.message, '\x1b[0m');
});
