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
var encodeBase64 = (function () {
    var END_OF_INPUT = -1;

    var base64Chars = new Array(
        'A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P',
        'Q','R','S','T','U','V','W','X',
        'Y','Z','a','b','c','d','e','f',
        'g','h','i','j','k','l','m','n',
        'o','p','q','r','s','t','u','v',
        'w','x','y','z','0','1','2','3',
        '4','5','6','7','8','9','+','/'
    );

    var base64Str;
    var base64Count;

    function readBase64 () {
        if (!base64Str) return END_OF_INPUT;
        if (base64Count >= base64Str.length) return END_OF_INPUT;
        var c = base64Str.charCodeAt(base64Count) & 0xff;
        base64Count++;
        return c;
    }

    return function encodeBase64 (str) {
        base64Str = str;
        base64Count = 0;
        var result = '';
        var inBuffer = new Array(3);
        var done = false;
        while (!done && (inBuffer[0] = readBase64()) != END_OF_INPUT){
            inBuffer[1] = readBase64();
            inBuffer[2] = readBase64();
            result += (base64Chars[ inBuffer[0] >> 2 ]);
            if (inBuffer[1] != END_OF_INPUT){
                result += (base64Chars [(( inBuffer[0] << 4 ) & 0x30) | (inBuffer[1] >> 4) ]);
                if (inBuffer[2] != END_OF_INPUT){
                    result += (base64Chars [((inBuffer[1] << 2) & 0x3c) | (inBuffer[2] >> 6) ]);
                    result += (base64Chars [inBuffer[2] & 0x3F]);
                } else {
                    result += (base64Chars [((inBuffer[1] << 2) & 0x3c)]);
                    result += ('=');
                    done = true;
                }
            } else {
                result += (base64Chars [(( inBuffer[0] << 4 ) & 0x30)]);
                result += ('=');
                result += ('=');
                done = true;
            }
        }
        return result;
    };
}());


function calculateSignature (toSign, key) {
    var hash = CryptoJS.HmacSHA256(toSign, key);
    var hex = hash.toString(CryptoJS.enc.Hex);
    return encodeBase64(hex);
}

function ManagementApi (spec) {
    this.id = spec.id;
    this.key = spec.key;
    this.url = spec.url || '/';
    this.send = function (method, body, url, callback) { // callback (err, resp)
        var service = this.id;
        var key = this.key;

        if(!service || !key) {
            if (typeof callback === 'function') callback(401, 'ServiceID and Key are required!!');
            return;
        }
        url = this.url + url;
        var timestamp = new Date().getTime();
        var cnounce = Math.floor(Math.random() * 99999);
        var toSign = timestamp + ',' + cnounce;
        var header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

        header += ',mauth_serviceid=';
        header += service;
        header += ',mauth_cnonce=';
        header += cnounce;
        header += ',mauth_timestamp=';
        header += timestamp;
        header += ',mauth_signature=';
        header += calculateSignature(toSign, key);

        var req = new XMLHttpRequest();
        req.onreadystatechange = function () {
            if (req.readyState == '4') {
                switch (req.status) {
                case 100:
                case 200:
                case 201:
                case 202:
                case 203:
                case 204:
                case 205:
                    if (typeof callback === 'function') callback(null, req.responseText);
                    break;
                default:
                    if (typeof callback === 'function') callback(req.status, req.responseText);
                }
            }
        };
        req.open(method, url, true);
        req.setRequestHeader('Authorization', header);
        if (body) {
            req.setRequestHeader('Content-Type', 'application/json');
            req.send(JSON.stringify(body));
        } else {
            req.send();
        }
    };
}

ManagementApi.init = function (serviceId, servicekey) {
    return new ManagementApi({
        id: serviceId,
        key: servicekey
    });
};

ManagementApi.prototype.createRoom = function (room, callback) {
    this.send('POST', {name: room.name, options: room.options}, 'v1/rooms', callback);
};

ManagementApi.prototype.getRooms = function (page, per_page, callback) {
    this.send('GET', undefined, 'v1/rooms?page=' + page + '&per_page=' + per_page, callback);
};

ManagementApi.prototype.getRoom = function (roomId, callback) {
    roomId = roomId || null;
    this.send('GET', undefined, 'v1/rooms/' + roomId, callback);
};

ManagementApi.prototype.deleteRoom = function (roomId, callback) {
    roomId = roomId || null;
    this.send('DELETE', undefined, 'v1/rooms/' + roomId, callback);
};

ManagementApi.prototype.updateRoom = function (roomId, updates, callback) {
    roomId = roomId || null;
    this.send('PUT', (updates || {}), 'v1/rooms/' + roomId, callback);
};

ManagementApi.prototype.createService = function (name, key, callback) {
    this.send('POST', {name: name, key: key}, 'services', callback);
};

ManagementApi.prototype.getServices = function (callback) {
    this.send('GET', undefined, 'services', callback);
};

ManagementApi.prototype.getService = function (serviceId, callback) {
    serviceId = serviceId || null;
    this.send('GET', undefined, 'services/' + serviceId, callback);
};

ManagementApi.prototype.deleteService = function (serviceId, callback) {
    serviceId = serviceId || null;
    this.send('DELETE', undefined, 'services/' + serviceId, callback);
};
