/*
CryptoJS v3.0.2
code.google.com/p/crypto-js
(c) 2009-2012 by Jeff Mott. All rights reserved.
code.google.com/p/crypto-js/wiki/License
*/
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
  
  /*global unescape*/

  var ICS_REST = ICS_REST || {};
  ICS_REST.Base64 = (function(ICS_REST) {
    "use strict";
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
  }(ICS_REST));
  ICS_REST.API = (function(ICS_REST) {

    'use strict';
    var version = 'v1';
    var params = {
      service: undefined,
      key: undefined,
      url: undefined,
      rejectUnauthorizedCert: undefined
    };
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

  function calculateSignature (toSign, key) {
    var hash, hex, signed;
    hash = CryptoJS.HmacSHA256(toSign, key);
    hex = hash.toString(CryptoJS.enc.Hex);
    signed = ICS_REST.Base64.encodeBase64(hex);
    return signed;
  };


  function sendHeader(method, resource, body, onOK, onError) {
    if (!params.service) {
      if (typeof onError === 'function') {
        onError(401, 'ICS REST API is not initialized!!');
      }
      return;
    }

    var timestamp = new Date().getTime();
    var cnounce = require('crypto').randomBytes(8).toString('hex');

    var toSign = timestamp + ',' + cnounce;
    var header = 'MAuth realm=http://marte3.dit.upm.es,mauth_signature_method=HMAC_SHA256';

    var signed = calculateSignature(toSign, params.key);

    header += ',mauth_serviceid=';
    header += params.service;
    header += ',mauth_cnonce=';
    header += cnounce;
    header += ',mauth_timestamp=';
    header += timestamp;
    header += ',mauth_signature=';
    header += signed;
    console.log("header : ",header);
    return header;}



  return {
    init: init,
    sendHeader:sendHeader,
  };
}(ICS_REST));
module.exports = ICS_REST;
 
