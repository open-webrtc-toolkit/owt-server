/*global require, exports*/
'use strict';
var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('Strategy');

var isWorkerAvailable = function (worker) {
    return worker.load < worker.max_load && (worker.state === undefined || worker.state === 2);
};

var leastUsed = function () {
    this.allocate = function (workers,  on_ok, on_error) {
        var least = 1.0, found = undefined;
        for (var id in workers) {
            if (workers[id].load < least && isWorkerAvailable(workers[id])) {
                least = workers[id].load;
                found = id;
            }
        }

        found ? on_ok(found) : on_error('No worker found.');
    };
};

var mostUsed = function () {
    this.allocate = function (workers, on_ok, on_error) {
        var most = 0, found = undefined;
        for (var id in workers) {
            if (workers[id].load > most && isWorkerAvailable(workers[id])) {
                most = workers[id].load;
                found = id;
            }
        }

        found ? on_ok(found) : on_error('No worker found.');
    };
};

var lastUsed = function () {
    var last_used = undefined;

    var findNew = function (workers, on_ok, on_error) {
        var found = undefined;
        for (var id in workers) {
            if (isWorkerAvailable(workers[id])) {
                found = id;
            }
        }

        found ? on_ok(found) : on_error('No worker found.');
    };

    this.allocate = function (workers, on_ok, on_error) {
        if (last_used !== undefined && workers[last_used] && isWorkerAvailable(workers[last_used])) {
            on_ok(last_used);
        } else {
            findNew(workers, function (id) { last_used = id; on_ok(id);}, on_error);
        }
    };
};

var roundRobin = function () {
    var latest_used = 65536 * 65536;

    this.allocate = function (workers, on_ok, on_error) {
        var keys = Object.keys(workers);
        var next_pick = keys.length > latest_used ? latest_used + 1 : 0,
            found = false;

        for (var i = 0; i < keys.length; i++) {
            if (isWorkerAvailable(workers[keys[next_pick]])) {
                found = true;
                break;
            }
            next_pick = next_pick >= keys.length ? 0 : next_pick + 1;
        }

        if (found) {
            latest_used = next_pick;
            on_ok(keys[next_pick]);
        } else {
            on_error('No proper worker found');
        }
    };
};

var randomlyPick = function () {
    this.allocate = function (workers, on_ok, on_error) {
        var ids = Object.keys(workers).filter(function(id) { return isWorkerAvailable(workers[id]);});
        if (ids.length > 0) {
            on_ok(ids[Math.floor(Math.random() * ids.length)]);
        } else {
            on_error('No worker found.');
        }
    };
};

exports.create = function (strategy) {
    switch (strategy) {
        case 'least-used':
            return new leastUsed();
        case 'most-used':
            return new mostUsed();
        case 'last-used':
            return new lastUsed();
        case 'round-robin':
            return new roundRobin();
        case 'randomly-pick':
            return new randomlyPick();
        default:
            log.warn('Invalid specified scheduling strategy:', strategy, ', apply "randomly-pick" instead.');
            return new randomlyPick();
    }
};
