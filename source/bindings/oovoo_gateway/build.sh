#!/bin/bash
if hash node-gyp 2>/dev/null; then
  echo 'building with node-gyp'
  node-gyp rebuild
fi
