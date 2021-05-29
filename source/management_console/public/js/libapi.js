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

function ManagementApi (spec) {
    this.url = spec.url || '/';
    this.send = function (method, body, url, callback) { // callback (err, resp)
        url = this.url + url;
        var header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

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

ManagementApi.init = function () {
    return new ManagementApi({});
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

ManagementApi.prototype.login = function(id, key, callback) {
    this.send('POST', {id: id, key: key}, 'login', callback);
};

ManagementApi.prototype.loginCheck = function(callback) {
    this.send('GET', undefined, 'login', callback);
};

ManagementApi.prototype.logout = function(callback) {
    this.send('GET', undefined, 'logout', callback);
};
