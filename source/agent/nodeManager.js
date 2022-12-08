// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var fs = require('fs');
var spawn = require('child_process').spawn;
var logger = require('./logger').logger;
var log = logger.getLogger('NodeManager');

/*
 * * @param {object}   spec                      -The specification of nodeManager.
 * * @param {string}   spec.parentId             -The rpcId of parent.
 * * @param {number}   spec.prerunNodeNum        -The number of pre-running nodes.
 * * @param {number}   spec.maxNodeNum           -The max number of running nodes.
 * * @param {bool}     spec.reuseNode            -Whether reuse the current in-use nodes if maxNodeNum has been reached.
 * * @param {bool}     spec.consumeNodeByRoom    -Whether tasks from the same room be scheduled to the same node.
 * * @param {object}   spawnOptions              -The options to spawn new nodes.
 * * @param {string}   spawnOptions.cmd          -The command name to execute.
 * * @param {object}   spawnOptions.config       -The node initial configuration.
 * * @param {function} onNodeAbnormallyQuit      -Callback called when node abnormally quit.
 * * @param {function} onTaskAdded               -Callback called when a new task is added.
 * * @param {function} onTaskRemoved             -Callback called when a task is removed.
*/
module.exports = function (spec, spawnOptions, onNodeAbnormallyQuit, onTaskAdded, onTaskRemoved) {
  var that = {};

  var node_index = 0;
  var idle_nodes = [];
  var nodes = [];
  var processes = {};
  var tasks = {}; // {node_id: {RoomID: [TaskID]}}
  
  var spawn_failed = false;
  
  function tasksOnNode(id) {
    return [];
  };

  function cleanupNode (id) {
      processes[id].check_alive_interval && clearInterval(processes[id].check_alive_interval);
      processes[id].check_alive_interval = undefined;
  
      delete processes[id];
      delete tasks[id];
      var index = nodes.indexOf(id);
      if (index !== -1) {
          nodes.splice(index, 1);
      }
      index = idle_nodes.indexOf(id);
      if (index !== -1) {
          idle_nodes.splice(index, 1);
      }
  }
  
  var launchNode = function() {
      var id = spec.parentId + '_' + node_index++;
      if (!fs.existsSync('../logs')){
          fs.mkdirSync('../logs');
      }
      var out = fs.openSync('../logs/' + id + '.log', 'a');
      var err = fs.openSync('../logs/' + id + '.log', 'a');

      var spawnArgs = [id, spec.parentId, JSON.stringify(spawnOptions.config)];
      (spawnOptions.cmd === 'node') && (spawnArgs.unshift('./workingNode'));
      var child = spawn(spawnOptions.cmd, spawnArgs, {
        detached: true,
        stdio: [ 'ignore', out, err, 'ipc' ]
      });

      child.unref();
      child.out_log_fd = out;
      child.err_log_fd = err;
  
      log.debug('launchNode, id:', id);
      child.on('close', function (code, signal) {
          log.debug('Node', id, 'exited with code:', code, 'signal:', signal);
          if (code !== 0) {
              log.info('Node', id, 'is closed on unexpected code:', code);
          }
  
          if (processes[id]) {
              onNodeAbnormallyQuit && onNodeAbnormallyQuit(id, tasksOnNode(id));
              cleanupNode(id);
          }
  
          try {
              fs.closeSync(child.out_log_fd);
              fs.closeSync(child.err_log_fd);
          } catch (e) {
              log.warn('Close fd failed');
          }
  
          if (!spawn_failed) {
              fillNodes();
          }
      });
      child.on('error', function (error) {
          log.error('failed to launch node', id, 'error:', JSON.stringify(error));
          child.READY = false;
          //FIXME: the 'error' message may be emitted in one of the following cases:
          // 1) The process could not be spawned, or
          // 2) The process could not be killed, or
          // 3) Sending a message to the child process failed.
          // And we supposed only the first case, accurately either error.code === 'ENOENT' or error.code === 'EMFILE' or error.code === 'EAGAIN' will happen in our usage
          //if (error.code === 'ENOENT' || error.code === 'EMFILE' || error.code === 'EAGAIN') {
              spawn_failed = true;
              //TODO: The status of this agent should be set to 'invalid' in this case.
              setTimeout(() => {spawn_failed = false;}, 2000);
          //}
      });
      child.on('message', function (message) { // currently only used for sending ready message from node to agent;          
          log[message === "IMOK" ? "trace" : "debug"]('message from node', id, ':', message);
          if (message === 'READY') {
              child.READY = true;
              child.alive_count = 0;
              child.check_alive_interval = setInterval(function() {
                if (child.READY && (child.alive_count === 0)) {
                    log.info('Node(', id, ') is no longer responsive!');
                    onNodeAbnormallyQuit && onNodeAbnormallyQuit(id, tasksOnNode(id));
                    dropNode(id);
                }
                child.alive_count = 0;
              }, 3000);
              spawn_failed = false;
          } else if (message === 'IMOK') {
            child.alive_count += 1;
          } else {
              child.READY = false;
              child.kill();
              try {
                  fs.closeSync(child.out_log_fd);
                  fs.closeSync(child.err_log_fd);
              } catch (e) {
                  log.warn('Close fd failed');
              }
              cleanupNode(id);
          }
      });
  
      processes[id] = child;
      tasks[id] = {};
      idle_nodes.push(id);
  };
  
  var dropNode = function(id) {
      log.debug('dropNode, id:', id);
      if (processes.hasOwnProperty(id)) {
          processes[id].kill();
          cleanupNode(id);
      }
  };
  
  var fillNodes = function() {
      var runningNodes = nodes.length + idle_nodes.length;
      var spaceInIdle = spec.prerunNodeNum - idle_nodes.length;
      var nodesToStart = spec.maxNodeNum < 0 ? spaceInIdle : Math.min(spaceInIdle, spec.maxNodeNum - runningNodes);

      for (var i = 0; i < nodesToStart; i++) {
          launchNode();
      }
  };
  
  var addTask = function(nodeId, task) {
      if (tasks[nodeId][task.room].indexOf(task.task) === -1) {
          onTaskAdded(task.task);
      }
  
      tasks[nodeId][task.room].push(task.task);
  };
  
  var removeTask = function(nodeId, task, on_last_task_leave) {
      if (tasks[nodeId]) {
          if (tasks[nodeId][task.room]) {
              var i = tasks[nodeId][task.room].indexOf(task.task);
              if (i > -1) {
                  tasks[nodeId][task.room].splice(i, 1);
              }
  
              if (tasks[nodeId][task.room].indexOf(task.task) === -1) {
                  onTaskRemoved(task.task);
              }
  
              if (tasks[nodeId][task.room].length === 0) {
                  delete tasks[nodeId][task.room];
                  if (Object.keys(tasks[nodeId]).length === 0) {
                      on_last_task_leave();
                  }
              }
          }
      }
  };
  
  let waitTillNodeReady = (id, timeout) => {
    return new Promise((resolve, reject) => {
      if (!processes[id]) {
        reject('node id not found');
      }
      else if (processes[id].READY === true) {
        log.debug('node', id, 'is ready');
        resolve(id);
      } else {
        let wait_count = 0;
        let wait_expiration = timeout / 100 + 1;
        let interval = setInterval(() => {
          if (!processes[id]) {
            clearInterval(interval);
            reject('node early recycled');
          } else if (processes[id].READY === true) {
            clearInterval(interval);
            resolve(id);
          } else if (processes[id].READY === undefined) {
            if (wait_count < wait_expiration) {
              wait_count += 1;
            } else {
              log.debug('Node not ready', id, 'give up'); 
              dropNode(id);
              reject('node not ready');
            }
          }
        }, 100);
      };
    });
  };

  let findNodeUsedByRoom = (nodeList, roomId) => {
      for (let i in nodeList) {
        let node_id = nodeList[i];
        if (tasks[node_id] !== undefined && tasks[node_id][roomId] !== undefined) {
          return node_id;
        }
      }
    return undefined;
  };

  let  pickInIdle = (room) => {
      if (idle_nodes.length < 1) {
        log.error('getNode error:', 'No available node');
        return undefined;
      }
  
      let node_id = idle_nodes.shift();
      tasks[node_id] = tasks[node_id] || {};
      tasks[node_id][room] = tasks[node_id][room] || [];
      nodes.push(node_id);
      setTimeout(() => {
        if ((spec.maxNodeNum < 0) || ((nodes.length + idle_nodes.length) < spec.maxNodeNum)) {
          fillNodes();
        } else if (spec.reuseNode) {
          idle_nodes.push(nodes.shift());
        }
      }, 0);

      return node_id;
  };

  that.getNode = (task) => {
    log.debug('getNode, task:', task);
    if (task.room === undefined || task.task === undefined) {
      return Promise.reject('Invalid task');
    }

    let nodeId = undefined;
    if (spec.consumeNodeByRoom) {
      nodeId = findNodeUsedByRoom(nodes, task.room);
      if (nodeId === undefined) {
        nodeId = findNodeUsedByRoom(idle_nodes, task.room);
      }
    }

    if (nodeId === undefined) {
      nodeId = pickInIdle(task.room);
    }

    if (nodeId === undefined) {
      return Promise.reject('Not found');
    }
    return waitTillNodeReady(nodeId, 1500/*FIXME: Use a more reasonable timeout value instead of hard coding*/)
      .then((nodeId) => {
        addTask(nodeId, task);
        return nodeId;
      }).catch(function(err) {
        tasks[nodeId] = undefined;
        tasks[nodeId][task.room] = undefined;
        return Promise.reject(err);
      });
  };
  
  that.recycleNode = (id, task) => {
    log.debug('recycleNode, id:', id, 'task:', task);
    return new Promise(resolve => {
      removeTask(id, task, function() {
        dropNode(id);
      });
      resolve('ok');
    });
  };
  
  that.queryNode = (task) => {
    log.debug('queryNode, task:', task);
    return new Promise((resolve, reject) => {
      for (var eid in tasks) {
        if (tasks[eid][task] !== undefined) {
          resolve(eid);
        }
      }
      reject('No such a task');
    });
  };

  that.recover = function() {
    spawn_failed = false;
    fillNodes();
  };
  
  that.dropAllNodes = function(quietly) {
      spawn_failed = true;
      Object.keys(processes).map(function (k) {
          !quietly && onNodeAbnormallyQuit && onNodeAbnormallyQuit(k, tasksOnNode(k));
          dropNode(k);
      });
  };
  
  fillNodes();
  return that;
};
