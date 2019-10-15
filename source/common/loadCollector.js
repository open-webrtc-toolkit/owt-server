// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('LoadCollector');

var child_process = require('child_process');
var os = require('os');

var cpuCollector = function (period, onLoad) {
    var olds = os.cpus();
    var begin = 0;
    var end = olds.length - 1;
    var interval = setInterval(function() {
        var cpus = os.cpus();
        var idle = 0;
        var total = 0;
        for (let i = begin; i <= end; i++) {
            for (let key in cpus[i].times) {
                let diff = cpus[i].times[key] - olds[i].times[key];
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

    this.stop = function () {
        log.debug("To stop cpu load collector.");
        clearInterval(interval);
    };
};

var memCollector = function (period, onLoad) {
    var interval = setInterval(function() {
        var usage = 1 - os.freemem() / os.totalmem();
        onLoad(usage);
        log.debug('mem usage:', usage);
    }, period);

    this.stop = function () {
        log.debug("To mem cpu load collector.");
        clearInterval(interval);
    };
};

var diskCollector = function (period, drive, on_load) {
    var interval = setInterval(function () {
        var total = 1, free = 0;
        child_process.exec("df -k '" + drive.replace(/'/g,"'\\''") + "'", function(err, stdout, stderr) {
            if (err) {
                log.error(stderr);
            } else {
                var lines = stdout.trim().split('\n');

                var str_disk_info = lines[lines.length - 1].replace( /[\s\n\r]+/g,' ');
                var disk_info = str_disk_info.split(' ');

                total = disk_info[1];
                free = disk_info[3];
                on_load(Math.round((1.0 - free / total) * 1000) / 1000);
            }
        });
    }, period);

    this.stop = function () {
        log.debug("To stop disk load collector.");
        clearInterval(interval);
    };
};

var networkCollector = function (period, interf, max_scale, on_load) {
    var rx_Mbps = 0, tx_Mbps = 0, rx_bytes = 0, tx_bytes = 0;
    var meter = setInterval(function () {
        child_process.exec("awk 'NR>2{if (index($1, \"" + interf + "\")==1){print $2, $10}}' /proc/net/dev", function (err, stdout, stderr) {
            if (err) {
                log.error(stderr);
            } else {
                var fields = stdout.trim().split(" ");
                if (fields.length < 2) {
                    return log.warn('not ordinary network load data');
                }
                var rx = Number(fields[0]), tx = Number(fields[1]);
                if (rx >= rx_bytes && rx_bytes > 0) {
                    rx_Mbps = Math.round(((rx - rx_bytes) * 8 / 1048576) * 1000) / 1000;
                }

                if (tx >= tx_bytes && tx_bytes > 0) {
                    tx_Mbps = Math.round(((tx - tx_bytes) * 8 / 1048576) * 1000) / 1000;
                }

                rx_bytes = rx;
                tx_bytes = tx;
            }
        });
    }, 1000);

    var reporter = setInterval(function () {
        var rt_load = Math.round(Math.max(rx_Mbps / max_scale, tx_Mbps / max_scale) * 1000) / 1000;
        on_load(rt_load);
    }, period);

    this.stop = function () {
        log.debug("To stop network load collector.");
        meter && clearInterval(meter);
        reporter && clearInterval(reporter);
        meter = undefined;
        reporter = undefined;
    };
};

var gpuCollector = function (period, on_load) {
    var child = child_process.exec('stdbuf -o0 metrics_monitor 100 1000');
    var cpu_load = 0,
        cpu_collector = new cpuCollector(period, function (data) {cpu_load = data;});

    var load = 0;
    child.stdout.on('data', function (data) {
        var usage_sum = 0, samples = 0;
        var lines = data.toString().split('\n');

        var i = lines.length > 10 ? lines.length - 10 : 0;
        for (; i < lines.length; i++) {
            var engine_list = lines[i].split('\t');
            var engine_max_usage = 0;
            for (var engine of engine_list) {
                var m = null;
                if ((m = engine.match(/\s+usage:\s+(\d+\.\d+)/)) && m !== null && m.length > 1) {
                    var engine_usage = Number(m[1]);
                    if (engine_max_usage < engine_usage)
                        engine_max_usage = engine_usage;
                }
            }
            usage_sum = usage_sum + engine_max_usage;
            samples = samples + 1;
        }

        if (samples > 0)
            load = (usage_sum / samples) / 100;
        else
            load = 0;
    });

    var interval = setInterval(function () {
        on_load(load);
    }, period);

    this.stop = function () {
        log.debug("To stop gpu load collector.");
        child && child.kill();
        child = undefined;
        interval && clearInterval(interval);
        interval = undefined;
    };
};

var vpuCollector = function (period, on_load) {    
    const resource = require('./resourceUtil/build/Release/resourceUtil');
    var resourceUtil = new resource();
    var interval = setInterval(function () {
        var vpu_load = resourceUtil.getVPUUtil();
        log.debug("VPU load is:",vpu_load);
        on_load(vpu_load);
    }, period);

    this.stop = function () {
        log.debug("To stop gpu load collector.");
        resourceUtil = undefined;
        interval && clearInterval(interval);
        interval = undefined;
    };
};

var resourceCollector = function (period, cpu, gpu, vpu, network, disk, memory, on_load) {    
    var cpu_load = 0,
        gpu_load = 0,
        vpu_load = 0,
        network_load = 0,
        disk_load =0,
        memory_load = 0

    if(cpu) {
        var cpu_collector = new cpuCollector(period, function (data) {cpu_load = data;});
    }

    if(gpu) {
        var gpu_collector = new gpuCollector(period, function (data) {gpu_load = data;});
    }

    if(vpu) {
        var vpu_collector = new vpuCollector(period, function (data) {vpu_load = data;});
    }

    if(network) {
        var network_collector = new networkCollector(period, function (data) {network_load = data;});
    }

    if(disk) {
        var disk_collector = new diskCollector(period, function (data) {disk_load = data;});
    }

    if(memory) {
        var memory_collector = new memoryCollector(period, function (data) {memory_load = data;});
    }

    var interval = setInterval(function () {
        var result = Math.max(cpu_load, gpu_load, vpu_load, network_load, disk_load, memory_load);
        log.debug("Report resource load:", result);
        on_load(result);
    }, period);


    this.stop = function () {
        log.debug("To stop resource load collector.");
        cpu_collector && cpu_collector.stop();
        cpu_collector = undefined;
        gpu_collector && gpu_collector.stop();
        gpu_collector = undefined;
        vpu_collector && vpu_collector.stop();
        vpu_collector = undefined;
        network_collector && network_collector.stop();
        network_collector = undefined;
        disk_collector && disk_collector.stop();
        disk_collector = undefined;
        memory_collector && memory_collector.stop();
        memory_collector = undefined;
    };
    
};

exports.LoadCollector = function (spec) {
    var that = {};

    var period = spec.period || 1000,
        item = spec.item,
        cpu = spec.item.cpu || false,
        gpu = spec.item.gpu || false,
        vpu = spec.item.vpu || false,
        network = spec.item.network || false,
        disk = spec.item.disk || false,
        memory = spec.item.memory || false,
        on_load = spec.onLoad || function (load) {log.debug('Got', item.name, 'load:', load);},
        collector = undefined;

    that.stop = function () {
        log.info("To stop load collector.");
        collector && collector.stop();
        collector = undefined;
    };

    collector = new resourceCollector(period, cpu, gpu, vpu, network, disk, memory, on_load);

    return that;
};
