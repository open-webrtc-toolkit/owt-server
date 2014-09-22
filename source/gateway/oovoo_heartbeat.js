(function(){
  var request = require('request');
  var config;

  function mkConn(opt) {
    return {
      uri: opt.backend,
      rejectUnauthorized: false,
      json: {
        "gw_key": config.gw_key,
        "keep-alive": {
          "ip": opt.ip,
          "type": "intel",
          "clients": opt.connections,
          "active": config.active || "true",
          "version": config.version || "unknown",
          "report_date": new Date().toISOString().replace(/T/, ' ').slice(0, 19)
        }
      },
      proxy: opt.proxy
    };
  }

  var that = {
    init: function (configuraton) {
      config = configuraton;
      return that;
    },
    start: function (connections, callback, callbackError) {
      var interval = (config.interval || 12) * 1000;
      return setInterval(function () {
        var opt = {
          ip: config.ip,
          backend: config.backend || 'https://api-sdk.oovoo.com/gw/1.0/iamalive',
          connections: Object.keys(connections).length+''
        };
        if (config.useProxy === true) {
          if (opt.backend.indexOf('https') === 0) {
            opt.proxy = process.env.https_proxy || process.env.http_proxy;
          } else {
            opt.proxy = process.env.http_proxy;
          }
        }
        request.post(mkConn(opt), function(err, resp, body) {
          if (err) {
            if (typeof callbackError === 'function') {
              callbackError(err);
            }
            return;
          }
          if (typeof callback === 'function') {
            callback(resp.statusCode, body);
          }
        });
      }, interval);
    }
  };
  exports = module.exports = that;
}());
