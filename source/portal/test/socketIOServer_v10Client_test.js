// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var socketIOServer = require('../socketIOServer');
var sioClient = require('socket.io-client');
var portal = require('../portal');

const testRoom = '573eab78111478bb3526421a';
const testStream = '573eab78111478bb3526421a-common';

const jsClientInfo = {sdk:{version: '4.0', type: 'JavaScript'}, runtime: {name: 'Chrome', version: '60.0.0.0'}, os:{name:'Linux (Ubuntu)', version:'14.04'}};
const iosClientInfo = {sdk:{version: '4.0', type: 'Objective-C'}, runtime: {name: 'WebRTC', version: '54'}, os:{name:'iPhone OS', version:'10.2'}};
const jsLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: jsClientInfo, protocol: '1.0'};
var testServerPort = 3100;

const insecureSocketIOServerConfig = () => {
  return {
    port: testServerPort,
    ssl: false,
    reconnectionTicketLifetime: 100,
    reconnectionTimeout: 300
  };
};

const presenter_join_result = {
  tokenCode: 'tokenCode',
  data: {
    role: 'presenter',
    user: 'user-id',
    permission: {
      publish: {
        audio: true,
        video: true
      },
      subscribe: {
        audio: true,
        video: true
      }
    },
    room: {
      id: testRoom,
      views: ['common'],
      streams: [{
          id: testRoom + '-common',
          type: 'mixed',
          media: {
            audio: {
              format: {
                codec: 'opus',
                sampleRate: 48000,
                channelNum: 2
              },
              optional: {
                format: [{
                    codec: 'pcmu',
                    sampleRate: 8000,
                    channelNum: 1
                  }, {
                    codec: 'pcma',
                    sampleRate: 8000,
                    channelNum: 1
                  }, {
                    codec: 'aac',
                    sampleRate: 44100,
                    channelNum: 1
                  }
                ]
              }
            },
            video: {
              format: {
                codec: 'h264',
                profile: 'constrained-baseline'
              },
              parameters: {
                resolution: {
                  width: 1280,
                  height: 720
                },
                frameRate: 30,
                bitrate: 2,
                keyFrameInterval: 100
              },
              optional: {
                format: [{
                    codec: 'h264',
                    profile: 'constrained-baseline'
                  }, {
                    codec: 'vp8'
                  }, {
                    codec: 'vp9'
                  }, {
                    codec: 'h265'
                  }
                ],
                parameters: {
                  resolution: [{
                      width: 640, height: 480
                    }, {
                      width: 320, height: 240
                    }
                  ],
                  framerate: [5, 15, 24, 48, 60],
                  bitrate: [3.2, 2.8, 2.4, 1.6, 1.2, 0.8],
                  keyFrameInterval: [30, 5, 2, 1]
                }
              }
            }
          },
          info: {
            label: 'common',
            layout: [{
                stream: 'forward-stream-id',
                region: {
                  id: '1',
                  shape: 'rectangle',
                  area: {
                    left: 0.0,
                    top: 0.0,
                    width: 1.0,
                    height: 1.0
                  }
                }
              }
            ]
          }
        }, {
          id: 'forward-stream-id',
          type: 'forward',
          media: {
            audio: {
              source: 'mic',
              status: 'active',
              format:{
                codec: 'opus',
                sampleRate: 48000,
                channelNum: 2
              },
              optional: {
                format: [{
                    codec: 'pcmu',
                    sampleRate: 8000,
                    channelNum: 1
                  }, {
                    codec: 'pcma',
                    sampleRate: 8000,
                    channelNum: 1
                  }, {
                    codec: 'aac',
                    sampleRate: 44100,
                    channelNum: 1
                  }
                ]
              }
            },
            video: {
              source: 'camera',
              status: 'active',
              format:{
                codec: 'h264',
                profile: 'constrained-baseline'
              },
              optional: {
                format: [{
                    codec: 'h264',
                    profile: 'constrained-baseline'
                  }, {
                    codec: 'vp8'
                  }, {
                    codec: 'vp9'
                  }, {
                    codec: 'h265'
                  }
                ]
              }
            }
          },
          info: {
            owner: 'participant-id',
            type: 'webrtc',
            attributes: {
              someKey: 'someValue'
            }
          }
        }
      ],
      participants: [{
          id: 'participant-id',
          role: 'presenter',
          user: 'user-id-2'
        }
      ]
    }
  }
};

describe('Clients connect to socket.io server.', function() {
  it('Connecting to an insecure socket.io server should succeed.', function(done) {
    var server = socketIOServer({port: 3001, ssl: false, reconnectionTicketLifetime: 100, reconnectionTimeout: 300});

    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');

        var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});
        client.on('connect', function() {
          expect(true).to.be.true;
          server.stop();
          done();
        });
      })
      .catch(function(err) {
        expect(err).to.be.false;
        done();
      });
  });

  it('Connecting to a secured socket.io server should succeed.', function(done) {
    var server = socketIOServer({port: 3005, ssl: true, keystorePath: './cert/certificate.pfx'});

    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');

        var https = require('https');
        https.globalAgent.options.rejectUnauthorized = false;
        var client = sioClient.connect('https://localhost:3005', {reconnect: false, secure: true, 'force new connection': true, agent: https.globalAgent});
        client.on('connect', function() {
          expect(true).to.be.true;
          server.stop();
          done();
        });
      })
      .catch(function(err) {
        expect(err).to.be.false;
        done();
      });
  });
});

describe('Logining and Relogining.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var mockServiceObserver = {onJoin: sinon.spy(), onLeave: sinon.spy()};
  var server = socketIOServer(insecureSocketIOServerConfig(), mockPortal, mockServiceObserver);
  var serverUrl = 'http://localhost:' + (testServerPort++);
  var client;

  before('Start the server.', function(done) {
    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');
        done();
      });
  });

  after('Stop the server.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    server.stop();
  });

  afterEach('Clients disconnect.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    client && client.close();
    client = undefined;
  });

  describe('on: login', function(){
    beforeEach('Clients connect to the server.', function(done) {
      client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});
      done();
    });

    afterEach('Clients disconnect.', function() {
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      client && client.close();
      client = undefined;
    });

    let someValidToken = (new Buffer(JSON.stringify('someValidToken'))).toString('base64');

    it('Joining with valid token and UA info should succeed.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockServiceObserver.onJoin = sinon.spy();

      client.emit('login', {token: someValidToken, userAgent:iosClientInfo, protocol: '1.0'}, function(status, resp) {
        expect(status).to.equal('ok');
        expect(mockPortal.join.getCall(0).args).to.deep.equal([client.id, 'someValidToken']);
        expect(mockServiceObserver.onJoin.getCall(0).args).to.deep.equal(['tokenCode']);
        expect(resp.id).to.be.a('string')
        expect(resp.user).to.equal(presenter_join_result.data.user);
        expect(resp.role).to.equal(presenter_join_result.data.role);
        expect(resp.permission).to.deep.equal(presenter_join_result.data.permission);
        expect(resp.room).to.deep.equal(presenter_join_result.data.room);
        var reconnection_ticket = JSON.parse(new Buffer(resp.reconnectionTicket, 'base64').toString());
        expect(reconnection_ticket.notBefore).to.be.at.most(Date.now());
        expect(reconnection_ticket.notAfter).to.be.at.least(Date.now());
        done();
      });
    });

    it('Joining with invalid token and valid UA info should fail and cause disconnection.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.rejects('invalid token');

      let resolveDisconnect, resolveJoin;
      Promise.all([new Promise(function(resolve){
        resolveDisconnect=resolve;
      }), new Promise(function(resolve){
        resolveJoin=resolve;
      })]).then(function(){
        done();
      });

      client.on('disconnect', function() {
        resolveDisconnect();
      });

      client.emit('login', {token: (new Buffer(JSON.stringify('someInvalidToken'))).toString('base64'), userAgent: jsClientInfo, protocol: '1.0'}, function(status, resp) {
        expect(status).to.equal('error');
        expect(resp).to.equal('invalid token');
        resolveJoin();
      });
    });

    it('Joining with token which is not base64 encoded should fail and cause disconnection.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.rejects('invalid token');

      let resolveDisconnect, resolveJoin;
      Promise.all([new Promise(function(resolve){
        resolveDisconnect=resolve;
      }), new Promise(function(resolve){
        resolveJoin=resolve;
      })]).then(function(){
        done();
      });

      client.on('disconnect', function() {
        resolveDisconnect();
      });

      client.emit('login', {token: 'someInvalidToken', userAgent: jsClientInfo, protocol: '1.0'}, function(status, resp) {
        expect(status).to.equal('error');
        expect(mockPortal.join.called).to.be.false;
        resolveJoin();
      });
    });

    it('Joining with valid tokens and invalid UA should fail and cause disconnection.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

      let resolveDisconnect, resolveToken;
      Promise.all([new Promise(function(resolve){
        resolveDisconnect=resolve;
      }), new Promise(function(resolve){
        resolveToken=resolve;
      })]).then(function(){
        done();
      });

      client.on('disconnect', function() {
        resolveDisconnect();
      });

      client.emit('login', {token: someValidToken, userAgent: 'invalidUserAgent'}, function(status, resp) {
        expect(status).to.equal('error');
        expect(resp).to.equal('User agent info is incorrect');
        resolveToken();
      });
    });

    // JavaScript client does not support reconnection. So this function should be disabled.
    xit('Do not issue reconnection ticket to JavaScript clients.', function(done){
      mockPortal.join = sinon.stub();
      mockServiceObserver.onJoin = sinon.spy();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

      client.emit('login', {token: someValidToken, userAgent:jsClientInfo, protocol: '1.0'}, function(status, resp) {
        expect(status).to.equal('ok');
        expect(resp.reconnectionTicket).to.be.undefined;
        done();
      });
    });
  });

  describe('on: relogin', function(){
    var reconnection_client;
    beforeEach('Clients connect to the server.', function(done) {
      reconnection_client = sioClient.connect(serverUrl, {reconnect: true, secure: false, 'force new connection': true});
      done();
    });

    afterEach('Clients connect to the server.', function(done) {
      reconnection_client && reconnection_client.close();
      reconnection_client = undefined;
      done();
    });

    const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: iosClientInfo, protocol: '1.0'};

    it('Relogin with correct ticket should succeed.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      const clientDisconnect = function(){
        return new Promise(function(resolve){
          reconnection_client.io.engine.close();
          resolve();
        });
      };
      const reconnection=function(){
        return new Promise(function(resolve){
          reconnection_client.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': false});
          resolve();
        });
      };
      reconnection_client.emit('login', someValidLoginInfo, function(status, resp){
        expect(status).to.equal('ok');
        clientDisconnect().then(reconnection).then(function(){
          reconnection_client.emit('relogin', resp.reconnectionTicket, function(status, resp){
            expect(status).to.equal('ok');
            done();
          });
        }).catch(function(err){
          done();
        });
      });
    });

    it('Relogin with incorrect ticket should fail.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      const clientDisconnect = function(){
        return new Promise(function(resolve){
          reconnection_client.io.engine.close();
          resolve();
        });
      };
      const reconnection=function(){
        return new Promise(function(resolve){
          reconnection_client.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': false});
          resolve();
        });
      };
      reconnection_client.emit('login', someValidLoginInfo, function(status, resp){
        expect(status).to.equal('ok');
        clientDisconnect().then(reconnection).then(function(){
          reconnection_client.emit('relogin', 'someInvalidReconnectionTicket', function(status, resp){
            expect(status).to.equal('error');
            done();
          });
        }).catch(function(err){
          assert(false);
          done();
        });
      });
    });

    xit('Relogin with correct ticket after logout should fail.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      const clientDisconnect = function(){
        return new Promise(function(resolve){
          reconnection_client.close();
          resolve();
        });
      };
      const reconnection=function(){
        return new Promise(function(resolve){
          reconnection_client.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': false});
          resolve();
        });
      };
      reconnection_client.emit('login', someValidLoginInfo, function(status, resp){
        expect(status).to.equal('ok');
        setTimeout(function(){  // Postpone disconnect. Otherwise, disconnect will be executed before establishing connection.
          reconnection_client.emit('logout');
          clientDisconnect().then(reconnection).then(function(){
            reconnection_client.emit('relogin', resp.reconnectionTicket, function(status, resp){
              expect(status).to.equal('error');
              done();
            });
          }).catch(function(err){
            done();
          });
        },10);
      });
    });

    it('Dropped user cannot be reconnected.', function(done){
      let client = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
      let ticket;

      client.on('connect', function() {
        mockPortal.join = sinon.stub();
        mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

        client.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('ok');
          ticket = resp.reconnectionTicket;
          server.drop(client.id);
        });
      });
      client.on('disconnect', function(){
        client = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
        client.emit('relogin', ticket, function(status, resp){
          expect(status).to.equal('error');
          done();
        });
      });
    });

    // Socket.IO server may detect network error slower than client does.
    it('Reconnect before original socket is disconnected is allowed.', function(done){
      let client1 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
      client1.on('connect', function() {
        mockPortal.join = sinon.stub();
        mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

        client1.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('ok');
          let client2 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
          client2.emit('relogin', resp.reconnectionTicket, function(status, resp){
            expect(status).to.equal('ok');
            done();
          });
        });
      });
    });

    it('Components created during old client\'s lifetime can still send message after reconnection.', function(done){
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('ok');
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');

      let client1 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
      client1.on('connect', function() {
        mockPortal.join = sinon.stub();
        mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
        var participant_id = client1.id;
        client1.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('ok');
          var pub_req = {
            media: {
              audio: {
                source: 'mic'
              },
              video: {
                source: 'camera'
              }
            }
          };
          client1.emit('publish', pub_req, function(status, res) {
            var stream_id = res.id;
            expect(status).to.equal('ok');
            client1.disconnect();
            let client2 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
            client2.on('progress', function(data) {
              if (data.status === 'soac' && data.data.type === 'candidate') {
                done();
              }
            });
            client2.emit('relogin', resp.reconnectionTicket, function(status, resp){
              expect(status).to.equal('ok');
              server.notify(participant_id, 'progress', {id: stream_id, status: 'soac', data: {type: 'candidate', candidate: 'I\'m a candidate.'}});
            });
          });
        });
      });
    });
  });

  describe('on: logout', function() {
    const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: iosClientInfo, protocol: '1.0'};

    beforeEach('Clients connect to the server.', function(done) {
      client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});
      done();
    });

    it('Logout before login should fail.', function(done){
      client.emit('logout', function(status){
        expect(status).to.equal('error');
        done();
      });
    });

    it('Logout after login should succeed.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

      client.emit('login', someValidLoginInfo, function(loginStatus, resp) {
        expect(loginStatus).to.equal('ok');
        client.emit('logout', function(logoutStatus){
          expect(logoutStatus).to.equal('ok');
          done();
        });
      });
    });
  });

  describe('on: refreshReconnectionTicket', function(){
    beforeEach('Clients connect to the server.', function(done) {
      client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});
      done();
    });

    it('Refresh reconnection ticket before login should fail.', function(done){
      client.emit('refreshReconnectionTicket', function(status, data){
        expect(status).to.equal('error');
        done();
      });
    });

    it('Refresh reconnection ticket after login should get new ticket.', function(done){
      const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: iosClientInfo, protocol: '1.0'};
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      client.emit('login', someValidLoginInfo, function(status, resp) {
        expect(status).to.equal('ok');
        client.emit('refreshReconnectionTicket', function(status, data){
          var reconnection_ticket = JSON.parse(new Buffer(data, 'base64').toString());
          expect(reconnection_ticket.notBefore).to.be.at.most(Date.now());
          expect(reconnection_ticket.notAfter).to.be.at.least(Date.now());
          done();
        });
      });
    });
  });

  describe('on: disconnect', function() {
    beforeEach('Clients connect to the server.', function(done) {
      client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});
      done();
    });

    xit('Disconnecting after joining should cause participant leaving.', function(done) {
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockServiceObserver.onLeave = sinon.spy();

      var participant_id;
      client.on('disconnect', function() {
        //NOTE: the detection method here is not acurate.
        setTimeout(function() {
          expect(mockPortal.leave.getCall(0).args).to.deep.equal([participant_id]);
          expect(mockServiceObserver.onLeave.getCall(0).args).to.deep.equal(['tokenCode']);
          done();
        }, 100);
      });

      client.emit('login', jsLoginInfo, function(status, resp) {
        participant_id = client.id;
        expect(status).to.equal('ok');
        client.disconnect();
      });
    });

    it('Disconnecting before joining should not cause participant leaving.', function(done) {
      mockPortal.leave = sinon.stub();
      mockPortal.leave.rejects('not reached');
      mockServiceObserver.onLeave = sinon.spy();

      client.on('disconnect', function() {
        //NOTE: the detection method here is not acurate.
        setTimeout(function() {
          expect(mockPortal.leave.callCount).to.equal(0);
          expect(mockServiceObserver.onLeave.callCount).to.equal(0);
          done();
        }, 10);
      });

      // Waiting for connection established. If disconnect is executed before connected, it will be deferred.
      setTimeout(function(){
        client.disconnect();
      }, 60);
    });
  });
});

describe('Notifying events to clients.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var mockServiceObserver = {onJoin: sinon.spy(), onLeave: sinon.spy()};
  var server = socketIOServer(insecureSocketIOServerConfig(), mockPortal, mockServiceObserver);
  var serverUrl = 'http://localhost:' + (testServerPort++);
  var client;

  before('Start the server.', function(done) {
    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');
        done();
      });
  });

  after('Stop the server.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    server.stop();
  });

  it('Notifying the unjoined clients should fail.', function() {
    return expect(server.notify('112233445566', 'event', 'data')).to.be.rejectedWith('participant does not exist');
  });

  it('Notifying the joined clients should succeed.', function(done) {
    var client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});

    client.on('whatever-event', function(data) {
      expect(data).to.equal('whatever-data');
      done();
    });

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

      client.emit('login', jsLoginInfo, function(status, resp) {
        mockPortal.join = null;
        expect(status).to.equal('ok');
        return server.notify(client.id, 'whatever-event', 'whatever-data')
        .then((result) => {
          expect(result).to.equal('ok');
        });
      });
    });
  });
});

describe('Drop users from sessions.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  mockPortal.leave = sinon.stub();
  mockPortal.leave.resolves('ok');

  var mockServiceObserver = {onJoin: sinon.spy(), onLeave: sinon.spy()};
  var server = socketIOServer(insecureSocketIOServerConfig(), mockPortal, mockServiceObserver);
  var serverUrl = 'http://localhost:' + (testServerPort++);
  var client;

  before('Start the server.', function(done) {
    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');
        done();
      });
  });

  after('Stop the server.', function() {
    server.stop();
  });

  xit('Dropping users after joining should succeed.', function(done) {
    var client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      mockServiceObserver.onLeave = sinon.spy();

      client.emit('login', jsLoginInfo, function(status, resp) {
        server.drop(client.id);
        expect(mockServiceObserver.onLeave.getCall(0).args).to.deep.equal(['tokenCode']);
        expect(mockPortal.leave.getCall(0).args).to.deep.equal([client.id]);
        done();
      });
    });
  });

  xit('Dropping all users should cause all users leaving.', function(done) {
    var client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));
      mockServiceObserver.onLeave = sinon.spy();

      client.emit('login', jsLoginInfo, function(status, resp) {
        server.drop('all');
        expect(mockServiceObserver.onLeave.getCall(0).args).to.deep.equal(['tokenCode']);
        done();
      });
    });
  });
});

describe('Responding to clients.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var mockServiceObserver = {onJoin: sinon.spy(), onLeave: sinon.spy()};
  var server = socketIOServer(insecureSocketIOServerConfig(), mockPortal, mockServiceObserver);
  var serverUrl = 'http://localhost:' + (testServerPort++);
  var client;

  before('Start the server.', function(done) {
    server.start()
      .then(function(result) {
        expect(result).to.equal('ok');
        done();
      });
  });

  after('Stop the server.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    server.stop();
  });

  beforeEach('Clients connect to the server.', function(done) {
    client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});
    done();
  });

  afterEach('Clients disconnect.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    client.close();
  });

  function joinFirstly() {
    return new Promise(function(resolve, reject) {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(JSON.parse(JSON.stringify(presenter_join_result)));

      client.emit('login', jsLoginInfo, function(status, resp) {
        expect(status).to.equal('ok');
        resolve('ok');
      });
    });
  }

  xit('Requests before joining should fail', function(done) {
    client.emit('publish', 'options', function(status, id) {
      expect(status).to.equal('error');
      expect(id).to.equal('Illegal request');
      client.emit('unpublish', 'streamId', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
        done();
      });
    });
  });

  describe('on: publish, soac', function() {
    it('Publishing webrtc streams after joining should succeed.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.onSessionSignaling = sinon.stub();

      mockPortal.publish.resolves('ok');
      mockPortal.onSessionSignaling.resolves('ok');

      var stream_id = undefined;
      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var pub_req = {
            media: {
              audio: {
                source: 'mic'
              },
              video: {
                source: 'camera'
              }
            }
          };
          client.emit('publish', pub_req, function(status, res) {
            expect(status).to.equal('ok');
            expect(res.id).to.be.a('string');
            stream_id = res.id;
            expect(mockPortal.publish.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.publish.getCall(0).args[1]).to.equal(stream_id);
            expect(mockPortal.publish.getCall(0).args[2].media).to.deep.equal(pub_req.media);

            client.emit('soac', {id: stream_id, signaling: {type: 'offer', sdp: 'offerSDPString'}}, function() {
              expect(mockPortal.onSessionSignaling.getCall(0).args).to.deep.equal([client.id, stream_id, {type: 'offer', sdp: 'offerSDPString'}]);
              client.emit('soac', {id: stream_id, signaling: {type: 'candidate', candidate: 'candidateString'}}, function() {
                expect(mockPortal.onSessionSignaling.getCall(1).args).to.deep.equal([client.id, stream_id, {type: 'candidate', candidate: 'candidateString'}]);
                server.notify(client.id, 'progress', {id: stream_id, status: 'ready'});
              });
            });
          });

          client.on('progress', function(data) {
            if (data.status === 'ready') {
              done();
            }
          });
        });
    });

    it('Publishing streams should fail if portal fails.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.rejects('whatever reason');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var pub_req = {
            media: {
              audio: {
                source: 'mic'
              },
              video: {
                source: 'camera'
              }
            }
          };
          client.emit('publish', pub_req, function(status, res) {
            expect(status).to.equal('error');
            expect(res).to.equal('whatever reason');
            done();
          });
        });
    });

    it('Publishing streams should fail if error occurs during communicating.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('ok');

      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      var stream_id;
      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var pub_req = {
            media: {
              audio: {
                source: 'mic'
              },
              video: {
                source: 'camera'
              }
            }
          };
          client.emit('publish', pub_req, function(status, res) {
            expect(status).to.equal('ok');
            expect(res.id).to.be.a('string');
            stream_id = res.id;
            server.notify(client.id, 'progress', {id: stream_id, status: 'error', data: 'whatever reason'});
          });

          client.on('progress', function(data) {
            if (data.status === 'error') {
              expect(data.data).to.equal('whatever reason');
              done();
            }
          });
        });
    });

    it('With invalid pubRequest should fail.', function(done) {
      mockPortal.publish = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('publish', {}, function(status, data) {
            expect(status).to.equal('error');
            client.emit('publish', {media: {}}, function(status, data) {
              expect(status).to.equal('error');
              done();
            });
          });
        });
    });
  });

  describe('on: unpublish', function() {
    it('Unpublishing streams should succeed if portal.unpublish succeeds.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.resolves('ok');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', {id: 'previouslyPublishedStreamId'}, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal([client.id, 'previouslyPublishedStreamId']);
            done();
          });
        });
    });

    it('Unpublishing streams should fail if portal.unpublish fails.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.rejects('stream does not exist');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', {id: 'non-ExistStreamId'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('stream does not exist');
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal([client.id, 'non-ExistStreamId']);
            done();
          });
        });
    });

    it('With invalid stream-id should fail.', function(done) {
      mockPortal.unpublish = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', {}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid stream id');
            client.emit('unpublish', {id: ''}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid stream id');
              client.emit('unpublish', {id: 1234}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid stream id');
                client.emit('unpublish', {id: {obj: 'yes'}}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid stream id');
                  expect(mockPortal.unpublish.callCount).to.equal(0);
                  done();
                });
              });
            });
          });
        });
    });
  });

  describe('on: stream-control', function() {
    it('Mixing/unmixing streams should succeed if portal.streamControl suceeds.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      joinFirstly()
        .then(function() {
          client.emit('stream-control', {id: 'streamId', operation: 'mix', data: 'view1'}, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'streamId', {operation: 'mix', data: 'view1'}]);

            client.emit('stream-control', {id: 'streamId', operation: 'unmix', data: 'view2'}, function(status, data) {
              expect(status).to.equal('ok');
              expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'streamId', {operation: 'unmix', data: 'view2'}]);
              done();
            });
          });
      });
    });

    it('Mixing/unmixing streams should fail if portal.streamControl fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('stream does not exist');

      joinFirstly()
        .then(function() {
          client.emit('stream-control', {id: 'streamId', operation: 'mix', data: 'view1'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('stream does not exist');

            client.emit('stream-control', {id: 'streamId', operation: 'unmix', data: 'view2'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('stream does not exist');
              done();
            });
          });
      });
    });

    it('Playing/pausing streams should succeed if portal.streamControl suceeds.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      joinFirstly()
        .then(function() {
          client.emit('stream-control', {id: 'streamId', operation: 'play', data: 'av'}, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'streamId', {operation: 'play', data: 'av'}]);

            client.emit('stream-control', {id: 'streamId', operation: 'pause', data: 'video'}, function(status, data) {
              expect(status).to.equal('ok');
              expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'streamId', {operation: 'pause', data: 'video'}]);
              done();
            });
          });
      });
    });

    it('Playing/pausing streams should fail if portal.streamControl fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('stream does not exist');

      joinFirstly()
        .then(function() {
          client.emit('stream-control', {id: 'streamId', operation: 'play', data: 'av'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('stream does not exist');

            client.emit('stream-control', {id: 'streamId', operation: 'pause', data: 'video'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('stream does not exist');
              done();
            });
          });
      });
    });

    it('Getting/setting region should succeed if portal.streamControl succeeds.', function(done) {
      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          mockPortal.streamControl = sinon.stub();
          mockPortal.streamControl.resolves({region: 'regionId'});
          client.emit('stream-control', {id: 'subStreamId', operation: 'get-region', data: 'common'}, function(status, res) {
            expect(status).to.equal('ok');
            expect(res.region).to.equal('regionId');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'get-region', data: 'common'}]);

            mockPortal.streamControl = sinon.stub();
            mockPortal.streamControl.resolves('ok');
            client.emit('stream-control', {id: 'subStreamId', operation: 'set-region', data: {view: 'common', region: '3'}}, function(status, data) {
              expect(status).to.equal('ok');
              expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'set-region', data: {region: '3', view: 'common'}}]);
              done();
            });
          });
        });
    });

    it('Getting/setting region should fail if portal.getRegion/portal.setRegion fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('some error');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('stream-control', {id: 'subStreamId', operation: 'get-region', data: 'common'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('some error');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'get-region', data: 'common'}]);

            client.emit('stream-control', {id: 'subStreamId', operation: 'set-region', data: {view: 'common', region: '3'}}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('some error');
              expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'subStreamId', {operation: 'set-region', data: {region: '3', view: 'common'}}]);
              done();
            });
          });
        });
    });

    it('With invalid stream-id should fail.', function(done) {
      mockPortal.streamControl = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('stream-control', {operation: 'mix', data: 'common'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid stream id');
            expect(mockPortal.streamControl.callCount).to.equal(0);

            client.emit('stream-control', {id: {a: 56}, operation: 'unmix', data: 'common'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid stream id');
              expect(mockPortal.streamControl.callCount).to.equal(0);

              client.emit('stream-control', {id: '', operation: 'mix', data: 'common'}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid stream id');
                expect(mockPortal.streamControl.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });

    it('With invalid operation or data should fail.', function(done) {
      mockPortal.streamControl = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('stream-control', {id: 'subStreamId', operation: 'set-region', data: {view: 'common', region: 789}}, function(status, data) {
            expect(status).to.equal('error');
            expect(mockPortal.streamControl.callCount).to.equal(0);

            client.emit('stream-control', {id: 'subStreamId', operation: 'set-region', data: {view: 'common', region: ''}}, function(status, data) {
              expect(status).to.equal('error');
              expect(mockPortal.streamControl.callCount).to.equal(0);
              done();
            });
          });
        });
    });
  });

  describe('on: subscribe, soac', function() {
    it('Subscription to a webrtc endpoint requesting neither audio nor video should fail.', function(done) {
      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_req = {
            media: {
              audio: false,
              video: false
            }
          };
          client.emit('subscribe', sub_req, function(status, res) {
            expect(status).to.equal('error');
            expect(res).to.equal('Bad subscription request');
            done();
          });
        });
    });

    it('Subscription to a webrtc endpoint with proper request after joining should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.onSessionSignaling = sinon.stub();

      mockPortal.subscribe.resolves('ok');
      mockPortal.onSessionSignaling.resolves('ok');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_req = {
            media: {
              audio: {
                from: 'stream1'
              },
              video: {
                from: 'stream2',
                format: {
                  codec: 'vp8'
                },
                parameters: {
                  resolution: {width: 1920, height: 1080}
                }
              }
            }
          };
          client.emit('subscribe', sub_req, function(status, res) {
            expect(status).to.equal('ok');
            expect(res.id).to.be.a('string');
            var subscription_id = res.id;
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.equal(subscription_id);
            expect(mockPortal.subscribe.getCall(0).args[2].media).to.deep.equal(sub_req.media);
            client.emit('soac', {id: subscription_id, signaling: {type: 'offer', sdp: 'offerSDPString'}}, function() {
              expect(mockPortal.onSessionSignaling.getCall(0).args).to.deep.equal([client.id, subscription_id, {type: 'offer', sdp: 'offerSDPString'}]);
              client.emit('soac', {id: subscription_id, signaling: {type: 'candidate', candidate: 'candidateString'}}, function() {
                expect(mockPortal.onSessionSignaling.getCall(1).args).to.deep.equal([client.id, subscription_id, {type: 'candidate', candidate: 'candidateString'}]);
                server.notify(client.id, 'progress', {id: subscription_id, status: 'ready'});
              });
            });
          });

          client.on('progress', function(data) {
            if (data.status === 'ready') {
              done();
            }
          });
        });
    });

    it('Subscribing streams to a webrtc endpoint should fail if portal.subscribe fails.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.rejects('some-error');

      var subscription_id;

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_req = {
            media: {
              audio: {
                from: 'stream1'
              },
              video: {
                from: 'stream2',
                format: {
                  codec: 'vp8'
                },
                parameters: {
                  resolution: {width: 1920, height: 1080}
                }
              }
            }
          };
          client.emit('subscribe', sub_req, function(status, res) {
            expect(status).to.equal('error');
            expect(res).to.equal('some-error');
            done();
          });
        });
    });

    it('Subscribing streams to a webrtc endpoint should fail if error occurs during communicating.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      var subscription_id;

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_req = {
            media: {
              audio: {
                from: 'stream1'
              },
              video: {
                from: 'stream2',
                format: {
                  codec: 'vp8'
                },
                parameters: {
                  resolution: {width: 1920, height: 1080}
                }
              }
            }
          };
          client.emit('subscribe', sub_req, function(status, res) {
            expect(status).to.equal('ok');
            expect(res.id).to.be.a('string');
            subscription_id = res.id;
            server.notify(client.id, 'progress', {id: subscription_id, status: 'error', data: 'some-reason'});
          });

          client.on('progress', function(data) {
            expect(data.status).to.equal('error');
            expect(data.data).to.equal('some-reason');
            done();
          });
        });
    });

    it('With invalid subRequest should fail.', function(done) {
      mockPortal.subscribe = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('subscribe', {}, function(status, data) {
            expect(status).to.equal('error');
            client.emit('subscribe', {type: 'webrtc', media: {'audio': {} }}, function(status, data) {
              expect(status).to.equal('error');
              done();
            });
          });
        });
    });
  });

  describe('on: unsubscribe', function() {
    it('Unsubscribing should succeed if portal.unsubscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unsubscribe', {id: 'subscriptionId'}, function(status, data) {
            expect(status).to.equal('ok');
            done();
          });
        });
    });

    it('Unsubscribing should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('some-error');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unsubscribe', {id: 'subscriptionId'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('some-error');
            done();
          });
        });
    });

    it('With invalid subscription id should fail.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unsubscribe', {}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid subscription id');
            client.emit('unsubscribe', {id: ''}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid subscription id');
              client.emit('unsubscribe', {id: 1234}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid subscription id');
                client.emit('unsubscribe', {id: {obj: 'yes'}}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid subscription id');
                  expect(mockPortal.unsubscribe.callCount).to.equal(0);
                  done();
                });
              });
            });
          });
        });
    });
  });

  describe('on: subscription-control', function() {
    it('Updating subscriptions without specifying a valid subscription id should fail.', function(done) {
      mockPortal.subscriptionControl = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('subscription-control', {}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid subscription id');
            client.emit('subscription-control', {id: ''}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid subscription id');
              client.emit('subscription-control', {id: 1234}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid subscription id');
                client.emit('subscription-control', {id: {obj: 'yes'}}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid subscription id');
                  expect(mockPortal.subscriptionControl.callCount).to.equal(0);
                  done();
                });
              });
            });
          });
        });
    });

    it('Updating subscriptions should succeed if portal.subscriptionControl succeeds.', function(done) {
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.resolves('ok');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_ctrl_req = {
            id: 'subscriptionId',
            operation: 'update',
            data: {
              audio: {
                from: 'stream1'
              }
            }
          };
          client.emit('subscription-control', sub_ctrl_req, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.subscriptionControl.getCall(0).args).to.deep.equal([client.id, sub_ctrl_req.id, {operation: 'update', data: sub_ctrl_req.data}]);
            var sub_ctrl_req1 = {
              id: 'subscriptionId',
              operation: 'update',
              data: {
                audio: {
                  from: 'stream1'
                },
                video: {
                  from: 'stream2',
                  parameters: {
                    resolution: {width: 648, height: 480},
                    bitrate: 300,
                    framerate: 24,
                    keyFrameInterval: 2
                  }
                }
              }
            };
            client.emit('subscription-control', sub_ctrl_req1, function(status, data) {
              expect(status).to.equal('ok');
              expect(mockPortal.subscriptionControl.getCall(1).args).to.deep.equal([client.id, sub_ctrl_req1.id, {operation: 'update', data: sub_ctrl_req1.data}]);
              done();
            });
          });
        });
    });

    it('Updating audio/video source of a subscription should fail if portal.subscriptionControl fails.', function(done) {
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.rejects('some-error');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var sub_ctrl_req = {
            id: 'subscriptionId',
            operation: 'update',
            data: {
              audio: {
                from: 'stream1'
              }
            }
          };
          client.emit('subscription-control', sub_ctrl_req, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('some-error');
            done();
          });
        });
    });

    it('Playing/pausing subscriptions should succeed if portal.streamControl suceeds.', function(done) {
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.resolves('ok');

      joinFirstly()
        .then(function() {
          client.emit('subscription-control', {id: 'subscriptionId', operation: 'play', data: 'av'}, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.subscriptionControl.getCall(0).args).to.deep.equal([client.id, 'subscriptionId', {operation: 'play', data: 'av'}]);

            client.emit('subscription-control', {id: 'subscriptionId', operation: 'pause', data: 'video'}, function(status, data) {
              expect(status).to.equal('ok');
              expect(mockPortal.subscriptionControl.getCall(1).args).to.deep.equal([client.id, 'subscriptionId', {operation: 'pause', data: 'video'}]);
              done();
            });
          });
      });
    });

    it('Playing/pausing subscriptions should fail if portal.streamControl fails.', function(done) {
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.rejects('subscription does not exist');

      joinFirstly()
        .then(function() {
          client.emit('subscription-control', {id: 'subscriptionId', operation: 'play', data: 'av'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('subscription does not exist');

            client.emit('subscription-control', {id: 'subscriptionId', operation: 'pause', data: 'video'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('subscription does not exist');
              done();
            });
          });
      });
    });
  });

  describe('on: text', function() {
    it('Should succeed if portal.text succeeds.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.text.resolves('ok');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('text', {to: 'all', message: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('ok');
            expect(mockPortal.text.getCall(0).args).to.deep.equal([client.id, 'all', 'Hi, there!']);
            done();
          });
        });
    });

    it('Should fail if portal.text fails.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.text.rejects('some-error');

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('text', {to: 'all', message: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('some-error');
            done();
          });
        });
    });

    it('Should fail if receiver is invalid.', function(done) {
      mockPortal.text = sinon.stub();

      joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('text', {to: '', message: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid receiver');

            client.emit('text', {to: 54321, message: 'Hi, there!'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid receiver');

              client.emit('text', {to: {obj: true}, message: 'Hi, there!'}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid receiver');
                expect(mockPortal.text.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });
  });
});
