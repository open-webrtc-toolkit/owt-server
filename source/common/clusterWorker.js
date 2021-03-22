// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

const logger = require('./logger').logger;
const makeRPC = require('./makeRPC').makeRPC;
const loadCollector = require('./loadCollector').LoadCollector;

// Logger
const log = logger.getLogger('ClusterWorker');

const genID = (function() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
        .toString(16)
        .substring(1);
  }
  return function() {
    return s4() + s4() +
    /* + '-' + s4()
               + '-' + s4()
               + '-' + s4()
               + '-'*/ s4() + s4() + s4();
  };
})();

module.exports = function(spec) {
  const that = {};

  /* 'unregistered' | 'registered' | 'recovering'*/
  let state = 'unregistered';
  let tasks = [];

  const rpcClient = spec.rpcClient;
  const id = spec.purpose + '-' + genID() + '@' +
    (spec.info.hostname || spec.info.ip);
  const purpose = spec.purpose;
  const info = spec.info;
  const cluster_name = spec.clusterName || 'owt-cluster';
  const join_retry = spec.joinRetry || 60;
  const keep_alive_period = 800/* MS*/;
  let keep_alive_interval = undefined;
  const on_join_ok = spec.onJoinOK || function() {
    log.debug('Join cluster successfully.');
  };
  const on_join_failed = spec.onJoinFailed || function(reason) {
    log.debug('Join cluster failed. reason:', reason);
  };
  const on_loss = spec.onLoss || function() {
    log.debug('Lost connection with cluster manager');
  };
  const on_recovery = spec.onRecovery || function() {
    log.debug('Rejoin cluster successfully.');
  };
  const on_overload = spec.onOverload || function() {
    log.debug('Overloaded!!');
  };

  let previous_load = 0.99;
  const reportLoad = function(load) {
    if (load == previous_load) {
      return;
    }
    previous_load = load;
    if (state === 'registered') {
      rpcClient.remoteCast(
          cluster_name,
          'reportLoad',
          [id, load]);
    }

    if (
      load > 0.98
      /* FIXME: Introduce a configuration item to specify the overload threshold here.*/
    ) {
      on_overload();
    }
  };

  const load_collector = loadCollector({period: spec.loadCollection.period,
    item: spec.loadCollection.item,
    onLoad: reportLoad});

  const join = function(on_ok, on_failed) {
    makeRPC(
        rpcClient,
        cluster_name,
        'join',
        [purpose, id, info],
        function(result) {
          state = 'registered';
          on_ok(result);
          previous_load = 0.99;
          keepAlive();
        }, on_failed);
  };

  const joinCluster = function(attempt) {
    const tryJoin = function(countDown) {
      log.debug('Try joining cluster', cluster_name,
          ', retry count:', attempt - countDown);
      join(function() {
        on_join_ok(id);
        log.info('Join cluster', cluster_name, 'OK.');
      }, function(error_reason) {
        if (state === 'unregistered') {
          log.info('Join cluster', cluster_name, 'failed.');
          if (countDown <= 0) {
            log.error('Join cluster', cluster_name,
                'failed. reason:', error_reason);
            on_join_failed(error_reason);
          } else {
            setTimeout(function() {
              tryJoin(countDown - 1);
            }, keep_alive_period);
          }
        }
      });
    };

    tryJoin(attempt);
  };

  const keepAlive = function() {
    keep_alive_interval && clearInterval(keep_alive_interval);

    const tryRecovery = function(on_success) {
      clearInterval(keep_alive_interval);
      keep_alive_interval = undefined;
      state = 'recovering';

      const tryJoining = function() {
        log.debug('Try rejoining cluster', cluster_name, '....');
        join(function(result) {
          log.debug('Rejoining result', result);
          if (result === 'initializing') {
            tasks.length > 0 && pickUpTasks(tasks);
          } else {
            on_loss();
            tasks = [];
          }
          on_success();
        }, function(reason) {
          setTimeout(function() {
            if (state === 'recovering') {
              log.debug('Rejoin cluster', cluster_name,
                  'failed. reason:', reason);
              tryJoining();
            }
          }, keep_alive_period);
        });
      };

      tryJoining();
    };

    let loss_count = 0;
    keep_alive_interval = setInterval(function() {
      makeRPC(
          rpcClient,
          cluster_name,
          'keepAlive',
          [id],
          function(result) {
            loss_count = 0;
            if (result === 'whoareyou') {
              if (state !== 'recovering') {
                log.info('Unknown by cluster manager', cluster_name);
                tryRecovery(function() {
                  log.info('Rejoin cluster', cluster_name, 'OK.');
                });
              }
            }
          }, function() {
            loss_count += 1;
            if (loss_count > 3) {
              if (state !== 'recovering') {
                log.info('Lost connection with cluster', cluster_name);
                tryRecovery(function() {
                  log.info('Rejoin cluster', cluster_name, 'OK.');
                  on_recovery(id);
                });
              }
            }
          });
    }, keep_alive_period);
  };

  const pickUpTasks = function(taskList) {
    rpcClient.remoteCast(
        cluster_name,
        'pickUpTasks',
        [id, taskList]);
  };

  const layDownTask = function(task) {
    rpcClient.remoteCast(
        cluster_name,
        'layDownTask',
        [id, task]);
  };

  const doRejectTask = function(task) {
    rpcClient.remoteCast(
        cluster_name,
        'unschedule',
        [id, task]);
  };

  that.quit = function() {
    if (state === 'registered') {
      if (keep_alive_interval) {
        clearInterval(keep_alive_interval);
        keep_alive_interval = undefined;
      }

      rpcClient.remoteCast(
          cluster_name,
          'quit',
          [id]);
    } else if (state === 'recovering') {
      keep_alive_interval && clearInterval(keep_alive_interval);
    }

    load_collector && load_collector.stop();
  };

  that.reportState = function(st) {
    if (state === 'registered') {
      rpcClient.remoteCast(
          cluster_name,
          'reportState',
          [id, st]);
    }
  };

  that.addTask = function(task) {
    const i = tasks.indexOf(task);
    if (i === -1) {
      tasks.push(task);
      if (state === 'registered') {
        pickUpTasks([task]);
      }
    }
  };

  that.removeTask = function(task) {
    const i = tasks.indexOf(task);
    if (i !== -1) {
      tasks.splice(i, 1);
      if (state === 'registered') {
        layDownTask(task);
      }
    }
  };

  that.rejectTask = function(task) {
    doRejectTask(task);
  };

  joinCluster(join_retry);
  return that;
};

