/*global require, setInterval, module*/
'use strict';

var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('LoadCollector');

var child_process = require('child_process');

var cpuCollector = function (period, on_load) {
    var child = child_process.spawn('top', ['-b', '-d', Math.max(Math.floor(period / 1000), 1)]);

    child.stdout.on('data', function (data) {
        var cpuline = data.toString().split('\n').filter(function (line) {return line.startsWith('%Cpu(s):');})[0];
        var regex = /\s+(\d+\.\d+)\s+id/;

        if (cpuline) {
            var m = regex.exec(cpuline);
            if (m && (m.length === 2)) {
                on_load(Math.floor(100 - Number(m[1])) / 100);
            }
        }
    });

    child.stderr.on('data', function (error) {
        log.error('cpu collector error:', error.toString());
    });

    this.stop = function () {
        log.debug("To stop cpu load collector.");
        child && child.kill();
        child = undefined;
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
    var child = child_process.spawn('intel_gpu_top', ['-s', '200']);
    var cpu_load = 0,
        cpu_collector = new cpuCollector(period, function (data) {cpu_load = data;});

    var renders = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], render_sum = 0,
        bitstreams = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], bitstream_sum = 0,
        blitters = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0], blitter_sum = 0,
        load = 0;
    child.stdout.on('data', function (data) {
       var lines = data.toString().split('\n');

       lines.forEach(function (line) {
           var m = null;
           if ((m = line.match(/\s+render busy:\s+(\d+)%/)) && m !== null && m.length > 1) {
               var render = Number(m[1]),
                   old = renders.shift();
               renders.push(render);
               render_sum = render_sum - old + render;
           } else if ((m = line.match(/\s+bitstream busy:\s+(\d+)%/)) && m !== null && m.length > 1) {
               var bitstream = Number(m[1]),
                   old = bitstreams.shift();
               bitstreams.push(bitstream);
               bitstream_sum = bitstream_sum - old + bitstream;
           } else if ((m = line.match(/\s+blitter busy:\s+(\d+)%/)) && m !== null && m.length > 1) {
               var blitter = Number(m[1]),
                   old = blitters.shift();
               blitters.push(blitter);
               blitter_sum = blitter_sum - old + blitter;
               load = (Math.floor(Math.max(render_sum, bitstream_sum, blitter_sum) / 10)) / 100;
           }
       });
    });

    var interval = setInterval(function () {
        var result = Math.max(load, cpu_load);
        on_load(result);
    }, period);

    this.stop = function () {
        log.debug("To stop gpu load collector.");
        cpu_collector && cpu_collector.stop();
        cpu_collector = undefined;
        child && child.kill();
        child = undefined;
        interval && clearInterval(interval);
        interval = undefined;
    };
};

exports.LoadCollector = function (spec) {
    var that = {};

    var period = spec.period || 1000,
        item = spec.item,
        on_load = spec.onLoad || function (load) {log.debug('Got', item.name, 'load:', load);},
        collector = undefined;

    that.stop = function () {
        log.info("To stop load collector.");
        collector && collector.stop();
        collector = undefined;
    };

    switch (item.name) {
        case 'network':
            collector = new networkCollector(period, item.interf, item.max_scale, on_load);
            break;
        case 'cpu':
            collector = new cpuCollector(period, on_load);
            break;
        case 'gpu':
            collector = new gpuCollector(period, on_load);
            break;
        case 'memory':
            log.error('Not support memory load currently.');
            return undefined;
            //break;
        case 'disk':
            collector = new diskCollector(period, item.drive, on_load);
            break;
        default:
            log.error('Unknown load item');
            return undefined;
            //break;
    }

    return that;
};
