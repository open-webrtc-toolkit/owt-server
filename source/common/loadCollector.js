// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const logger = require('./logger').logger;

// Logger
const log = logger.getLogger('LoadCollector');

const child_process = require('child_process');
const os = require('os');

const cpuCollector = function(period, onLoad) {
  let olds = os.cpus();
  const begin = 0;
  const end = olds.length - 1;
  const interval = setInterval(function() {
    const cpus = os.cpus();
    let idle = 0;
    let total = 0;
    for (let i = begin; i <= end; i++) {
      for (const key in cpus[i].times) {
        const diff = cpus[i].times[key] - olds[i].times[key];
        if (key === 'idle') {
          idle += diff;
        }
        total += diff;
      }
    }
    olds = cpus;
    onLoad(1 - idle/total);
    log.debug('cpu usage:', 1 - idle/total);
  }, period);

  this.stop = function() {
    log.debug('To stop cpu load collector.');
    clearInterval(interval);
  };
};

const memCollector = function(period, onLoad) {
  const interval = setInterval(function() {
    const usage = 1 - os.freemem() / os.totalmem();
    onLoad(usage);
    log.debug('mem usage:', usage);
  }, period);

  this.stop = function() {
    log.debug('To mem cpu load collector.');
    clearInterval(interval);
  };
};

const diskCollector = function(period, drive, on_load) {
  const interval = setInterval(function() {
    let total = 1; let free = 0;
    child_process.exec('df -k \'' + drive.replace(/'/g, '\'\\\'\'') + '\'',
        function(err, stdout, stderr) {
          if (err) {
            log.error(stderr);
          } else {
            const lines = stdout.trim().split('\n');

            const str_disk_info =
              lines[lines.length - 1].replace( /[\s\n\r]+/g, ' ');
            const disk_info = str_disk_info.split(' ');

            total = disk_info[1];
            free = disk_info[3];
            on_load(Math.round((1.0 - free / total) * 1000) / 1000);
          }
        });
  }, period);

  this.stop = function() {
    log.debug('To stop disk load collector.');
    clearInterval(interval);
  };
};

const networkCollector = function(period, interf, max_scale, on_load) {
  let rx_Mbps = 0; let tx_Mbps = 0; let rx_bytes = 0; let tx_bytes = 0;
  let meter = setInterval(function() {
    child_process.exec(
        'awk \'NR>2{if (index($1, "' +
        interf + '")==1){print $2, $10}}\' /proc/net/dev',
        function(err, stdout, stderr) {
          if (err) {
            log.error(stderr);
          } else {
            const fields = stdout.trim().split(' ');
            if (fields.length < 2) {
              return log.warn('not ordinary network load data');
            }
            const rx = Number(fields[0]); const tx = Number(fields[1]);
            if (rx >= rx_bytes && rx_bytes > 0) {
              rx_Mbps =
                Math.round(((rx - rx_bytes) * 8 / 1048576) * 1000) / 1000;
            }

            if (tx >= tx_bytes && tx_bytes > 0) {
              tx_Mbps =
                Math.round(((tx - tx_bytes) * 8 / 1048576) * 1000) / 1000;
            }

            rx_bytes = rx;
            tx_bytes = tx;
          }
        });
  }, 1000);

  let reporter = setInterval(function() {
    const rt_load =
      Math.round(
          Math.max(rx_Mbps / max_scale, tx_Mbps / max_scale) * 1000) / 1000;
    on_load(rt_load);
  }, period);

  this.stop = function() {
    log.debug('To stop network load collector.');
    meter && clearInterval(meter);
    reporter && clearInterval(reporter);
    meter = undefined;
    reporter = undefined;
  };
};

const gpuCollector = function(period, on_load) {
  let child = child_process.exec('stdbuf -o0 metrics_monitor 100 1000');
  let cpu_load = 0;
  let cpu_collector = new cpuCollector(period, function(data) {
    cpu_load = data;
  });

  let load = 0;
  child.stdout.on('data', function(data) {
    let usage_sum = 0; let samples = 0;
    const lines = data.toString().split('\n');

    let i = lines.length > 10 ? lines.length - 10 : 0;
    for (; i < lines.length; i++) {
      const engine_list = lines[i].split('\t');
      let engine_max_usage = 0;
      for (const engine of engine_list) {
        let m = null;
        if (
          (m = engine.match(/\s+usage:\s+(\d+\.\d+)/)) &&
          m !== null && m.length > 1
        ) {
          const engine_usage = Number(m[1]);
          if (engine_max_usage < engine_usage) {
            engine_max_usage = engine_usage;
          }
        }
      }
      usage_sum = usage_sum + engine_max_usage;
      samples = samples + 1;
    }

    if (samples > 0) {
      load = (usage_sum / samples) / 100;
    } else {
      load = 0;
    }
  });

  let interval = setInterval(function() {
    const result = Math.max(load, cpu_load);
    on_load(result);
  }, period);

  this.stop = function() {
    log.debug('To stop gpu load collector.');
    cpu_collector && cpu_collector.stop();
    cpu_collector = undefined;
    child && child.kill();
    child = undefined;
    interval && clearInterval(interval);
    interval = undefined;
  };
};

exports.LoadCollector = function(spec) {
  const that = {};

  const period = spec.period || 1000;
  const item = spec.item;
  const on_load = spec.onLoad || function(load) {
    log.debug('Got', item.name, 'load:', load);
  };
  let collector = undefined;

  that.stop = function() {
    log.info('To stop load collector.');
    collector && collector.stop();
    collector = undefined;
  };

  switch (item.name) {
    case 'network':
      collector =
        new networkCollector(period, item.interf, item.max_scale, on_load);
      break;
    case 'cpu':
      collector = new cpuCollector(period, on_load);
      break;
    case 'gpu':
      collector = new gpuCollector(period, on_load);
      break;
    case 'memory':
      collector = new memCollector(period, on_load);
      break;
    case 'disk':
      collector = new diskCollector(period, item.drive, on_load);
      break;
    default:
      log.error('Unknown load item');
      return undefined;
            // break;
  }

  return that;
};
