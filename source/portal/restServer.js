/* global require */
'use strict';

var path = require('path');
var url = require('url');
var log = require('./logger').logger.getLogger('RestServer');

//FIXME: to keep compatible to previous MCU, should be removed later.
var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720}
};

var resolutionValue2Name = {
    'r352x288': 'cif',
    'r640x480': 'vga',
    'r800x600': 'svga',
    'r1024x768': 'xga',
    'r640x360': 'r640x360',
    'r1280x720': 'hd720p',
    'r320x240': 'sif',
    'r480x320': 'hvga',
    'r480x360': 'r480x360',
    'r176x144': 'qcif',
    'r192x144': 'r192x144',
    'r1920x1080': 'hd1080p',
    'r3840x2160': 'uhd_4k',
    'r360x360': 'r360x360',
    'r480x480': 'r480x480',
    'r720x720': 'r720x720'
};

function widthHeight2Resolution(width, height) {
  var k = 'r' + width + 'x' + height;
  return resolutionValue2Name[k] ? resolutionValue2Name[k] : k;
}

function resolution2WidthHeight(resolution) {
  var r = resolution.indexOf('r'),
    x = resolution.indexOf('x'),
    w = (r === 0 && x > 1 ? Number(resolution.substring(1, x)) : 640),
    h = (r === 0 && x > 1 ? Number(resolution.substring(x, resolution.length)) : 480);
  return resolutionName2Value[resolution] ? resolutionName2Value[resolution] : {width: w, height: h};
}

var idPattern = /^[0-9a-zA-Z]+$/;
function isValidIdString(str) {
  return (typeof str === 'string') && idPattern.test(str);
}

var Client = function(clientId, inRoom, queryInterval, portal, on_loss) {
  var that = {inRoom: inRoom};
  var client_id = clientId;

  var absent_count = 0,
      check_alive_interval,
      published = {},
      subscribed = {},
      notifications = [];

  that.query = function(on_result) {
    absent_count = 0;
    on_result({published: published, subscribed: subscribed, notifications: notifications});
    for (var stream_id in published) {
      published[stream_id] = [];
    }
    for (var subscription_id in subscribed) {
      subscribed[subscription_id] = [];
    }
    notifications = [];
  };

  that.publish = function(type, options, on_ok, on_failure, on_error) {
    var connection_type, stream_id = Math.round(Math.random() * 1000000000000000000) + '';
    var stream_description = {};

    if (type === 'webrtc') {
      connection_type = 'webrtc';
    } else if (type === 'url') {
      connection_type = 'avstream';
      stream_description.url = options.url;
      stream_description.transport = options.transport;
      stream_description.bufferSize = options.bufferSize;
    } else {
      return on_failure('stream type error.');
    }

    stream_description.audio = (options.audio === undefined ? true : !!options.audio);
    stream_description.video = (options.video === false ? false : (typeof options.video === 'object' ? options.video : {}));
    ((stream_description.video && options.video && options.video.resolution && (typeof options.video.resolution.width === 'number') && (typeof options.video.resolution.height === 'number')) && (stream_description.video.resolution = widthHeight2Resolution(options.video.resolution.width, options.video.resolution.height))) ||
    (stream_description.video && (typeof stream_description.video.resolution !== 'string' || stream_description.video.resolution === '') && (stream_description.video.resolution = 'unknown'));
    stream_description.video && (typeof stream_description.video.device !== 'string' || stream_description.video.device === '') && (stream_description.video.device = 'unknown');
    var unmix = (options.unmix === true || (stream_description.video && (stream_description.video.device === 'screen'))) ? true : false;

    var once_ok = false;
    return portal.publish(clientId, stream_id, connection_type, stream_description, function(status) {
      if (status.type === 'failed') {
        published[stream_id] && (delete published[stream_id]);
        once_ok ? on_error(status.reason) : on_failure(status.reason);
      } else {
        published[stream_id] === undefined && (published[stream_id] = []);
        published[stream_id].push(status);
      }
    }, unmix).then(function(connectionLocality) {
      log.debug('portal.publish succeeded, connection locality:', connectionLocality);
      published[stream_id] === undefined && (published[stream_id] = []);
      on_ok(stream_id);
      once_ok = true;
    }).catch(function(err) {
      var err_message = (typeof err === 'string' ? err: err.message);
      log.info('portal.publish failed:', err_message);
      published[stream_id] && (delete published[stream_id]);
      on_failure(err_message);
    });
  };

  that.unpublish = function(streamId, on_ok, on_failure) {
    return portal.unpublish(clientId, streamId)
    .then(function() {
      delete published[streamId];
      on_ok();
    }).catch(function(err) {
      var err_message = (typeof err === 'string' ? err: err.message);
      log.info('portal.unpublish failed:', err_message);
      delete published[streamId];
      on_failure(err_message);
    });
  };

  that.updateStream = function(streamId, message) {
    if (message.type === 'control') {
      if (message.control) {
        if (message.control.attr === 'region-in-mix') {
          return portal.setRegion(clientId, streamId, message.control.value);
        } else if (message.control.attr === 'audio-on-off' || message.control.attr === 'video-on-off') {
          var track = (message.control.attr.startsWith('audio') ? 'audio' : 'video');
          var on_off = (message.control.value === 'on' ? 'on': 'off');
          return portal.mediaOnOff(clientId, streamId, track, 'in', on_off);
        } else if (message.control.attr === 'mix') {
          return message.control.value === 'yes' ? portal.mix(clientId, streamId) : portal.unmix(clientId, streamId);
        }
        return Promise.reject('unknown attr to control');
      }
      return Promise.reject('control data empty');
    }
    return Promise.reject('unknown updating type');
  };

  that.subscribe = function(type, options, on_ok, on_failure, on_error) {
    var connection_type, subscription_id;
    var subscription_description = {};

    if (type === 'webrtc') {
      connection_type = 'webrtc';
    } else if (type === 'url') {
      connection_type = 'avstream';

      if (typeof options.url !== 'string' || options.url === '') {
        return on_failure('Invalid RTSP/RTMP server url');
      }

      var parsed_url = url.parse(options.url);
      if ((parsed_url.protocol !== 'rtsp:' && parsed_url.protocol !== 'rtmp:' && parsed_url.protocol !== 'http:') || !parsed_url.slashes || !parsed_url.host) {
        return on_failure('Invalid RTSP/RTMP server url');
      }
      subscription_description.url = parsed_url.format();
    } else {
      return on_failure('subscription type error.');
    }

    if (options.audio === undefined || options.audio === true) {
      subscription_description.audio = {fromStream: that.inRoom/*FIXME: should be 'unspecified' here and let the session controller to determine*/, codecs: ['aac']};
    } else if (typeof options.audio === 'object') {
      subscription_description.audio = {};
      if (options.audio.from === undefined) {
        subscription_description.audio.fromStream = that.inRoom;
      } else if (typeof options.audio.from === 'string' && options.audio.from !== '') {
        subscription_description.audio.fromStream = options.audio.from;
      } else {
        return on_failure('Bad request: options.audio.from');
      }

      if (options.audio.codecs === undefined) {
        subscription_description.audio.codecs = ['aac'];
      } else if ((options.audio.codecs instanceof Array) && options.audio.codecs.length > 0) {
        subscription_description.audio.codecs = options.audio.codecs;
      } else {
        return on_failure('Bad request: options.audio.codecs');
      }
    } else {
      return on_failure('Bad request: options.audio');
    }

    if (options.video === undefined || options.video === true) {
      subscription_description.video = {fromStream: that.inRoom, codecs: ['h264']};
    } else if (typeof options.video === 'object') {
      subscription_description.video = {};
      if (options.video.from === undefined) {
        subscription_description.video.fromStream = that.inRoom;
      } else if (typeof options.video.from === 'string' && options.video.from !== '') {
        subscription_description.video.fromStream = options.video.from;
      } else {
        return on_failure('Bad request: options.video.from');
      }

      if (options.video.codecs === undefined) {
        subscription_description.video.codecs = ['h264'];
      } else if ((options.video.codecs instanceof Array) && options.video.codecs.length > 0) {
        subscription_description.video.codecs = options.video.codecs;
      } else {
        return on_failure('Bad request: options.video.codecs');
      }

      if (options.video.resolution === undefined) {
        subscription_description.video.resolution = undefined;
      } else if ((typeof options.video.resolution.width === 'number') && (typeof options.video.resolution.height === 'number')) {
        subscription_description.video.resolution = widthHeight2Resolution(options.video.resolution.width, options.video.resolution.height);
      } else {
        return on_failure('Bad request: options.video.resolution');
      }
    } else {
      return on_failure('Bad request: options.audio');
    }

    var subscription_id = Math.round(Math.random() * 1000000000000000000) + '';

    var once_ok = false;
    return portal.subscribe(clientId, subscription_id, connection_type, subscription_description, function(status) {
      if (status.type === 'failed') {
        subscribed[subscription_id] && (delete subscribed[subscription_id]);
        once_ok ? on_error(status.reason) : on_failure(status.reason);
      } else {
        subscribed[subscription_id] === undefined && (subscribed[subscription_id] = []);
        subscribed[subscription_id].push(status);
      }
    }).then(function(connectionLocality) {
      log.debug('portal.subscribe succeeded, connection locality:', connectionLocality);
      subscribed[subscription_id] === undefined && (subscribed[subscription_id] = []);
      on_ok(subscription_id);
      once_ok = true;
    }).catch(function(err) {
      var err_message = (typeof err === 'string' ? err: err.message);
      log.info('portal.subscribe failed:', err_message);
      subscribed[subscription_id] && (delete subscribed[subscription_id]);
      on_failure(err_message);
    });
  };

  that.unsubscribe = function(subscriptionId, on_ok, on_failure) {
    return portal.unsubscribe(clientId, subscriptionId)
    .then(function() {
      delete subscribed[subscriptionId];
      on_ok();
    }).catch(function(err) {
      var err_message = (typeof err === 'string' ? err: err.message);
      log.info('portal.unsubscribe failed:', err_message);
      delete subscribed[subscriptionId];
      on_failure(err_message);
    });
  };

  that.notify = function(event, data) {
    log.debug('socket.emit, event:', event, 'data:', data);
    notifications.push({event: event, data: data});
  };

  that.drop = function() {
    on_loss();
  };

  that.check_alive_interval = setInterval(function() {
    absent_count += 1;
    if (absent_count > 3) {
      clearInterval(that.check_alive_interval);
      on_loss();
    }
  }, queryInterval);
  return that;
};

var RestServer = function(spec, portal, observer) {
  var that = {};
  var express = require('express');
  var bodyParser = require('body-parser');
  var app = express();
  var server;
  var clients = {};

  var clientJoin = function(req, res) {
    var client_id = Math.round(Math.random() * 100000000000000000) + '',
        token = req.body.token,
        query_interval = req.body.queryInterval;

    return portal.join(client_id, token)
      .then(function(result) {
        observer.onJoin(result.tokenCode);
        var commonViewStream = result.streams.filter((st) => (st.view === 'common'))[0];
        clients[client_id] = new Client(client_id, commonViewStream, query_interval, portal, function() {
          portal.leave(client_id);
          delete clients[client_id];
          observer.onLeave(result.tokenCode);
        });
        var joinResult = {
          id: client_id,
          streams: result.streams.map(function(st) {
            st.video && (st.video.resolutions instanceof Array) && (st.video.resolutions = st.video.resolutions.map(resolution2WidthHeight));
            return st;
          }),
          clients: result.participants};
        res.send(joinResult);
      }, function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        res.status(404).send({reason: err_message});
      });
  };

  var checkClientExistence = function(req, res, next) {
    var client_id = req.params.client;
    if (clients[client_id]) {
      next();
    } else {
      res.status(404).send({reason: 'Client absent'});
    }
  };

  var clientQuery = function(req, res) {
    var client_id = req.params.client;
    clients[client_id].query(function(result) {
      res.send(result);
    });
  };

  var clientLeave = function(req, res) {
    var client_id = req.params.client;
    clients[client_id].drop();
    res.send();
  };

  var publish = function(req, res) {
    var client_id = req.params.client;
    clients[client_id].publish(req.body.type, req.body.options, function(streamId) {
      res.send({id: streamId});
    }, function(reason) {
      log.debug('publish failed:', reason);
      res.status(404).send({reason: reason});
    }, function(reason) {
      log.debug('publish error:', reason);
    });
  };

  var unpublish = function(req, res) {
    var client_id = req.params.client,
        stream_id = req.params.streamId;
    clients[client_id].unpublish(stream_id, function() {
      res.send();
    }, function(reason) {
      log.debug('unpublish failed:', reason);
      res.status(404).send({reason: reason});
    });
  };

  var updateStream = function(req, res) {
    var client_id = req.params.client,
        stream_id = req.params.streamId;
    return clients[client_id].updateStream(stream_id, req.body)
      .then(function() {
        res.send();
      }, function(err) {
        var err_message = (typeof err === 'string' ? err: err.message);
        log.debug('updating stream failed:', err_message);
        res.status(404).send({reason: err_message});
      });
  };

  var subscribe = function(req, res) {
    var client_id = req.params.client;
    clients[client_id].subscribe(req.body.type, req.body.options, function(subscriptionId) {
      res.send({id: subscriptionId});
    }, function(reason) {
      log.debug('subscribe failed:', reason);
      res.status(404).send({reason: reason});
    }, function(reason) {
      log.debug('subscribe error:', reason);
    });
  };

  var unsubscribe = function(req, res) {
    var client_id = req.params.client,
        subscription_id = req.params.subscriptionId;
    clients[client_id].unsubscribe(subscription_id, function() {
      res.send();
    }, function(reason) {
      log.debug('unsubscribe failed:', reason);
      res.status(404).send({reason: reason});
    });
  };

  var setup = function() {
    app.use(bodyParser.json());
    app.use(bodyParser.urlencoded({
        extended: true
    }));

    app.set('view options', {
        layout: false
    });

    app.use(function (req, res, next) {
        res.header('Access-Control-Allow-Origin', '*');
        res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE');
        res.header('Access-Control-Allow-Headers', 'origin, content-type');
        next();
    });
    app.options('*', function(req, res) {
        res.send(200);
    });

    app.post('/clients', clientJoin);
    app.use('/clients/:client', checkClientExistence);
    app.get('/clients/:client', clientQuery);
    app.delete('/clients/:client', clientLeave);

    app.use('/pub/:client*', checkClientExistence);
    app.post('/pub/:client', publish);
    app.delete('/pub/:client/:streamId', unpublish);
    app.put('/pub/:client/:streamId', updateStream);

    app.use('/sub/:client*', checkClientExistence);
    app.post('/sub/:client', subscribe);
    app.delete('/sub/:client/:subscriptionId', unsubscribe);
  };

  var startInsecure = function(port) {
    server = app.listen(port);
    return Promise.resolve('ok');
  };

  var startSecured = function(port, keystorePath) {
    return new Promise(function(resolve, reject) {
      var cipher = require('./cipher');
      var keystore = path.resolve(path.dirname(keystorePath), '.woogeen.keystore');
      cipher.unlock(cipher.k, keystore, function(err, passphrase) {
        if (!err) {
          try {
            server = require('https').createServer({pfx: require('fs').readFileSync(keystorePath), passphrase: passphrase}, app).listen(port);
            resolve('ok');
          } catch (e) {
            err = e;
            reject(typeof err === 'string' ? err : err.message);
          }
        } else {
          log.warn('Failed to setup secured server:', err);
          reject(err);
        }
      });
    });
  };

  that.start = function() {
    if (!spec.ssl) {
      return startInsecure(spec.port);
    } else {
      return startSecured(spec.port, spec.keystorePath);
    }
  };

  that.stop = function() {
    clients = {};
    server && server.close();
    server = undefined;
  };

  that.notify = function(clientId, event, data) {
    log.debug('notify participant:', clientId, 'event:', event, 'data:', data);
    if (clients[clientId]) {
      clients[clientId].notify(event, data);
      return Promise.resolve('ok');
    } else {
      return Promise.reject('participant does not exist');
    }
  };

  that.drop = function(clientId, fromRoom) {
    if (clientId === 'all') {
      for(var cid in clients) {
        clients[cid].drop();
      }
      return Promise.resolve('ok');
    } else if (clients[clientId] && (fromRoom === undefined || clients[clientId].inRoom === fromRoom)) {
      clients[clientId].drop();
      return Promise.resolve('ok');
    } else {
      return Promise.reject('user not in room');
    }
  };

  setup();
  return that;
};

module.exports = RestServer;

