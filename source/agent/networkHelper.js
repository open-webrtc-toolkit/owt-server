// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var toml = require('toml');

module.exports.getAddress = (networkInterface) => {
  let interfaces = require('os').networkInterfaces();
  for (let k in interfaces) {
    if (interfaces.hasOwnProperty(k)) {
      for (let k2 in interfaces[k]) {
        if (interfaces[k].hasOwnProperty(k2)) {
          let address = interfaces[k][k2];
          if (address.family === 'IPv4' && !address.internal) {
            if ((networkInterface === k) || (networkInterface === 'firstEnumerated')) {
              return {
                ip: address.address,
                interf: k
              };
            }
          }
        }
      }
    }
  }

  return null;
}
