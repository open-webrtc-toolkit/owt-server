// MIT License
//
// Copyright (c) 2012 Universidad Politécnica de Madrid
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';
var crypto = require('crypto');

/*
 * Parses a string header to a json with the fields of the authentication header.
 */
exports.parseHeader = function (header) {
    var params = {},
        array = [],
        p = header.split(','),
        i,
        j,
        val;

    for (i = 0; i < p.length; i += 1) {

        array = p[i].split('=');
        val = '';

        for (j = 1; j < array.length; j += 1) {
            if (array[j] === '') {
                val += '=';
            } else {
                val += array[j];
            }
        }

        params[array[0].slice(6)] = val;

    }
    return params;
};

/*
 * Makes a string header from a json with the fields of an authentication header.
 */
exports.makeHeader = function (params) {
    if (params.realm === undefined) {
        return undefined;
    }

    var header = 'MAuth realm=\"' + params.realm + '\"',
        key;

    for (key in params) {
        if (params.hasOwnProperty(key)) {
            if (key !== 'realm') {
                header += ',mauth_' + key + '=\"' + params[key] + '\"';
            }
        }
    }
    return header;
};

/*
 * Given a json with the header params and the key calculates the signature.
 */
exports.calculateClientSignature = function (params, key) {
    var toSign = params.timestamp + ',' + params.cnonce,
        signed;

    if (params.username !== undefined && params.role !== undefined) {
        toSign += ',' + params.username + ',' + params.role;
    }

    signed = crypto.createHmac('sha256', key).update(toSign).digest('hex');
    return Buffer.from(signed).toString('base64');
};

/*
 * Given a json with the header params and the key calculates the signature.
 */
exports.calculateServerSignature = function (params, key) {
    var toSign = params.timestamp,
        signed = crypto.createHmac('sha256', key).update(toSign).digest('hex');

    return Buffer.from(signed).toString('base64');
};
