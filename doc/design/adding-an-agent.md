| Adding an agent in OWT |


# Introduction

OWT agent is an independent module which provides RPC interfaces for external callers. It has following characteristics:
1. Use same RPC mechanism as other OWT modules.
2. Register on OWT cluster manager as cluster worker.
3. Spawn child process to handle actual tasks.


# How to add an agent

1. Create a directory under source, take `source/agent/sample` as an example.
2. Create `package.json` in the new agent directory. This file plays a same role as other `package.json` in Node.js. Besides, we need define a `start` command in `scripts` which will be used when we start the new agent using default daemon script(e.g. bin/start-all.sh). The `start` command should have format as `node . -U [name]`. The [name] is also the entry file name of the new agent.
3. Create `dist.json` in the new agent directory. This file defines packing rules for the new agent. See `source/agent/sample/dist.json` for reference.
4. Create js files with new agent implementation following RPC-agent interfaces. The entry file should have same name as defined in start script of `package.json`. See `source/agent/sample/sample.js` for reference.
