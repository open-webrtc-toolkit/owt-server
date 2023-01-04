// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var Url = require("url");
const crypto = require('crypto');

var restClient = function(spec) {

    var version = 'v1';

    var that = {},
        restservice = spec.service,
        restkey = spec.key,
        resturl = (spec.url.endsWith('/') ? (spec.url + version + '/') : (spec.url + '/' + version + '/')),
        rejectUnauthorizedCert = spec.rejectUnauthorizedCert;

    console.log("spec url is:", resturl, " service is:", restservice, " key is:", restkey);
    var version = 'v1';

    function calculateSignature(toSign, key) {
        const hash = crypto.createHmac("sha256", key).update(toSign);
        const hex = hash.digest('hex');
        return Buffer.from(hex).toString('base64');
    }

    var send = function(method, resource, body, onOK, onError) {
        console.log("url before is:", resturl, " service is:", restservice, " key is:", restkey);
        var url = Url.parse(resturl + resource);
        console.log("url is:", url);
        var ssl = (url.protocol === 'https:' ? true : false);
        var timestamp = new Date().getTime();
        var cnounce = require('crypto').randomBytes(8).toString('hex');
        var toSign = timestamp + ',' + cnounce;
        var header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

        header += ',mauth_serviceid=';
        header += restservice;
        header += ',mauth_cnonce=';
        header += cnounce;
        header += ',mauth_timestamp=';
        header += timestamp;
        header += ',mauth_signature=';
        header += calculateSignature(toSign, restkey);

        var options = {
            hostname: url.hostname,
            port: url.port || (ssl ? 443 : 80),
            path: url.pathname + (url.search ? url.search : ''),
            method: method,
            headers: {
                'Host': url.hostname,
                'Authorization': header
            }
        };
        ssl && (rejectUnauthorizedCert !== undefined) && (options.rejectUnauthorized = rejectUnauthorizedCert);

        var bodyJSON;
        if (body) {
            bodyJSON = JSON.stringify(body);
            options.headers['Content-Type'] = 'application/json';
            options.headers['Content-Length'] = Buffer.byteLength(bodyJSON);
        } else {
            options.headers['Content-Type'] = 'text/plain;charset=UTF-8';
        }

        var doRequest = (ssl ? require('https').request : require('http').request);
        var req = doRequest(options, (res) => {
            res.setEncoding("utf8");
            var resTxt = '';
            var status = res.statusCode;

            res.on('data', (chunk) => {
                resTxt += chunk;
            });

            res.on('end', () => {
                if (status === 100 || (status >= 200 && status <= 205)) {
                    onOK(resTxt);
                } else {
                    onError(status, resTxt);
                }
            });

            req.on('error', (err) => {
                console.error('Send http(s) error:', err);
                onError(503, err);
            });
        }).on('error', (err) => {
            console.error('Build http(s) req error:', err);
            onError(503, err);
        });

        console.log("send bodyjson:", bodyJSON, " options are:", options);
        bodyJSON && req.write(bodyJSON);

        req.end();
    };


    /**
       * @function createRoom
       * @desc This function creates a room.
       <br><b>Remarks:</b><br>
    <b>options:</b>
    <br>
    <ul>
        <li><b>mode:</b>"hybrid" for room with mixing and forward streams.</li>
        <li><b>publishLimit:</b>limiting number of publishers in the room. Value should be equal to or greater than -1. -1 for unlimited.</li>
        <li><b>userLimit:</b>limiting number of users in the room. Value should be equal to or greater than -1. -1 for unlimited.</li>
        <li><b>enableMixing:</b>control whether to enable media mixing in the room, with value choices 0 or 1.</li>
        <li><b>viewports:</b>viewport setting for mixed stream in the room if mixing is enabled. A corresponding mixed stream will be created for each viewport. Values should be an array. Each item has two properties listed as follow</li>
        <ul>
          <li><b>name:</b>the name for this viewport.</li>
          <li><b>mediaMixing:</b>media setting for mixed stream in the room if mixing is enabled. Value should be a JSON object contains two entries: "video" and "audio". Audio entry is currently not used and should be null.</li>
          <ul>
              <li>audio: null</li>
              <li>video: maxInput, resolution, quality_level, bkColor, layout, avCoordinate, crop</li>
              <ul>
                  <li>maxInput is for maximum number of slots in the mix stream</li>
                  <li>resolution denotes the resolution of the video size of mix stream.Valid resolution list:</li>
                      <ul>
                          <li>'sif'</li>
                          <li>'vga'</li>
                          <li>'svga'</li>
                          <li>'xga'</li>
                          <li>'hd720p'</li>
                          <li>'hd1080p'</li>
                          <li>'uhd_4k'</li>
                          <li>'r720x720'</li>
                          <li>'r720x1080'</li>
                          <li>'r1080x1920'</li>
                      </ul>
                  <li>quality_level indicates the default video quality of the mix stream (choose from "bestSpeed", "betterSpeed", "standard", "betterQuality", "bestQuality").</li>
                  <li>bkColor sets the background color, supporting RGB color format: {"r":red-value, "g":green-value, "b":blue-value}.</li>
                  <li>layout describes video layout in mix stream</li>
                      <ul>
                          <li>"base" is the base template (choose from "void", "fluid", "lecture")</li>
                          <li>If base layout is set to 'void', user must input customized layout for the room, otherwise the video layout would be treated as invalid. </li>
                          <li>"custom" is user-defined customized video layout. Here we give out an example to show you the details of a valid customized video layout.A valid customized video layout should be a JSON string which represents an array of video layout definition. More details see [customized video layout](@ref layout) . </li>
                          <li>MCU would try to combine the two entries for mixing video if user sets both.</li>
                      </ul>
                  <li>avCoordinated (0 or 1) is for disabling/enabling VAD(Voice activity detection). When VAD is applied, main pane(layout id=1) will be filled with the user stream which is the most active in voice currently.</li>
                  <li>crop (0 or 1) is for disabling/enabling video cropping to fit in the region assigned to it in the mixed video.</li>
              </ul>
          </ul>
        </ul>
      </ul>
    Omitted entries are set with default values.
    All supported resolutions are list in the following table.
    @htmlonly
    <table class="doxtable">
    <caption><b>Table : Resolution Mapping for Multistreaming</b></caption>
        <tbody>
        <thead>
            <tr>
                <th><b>Base resolution</b></th>
                <th><b>Available resolution list</b></th>
            </tr>
        </thead>
            <tr>
                <td>sif</td>
                <td>{width: 320, height: 240}</td>
            </tr>
            <tr>
                <td>vga</td>
                <td>{width: 640, height: 480}</td>
            </tr>
            <tr>
                <td>svga</td>
                <td>{width: 800, height: 600}</td>
            </tr>
            <tr>
                <td>xga</td>
                <td>{width: 1024, height: 768}</td>
            </tr>
            <tr>
                <td>hd720p</td>
                <td>{width: 1280, height: 720}, {width: 640, height: 480}, {width: 640, height: 360}</td>
            </tr>
            <tr>
                <td>hd1080p</td>
                <td>{width: 1920, height: 1080}, {width: 1280, height: 720}, {width: 800, height: 600}, {width: 640, height: 480}, {width: 640, height: 360}</td>
            </tr>
            <tr>
                <td>uhd_4k</td>
                <td>{width: 3840, height: 2160}, {width: 1920, height: 1080}, {width: 1280, height: 720}, {width: 800, height: 600}, {width: 640, height: 480}</td>
            </tr>
            <tr>
                <td>r720x720</td>
                <td>{width: 720, height: 720}, {width: 480, height: 480}, {width: 360, height: 360}</td>
            </tr>
            <tr>
                <td>r720x1080</td>
                <td>{width: 720, height: 1280}, {width: 540, height: 960}, {width: 480, height: 853}, {width: 360, height: 640}, {width: 240, height: 426}, {width: 180, height: 320}, {width: 640, height: 480}, {width: 352, height: 288}</td>
            </tr>
            <tr>
                <td>r1080x1920</td>
                <td>{width: 1080, height: 1920}, {width: 810, height: 1440}, {width: 720, height: 1280}, {width: 540, height: 960}, {width: 360, height: 640}, {width: 270, height: 480}, {width: 800, height: 600}, {width: 640, height: 480}, {width: 352, height: 288}</td>
            </tr>
        </tbody>
    </table>
    @endhtmlonly
       * @memberOf restClient
       * @param {string} name                          -Room name.
       * @param {json} options                         -Room configuration.
       * @param {function} callback                    -Callback function on success.
       * @param {function} callbackError               -Callback function on error.
       * @example
    that.createRoom('myRoom', {
      mode: 'hybrid',
      publishLimit: -1,
      userLimit: 30,
      viewports: [
        {
          name: "common",
          mediaMixing: {
            video: {
              maxInput: 15,
              resolution: 'hd720p',
              quality_level: 'standard',
              bkColor: {"r":1, "g":2, "b":255},
              layout: {
                base: 'lecture',
              },
              avCoordinated: 1,
              crop: 1
            },
            audio: null
          },
        },
        {
          name: "another",
          mediaMixing: {
            video: {
              maxInput: 15,
              resolution: 'hd1080p',
              quality_level: 'standard',
              bkColor: {"r":1, "g":2, "b":255},
              layout: {
                base: 'lecture',
              },
              avCoordinated: 1,
              crop: 1
            },
            audio: null
          },
        }
      ]
    }, function (res) {
      console.log ('Room', res.name, 'created with id:', res._id);
    }, function (err) {
      console.log ('Error:', err);
    });
       */
    that.createRoom = function(name, options, callback, callbackError) {
        if (!options) {
            options = {};
        }

        if (options.viewports) {
            options.views = viewportsToViews(options.viewports);
            delete options.viewports;
        }

        send('POST', 'rooms', {
            name: name,
            options: options
        }, function(roomRtn) {
            var room = JSON.parse(roomRtn);
            callback(room);
        }, callbackError);
    };

    that.startEventCascading = function(room, options, callback, callbackError) {
        send('POST', 'rooms/' + room + '/cascading', {
            options: options
        }, function(result) {
            callback(result);
        }, callbackError);
    }

    that.getBridges = function(room, clusterID, callback, callbackError) {
        send('GET', 'rooms/' + room + '/bridges', {
            targetCluster: clusterID
        }, function(result) {
            callback(result);
        }, callbackError);
    }

    /**
       * @function getRoom
       * @desc This function returns information on the specified room.
       * @memberOf OWT_REST.API
       * @param {string} room                          -Room ID
       * @param {function} callback                    -Callback function on success
       * @param {function} callbackError               -Callback function on error
       * @example
    var roomId = '51c10d86909ad1f939000001';
    OWT_REST.API.getRoom(roomId, function(room) {
      console.log('Room name:', room.name);
    }, function(status, error) {
      // HTTP status and error
      console.log(status, error);
    });
       */
    that.getRoom = function(room, callback, callbackError) {
        if (typeof room !== 'string') {
            callbackError(401, 'Invalid room ID.');
            return;
        }
        if (room.trim() === '') {
            callbackError(401, 'Empty room ID');
            return;
        }
        send('GET', 'rooms/' + room, undefined, function(roomRtn) {
            var room = JSON.parse(roomRtn);
            callback(room);
        }, callbackError);
    };

    that.createToken = function(room, user, role, preference, callback, callbackError) {
        if (typeof room !== 'string' || typeof user !== 'string' || typeof role !== 'string') {
            if (typeof callbackError === 'function')
                callbackError(400, 'Invalid argument.');
            return;
        }
        send('POST', 'rooms/' + room + '/tokens/', {
            preference: preference,
            user: user,
            role: role
        }, callback, callbackError);
    };


    return that;
};



module.exports = restClient;
