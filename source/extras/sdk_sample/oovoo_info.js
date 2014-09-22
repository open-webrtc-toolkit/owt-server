var url = require("url");
var request = require('request');
var http_proxy = process.env.http_proxy;
var https_proxy = process.env.https_proxy;

var Header = {
  "Accept": "application/json",
  "HOST": "social.oovoo.com",
  "User-Agent": "Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.107 Safari/537.36"
};

function factory(proto) {
  if (proto !== 'https') {
    proto = 'http';
  }
  var proxy = proto === 'http' ? http_proxy : https_proxy;
  var base = proto+'://social.oovoo.com/webrtc/inteldemo/confinfo/';
  var Fn = function (Id, callback) {
    if (Id === '' || Id === undefined || Id === null) {
      callback(404, {});
      return;
    }
    var target = base+Id;
    var option = {
      url: target,
      headers: Header
    };
    if (proxy) {
      option.proxy = proxy;
    }
    request(option, function(err, resp, body) {
      if (typeof body === 'string') {
        callback(resp.statusCode, JSON.parse(body));
      } else {
        callback(resp.statusCode, body);
      }
    });
  };
  return Fn;
}

module.exports = {
  http: factory('http'),
  https: factory('https')
};
