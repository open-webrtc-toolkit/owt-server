/*
CryptoJS v3.0.2
code.google.com/p/crypto-js
(c) 2009-2012 by Jeff Mott. All rights reserved.
code.google.com/p/crypto-js/wiki/License
*/
'use strict';
var CryptoJS=CryptoJS||function(h,i){var e={},f=e.lib={},l=f.Base=function(){function a(){}return{extend:function(j){a.prototype=this;var d=new a;j&&d.mixIn(j);d.$super=this;return d},create:function(){var a=this.extend();a.init.apply(a,arguments);return a},init:function(){},mixIn:function(a){for(var d in a)a.hasOwnProperty(d)&&(this[d]=a[d]);a.hasOwnProperty("toString")&&(this.toString=a.toString)},clone:function(){return this.$super.extend(this)}}}(),k=f.WordArray=l.extend({init:function(a,j){a=
this.words=a||[];this.sigBytes=j!=i?j:4*a.length},toString:function(a){return(a||m).stringify(this)},concat:function(a){var j=this.words,d=a.words,c=this.sigBytes,a=a.sigBytes;this.clamp();if(c%4)for(var b=0;b<a;b++)j[c+b>>>2]|=(d[b>>>2]>>>24-8*(b%4)&255)<<24-8*((c+b)%4);else if(65535<d.length)for(b=0;b<a;b+=4)j[c+b>>>2]=d[b>>>2];else j.push.apply(j,d);this.sigBytes+=a;return this},clamp:function(){var a=this.words,b=this.sigBytes;a[b>>>2]&=4294967295<<32-8*(b%4);a.length=h.ceil(b/4)},clone:function(){var a=
l.clone.call(this);a.words=this.words.slice(0);return a},random:function(a){for(var b=[],d=0;d<a;d+=4)b.push(4294967296*h.random()|0);return k.create(b,a)}}),o=e.enc={},m=o.Hex={stringify:function(a){for(var b=a.words,a=a.sigBytes,d=[],c=0;c<a;c++){var e=b[c>>>2]>>>24-8*(c%4)&255;d.push((e>>>4).toString(16));d.push((e&15).toString(16))}return d.join("")},parse:function(a){for(var b=a.length,d=[],c=0;c<b;c+=2)d[c>>>3]|=parseInt(a.substr(c,2),16)<<24-4*(c%8);return k.create(d,b/2)}},q=o.Latin1={stringify:function(a){for(var b=
a.words,a=a.sigBytes,d=[],c=0;c<a;c++)d.push(String.fromCharCode(b[c>>>2]>>>24-8*(c%4)&255));return d.join("")},parse:function(a){for(var b=a.length,d=[],c=0;c<b;c++)d[c>>>2]|=(a.charCodeAt(c)&255)<<24-8*(c%4);return k.create(d,b)}},r=o.Utf8={stringify:function(a){try{return decodeURIComponent(escape(q.stringify(a)))}catch(b){throw Error("Malformed UTF-8 data");}},parse:function(a){return q.parse(unescape(encodeURIComponent(a)))}},b=f.BufferedBlockAlgorithm=l.extend({reset:function(){this._data=k.create();
this._nDataBytes=0},_append:function(a){"string"==typeof a&&(a=r.parse(a));this._data.concat(a);this._nDataBytes+=a.sigBytes},_process:function(a){var b=this._data,d=b.words,c=b.sigBytes,e=this.blockSize,g=c/(4*e),g=a?h.ceil(g):h.max((g|0)-this._minBufferSize,0),a=g*e,c=h.min(4*a,c);if(a){for(var f=0;f<a;f+=e)this._doProcessBlock(d,f);f=d.splice(0,a);b.sigBytes-=c}return k.create(f,c)},clone:function(){var a=l.clone.call(this);a._data=this._data.clone();return a},_minBufferSize:0});f.Hasher=b.extend({init:function(){this.reset()},
reset:function(){b.reset.call(this);this._doReset()},update:function(a){this._append(a);this._process();return this},finalize:function(a){a&&this._append(a);this._doFinalize();return this._hash},clone:function(){var a=b.clone.call(this);a._hash=this._hash.clone();return a},blockSize:16,_createHelper:function(a){return function(b,d){return a.create(d).finalize(b)}},_createHmacHelper:function(a){return function(b,d){return g.HMAC.create(a,d).finalize(b)}}});var g=e.algo={};return e}(Math);
(function(h){var i=CryptoJS,e=i.lib,f=e.WordArray,e=e.Hasher,l=i.algo,k=[],o=[];(function(){function e(a){for(var b=h.sqrt(a),d=2;d<=b;d++)if(!(a%d))return!1;return!0}function f(a){return 4294967296*(a-(a|0))|0}for(var b=2,g=0;64>g;)e(b)&&(8>g&&(k[g]=f(h.pow(b,0.5))),o[g]=f(h.pow(b,1/3)),g++),b++})();var m=[],l=l.SHA256=e.extend({_doReset:function(){this._hash=f.create(k.slice(0))},_doProcessBlock:function(e,f){for(var b=this._hash.words,g=b[0],a=b[1],j=b[2],d=b[3],c=b[4],h=b[5],l=b[6],k=b[7],n=0;64>
n;n++){if(16>n)m[n]=e[f+n]|0;else{var i=m[n-15],p=m[n-2];m[n]=((i<<25|i>>>7)^(i<<14|i>>>18)^i>>>3)+m[n-7]+((p<<15|p>>>17)^(p<<13|p>>>19)^p>>>10)+m[n-16]}i=k+((c<<26|c>>>6)^(c<<21|c>>>11)^(c<<7|c>>>25))+(c&h^~c&l)+o[n]+m[n];p=((g<<30|g>>>2)^(g<<19|g>>>13)^(g<<10|g>>>22))+(g&a^g&j^a&j);k=l;l=h;h=c;c=d+i|0;d=j;j=a;a=g;g=i+p|0}b[0]=b[0]+g|0;b[1]=b[1]+a|0;b[2]=b[2]+j|0;b[3]=b[3]+d|0;b[4]=b[4]+c|0;b[5]=b[5]+h|0;b[6]=b[6]+l|0;b[7]=b[7]+k|0},_doFinalize:function(){var e=this._data,f=e.words,b=8*this._nDataBytes,
g=8*e.sigBytes;f[g>>>5]|=128<<24-g%32;f[(g+64>>>9<<4)+15]=b;e.sigBytes=4*f.length;this._process()}});i.SHA256=e._createHelper(l);i.HmacSHA256=e._createHmacHelper(l)})(Math);
(function(){var h=CryptoJS,i=h.enc.Utf8;h.algo.HMAC=h.lib.Base.extend({init:function(e,f){e=this._hasher=e.create();"string"==typeof f&&(f=i.parse(f));var h=e.blockSize,k=4*h;f.sigBytes>k&&(f=e.finalize(f));for(var o=this._oKey=f.clone(),m=this._iKey=f.clone(),q=o.words,r=m.words,b=0;b<h;b++)q[b]^=1549556828,r[b]^=909522486;o.sigBytes=m.sigBytes=k;this.reset()},reset:function(){var e=this._hasher;e.reset();e.update(this._iKey)},update:function(e){this._hasher.update(e);return this},finalize:function(e){var f=
this._hasher,e=f.finalize(e);f.reset();return f.finalize(this._oKey.clone().concat(e))}})})();

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

// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';

/*global unescape*/
var OWT_REST = OWT_REST || {};
OWT_REST.Base64 = (function(OWT_REST) {
  var END_OF_INPUT, base64Chars, reverseBase64Chars, base64Str, base64Count,
    i, setBase64Str, readBase64, encodeBase64, readReverseBase64, ntos,
    decodeBase64;

  END_OF_INPUT = -1;

  base64Chars = [
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
  ];

  reverseBase64Chars = [];

  for (i = 0; i < base64Chars.length; i = i + 1) {
    reverseBase64Chars[base64Chars[i]] = i;
  }

  setBase64Str = function(str) {
    base64Str = str;
    base64Count = 0;
  };

  readBase64 = function() {
    var c;
    if (!base64Str) {
      return END_OF_INPUT;
    }
    if (base64Count >= base64Str.length) {
      return END_OF_INPUT;
    }
    c = base64Str.charCodeAt(base64Count) & 0xff;
    base64Count = base64Count + 1;
    return c;
  };

  encodeBase64 = function(str) {
    var result, inBuffer, done;
    setBase64Str(str);
    result = '';
    inBuffer = new Array(3);
    done = false;
    while (!done && (inBuffer[0] = readBase64()) !== END_OF_INPUT) {
      inBuffer[1] = readBase64();
      inBuffer[2] = readBase64();
      result = result + (base64Chars[inBuffer[0] >> 2]);
      if (inBuffer[1] !== END_OF_INPUT) {
        result = result + (base64Chars[((inBuffer[0] << 4) & 0x30) | (
          inBuffer[1] >> 4)]);
        if (inBuffer[2] !== END_OF_INPUT) {
          result = result + (base64Chars[((inBuffer[1] << 2) & 0x3c) | (
            inBuffer[2] >> 6)]);
          result = result + (base64Chars[inBuffer[2] & 0x3F]);
        } else {
          result = result + (base64Chars[((inBuffer[1] << 2) & 0x3c)]);
          result = result + ('=');
          done = true;
        }
      } else {
        result = result + (base64Chars[((inBuffer[0] << 4) & 0x30)]);
        result = result + ('=');
        result = result + ('=');
        done = true;
      }
    }
    return result;
  };

  readReverseBase64 = function() {
    if (!base64Str) {
      return END_OF_INPUT;
    }
    while (true) {
      if (base64Count >= base64Str.length) {
        return END_OF_INPUT;
      }
      var nextCharacter = base64Str.charAt(base64Count);
      base64Count = base64Count + 1;
      if (reverseBase64Chars[nextCharacter]) {
        return reverseBase64Chars[nextCharacter];
      }
      if (nextCharacter === 'A') {
        return 0;
      }
    }
  };

  ntos = function(n) {
    n = n.toString(16);
    if (n.length === 1) {
      n = "0" + n;
    }
    n = "%" + n;
    return unescape(n);
  };

  decodeBase64 = function(str) {
    var result, inBuffer, done;
    setBase64Str(str);
    result = "";
    inBuffer = new Array(4);
    done = false;
    while (!done && (inBuffer[0] = readReverseBase64()) !== END_OF_INPUT &&
      (inBuffer[1] = readReverseBase64()) !== END_OF_INPUT) {
      inBuffer[2] = readReverseBase64();
      inBuffer[3] = readReverseBase64();
      result = result + ntos((((inBuffer[0] << 2) & 0xff) | inBuffer[1] >>
        4));
      if (inBuffer[2] !== END_OF_INPUT) {
        result += ntos((((inBuffer[1] << 4) & 0xff) | inBuffer[2] >> 2));
        if (inBuffer[3] !== END_OF_INPUT) {
          result = result + ntos((((inBuffer[2] << 6) & 0xff) | inBuffer[
            3]));
        } else {
          done = true;
        }
      } else {
        done = true;
      }
    }
    return result;
  };

  return {
    encodeBase64: encodeBase64,
    decodeBase64: decodeBase64
  };
}(OWT_REST));

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

// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

// This file is borrowed from lynckia/licode with some modifications.

'use strict';

/*global require, CryptoJS, Buffer, url, http, https*/
var Url = require("url");

var OWT_REST = OWT_REST || {};

/**@namespace OWT_REST
 * @classDesc Namespace for OWT(Intel Collaboration Suite) REST API definition.
 */
/**
 * @class OWT_REST.API
 * @classDesc Server-side APIs should be called by RTC service integrators, as demostrated in sampleRTCService.js. Server-side APIs are RESTful, provided as a Node.js module. All APIs, except OWT_REST.API.init(), should not be called too frequently. These API calls carry local timestamps and are grouped by serviceID. Once the server is handling an API call from a certain serviceID, all other API calls from the same serviceID, whose timestamps are behind, would be expired or treated as invalid.<br>
We recommend that API calls against serviceID should have interval of at least 100ms. Also, it is better to retry the logic if it fails with an unexpected timestamp error.
 */
OWT_REST.API = (function(OWT_REST) {
  'use strict';
  var version = 'v1';
  var params = {
    service: undefined,
    key: undefined,
    url: undefined,
    rejectUnauthorizedCert: undefined
  };

  function calculateSignature (toSign, key) {
    var hash, hex, signed;
    hash = CryptoJS.HmacSHA256(toSign, key);
    hex = hash.toString(CryptoJS.enc.Hex);
    signed = OWT_REST.Base64.encodeBase64(hex);
    return signed;
  };

  function send (method, resource, body, onOK, onError) {
    var url = Url.parse(params.url + resource);
    var ssl = (url.protocol === 'https:' ? true : false);
    var timestamp = new Date().getTime();
    var cnounce = require('crypto').randomBytes(8).toString('hex');
    var toSign = timestamp + ',' + cnounce;
    var header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

    header += ',mauth_serviceid=';
    header += params.service;
    header += ',mauth_cnonce=';
    header += cnounce;
    header += ',mauth_timestamp=';
    header += timestamp;
    header += ',mauth_signature=';
    header += calculateSignature(toSign, params.key);

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
    ssl && (params.rejectUnauthorizedCert !== undefined) && (options.rejectUnauthorized = params.rejectUnauthorizedCert);

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

    bodyJSON && req.write(bodyJSON);

    req.end();
  };

  /**
     * @function init
     * @desc This function completes the essential configuration.
  <br><b>Remarks:</b><br>
  Make sure you use the correct OWT_REST server url, according to the OWT_REST ssl configuration.
     * @memberOf OWT_REST.API
     * @param {string} service                       -The ID of your service.
     * @param {string} key                           -The key of your service.
     * @param {string} url                           -The URL of OWT service.
     * @param {boolean} rejectUnauthorizedCert       -Flag to determine whether reject unauthorized certificates, with value being true or false, true by default.
     * @example
  OWT_REST.API.init('5188b9af6e53c84ffd600413', '21989', 'http://61.129.90.140:3000/', true)
     */
  var init = function(service, key, url, rejectUnauthorizedCert) {
    if (typeof service !== 'string' || service === '') {
      throw new TypeError('Invalid service ID');
    }
    if (typeof key !== 'string' || key === '') {
      throw new TypeError('Invalid service key');
    }
    if (typeof url !== 'string' || url === '') {
      throw new TypeError('Invalid URL.');
    }
    if (typeof rejectUnauthorizedCert !== 'boolean' && rejectUnauthorizedCert !== undefined) {
      throw new TypeError('Invalid certificate setting');
    }
    params.service = service;
    params.key = key;
    params.url = (url.endsWith('/') ? (url + version + '/') : (url + '/' + version + '/'));
    params.rejectUnauthorizedCert = (rejectUnauthorizedCert === undefined ? true : rejectUnauthorizedCert);
  };

  // Convert a viewports object to views which is defined in MCU.
  function viewportsToViews(viewports) {
    var view = {};
    viewports.forEach(function(viewport) {
      view[viewport.name] = {
        mediaMixing: viewport.mediaMixing
      };
    });
    return view;
  }

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
     * @memberOf OWT_REST.API
     * @param {string} name                          -Room name.
     * @param {json} options                         -Room configuration.
     * @param {function} callback                    -Callback function on success.
     * @param {function} callbackError               -Callback function on error.
     * @example
  OWT_REST.API.createRoom('myRoom', {
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
  var createRoom = function(name, options, callback, callbackError) {
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

  /**
     * @function getRooms
     * @desc This function lists the rooms in your service.
     * @memberOf OWT_REST.API
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  OWT_REST.API.getRooms(function(rooms) {
    for(var i in rooms) {
      console.log('Room', i, ':', rooms[i].name);
    }
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getRooms = function(option, callback, callbackError) {
    option = option || {};
    var page = option.page || 1;
    var per_page = option.per_page || 50;
    var query = '?page=' + page + '&per_page=' + per_page;
    send('GET', 'rooms' + query, undefined, function(roomsRtn) {
      var rooms = JSON.parse(roomsRtn);
      callback(rooms);
    }, callbackError);
  };

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
  var getRoom = function(room, callback, callbackError) {
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

  /**
     * @function deleteRoom
     * @desc This function deletes the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID to be deleted
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var room = '51c10d86909ad1f939000001';
  OWT_REST.API.deleteRoom(room, function(result) {
    console.log ('Result:' result);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var deleteRoom = function(room, callback, callbackError) {
    send('DELETE', 'rooms/' + room, undefined, function(room) {
      callback(room);
    }, callbackError);
  };

  /**
     * @function updateRoom
     * @desc This function updates a room's configuration entirely.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {json} options                         -Room configuration. See details about options in {@link OWT_REST.API#createRoom createRoom(name, options, callback, callbackError)}.
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  OWT_REST.API.updateRoom(XXXXXXXXXX, {
    publishLimit: -1,
    userLimit: -1,
    enableMixing: 1,
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
        name: "another":,
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
    console.log ('Room', res._id, 'updated');
  }, function (err) {
    console.log ('Error:', err);
  });
     */

  var updateRoom = function(room, options, callback, callbackError) {
    if (options && options.viewports) {
      options.views = viewportsToViews(options.viewports);
      delete options.viewports;
    }
    send('PUT', 'rooms/' + room, (options || {}), function(roomRtn) {
      var room = JSON.parse(roomRtn);
      callback(room);
    }, callbackError);
  };

  /**
     * @function updateRoomPartially
     * @desc This function updates a room's configuration partially.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {Array.<{op: string, path: string, value: json}>} items  -Configuration item list to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  OWT_REST.API.updateRoomPartially(XXXXXXXXXX, [
    {op: 'replace', path: '/enableMixing', value: 0},
    {op: 'replace', path: '/viewports/0/mediaMixing/video/avCoordinated', value: 1}
  ], function (res) {
    console.log ('Room', res._id, 'updated');
  }, function (err) {
    console.log ('Error:', err);
  });
     */
  var updateRoomPartially = function(room, items, callback, callbackError) {
    send('PATCH', 'rooms/' + room, (items || []), function(roomRtn) {
      var new_room = JSON.parse(roomRtn);
      callback(new_room);
    }, callbackError);
  };

  /*
     * * @callback onParticipantList
     * * @param {Array.<{Object} ParticipantDetail>} participantList  -The list of object "ParticipantDetail" same as defined in "onParticipantDetail" callback.
  */
  /**
     * @function getParticipants
     * @desc This function lists participants currently in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {onParticipantList} callback           -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getParticipants(roomId, function(participants) {
    var l = JSON.parse(participants);
    console.log ('This room has ', l.length, 'participants');
    for (var i in l) {
      console.log(i, ':', l[i]);
    }
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getParticipants = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/participants/', undefined, function(participantsRtn) {
      var participants = JSON.parse(participantsRtn);
      callback(participants);
    }, callbackError);
  };

  /*
     * * @callback onParticipantDetail
     * * @param {Object} ParticipantDetail           -The object containing the detailed info of the specified participant.
     * * @param {string} ParticipantDetail.id        -The participant ID.
     * * @param {string} ParticipantDetail.role      -The participant role.
     * * @param {string} ParticipantDetail.user      -The user ID of the participant.
     * * @param {Object} ParticipantDetail.permission      -The "Permission" object defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
  */
  /**
     * @function getParticipant
     * @desc This function gets a participant's information from the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} participant                   -Participant ID
     * @param {onParticipantDetail} callback         -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var participantID = 'JdlUI29yjfVY6O4yAAAB';
  OWT_REST.API.getParticipant(roomId, participantID, function(participant) {
    console.log('Participant:', participant);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getParticipant = function(room, participant, callback, callbackError) {
    if (typeof participant !== 'string' || participant.trim().length === 0) {
      return callbackError('Invalid participant ID');
    }
    send('GET', 'rooms/' + room + '/participants/' + participant, undefined, function(participantRtn) {
      var p = JSON.parse(participantRtn);
      callback(p);
    }, callbackError);
  };

  /*
     * * @callback onParticipantDetail
     * * @param {Object} participantDetail           -The object containing the updated detailed info of the specified participant, same as in getParticipant.
  */
  /**
     * @function updateParticipant
     * @desc This function updates the permission of a participant in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} participant                   -Participant ID
     * @param {Array.<{op: string, path: string, value: json}>} items   -Permission item list to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {onParticipantDetail} callback         -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var participantID = 'JdlUI29yjfVY6O4yAAAB';
  OWT_REST.API.getParticipant(roomId, participantID, function(participant) {
    console.log('Participant:', participant);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var updateParticipant = function(room, participant, items, callback, callbackError) {
    if (typeof participant !== 'string' || participant.trim().length === 0) {
      return callbackError('Invalid participant ID');
    }
    if (!(items instanceof Array)) {
      return callbackError('Invalid update list');
    }
    send('PATCH', 'rooms/' + room + '/participants/' + participant, items, function(participantRtn) {
      var p = JSON.parse(participantRtn);
      callback(p);
    }, callbackError);
  };

  /**
     * @function dropParticipant
     * @desc This function drops a participant from a room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} participant                   -Participant ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var participantID = 'JdlUI29yjfVY6O4yAAAB';
  OWT_REST.API.dropParticipant(roomId, participantID, function(res) {
    console.log('Participant', participantID, 'in room', roomId, 'deleted');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var dropParticipant = function(room, participant, callback, callbackError) {
    if (typeof participant !== 'string' || participant.trim().length === 0) {
      return callbackError('Invalid participant ID');
    }
    send('DELETE', 'rooms/' + room + '/participants/' + participant, undefined, function(participant) {
      callback(participant);
    }, callbackError);
  };

  /*
     * * @callback onStreamList
     * * @param {Array.<Object>} streamList
     * * @param {Object} streamList[x]               -Object "StreamInfo" defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
  */
  /**
     * @function getStreams
     * @desc This function lists streams currently in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {onStreamList} callback                -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getStreams(roomId, function(streams) {
    var l = JSON.parse(streams);
    console.log ('This room has ', l.length, 'streams');
    for (var i in l) {
      console.log(i, ':', l[i]);
    }
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getStreams = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/streams/', undefined, function(streamsRtn) {
      var streams = JSON.parse(streamsRtn);
      callback(streams);
    }, callbackError);
  };

  /*
     * * @callback onStreamInfo
     * * @param {Object} streamInfo                  -Object "StreamInfo" defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
  */
  /**
     * @function getStream
     * @desc This function gets a stream's information from the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} stream                        -Stream ID
     * @param {onStreamInfo} callback                -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var streamID = '878889273471677';
  OWT_REST.API.getStream(roomId, streamID, function(stream) {
    console.log('Stream:', stream);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getStream = function(room, stream, callback, callbackError) {
    if (typeof stream !== 'string' || stream.trim().length === 0) {
      return callbackError('Invalid stream ID');
    }
    send('GET', 'rooms/' + room + '/streams/' + stream, undefined, function(streamRtn) {
      var st = JSON.parse(streamRtn);
      callback(st);
    }, callbackError);
  };

  /*
     * * @callback onStreamInfo
     * * @param {Object} streamInfo                  -Object "StreamInfo" defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
  */
  /**
     * @function updateStream
     * @desc This function updates a stream's given attributes in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} stream                        -Stream ID
     * @param {Array.<{op: string, path: string, value: json}>} items   -Attributes to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {onStreamInfo} callback                -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var streamID = '878889273471677';
  OWT_REST.API.updateStream(roomId, streamID, [{op: 'replace', path: '/media/audio/status', value: 'inactive'}], function(stream) {
    console.log('Stream:', stream);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var updateStream = function(room, stream, items, callback, callbackError) {
    if (typeof stream !== 'string' || stream.trim().length === 0) {
      return callbackError('Invalid stream ID');
    }
    if (!(items instanceof Array)) {
      return callbackError('Invalid update list');
    }
    send('PATCH', 'rooms/' + room + '/streams/' + stream, items, function(streamRtn) {
      var st = JSON.parse(streamRtn);
      callback(st);
    }, callbackError);
  };

  /**
     * @function deleteStream
     * @desc This function deletes the specified stream from the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} stream                        -Stream ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var streamID = '878889273471677';
  OWT_REST.API.deleteStream(roomId, streamID, function(result) {
    console.log('Stream:', streamID, 'in room:', roomId, 'deleted');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var deleteStream = function(room, stream, callback, callbackError) {
    if (typeof stream !== 'string' || stream.trim().length === 0) {
      return callbackError('Invalid stream ID');
    }
    send('DELETE', 'rooms/' + room + '/streams/' + stream, undefined, function(result) {
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onStartingStreamingInOK
     * * @param {Object} streamInfo                  -The object "StreamInfo" defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
  */
  /**
   ***
     * @function startStreamingIn
     * @desc This function adds an external RTSP/RTMP stream to the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} url                           -URL of the streaming source, e.g. the source URL of IPCamera.
     * @param {Object} transport                     -Transport parameters.
     * @param {string} transport.protocol            -Transport protocol, "tcp" or "udp", "tcp" by default.
     * @param {number} transport.bufferSize          -The buffer size in bytes in case "udp" is specified, 2048 by default.
     * @param {Object} media Media requirements.
     * @param {string='auto' | boolean}  media.video -If video is required, "auto" or true or false, "auto" by default.
     * @param {string='auto' | boolean}  media.audio -If audio is required, "auto" or true or false, "auto" by default.
     * @param {onStartingStreamingInOK} callback     -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var url = 'rtsp://10.239.44.7:554/rtsp_tunnel%3Fh26x=4%26line=1';
  var transport = {
    protocol: 'udp',
    bufferSize: 2048
  };
  var media = {
    audio: 'auto',
    video: true
  };

  OWT_REST.API.startStreamingIn(roomId, url, transport, media, function(stream) {
    console.log('Streaming-In:', stream);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var startStreamingIn = function(room, url, transport, media, callback, callbackError) {
    var pub_req = {
      connection: {
        url: url,
        transportProtocol: transport.protocol,
        bufferSize: transport.bufferSize
      },
      media: media
    };
    send('POST', 'rooms/' + room + '/streaming-ins/', pub_req, function(streamRtn) {
      var st = JSON.parse(streamRtn);
      callback(st);
    }, callbackError);
  };

  /**
     * @function stopStreamingIn
     * @desc This function stops the specified external streaming-in in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} stream                        -Stream ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var streamID = '878889273471677';
  OWT_REST.API.stopStreamingIn(roomId, streamID, function(result) {
    console.log('External streaming-in:', streamID, 'in room:', roomId, 'stopped');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var stopStreamingIn = function(room, stream, callback, callbackError) {
    if (typeof stream !== 'string' || stream.trim().length === 0) {
      return callbackError('Invalid stream ID');
    }
    send('DELETE', 'rooms/' + room + '/streaming-ins/' + stream, undefined, function(result) {
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onStreamingOutList
     * * @param {Array.<id: string, protocol: string, url: string, parameters: Object, media: Object>} streamingOutList    -The list of streaming-outs.
     * * @param {Object} streamingOutList[x].media   -The media description of the streaming-out, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function getStreamingOuts
     * @desc This function gets all the ongoing streaming-outs in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {onStreamingOutList} callback          -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getStreamingOuts(roomId, function(streamingOuts) {
    console.log('Streaming-outs:', streamingOuts);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getStreamingOuts = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/streaming-outs/', undefined, function(streamingOutList) {
      var result = JSON.parse(streamingOutList);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onStartingStreamingOutOK
     * * @param {Object} streamingOutInfo              -The object containing the information of the external streaming-out.
     * * @param {string} streamingOutInfo.id         -The streaming-out ID.
     * * @param {string} streamingOutInfo.protocol   -The streaming-out protocol.
     * * @param {string} streamingOutInfo.url        -The URL of the target streaming-out.
     * * @param {string} streamingOutInfo.parameters -The connection parameters of the target streaming-out.
     * * @param {Object} streamingOutInfo.media      -The media description of the streaming-out, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function startStreamingOut
     * @desc This function starts a streaming-out to the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {string} protocol                      -Streaming-out protocol.
     * @param {string} url                           -The URL of the target streaming-out.
     * @param {Object} parameters                    -The connection parameters of the target streaming-out.
     * @param {string} parameters.method             -The HTTP(s) method to create file on streaming servers, 'PUT' or 'POST, 'PUT' by default.
     * @param {string} parameters.hlsTime            -The hls segment length in seconds, 2 by default, required in case protocol is 'hls'.
     * @param {string} parameters.hlsListSize        -The maximum number of playlist entries, 5 by default, required in case protocol is 'hls'.
     * @param {string} parameters.dashSegDuration    -The segment length in seconds, 2 by default, required in case protocol is 'dash'.
     * @param {string} parameters.dashWindowSize     -The maximum number of segments kept in the manifest, 5 by default, required in case protocol is 'dash'.
     * @param {Object} media                         -The media description of the streaming-out, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
     * @param {onStartingStreamingOutOK} callback    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var protocol = 'hls'
  var url = 'https://USER:PASS@localhost:443/live.m3u8';
  var parameters = {
    method: 'PUT',
    hlsTime: 2,
    hlsListSize: 5
  },
  var media = {
    audio: {
      from: '7652773772543651'
    },
    video: {
      from: '7652773772543651',
      parameters: {
        keyFrameInterval: 2
      }
    }
  };
  OWT_REST.API.startStreamingOut(roomId, protocol, url, parameters, media, function(streamingOut) {
    console.log('Streaming-out:', streamingOut);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var startStreamingOut = function(room, protocol, url, parameters, media, callback, callbackError) {
    var options = {
      protocol: protocol,
      url: url,
      parameters: parameters,
      media: media
    };

    send('POST', 'rooms/' + room + '/streaming-outs/', options, function(streamingOutRtn) {
      var result = JSON.parse(streamingOutRtn);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onUpdatingStreamingOutOK
     * * @param {Object} streamingOutInfo              -The object containing the information of the updated streaming-out, same as defined in onStartingStreamingOutOk.
  */
  /**
     * @function updateStreamingOut
     * @desc This function updates a streaming-out's given attributes in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Streaming-out ID
     * @param {Array.<{op: string, path: string, value: json}>} items -Attributes to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {onUpdatingStreamingOutOk} callback    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.updateStreamingOut(roomId, id, [{op: 'replace', path: '/media/audio/from', value: '9836636255531'}], function(subscription) {
    console.log('Subscription:', subscription);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var updateStreamingOut = function(room, id, items, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid streamingOut ID');
    }
    if (!(items instanceof Array)) {
      return callbackError('Invalid update list');
    }
    send('PATCH', 'rooms/' + room + '/streaming-outs/' + id, items, function(streamingOutRtn) {
      var result = JSON.parse(streamingOutRtn);
      callback(result);
    }, callbackError);
  };

  /**
     * @function stopStreamingOut
     * @desc This function stops the specified streaming-out in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Streaming-out ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.stopStreamingOut(roomId, id, function(result) {
    console.log('Streaming-out:', id, 'in room:', roomId, 'stopped');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var stopStreamingOut = function(room, id, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid streamingOut ID');
    }
    send('DELETE', 'rooms/' + room + '/streaming-outs/' + id, undefined, function(result) {
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onRecordingList
     * * @param {Array.<{id: string, storage: Object, media: Object}>} recordingList            -The recording list.
     * * @param {Object} recordingList[x].storage       -The storage information of the recording.
     * * @param {string} recordingList[x].storage.host  -The host-name or IP address where the recording file is stored.
     * * @param {string} recordingList[x].storage.file  -The full-path name of the recording file.
     * * @param {Object} recordingList[x].media         -The media description of the recording, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function getRecordings
     * @desc This function gets the all the ongoing recordings in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getRecordings(roomId, function(recordings) {
    console.log('Recordings:', recordings);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getRecordings = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/recordings/', undefined, function(recordingList) {
      var result = JSON.parse(recordingList);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onStartingRecordingOK
     * * @param {Object} recordingInfo               -The object containing the information of the server-side recording.
     * * @param {string} recordingInfo.id            -The recording ID.
     * * @param {Object} recordingInfo.storage       -The storage information of the recording.
     * * @param {string} recordingInfo.storage.host  -The host-name or IP address where the recording file is stored.
     * * @param {string} recordingInfo.storage.file  -The full-path name of the recording file.
     * * @param {Object} recordingInfo.media         -The media description of the recording, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function startRecording
     * @desc This function starts a recording in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {string='mp4' | 'mkv' | 'auto'} container -The container type of the recording file, 'auto' by default.
     * @param {Object} media                         -The media description of the recording, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
     * @param {onStartingRecordingOK} callback       -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var container = 'mkv';
  var media = {
    audio: {
      from: '7652773772543651'
    },
    video: {
      from: '7652773772543651',
      parameters: {
        keyFrameInterval: 2
      }
    }
  };
  OWT_REST.API.startRecording(roomId, container, media, function(recording) {
    console.log('recording:', recording);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var startRecording = function(room, container, media, callback, callbackError) {
    var options = {
      container: container,
      media: media
    };

    send('POST', 'rooms/' + room + '/recordings/', options, function(recordingRtn) {
      var result = JSON.parse(recordingRtn);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onUpdatingRecordingOK
     * * @param {Object} recordingInfo               -The object containing the information of the server-side recording, same as defined in onStartingRecordingOk.
  */
  /**
     * @function updateRecording
     * @desc This function updates a recording's given attributes in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Recording ID
     * @param {Array.<{op: string, path: string, value: json}>} items -Attributes to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {onUpdatingRecordingOk} callback       -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.updateRecording(roomId, id, [{op: 'replace', path: '/media/audio/from', value: '9836636255531'}], function(subscription) {
    console.log('Subscription:', subscription);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var updateRecording = function(room, id, items, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid recording ID');
    }
    if (!(items instanceof Array)) {
      return callbackError('Invalid update list');
    }
    send('PATCH', 'rooms/' + room + '/recordings/' + id, items, function(recordingRtn) {
      var result = JSON.parse(recordingRtn);
      callback(result);
    }, callbackError);
  };

  /**
     * @function stopRecording
     * @desc This function stops the specified recording in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Recording ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.stopRecording(roomId, id, function(result) {
    console.log('Recording:', id, 'in room:', roomId, 'stopped');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var stopRecording = function(room, id, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid recording ID');
    }
    send('DELETE', 'rooms/' + room + '/recordings/' + id, undefined, function(result) {
      callback(result);
    }, callbackError);
  };


  /*
     * * @callback onSipCallList
     * * @param {Array.<{Object} SipCallInfo>} sipCallList -The sip call list, the 'SipCallInfo' is the same as defined in onSipCallOK callback parameters.
  */
  /**
     * @function getSipCalls
     * @desc This function gets the all the ongoing sip calls in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {onSipCallList} callback               -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getSipCalls(roomId, function(sipCalls) {
    console.log('SipCalls:', sipCalls);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getSipCalls = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/sipcalls/', undefined, function(sipCallList) {
      var result = JSON.parse(sipCallList);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onSipCallOK
     * * @param {Object} SipCallInfo                       -The sip call information.
     * * @param {string} SipCallInfo.id                    -Sip call ID.
     * * @param {string 'dial-in' | 'dial-out'} SipCallInfo.type      -Sip call type.
     * * @param {string} SipCallInfo.peer                  -Peer URI of the sip call.
     * * @param {Object} SipCallInfo.input                 -Object "StreamInfo" defined in section "3.3.1 Participant Joins a Room" in "Client-Portal Protocol" doc.
     * * @param {Object} SipCallInfo.output                -The subscription consumed by sip peer.
     * * @param {string} SipCallInfo.output.id             -ID of the subscription.
     * * @param {Object} SipCallInfo.output.media          -The media description of the recording, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function makeSipCall
     * @desc This function makes a SIP call to the specified peer in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {string} peerUri                       -The the peer URI to call.
     * @param {Object} mediaIn                       -The media requirements from peer sip endpoint to room.
     * @param {boolean} mediaIn.audio                -The if audio is required.
     * @param {boolean} mediaIn.video                -The if video is required.
     * @param {Object} mediaOut                      -The media description from room to peer sip endpoint, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc, with type being 'webrtc'.
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var peerUri = 'alice@company.com';
  var mediaIn = {
    audio: true,
    video: true
  };
  var mediaOut = {
    audio: {
      from: '51c10d86909ad1f939000001-common'
    },
    video: {
      from: '51c10d86909ad1f939000001-common'
      parameters: {
        resolution: {
          width: 640,
          height: 480
        }
      }
    }
  };
  OWT_REST.API.makeSipCall(roomId, peerUri, mediaIn, mediaOut, function(sipCallInfo) {
    console.log('initiate sip call OK', sipCallInfo);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var makeSipCall = function(room, peerUri, mediaIn, mediaOut, callback, callbackError) {
    var options = {
      peerURI: peerUri,
      mediaIn: mediaIn,
      mediaOut: mediaOut
    };

    send('POST', 'rooms/' + room + '/sipcalls/', options, function(sipCallInfo) {
      var result = JSON.parse(sipCallInfo);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onUpdatingSipCallOK
     * * @param {Object} SipCallInfo                 -The updated sip call information, same as defined in parameters of callback onSipCallOk.
  */
  /**
     * @function updateSipCall
     * @desc This function updates a sip call's specified output attributes in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Sip call ID
     * @param {Array.<{op: string, path: string, value: json}>} items -Attributes to be updated, with format following RFC6902(https://tools.ietf.org/html/rfc6902).
     * @param {onUpdatingSipCallOk} callback         -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.updateSipCall(roomId, id, [{op: 'replace', path: '/media/audio/from', value: '9836636255531'}], function(sipCallInfo) {
    console.log('updated sip call infor:', sipCallInfo);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var updateSipCall = function(room, id, items, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid sip call ID');
    }
    if (!(items instanceof Array)) {
      return callbackError('Invalid update list');
    }
    send('PATCH', 'rooms/' + room + '/sipcalls/' + id, items, function(sipCallInfoRtn) {
      var result = JSON.parse(sipCallInfoRtn);
      callback(result);
    }, callbackError);
  };

  /**
     * @function endSipCall
     * @desc This function ends the specified sip call in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Sip call ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.endSipCall(roomId, id, function(result) {
    console.log('Sip call:', id, 'in room:', roomId, 'ended');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var endSipCall = function(room, id, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid sip call ID');
    }
    send('DELETE', 'rooms/' + room + '/sipcalls/' + id, undefined, function(result) {
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onAnalyticsList
     * * @param {Array.<{id: string, analytics: Object, media: Object}>} analyticsList            -The analytics list.
     * * @param {Object} analyticsList[x].analytics            -The information of the analytics.
     * * @param {string} analyticsList[x].analytics.algorithm  -The algorithm of the analytics.
     * * @param {Object} analyticsList[x].media                -The media description of the analytics, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function getAnalytics
     * @desc This function gets the all the ongoing analytics in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  OWT_REST.API.getAnalytics(roomId, function(analyticsList) {
    console.log('Analytics:', analyticsList);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var getAnalytics = function(room, callback, callbackError) {
    send('GET', 'rooms/' + room + '/analytics/', undefined, function(analyticsList) {
      var result = JSON.parse(analyticsList);
      callback(result);
    }, callbackError);
  };

  /*
     * * @callback onStartingAnalyticsOK
     * * @param {Object} analyticsInfo               -The object containing the information of the server-side analytics.
     * * @param {string} analyticsInfo.id            -The analytics ID.
     * * @param {Object} analyticsInfo.analytics     -The information of the analytics.
     * * @param {string} analyticsInfo.analytics.algorithm  -The algorithm of the analytics.
     * * @param {Object} analyticsInfo.media         -The media description of the analytics, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
  */
  /**
     * @function startAnalytics
     * @desc This function starts a analytics in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID.
     * @param {string} algorithm                     -The algorithm ID.
     * @param {Object} media                         -The media description of the analytics, which must follow the definition of object "MediaSubOptions" in section "3.3.11 Participant Starts a Subscription" in "Client-Portal Protocol.md" doc.
     * @param {onStartingAnalyticsOK} callback       -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var algorithm = 'b849f44bee074b08bf3e627f3fc927c7'; //guid in plugin.cfg
  var media = {
    audio: {
      from: '7652773772543651'
    },
    video: {
      from: '7652773772543651',
      parameters: {
        keyFrameInterval: 2
      }
    }
  };
  OWT_REST.API.startAnalytics(roomId, algorithm, media, function(analytics) {
    console.log('analytics:', analytics);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var startAnalytics = function(room, algorithm, media, callback, callbackError) {
    var options = {
      algorithm: algorithm,
      media: media
    };

    send('POST', 'rooms/' + room + '/analytics/', options, function(analyticsRtn) {
      var result = JSON.parse(analyticsRtn);
      callback(result);
    }, callbackError);
  };

  /**
     * @function stopAnalytics
     * @desc This function stops the specified analytics in the specified room.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} id                            -Analytics ID
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var id = '878889273471677';
  OWT_REST.API.stopAnalytics(roomId, id, function(result) {
    console.log('Analytics:', id, 'in room:', roomId, 'stopped');
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var stopAnalytics = function(room, id, callback, callbackError) {
    if (typeof id !== 'string' || id.trim().length === 0) {
      return callbackError('Invalid analytics ID');
    }
    send('DELETE', 'rooms/' + room + '/analytics/' + id, undefined, function(result) {
      callback(result);
    }, callbackError);
  };

  /**
     * @function createToken
     * @desc This function creates a new token when a new participant to a room needs to be added.
     * @memberOf OWT_REST.API
     * @param {string} room                          -Room ID
     * @param {string} user                          -Participant's user ID
     * @param {string} role                          -Participant's role
     * @param {object} preference                    -Preference of this token would be used to connect through
     * @param {function} callback                    -Callback function on success
     * @param {function} callbackError               -Callback function on error
     * @example
  var roomId = '51c10d86909ad1f939000001';
  var user = 'user-id@company.com';
  var role = 'guest';
  // Only isp and region are supported in preference currently, please see server's document for details.
  var preference = {isp: 'isp', region: 'region'};
  OWT_REST.API.createToken(roomId, user, role, preference, function(token) {
    console.log ('Token created:' token);
  }, function(status, error) {
    // HTTP status and error
    console.log(status, error);
  });
     */
  var createToken = function(room, user, role, preference, callback, callbackError) {
    if (typeof room !== 'string' || typeof user !== 'string' || typeof role !== 'string') {
      if (typeof callbackError === 'function')
        callbackError(400, 'Invalid argument.');
      return;
    }
    send('POST', 'rooms/' + room + '/tokens/', {preference: preference, user: user, role: role}, callback, callbackError);
  };

  return {
    init: init,

    //Room management.
    createRoom: createRoom,
    getRooms: getRooms,
    getRoom: getRoom,
    updateRoom: updateRoom,
    updateRoomPartially: updateRoomPartially,
    deleteRoom: deleteRoom,

    //Participants management.
    getParticipants: getParticipants,
    getParticipant: getParticipant,
    updateParticipant: updateParticipant,
    dropParticipant: dropParticipant,

    //Streams management.
    getStreams: getStreams,
    getStream: getStream,
    updateStream: updateStream,
    deleteStream: deleteStream,

    //Streaming-ins management.
    startStreamingIn: startStreamingIn,
    stopStreamingIn: stopStreamingIn,

    //Streaming-outs management
    getStreamingOuts: getStreamingOuts,
    startStreamingOut: startStreamingOut,
    updateStreamingOut: updateStreamingOut,
    stopStreamingOut: stopStreamingOut,

    //Server-side recordings management
    getRecordings: getRecordings,
    startRecording: startRecording,
    updateRecording: updateRecording,
    stopRecording: stopRecording,

    //Analytics management
    getAnalytics: getAnalytics,
    startAnalytics: startAnalytics,
    stopAnalytics: stopAnalytics,

    //Sip calls management
    getSipCalls: getSipCalls,
    makeSipCall: makeSipCall,
    updateSipCall: updateSipCall,
    endSipCall: endSipCall,

    //Tokens management.
    createToken: createToken
  };
}(OWT_REST));
module.exports = OWT_REST;