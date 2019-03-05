// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('Strategy');

var leastUsed = function () {
    this.allocate = function (workers, candidates, on_ok, on_error) {
        var least = 1.0, found = undefined;
        for (var i in candidates) {
            var id = candidates[i];
            if (workers[id].load < least) {
                least = workers[id].load;
                found = id;
            }
        }

        on_ok(found);
    };
};

var mostUsed = function () {
    this.allocate = function (workers, candidates, on_ok, on_error) {
        var most = 0, found = undefined;
        for (var i in candidates) {
            var id = candidates[i];
            if (workers[id].load >= most) {
                most = workers[id].load;
                found = id;
            }
        }

        on_ok(found);
    };
};

var lastUsed = function () {
    var last_used = undefined;

    this.allocate = function (workers, candidates, on_ok, on_error) {
        if (last_used === undefined || workers[last_used] === undefined || candidates.indexOf(last_used) === -1) {
            last_used = candidates[0];
        }
        on_ok(last_used);
    };
};

var roundRobin = function () {
    var latest_used = 65536 * 65536;

    this.allocate = function (workers, candidates, on_ok, on_error) {
        var i = candidates.indexOf(latest_used);
        if (i === -1) {
            latest_used = candidates[0];
        } else {
            latest_used = (i === candidates.length - 1) ? candidates[0] : candidates[i + 1];
        }
        on_ok(latest_used);
    };
};

var randomlyPick = function () {
    this.allocate = function (workers, candidates, on_ok, on_error) {
       on_ok(candidates[Math.floor(Math.random() * candidates.length)]);
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
