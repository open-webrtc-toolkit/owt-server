/*global console, CryptoJS, XMLHttpRequest*/
var N = N || {};

N.API = (function (N) {
    "use strict";
    var createRoom, getRooms, getRoom, deleteRoom, updateRoom, createToken, createService, getServices, getService, deleteService, getUsers, getUser, deleteUser, params, send, calculateSignature, init;

    params = {
        service: undefined,
        key: undefined,
        url: undefined
    };

    init = function (service, key, url) {
        N.API.params.service = service;
        N.API.params.key = key;
        N.API.params.url = url;
    };

    createRoom = function (name, callback, callbackError, options, params) {

        if (!options) {
            options = {};
        }

        send(function (roomRtn) {
            var room = JSON.parse(roomRtn);
            callback(room);
        }, callbackError, 'POST', {name: name, options: options}, 'rooms', params);
    };

    getRooms = function (callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'rooms', params);
    };

    getRoom = function (room, callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'rooms/' + room, params);
    };

    deleteRoom = function (room, callback, callbackError, params) {
        send(callback, callbackError, 'DELETE', undefined, 'rooms/' + room, params);
    };

    updateRoom = function (roomId, options, callback, callbackError, params) {
        send(callback, callbackError, 'PUT', (options || {}), 'rooms/' + roomId, params);
    };

    createToken = function (room, username, role, callback, callbackError, params) {
        send(callback, callbackError, 'POST', undefined, 'rooms/' + room + "/tokens", params, username, role);
    };

    createService = function (name, key, callback, callbackError, params) {
        send(callback, callbackError, 'POST', {name: name, key: key}, 'services/', params);
    };

    getServices = function (callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'services/', params);
    };

    getService = function (service, callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'services/' + service, params);
    };

    deleteService = function (service, callback, callbackError, params) {
        send(callback, callbackError, 'DELETE', undefined, 'services/' + service, params);
    };

    getUsers = function (room, callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'rooms/' + room + '/users/', params);
    };

    getUser = function (room, user, callback, callbackError, params) {
        send(callback, callbackError, 'GET', undefined, 'rooms/' + room + '/users/' + user, params);
    };

    deleteUser = function (room, user, callback, callbackError, params) {
        send(callback, callbackError, 'DELETE', undefined, 'rooms/' + room + '/users/' + user, params);
    };

    send = function (callback, callbackError, method, body, url, params, username, role) {
        var service, key, timestamp, cnounce, toSign, header, signed, req;

        if (params === undefined) {
            service = N.API.params.service;
            key = N.API.params.key;
            url = N.API.params.url + url;
        } else {
            service = params.service;
            key = params.key;
            url = params.url + url;
        }

        if (service === '' || key === '') {
            if (typeof callbackError === 'function')
                callbackError(401, 'ServiceID and Key are required!!');
            return;
        }

        timestamp = new Date().getTime();
        cnounce = Math.floor(Math.random() * 99999);

        toSign = timestamp + ',' + cnounce;

        header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

        if (username && role) {

            username = formatString(username);

            header += ',mauth_username=';
            header +=  username;
            header += ',mauth_role=';
            header +=  role;

            toSign += ',' + username + ',' + role;
        }

        signed = calculateSignature(toSign, key);

        header += ',mauth_serviceid=';
        header +=  service;
        header += ',mauth_cnonce=';
        header += cnounce;
        header += ',mauth_timestamp=';
        header +=  timestamp;
        header += ',mauth_signature=';
        header +=  signed;

        req = new XMLHttpRequest();

        req.onreadystatechange = function () {
            if (req.readyState === 4) {
                switch (req.status) {
                case 100:
                case 200:
                case 201:
                case 202:
                case 203:
                case 204:
                case 205:
                    if (typeof callback === 'function') callback(req.responseText);
                    break;
                case 400:
                case 401:
                case 403:
                default:
                    if (typeof callbackError === 'function') callbackError(req.status, req.responseText);
                }
            }
        };

        req.open(method, url, true);

        req.setRequestHeader('Authorization', header);

        if (body !== undefined) {
            req.setRequestHeader('Content-Type', 'application/json');
            req.send(JSON.stringify(body));
        } else {
            req.send();
        }

    };

    calculateSignature = function (toSign, key) {
        var hash, hex, signed;
        hash = CryptoJS.HmacSHA256(toSign, key);
        hex = hash.toString(CryptoJS.enc.Hex);
        signed = N.Base64.encodeBase64(hex);
        return signed;
    };

    formatString = function(s){
        var r = s.toLowerCase();
        non_asciis = {'a': '[àáâãäå]', 'ae': 'æ', 'c': 'ç', 'e': '[èéêë]', 'i': '[ìíîï]', 'n': 'ñ', 'o': '[òóôõö]', 'oe': 'œ', 'u': '[ùúûűü]', 'y': '[ýÿ]'};
        for (i in non_asciis) { r = r.replace(new RegExp(non_asciis[i], 'g'), i); }
        return r;
    };

    return {
        params: params,
        init: init,
        createRoom: createRoom,
        getRooms: getRooms,
        getRoom: getRoom,
        deleteRoom: deleteRoom,
        updateRoom: updateRoom,
        createToken: createToken,
        createService: createService,
        getServices: getServices,
        getService: getService,
        deleteService: deleteService,
        getUsers: getUsers,
        getUser: getUser,
        deleteUser: deleteUser
    };
}(N));
