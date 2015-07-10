#!/usr/bin/env node

'use strict';
var WoogeenNodeConference = (function () {
  function safeCall () {
    var callback = arguments[0];
    if (typeof callback === 'function') {
      var args = Array.prototype.slice.call(arguments, 1);
      callback.apply(null, args);
    }
  }
  var DISCONNECTED = 0, CONNECTING = 1, CONNECTED = 2;
  function WoogeenNodeConference (spec) {
    var that = {};
    that.state = DISCONNECTED;

    this.join = function join (token, callback) {
      var self = this;
      if (that.state !== DISCONNECTED) {
        return safeCall(callback, 'connection state invalid');
      }
      try {
        token = JSON.parse(new Buffer(token, 'base64').toString('ascii'));
      } catch (e) {
        return safeCall(callback, 'token error', e);
      }
      that.state = CONNECTING;
      self.socket = require('socket.io-client').connect(spec.host, {
        reconnect: false,
        secure: spec.secure || false,
        'force new connection': true
      });
      self.socket.on('onAddStream', function (spec) {
        console.log('stream added:', spec);
      });
      self.socket.on('onRemoveStream', function (spec) {
        console.log('stream removed:', spec);
      });
      self.socket.on('disconnect', function () {
        if (that.state !== DISCONNECTED) {
          self.socket.disconnect();
        }
      });
      self.socket.on('onUserJoin', function (spec) {
        console.log('user joined:', spec);
      });
      self.socket.on('onUserLeave', function (spec) {
        console.log('user left:', spec);
      });
      self.socket.on('connect_failed', function (err) {
        safeCall(callback, err || 'connection_failed');
        process.exit();
      });
      self.socket.on('error', function (err) {
        safeCall(callback, err || 'connection_error');
        process.exit();
      });
      try {
        self.socket.emit('token', token, function (status, resp) {
          if (status === 'success') {
            self.connSettings = {
              turn: resp.turnServer,
              stun: resp.stunServerUrl,
              defaultVideoBW: resp.defaultVideoBW,
              maxVideoBW: resp.maxVideoBW
            };
            self.myId = resp.clientId;
            self.conferenceId = resp.id;
            self.p2p = resp.p2p;
            that.state = CONNECTED;

            self.publish = function publish (url, callback) {
              try {
                self.socket.emit('publish', {state: 'url', audio: true, video: true}, url, function (status, resp) {
                  if (status !== 'success') {
                    return safeCall(callback, status, resp);
                  }
                  safeCall(callback, null, resp);
                });
              } catch (e) {
                return safeCall(callback, 'socket error', e);
              }
            };

            self.unpublish = function unpublish (id, callback) {
              try {
                self.socket.emit('unpublish', id, function (resp, mesg) {
                  if (resp === 'success') {
                    return safeCall(callback, null, mesg);
                  }
                  return safeCall(callback, mesg||'response error');
                });
              } catch (err) {
                callback('socket emit error');
              }
            };
            /*
            options: {
              streamId: xxxxxx,
              recorderId: yyyyyy
            }
            */
            self.startRecorder = function startRecorder (options, callback) {
              if (typeof options === 'function') {
                callback = options;
                options = {};
              } else if (typeof options !== 'object' || options === null) {
                options = {};
              }
              try {
                self.socket.emit('startRecorder', options, function (resp, mesg) {
                  if (resp === 'success') {
                    return safeCall(callback, null, mesg);
                  }
                  return safeCall(callback, mesg||'response error');
                });
              } catch (err) {
                callback('socket emit error');
              }
            };
            /*
            options: {
              recorderId: xxxxxx
            }
            */
            self.stopRecorder = function stopRecorder (options, callback) {
              if (typeof options === 'function') {
                callback = options;
                options = {};
              } else if (typeof options !== 'object' || options === null) {
                options = {};
              }
              try {
                self.socket.emit('stopRecorder', options, function (resp, mesg) {
                  if (resp === 'success') {
                    return safeCall(callback, null, mesg);
                  }
                  return safeCall(callback, mesg||'response error');
                });
              } catch (err) {
                callback('socket emit error');
              }
            };

            return safeCall(callback, null, {streams: resp.streams});
          }
          return safeCall(callback, resp||'response error');
        });
      } catch (e) {
        safeCall(callback, 'socket emit error');
      }
    };
    this.leave = function leave () {
      if (that.state === DISCONNECTED) return;
      that.state = DISCONNECTED;
      try {
        this.socket.disconnect();
      } catch (err) {}
    };
  }
  return WoogeenNodeConference;
}());


var client = new WoogeenNodeConference({
  host: 'http://localhost:8080'
});

var nuve = require('./basic_example/nuve').API;
nuve.init('_auto_generated_ID_', '_auto_generated_KEY_', 'http://localhost:3000/');

nuve.getRooms(function (resp) {
  var rooms = JSON.parse(resp);
  var room = rooms[0]._id;
  console.log('get', rooms.length, 'rooms; using room:', room);
  nuve.createToken(room, 'rtsp_user', 'presenter', function (token) {
    client.join(token, function (err, resp) {
      if (err) {
        console.log('error in joining room:', err);
        return process.exit();
      }
      console.log('room joined:', resp);
      client.publish('rtsp://10.239.61.102:554/rtsp_tunnel', function (err, resp) {
        if (err) {
          console.log('error in publishing stream:', err);
          return process.exit();
        }
        console.log('stream published:', resp);
        setTimeout(function () {
          console.log('try unpublishing...');
          client.unpublish(resp, function (err) {
            if (err) {
              console.log('error in unpublishing stream:', err);
              return process.exit();
            }
            console.log('stream unpublished');
            client.leave();
          });
        }, 10000);
      });
    });
  }, function (err) {
    console.log('error in retrieving token:', err);
    return process.exit();
  });
});
