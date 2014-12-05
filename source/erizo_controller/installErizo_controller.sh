#!/bin/bash

echo [erizo_controller] Installing node_modules for erizo_controller

npm install --loglevel error amqp socket.io@0.9.16 log4js node-getopt

echo [erizo_controller] Done, node_modules installed
