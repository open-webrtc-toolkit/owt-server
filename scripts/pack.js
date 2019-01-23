#!/usr/bin/env node

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
optParser.addOption('d', 'debug', 'boolean', 'Whether pack debug version (Eg. pack.js -t portal -d)');
optParser.addOption('o', 'addon-debug', 'boolean', 'Whether pack debug addon (Eg. pack.js -t webrtc-agent -o)');
optParser.addOption('f', 'full', 'boolean', 'Whether perform a full pack (--full is the equalivation of pack.js -t all -r -i)');
optParser.addOption('s', 'sample-path', 'string', 'Specify sample path (Eg. pack.js -t all -s ${samplePath})');
optParser.addOption('a', 'archive', 'string', 'Specify archive name (Eg. pack.js -t all -a ${archiveName})');
optParser.addOption('n', 'node-module-path', 'string', 'Specify shared-node-module directory');
optParser.addOption('c', 'copy-module-path', 'string', 'Specify copy node modules directory');
optParser.addOption('b', 'binary', 'boolean', 'Pack binary');
optParser.addOption('h', 'help', 'boolean', 'Show help');

const options = optParser.parseArgs(process.argv);

const rootDir = path.join(path.dirname(module.filename), '..');
const distDir = path.join(rootDir, options.debug ? 'dist-debug' : 'dist');
const depsDir = path.join(rootDir, 'build/libdeps/build');
const originCwd = cwd();

// Detect OS script
const osScript = path.join(rootDir, 'scripts/detectOS.sh');
const osType = execSync(`. ${osScript}`).toString().toLowerCase();

var allTargets = [];

if (options.full) {
  options.target = ['all'];
  options.repack = true;
  options['install-module'] = true;
}

if (options.help || Object.keys(options).length === 0) {
  optParser.printHelp();
  process.exit(0);
}

var npmInstallOption = '';
if (options.archive) {
  npmInstallOption = ' --production';
}

if (options.encrypt) {
  const encryptDeps = [
    'uglify-js',
    'babel-cli',
    'babel-preset-es2015',
    'babel-preset-stage-0',
  ];

  // Check encrypt deps
  console.log('Checking encrypt dependencies...');
  try {
    for (const dep of encryptDeps) {
      execSync(`npm list -g ${dep}`);
    }
    console.log('Encrypt dependencies OK.');
  } catch (e) {
    console.log('Install node dependencies for Encrypt...');
    for (const dep of encryptDeps) {
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
  return exec(`find ${rootDir}/source -type f -name "packrule.json"`)
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
      console.log(`\x1b[32m${target.rules.name}\x1b[0m`);
      console.log(' ->', target.path);
    }
    process.exit(0);
  }

  if (!options.target) {
    throw new Error('No target specified');
  }

  var packList = targets.filter((element) => {
    if (options.target.includes('all')) return true;
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
      return (options.debug ? packNodeDebug(target) : packNode(target));
    })
    .then((nodePacked) => {
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
            execSync('sed -i "/AgentLoader/d" erizoJS.js');
            execSync('rm -f AgentLoader.js');
            execSync(`pkg erizoJS.js -t ${pkg.targets}`);
            var loaderData = { bin: 'erizoJS' };
            fs.writeFileSync(path.join(packDist, 'loader.json'), JSON.stringify(loaderData));
          }
          if (fs.existsSync('initdb.js')) {
            execSync(`pkg initdb.js -t ${pkg.targets}`);
          }
          if (fs.existsSync('initcert.js')) {
            execSync(`pkg initcert.js -t ${pkg.targets}`);
          }
          execSync('pkg .');
          execSync('find . -maxdepth 1 -name "*.js" -not -name "AgentLoader*" -delete');
          execSync('rm -rf auth');
          execSync('rm -rf data_access');
          execSync('rm -rf resource');
          execSync('rm -rf rpc');
          execSync('rm -rf package.json');
          execSync('rm -rf packrule.json');
          if (!target.rules.node) {
            execSync('rm -rf node_modules');
          }
        }
      }
      if (options.encrypt)
        return encrypt(target);
    });
}

function packNode(target) {
  if (!target.rules.node) return;
  const node = target.rules.node;
  const packSrc = path.dirname(target.path);
  const packDist = path.join(distDir, target.rules.dest);
  const nodeDir = path.join(rootDir, `third_party/node-${process.version}`);

  console.log('\x1b[32mPack node\x1b[0m -', target.rules.name);
  console.log(`\x1b[33mUsing node: ${process.version}\x1b[0m`);

  chdir(path.join(rootDir, 'third_party'));
  console.log('Wget and unzip node...');
  execSync(`wget -c http:\/\/nodejs.org\/dist\/${process.version}\/node-${process.version}.tar.gz 2>\/dev\/null`);
  execSync(`tar -xzf node-${process.version}.tar.gz`);

  console.log(`Switch to node directory: ${nodeDir}`);
  chdir(nodeDir);
  const entryFile = node.entry;
  if (!entryFile) throw new Error('No Entry file when packing node');

  // Copy entry file
  execSync(`cp ${path.join(packSrc, entryFile)} lib/_third_party_main.js`);
  execSync(`sed -i "/'library_files': \\[/a 'lib/_third_party_main.js'," node.gyp`);

  // Copy lib files
  var libpath;
  if (node.libpath && node.libfiles) {
    libpath = path.join('lib', node.libpath);
    execSync(`rm -rf ${libpath}`);
    execSync(`mkdir -p ${libpath}`);
    for (let file of node.libfiles) {
      let filePath = path.join(packSrc, file);
      execSync(`cp ${filePath} ${libpath}`);
      // Insert into node.gyp
      execSync(`sed -i "/'library_files': \\[/a '${path.join(libpath, path.basename(file))}'," node.gyp`)
    }

    if (options.binary && target.rules.common) {
      const common = target.rules.common;
      if (common.files) {
        for (let file of common.files) {
          if (file.indexOf('AgentLoader') >= 0 || file.indexOf('index') >= 0)
            continue;
          let filePath = path.join(packSrc, file);
          let dstFile = path.join(libpath, path.basename(file));
          execSync(`cp -a ${filePath} ${libpath}`);
          execSync(`sed -i "1s/^/require=require('module')._load('.\\/AgentLoader');\\n/" ${dstFile}`)
          // Insert into node.gyp
          execSync(`sed -i "/'library_files': \\[/a '${dstFile}'," node.gyp`)
        }
      }
    }
  }
  // Construct env
  var env = {};
  for (const key in process.env) {
    env[key] = process.env[key];
  }
  var pkgConfigPath =
  env['PKG_CONFIG_PATH'] = (env['PKG_CONFIG_PATH'] || '');
  env['PKG_CONFIG_PATH'] = path.join(depsDir, 'lib/pkgconfig') +
    ':' + path.join(depsDir, 'lib64/pkgconfig') +
    ':' + env['PKG_CONFIG_PATH'];

  execSync(
    `.\/configure --without-npm --prefix=${depsDir} --shared-openssl ` +
    `--shared-openssl-libpath=${depsDir}\/lib ` +
    `--shared-openssl-includes=${depsDir}\/include ` +
    `--shared-openssl-libname=ssl,crypto`, { env });

  env = {};
  for (const key in process.env) {
    env[key] = process.env[key];
  }
  env['LD_LIBRARY_PATH'] = (env['LD_LIBRARY_PATH'] || '');
  env['LD_LIBRARY_PATH'] = path.join(depsDir, 'lib') + ':' + env['LD_LIBRARY_PATH'];

  console.log('Building node...');
  execSync('make V= -j5', { env });
  execSync('make uninstall');
  execSync('make install');
  execSync(`cp ${path.join(depsDir, 'bin/node')} ${path.join(packDist, node.bin)}`);
  execSync('make uninstall');

  if (libpath) {
    // Remove lib folders after build
    execSync(`rm -rf ${libpath}`);
  }

  // Generate loader.json
  var loaderData = { bin: node.bin, lib: node.libpath };
  fs.writeFileSync(path.join(packDist, 'loader.json'), JSON.stringify(loaderData));

  console.log(target.rules.name, '- Pack node finished.')
}

function packNodeDebug(target) {
  if (!target.rules.node) return;

  console.log('\x1b[32mPack node (debug) \x1b[0m -', target.rules.name);
  const node = target.rules.node;
  const packSrc = path.dirname(target.path);
  const packDist = path.join(distDir, target.rules.dest);

  // Copy entry file
  const entryFile = node.entry;
  if (!entryFile) throw new Error('No Entry file when packing node');
  execSync(`cp ${path.join(packSrc, entryFile)} ${packDist}`);
  // Copy lib files
  if (node.libpath && node.libfiles) {
    const libpath = path.join(packDist, node.libpath);
    execSync(`rm -rf ${libpath}`);
    execSync(`mkdir -p ${libpath}`);
    for (let file of node.libfiles) {
      let filePath = path.join(packSrc, file);
      execSync(`cp ${filePath} ${libpath}`);
    }
  }

  // Generate loader.json
  var loaderData = { bin: 'node', lib: node.libpath };
  fs.writeFileSync(path.join(packDist, 'loader.json'), JSON.stringify(loaderData));

  console.log(target.rules.name, '- Pack node (debug) finished.')
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
      if (options['archive'] && fs.existsSync(libOpenh264)) {
        let dummyOpenh264 = path.join(rootDir, 'third_party/openh264/pseudo-openh264.so');
        execSync(`cp -av ${dummyOpenh264} ${libOpenh264}`);
      }
      if (target.rules.name.indexOf('video') === 0) {
        let vasrc = path.join(depsDir, 'bin/vainfo');
        let vadist = path.join(packDist, 'bin');
        if (fs.existsSync(vasrc)) {
          if (!fs.existsSync(vadist)) {
            execSync(`mkdir -p ${vadist}`);
          }
          execSync(`cp -av ${vasrc} ${vadist}`);
        }
      }
      if (osType.includes('ubuntu')) {
        let libSvtHevcEnc = path.join(libDist, 'libSvtHevcEnc.so.1');
        let dummySvtHevcEnc = path.join(rootDir, 'third_party/SVT-HEVC/pseudo-svtHevcEnc.so');
        execSync(`cp -av ${dummySvtHevcEnc} ${libSvtHevcEnc}`);
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
    ':' + env['LD_LIBRARY_PATH'];

    if (osType.includes('ubuntu')) {
        env['LD_LIBRARY_PATH'] = ':' + path.join(rootDir, 'third_party/SVT-HEVC') +
            ':' + env['LD_LIBRARY_PATH'];
    }

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
          });
        checks.push(checkPros[line]);
      }
      return Promise.all(checks);
    })
    .then((results) => {
      return results.filter((element) => filterLib(element));
    });
}

function filterLib(libSrc) {
  if (!libSrc) return false;

  //console.log('require:', libSrc);
  const libName = path.basename(libSrc);
  // Remove libmsdk
  if (libName.indexOf('libmfxhw') === 0) return false;
  // Remove libva
  if (libName.indexOf('libva') === 0) return false;
  // Remove libfdk-aac
  if (libName.indexOf('libfdk-aac') === 0 && options['archive']) return false;
  // Remove libav/ffmpeg if aac
  if (libName.indexOf('libav') === 0 || libName.indexOf('libsw') === 0) {
    let output = execSync(`ldd ${libSrc}`).toString();
    //console.log('libav output:', output);
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
  return true;
}

function generateStart(target) {
  // Generate a start.sh
  if (!target.rules.start) return;
  const packDist = path.join(distDir, target.rules.dest);
  const stdout = `../logs/woogeen-${target.rules.name}.stdout`;
  const pid = `../logs/woogeen-${target.rules.name}.pid`;
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
      return exec(`babel --presets es2015,stage-0 ${line} -o "${line}.es5"`, { env })
        .then(() => {
          return exec(`uglifyjs ${line}.es5 -o ${line} -c -m`, { env });
        })
        .then(() => {
          return exec(`rm "${line}.es5"`);
        });
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
  execSync(`cp -a ${rootDir}/scripts/release/init-rabbitmq.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/init-mongodb.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/install-runtime-deps.sh ${binDir}`);
  execSync(`cp -a ${rootDir}/scripts/release/package.mcu.json ${distDir}/package.json`);
  execSync(`cp -a ${rootDir}/third_party/NOTICE ${distDir}`);
  execSync(`cp -a ${rootDir}/third_party/ThirdpartyLicenses.txt ${distDir}`);
  if (options.binary) {
    execSync(`cp -a ${rootDir}/scripts/release/daemon-bin.sh ${binDir}/daemon.sh`);
  } else {
    execSync(`cp -a ${rootDir}/scripts/release/daemon-mcu.sh ${binDir}/daemon.sh`);
  }

  if (options.debug) {
    // Add debug variable
    execSync(`sed -i "/ROOT=/aexport NODE_DEBUG_ERIZO=1" ${binDir}/daemon.sh`)
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
  scriptItems.push('nuve');
  scriptItems.push('cluster-manager');
  for (let i = 0; i < allTargets.length; i++) {
    let name = allTargets[i].rules.name;
    if (scriptItems.indexOf(name) < 0) scriptItems.push(name);
  }
  scriptItems.push('app');
  scriptItems.forEach((m) => {
    startCommands += '${bin}/daemon.sh start ' + m + ' $1\n';
    stopCommands += '${bin}/daemon.sh stop ' + m + '\n';
  });

  fs.appendFileSync(startScript, startCommands);
  fs.appendFileSync(stopScript, stopCommands);
  fs.appendFileSync(restartScript, stopCommands);
  fs.appendFileSync(restartScript, startCommands);

  execSync(`chmod +x ${binDir}/\*.sh`);
}

function packSamples() {
  if (!options['sample-path']) return;
  chdir(originCwd);
  var samplePath = options['sample-path'];
  if (!fs.existsSync(samplePath)) {
    console.log(`\x1b[31mError: ${samplePath} does not exist\x1b[0m`);
    return;
  }
  execSync(`rm -rf ${distDir}/extras`);
  execSync(`mkdir -p ${distDir}/extras`);
  execSync(`cp -a ${samplePath} ${distDir}/extras/basic_example`);

  const certScript = `${distDir}/extras/basic_example/initcert.js`;
  if (fs.existsSync(certScript))
    execSync(`chmod +x ${certScript}`);

  if (options['install-module']) {
    chdir(`${distDir}/extras/basic_example`);
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
  .then(packSamples)
  .then(archive)
  .then(() => {
    console.log('\x1b[32mWork finished in directory:', distDir, '\x1b[0m');
  })

.catch((err) => {
  console.error('\x1b[31mERROR:', err.message, '\x1b[0m');
});
