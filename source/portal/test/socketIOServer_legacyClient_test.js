'use strict';
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var socketIOServer = require('../socketIOServer');
var sioClient = require('socket.io-client');
var portal = require('../portal');

const testRoom = '573eab78111478bb3526421a';
const testStream = '573eab78111478bb3526421a-common';

const clientInfo = {sdk:{version: '3.3', type: 'JavaScript'}, runtime: {name: 'Chrome', version: '53.0.0.0'}, os:{name:'Linux (Ubuntu)', version:'14.04'}};
// JavaScript SDK 3.3 does not support reconnection. So use iOS UA info for reconnection tests.
const reconnectionClientInfo = {sdk:{version: '3.3', type: 'Objective-C'}, runtime: {name: 'WebRTC', version: '54'}, os:{name:'iPhone OS', version:'10.2'}};
const jsLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: clientInfo};
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
                    left: { numerator: 0, denominator: 1},
                    top: { numerator: 0, denominator: 1},
                    width: { numerator: 1, denominator: 1},
                    height: { numerator: 1, denominator: 1}
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

    return server.start()
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

    return server.start()
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
    return server.start()
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

    let someValidToken = (new Buffer(JSON.stringify('someValidToken'))).toString('base64');

    it('Joining with valid token and UA info should succeed.', function(done) {
      mockPortal.join = sinon.stub();
      mockServiceObserver.onJoin = sinon.spy();

      mockPortal.join.resolves(presenter_join_result);

      var transformed_streams = [
        {
          id: testStream,
          view: 'common',
          audio: true,
          video: {
            device: 'mcu',
            resolutions: [{width: 1280, height: 720}, {width: 640, height: 480}, {width: 320, height: 240}],
            layout: [
            {
              id: "1",
              left: 0,
              top: 0,
              relativeSize: 1,
              streamID: "forward-stream-id"
            }
           ]
          },
          from: '',
          socket: '',
        }, {
          id: 'forward-stream-id',
          audio: true,
          video: {device: 'camera'},
          from: 'participant-id',
          socket: '',
          attributes: {someKey: 'someValue'}
        }
      ];

      var transformed_participants = [
          {id: 'participant-id',
           name: 'user-id-2',
           role: 'presenter'}
        ];

      client.emit('login', {token: someValidToken, userAgent:reconnectionClientInfo}, function(status, resp) {
        expect(status).to.equal('success');
        expect(mockPortal.join.getCall(0).args).to.deep.equal([client.id, 'someValidToken']);
        expect(mockServiceObserver.onJoin.getCall(0).args).to.deep.equal(['tokenCode']);
        expect(resp.id).to.equal(presenter_join_result.data.room.id);
        expect(resp.clientId).to.equal(client.id);
        expect(resp.streams).to.deep.equal(transformed_streams);
        expect(resp.users).to.deep.equal(transformed_participants);
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

      client.emit('login', {token: (new Buffer(JSON.stringify('someInvalidToken'))).toString('base64'), userAgent: clientInfo}, function(status, resp) {
        expect(status).to.equal('error');
        expect(resp).to.have.string('invalid token');
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

      client.emit('login', {token: 'someInvalidToken', userAgent: clientInfo}, function(status, resp) {
        expect(status).to.equal('error');
        expect(mockPortal.join.called).to.be.false;
        resolveJoin();
      });
    });

    it('Joining with valid tokens and invalid UA should fail and cause disconnection.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);

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
        expect(resp).to.have.string('User agent info is incorrect');
        resolveToken();
      });
    });

    // JavaScript client does not support reconnection. So this function should be disabled.
    it('Do not issue reconnection ticket to JavaScript clients.', function(done){
      mockPortal.join = sinon.stub();
      mockServiceObserver.onJoin = sinon.spy();

      mockPortal.join.resolves(presenter_join_result);

      client.emit('login', {token: someValidToken, userAgent:clientInfo}, function(status, resp) {
        expect(status).to.equal('success');
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

    const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: reconnectionClientInfo};

    it('Relogin with correct ticket should success.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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
        expect(status).to.equal('success');
        clientDisconnect().then(reconnection).then(function(){
          reconnection_client.emit('relogin', resp.reconnectionTicket, function(status, resp){
            expect(status).to.equal('success');
            done();
          });
        }).catch(function(err){
          done();
        });
      });
    });

    it('Relogin with incorrect ticket should fail.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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
        expect(status).to.equal('success');
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

    it('Relogin with correct ticket after logout should fail.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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
        expect(status).to.equal('success');
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
        mockPortal.join.resolves(presenter_join_result);

        client.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('success');
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
        mockPortal.join.resolves(presenter_join_result);

        client1.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('success');
          let client2 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
          client2.emit('relogin', resp.reconnectionTicket, function(status, resp){
            expect(status).to.equal('success');
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
        mockPortal.join.resolves(presenter_join_result);
        var participant_id = client1.id;
        client1.emit('login', someValidLoginInfo, function(status, resp) {
          expect(status).to.equal('success');
          let options = {state: 'erizo', audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}};
          client1.emit('publish', options, undefined, function(status, id) {
            var stream_id = id;
            expect(status).to.equal('initializing');
            //client1.disconnect();
            let client2 = sioClient.connect(serverUrl, {reconnection: true, secure: false, 'force new connection': false});
            client2.on('signaling_message_erizo', function(){
              done();
            });
            client2.emit('relogin', resp.reconnectionTicket, function(status, resp){
              expect(status).to.equal('success');
              server.notify(participant_id, 'progress', {id: stream_id, status: 'soac', data: {type: 'candidate', candidate: 'I\'m a candidate.'}});
            });
          });
        });
      });
    });
  });

  describe('on: logout', function() {
    const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: reconnectionClientInfo};

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

    it('Logout after login should success.', function(done){
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);

      client.emit('login', someValidLoginInfo, function(loginStatus, resp) {
        expect(loginStatus).to.equal('success');
        client.emit('logout', function(logoutStatus){
          expect(logoutStatus).to.equal('success');
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
      const someValidLoginInfo = {token: (new Buffer(JSON.stringify('someValidToken'))).toString('base64'), userAgent: reconnectionClientInfo};
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
      client.emit('login', someValidLoginInfo, function(status, resp) {
        expect(status).to.equal('success');
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

    it('Disconnecting after joining should cause participant leaving.', function(done) {
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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
        expect(status).to.equal('success');
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
    return server.start()
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

    client.on('user_join', function(data) {
      expect(data).to.deep.equal({user: {id: 'participantId', name: 'user-name', role: 'presenter'}});
      client.on('add_stream', function(data) {
        expect(data).to.deep.equal({id: 'streamId', audio: true, video: {device: 'camera'}, from: 'participantId', socket: '', attributes: {key: 'value'}});
        client.on('update_stream', function(data) {
          expect(data).to.deep.equal({id: 'streamId', event: 'StateChange', state: 'inactive'});
          client.on('remove_stream', function(data) {
            expect(data).to.deep.equal({id: 'streamId'});
            client.on('user_leave', function(data) {
              expect(data).to.deep.equal({user: {id: 'participantId'}});
              done();
            });
          });
        });
      });
    });

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);

      client.emit('login', jsLoginInfo, function(status, resp) {
        mockPortal.join = null;
        expect(status).to.equal('success');
        return server.notify(client.id, 'participant', {action: 'join', data: {id: 'participantId', user: 'user-name', role: 'presenter'}})
        .then((result) => {
          expect(result).to.equal('ok');
          return server.notify(client.id, 'stream', {status: 'add', id: 'streamId', data: {id: 'streamId', type: 'forward', media: {audio: {format: {codec: 'opus', sampleRate: 48000, channelNum: 2}, status: 'active'}, video: {format: {codec: 'h264', profile: 'constrained-baseline'}, source: 'camera', status: 'active'}}, info: {owner: 'participantId', type: 'webrtc', attributes: {key: 'value'}}}});
        }).then((result) => {
          expect(result).to.equal('ok');
          return server.notify(client.id, 'stream', {status: 'update', id: 'streamId', data: {field: 'video.status', value: 'inactive'}});
        }).then((result) => {
          expect(result).to.equal('ok');
          return server.notify(client.id, 'stream', {status: 'remove', id: 'streamId'});
        }).then((result) => {
          expect(result).to.equal('ok');
          return server.notify(client.id, 'participant', {action: 'leave', data: 'participantId'});
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
    return server.start()
      .then(function(result) {
        expect(result).to.equal('ok');
        done();
      });
  });

  after('Stop the server.', function() {
    server.stop();
  });

  it('Dropping users after joining should succeed.', function(done) {
    var client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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

  it('Dropping all users should cause all users leaving.', function(done) {
    var client = sioClient.connect(serverUrl, {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.resolves(presenter_join_result);
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
    return server.start()
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
      mockPortal.join.resolves(presenter_join_result);

      client.emit('login', jsLoginInfo, function(status, resp) {
        expect(status).to.equal('success');
        resolve('ok');
      });
    });
  }

  it.skip('Requests before joining should fail', function(done) {
    client.emit('publish', 'options', undefined, function(status, id) {
      console.log('status:', status, 'id:', id);
      expect(status).to.equal('error');
      expect(id).to.equal('Illegal request');
      done();
      client.emit('unpublish', 'streamId', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('mix', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('unmix', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('subscribe', 'options', undefined, function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('unsubscribe', 'streamId', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('signaling_message', 'message', undefined, function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('addExternalOutput', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('updateExternalOutput', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('removeExternalOutput', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('startRecorder', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('stopRecorder', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('getRegion', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('setRegion', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('mute', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('unmute', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('setPermission', 'options', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('customMessage', 'msg', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('logout', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
      client.emit('refreshReconnectionTicket', function(status, id) {
        expect(status).to.equal('error');
        expect(id).to.equal('Illegal request');
        done();
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
      });
    });
  });

  describe('on: publish, signaling_message', function() {
    it('Publishing webrtc streams after joining should succeed.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.onSessionSignaling = sinon.stub();
      mockPortal.streamControl = sinon.stub();

      mockPortal.publish.resolves('ok');
      mockPortal.onSessionSignaling.resolves('ok');
      mockPortal.streamControl.resolves('ok');

      var stream_id = undefined;

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {state: 'erizo', audio: true, video: {device: 'camera'}};
          client.emit('publish', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            stream_id = id;
            expect(mockPortal.publish.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.publish.getCall(0).args[1]).to.equal(stream_id);
            expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({
              type: 'webrtc',
              media: {
                audio: {
                  source: 'mic'
                },
                video: {
                  source: 'camera'
                }
              }
            });

            client.emit('signaling_message', {streamId: stream_id, msg: {type: 'offer', sdp: 'offerSDPString'}}, undefined, function() {
              expect(mockPortal.onSessionSignaling.getCall(0).args).to.deep.equal([client.id, stream_id, {type: 'offer', sdp: 'offerSDPString'}]);
              client.emit('signaling_message', {streamId: stream_id, msg: {type: 'candidate', candidate: 'candidateString'}}, undefined, function() {
                expect(mockPortal.onSessionSignaling.getCall(1).args).to.deep.equal([client.id, stream_id, {type: 'candidate', candidate: 'candidateString'}]);
                server.notify(client.id, 'progress', {id: stream_id, status: 'ready'}); 
              });
            });
          });

          client.on('signaling_message_erizo', function(arg) {
            if (arg.mess.type === 'ready') {
              done();
              expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, stream_id, {operation: 'mix', data: 'common'}]);
            }
          });
        });
    });

    it('Publishing rtsp/rtmp streams after joining should succeed.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('');

      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {state: 'url', audio: true, video: true, transport: 'tcp', bufferSize: 2048, unmix: true, attributes: {key: 'value'}};
          client.emit('publish', options, 'urlOfRtspOrRtmpSource', function(status, id) {
            expect(status).to.equal('success');
            expect(id).to.be.a('string');
            expect(mockPortal.publish.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.publish.getCall(0).args[1]).to.equal(id);
            expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({
              type: 'streaming',
              connection: {
                url: 'urlOfRtspOrRtmpSource',
                transportProtocol: 'tcp',
                bufferSize: 2048
              },
              media: {
                audio: true,
                video: true
              },
              attributes: {
                key: 'value'
              }
            });
            expect(mockPortal.streamControl.callCount).to.equal(0);
            done();
          });
        });
    });

    it('Publishing streams should fail if portal fails.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.rejects('whatever reason');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {state: 'erizo', audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}};
          client.emit('publish', options, undefined, function(status, result) {
            expect(status).to.equal('error');
            expect(result).to.equal('whatever reason');
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
      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {state: 'erizo', audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}};
          client.emit('publish', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            stream_id = id;
            server.notify(client.id, 'progress', {id: stream_id, status: 'error', data: 'whatever reason'});
          });

          client.on('connection_failed', function(arg) {
            expect(arg.streamId).to.equal(stream_id);
            done();
          });
        });
    });

    it('The stream description should be adjusted if not in expected format while publishing.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('ok');

      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options1 = {state: 'erizo'};
          client.emit('publish', options1, undefined, function(status, id) {
            expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({
              type: 'webrtc',
              media: {
                audio: {
                  source: 'mic'
                },
                video: {
                  source: 'camera'
                }
              }
            });

            var options2 = {state: 'erizo', audio: false, video: true, unmix: true};
            client.emit('publish', options2, undefined, function(status, id) {
              expect(mockPortal.publish.getCall(1).args[2]).to.deep.equal({
                type: 'webrtc',
                media: {
                  audio: false,
                  video: {
                    source: 'camera'
                  }
                }
              });

              var options3 = {state: 'erizo', audio: true, video: false};
              client.emit('publish', options3, undefined, function(status, id) {
                expect(mockPortal.publish.getCall(2).args[2]).to.deep.equal({
                  type: 'webrtc',
                  media: {
                    audio: {
                      source: 'mic'
                    },
                    video: false
                  }
                });

                var options4 = {state: 'erizo', audio: false, video: {resolution: 'vga', device: 'screen'}};
                client.emit('publish', options4, undefined, function(status, id) {
                  expect(mockPortal.publish.getCall(3).args[2]).to.deep.equal({
                    type: 'webrtc',
                    media: {
                      audio: false,
                      video: {
                        source: 'screen-cast'
                      }
                    }
                  });

                  var options5 = {state: 'url', video: true};
                  client.emit('publish', options5, 'urlOfRtspOrRtmpSource', function(status, id) {
                    expect(mockPortal.publish.getCall(4).args[2]).to.deep.equal({
                      type: 'streaming',
                      connection: {
                        url: 'urlOfRtspOrRtmpSource',
                        transportProtocol: 'tcp',
                        bufferSize: 2048
                      },
                      media: {
                        audio: 'auto',
                        video: true
                      }
                    });
                    done();
                  });
                });
              });
            });
          });
        });
    });
  });

  describe('on: unpublish', function() {
    it('Unpublishing streams should succeed if portal.unpublish succeeds.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', 'previouslyPublishedStreamId', function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal([client.id, 'previouslyPublishedStreamId']);
            done();
          });
        });
    });

    it('Unpublishing streams should fail if portal.unpublish fails.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.rejects('stream does not exist');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', 'non-ExistStreamId', function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('stream does not exist');
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal([client.id, 'non-ExistStreamId']);
            done();
          });
        });
    });
  });

  describe('on: mix, unmix', function() {
    it('Mixing/unmixing streams should succeed if portal.streamControl suceeds.', function(done) {
      mockPortal.streamControl = sinon.stub();

      mockPortal.streamControl.resolves('ok');

      return joinFirstly()
        .then(function() {
          client.emit('mix', {streamId: 'streamId', mixStreams: ['mix-view1', 'mix-view2']}, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'streamId', {operation: 'mix', data: 'view1'}]);
            expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'streamId', {operation: 'mix', data: 'view2'}]);

            client.emit('unmix', {streamId: 'streamId', mixStreams: ['mix-view1', 'mix-view2']}, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.streamControl.getCall(2).args).to.deep.equal([client.id, 'streamId', {operation: 'unmix', data: 'view1'}]);
              expect(mockPortal.streamControl.getCall(3).args).to.deep.equal([client.id, 'streamId', {operation: 'unmix', data: 'view2'}]);
              done();
            });
          });
      });
    });

    it('Mixing/unmixing streams should fail if portal.streamControl fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('stream does not exist');

      return joinFirstly()
        .then(function() {
          client.emit('mix', {streamId: 'streamId', mixStreams: ['mix-view1', 'mix-view2']}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('stream does not exist');

            client.emit('unmix', {streamId: 'streamId', mixStreams: ['mix-view1', 'mix-view2']}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('stream does not exist');
              done();
            });
          });
      });
    });
  });

  describe('on: subscribe, signaling_message', function() {
    it('Subscribing with options requesting neither audio nor video should fail.', function(done) {
      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: false, video: false};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('error');
            expect(id).to.equal('bad options');
            done();
          });
        });
    });

    it('Subscribing with proper options after joining should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.onSessionSignaling = sinon.stub();

      mockPortal.subscribe.resolves('ok');
      mockPortal.onSessionSignaling.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            expect(id).to.equal('targetStreamId');//FIXME: To be compatible with clients before v3.4
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.be.a('string');
            var subscription_id = mockPortal.subscribe.getCall(0).args[1];
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'webrtc', media: {audio: {from: 'targetStreamId'}, video: {from: 'targetStreamId', parameters: {resolution: {width: 640, height: 480}, bitrate: undefined}}}});
            client.emit('signaling_message', {streamId: 'targetStreamId', msg: {type: 'offer', sdp: 'offerSDPString'}}, undefined, function() {
              expect(mockPortal.onSessionSignaling.getCall(0).args).to.deep.equal([client.id, subscription_id, {type: 'offer', sdp: 'offerSDPString'}]);
              client.emit('signaling_message', {streamId: 'targetStreamId', msg: {type: 'candidate', candidate: 'candidateString'}}, undefined, function() {
                expect(mockPortal.onSessionSignaling.getCall(1).args).to.deep.equal([client.id, subscription_id, {type: 'candidate', candidate: 'candidateString'}]);
                server.notify(client.id, 'progress', {id: subscription_id, status: 'ready'});
              });
            });
          });

          client.on('signaling_message_erizo', function(arg) {
            if (arg.mess.type === 'ready') {
              done();
            }
          });
        });
    });

    it('The subscription description should be adjusted if not in expected format.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'webrtc', media: {audio: {from: 'targetStreamId'}, video: {from: 'targetStreamId', parameters: {resolution: {width: 640, height: 480}, bitrate: undefined}}}});
            var options1 = {streamId: 'targetStreamId1', video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
            client.emit('subscribe', options1, undefined, function(status, id) {
              expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'webrtc', media: {audio: {from: 'targetStreamId'}, video: {from: 'targetStreamId', parameters: {resolution: {width: 640, height: 480}, bitrate: undefined}}}});
              var options2 = {streamId: 'targetStreamId2', video: {resolution: 'vga', quality_level: 'standard'}};
              client.emit('subscribe', options2, undefined, function(status, id) {
                expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'webrtc', media: {audio: {from: 'targetStreamId'}, video: {from: 'targetStreamId', parameters: {resolution: {width: 640, height: 480}, bitrate: undefined}}}});
                done();
              });
            });
          });
        });
    });

    it('Subscribing streams should fail if portal.subscribe fails.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.rejects('some-error');

      var subscription_id;

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('error');
            expect(id).to.equal('some-error');
            done();
          });
        });
    });

    it('Subscribing streams should fail if error occurs during communicating.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      var subscription_id;

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            subscription_id = mockPortal.subscribe.getCall(0).args[1];
            server.notify(client.id, 'progress', {id: subscription_id, status: 'error', data: 'some-reason'});
          });

          client.on('connection_failed', function(arg) {
            expect(arg.streamId).to.equal(options.streamId);
            done();
          });
        });
    });
  });

  describe('on: unsubscribe', function() {
    it('Unsubscribing should succeed if portal.unsubscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            client.emit('unsubscribe', 'targetStreamId', function(status, data) {
              expect(status).to.equal('success');
              done();
            });
          });
        });
    });

    it('Unsubscribing should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('some-error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            client.emit('unsubscribe', 'targetStreamId', function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('some-error');
              done();
            });
          });
        });
    });
  });

  describe('on: addExternalOutput', function() {
    it('Starting pulling out rtsp/rtmp streams with proper options after joining should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', resolution: {width: 640, height: 480}, url: 'rtsp://target.host'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.be.a('string');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'streaming', media: {audio: {from: 'targetStreamId', format: {codec: 'aac', sampleRate: 48000, channelNum: 2}}, video: {from: 'targetStreamId', format: {codec: 'h264'}, parameters: {resolution: {width: 640, height: 480}}}}, connection: {url: 'rtsp://target.host'}});

            options = {streamId: 'targetStreamId', video: true, audio: false, resolution: {width: 640, height: 480}, url: 'rtsp://target2.host'};
            client.emit('addExternalOutput', options, function(status, data) {
              expect(status).to.equal('success');
              expect(data.url).to.equal('rtsp://target2.host');
              expect(mockPortal.subscribe.getCall(1).args[0]).to.equal(client.id);
              expect(mockPortal.subscribe.getCall(1).args[1]).to.be.a('string');
              expect(mockPortal.subscribe.getCall(1).args[2]).to.deep.equal({type: 'streaming', media: {audio: false, video: {from: 'targetStreamId', format: {codec: 'h264'}, parameters: {resolution: {width: 640, height: 480}}}}, connection: {url: 'rtsp://target2.host'}});

              options = {streamId: 'targetStreamId', video: false, audio: true, resolution: {width: 640, height: 480}, url: 'rtsp://target3.host'};
              client.emit('addExternalOutput', options, function(status, data) {
                expect(status).to.equal('success');
                expect(data.url).to.equal('rtsp://target3.host');
                expect(mockPortal.subscribe.getCall(2).args[0]).to.equal(client.id);
                expect(mockPortal.subscribe.getCall(2).args[1]).to.be.a('string');
                expect(mockPortal.subscribe.getCall(2).args[2]).to.deep.equal({type: 'streaming', media: {audio: {from: 'targetStreamId', format: {codec: 'aac', sampleRate: 48000, channelNum: 2}}, video: false }, connection: {url: 'rtsp://target3.host'}});
                done();
              });
            });
          });
        });
    });

    it('The rtsp/rtmp subscribing description should be adjusted if not in expected format.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host', resolution: 'hd720p'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'streaming', media: {audio: {from: testStream, format: {codec: 'aac', sampleRate: 48000, channelNum: 2}}, video: {from: testStream, format: {codec: 'h264'}, parameters: {resolution: {width: 1280, height: 720}}}}, connection: {url: 'rtsp://target.host'}});
            done();
          });
        });
    });

    it('Starting pulling out rtsp/rtmp streams should fail if the url is invalid.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: ''};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid RTSP/RTMP server url');
            var options1 = {url: 1234567};
            client.emit('addExternalOutput', options1, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid RTSP/RTMP server url');
              var options2 = {url: 'an invalid url'};
              client.emit('addExternalOutput', options2, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid RTSP/RTMP server url');

                var options22 = {url: 'sip://another invalid url'};
                client.emit('addExternalOutput', options22, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid RTSP/RTMP server url');
                  done();
                });
              });
            });
          });
        });
    });

    it('Starting pulling out rtsp/rtmp streams should fail if error occurs during communicating.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', resolution: {width: 640, height: 480}, url: 'rtsp://target.host'};

          client.on('connection_failed', function(arg) {
            expect(arg.url).to.equal(options.url);
            done();
          });

          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            var sub_id = mockPortal.subscribe.getCall(0).args[1];

            server.notify(client.id, 'progress', {status: 'error', id: sub_id, data: 'some-reason'});
          });
        });
    });
  });

  describe('on: updateExternalOutput', function() {
    it('Updating out rtsp/rtmp streams without specifying a valid url should fail.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: ''};
          client.emit('updateExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid RTSP/RTMP server url');
            var options = {url: 7878878};
            client.emit('updateExternalOutput', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid RTSP/RTMP server url');
              expect(mockPortal.unsubscribe.callCount).to.equal(0);
              done();
            });
          });
        });
    });

    it('Updating pulling out rtsp/rtmp streams should succeed if portal.subscriptionControl succeeds.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            var options1 = {url: 'rtsp://target.host', streamId: 'stream1', resolution: {width: 640, height: 480}};
            client.emit('updateExternalOutput', options1, function(status, data) {
              expect(status).to.equal('success');
              expect(data.url).to.equal('rtsp://target.host');
              expect(mockPortal.subscriptionControl.getCall(0).args[0]).to.equal(client.id);
              expect(mockPortal.subscriptionControl.getCall(0).args[1]).to.be.a('string');
              expect(mockPortal.subscriptionControl.getCall(0).args[2]).to.deep.equal({operation: 'update', data: {audio: {from: 'stream1'}, video: {from: 'stream1', parameters: {resolution: {width: 640, height: 480}}}}});
              done();
            });
          });
        });
    });

    it('Updating pulling out rtsp/rtmp streams should fail if portal.subscriptionControl fails.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.rejects('some-error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            var options1 = {url: 'rtsp://target.host', streamId: 'stream1', resolution: {width: 640, height: 480}};
            client.emit('updateExternalOutput', options1, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('some-error');
              done();
            });
          });
        });
    });

  });

  describe('on: removeExternalOutput', function() {
    it('Stopping pulling out rtsp/rtmp streams without specifying a valid url should fail.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: ''};
          client.emit('removeExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid RTSP/RTMP server url');
            var options = {url: 7878878};
            client.emit('removeExternalOutput', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid RTSP/RTMP server url');
              expect(mockPortal.unsubscribe.callCount).to.equal(0);
              done();
            });
          });
        });
    });

    it('Stopping pulling out rtsp/rtmp streams should succeed if portal.unsubscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('removeExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('Streaming-out does NOT exist');
            done();
          });
        });
    });

    it('Stopping pulling out rtsp/rtmp streams should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('Invalid RTSP/RTMP server url');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('removeExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('Streaming-out does NOT exist');
            done();
          });
        });
    });
  });

  describe('on: startRecorder', function() {
    it('Starting recording after joining with proper options should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {audioStreamId: 'targetStreamId-01', videoStreamId: 'targetStreamId-02', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.be.a('string');
            expect(data.path).to.equal(data.recorderId + '.mkv');
            expect(data.host).to.equal('unknown');
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal(client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.equal(data.recorderId);
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'recording', media: {audio: {from: 'targetStreamId-01', format: {codec: 'pcmu'}}, video: {from: 'targetStreamId-02', format: {codec: 'vp8'}}}, connection: {container: 'auto'}});

            options = {audioStreamId: testStream, videoStreamId: testStream, audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('success');
              expect(data.recorderId).to.be.a('string');
              expect(data.path).to.equal(data.recorderId + '.mkv');
              expect(data.host).to.equal('unknown');
              expect(mockPortal.subscribe.getCall(1).args[0]).to.equal(client.id);
              expect(mockPortal.subscribe.getCall(1).args[1]).to.equal(data.recorderId);
              expect(mockPortal.subscribe.getCall(1).args[2]).to.deep.equal({type: 'recording', media: {audio: {from: testStream, format: {codec: 'pcmu'}}, video: {from: testStream, format: {codec: 'vp8'}}}, connection: {container: 'auto'}});
              done();
            });
          });
        });
    });

    it('Exceptions should cause aborting recording and notifying clients.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      var recorder_id;
      client.on('remove_recorder', function(data) {
        expect(data.id).to.equal(recorder_id);
        expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal([client.id, recorder_id]);
        done();
      });

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId2', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            recorder_id = data.recorderId;
            server.notify(client.id, 'progress', {id: recorder_id, status: 'error', data: 'error-reason'});
          });
        });
    });

    it('The recording subscribing description should be adjusted if not in expected format.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {}; // unspecified both audio and video stream-ids.
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.subscribe.getCall(0).args[1]).to.be.a('string');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({type: 'recording', media: {audio: {from: testStream, format: {codec: 'opus', sampleRate:48000, channelNum: 2}}, video: {from: testStream, format: {codec: 'vp8'}}}, connection: {container: 'auto'}});

            var options = {audioStreamId: 'targetStreamId1', audioCodec: 'opus'}; //unspecified video stream-id, audio codec is 'opus'.
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.subscribe.getCall(1).args[2]).to.deep.equal({type: 'recording', media: {audio: {from: 'targetStreamId1', format: {codec: 'opus', sampleRate: 48000, channelNum: 2}}, video: false}, connection: {container: 'auto'}});

              var options = {videoStreamId: 'targetStreamId2', videoCodec: 'h264', interval: 2000}; //unspecified audio stream-id.
              client.emit('startRecorder', options, function(status, data) {
                expect(status).to.equal('success');
                expect(mockPortal.subscribe.getCall(2).args[2]).to.deep.equal({type: 'recording', media: {audio: false, video: {from: 'targetStreamId2', format: {codec: 'h264'}}}, connection: {container: 'auto'}});
                done();
              });
            });
          });
        });
    });

    it('Starting recording with invalid recorderId or codec-names specified should fail.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {recorderId: 987654321};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid recorder id');
            expect(mockPortal.subscribe.callCount).to.equal(0);

            var options = {recorderId: 0};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid recorder id');
              expect(mockPortal.subscribe.callCount).to.equal(0);

              var options = {recorderId: -1};
              client.emit('startRecorder', options, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid recorder id');
                expect(mockPortal.subscribe.callCount).to.equal(0);

                var options = {audioStreamId: 'targetStreamId', audioCodec: 24};
                client.emit('startRecorder', options, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid audio codec');
                  expect(mockPortal.subscribe.callCount).to.equal(0);

                  var options = {videoStreamId: 'targetStreamId', videoCodec: {a: 1}};
                  client.emit('startRecorder', options, function(status, data) {
                    expect(status).to.equal('error');
                    expect(data).to.equal('Invalid video codec');
                    expect(mockPortal.subscribe.callCount).to.equal(0);

                    var options = {recorderId: 'aaaa', audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId1', audioCodec: '', videoCodec: 'vp8'}; //
                    client.emit('startRecorder', options, function(status, data) {
                      expect(status).to.equal('error');
                      expect(data).to.equal('Invalid audio codec');
                      expect(mockPortal.subscribe.callCount).to.equal(0);

                      var options = {recorderId: 'aaaa', audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId1', audioCodec: 'opus', videoCodec: ''}; //
                      client.emit('startRecorder', options, function(status, data) {
                        expect(status).to.equal('error');
                        expect(data).to.equal('Invalid video codec');
                        expect(mockPortal.subscribe.callCount).to.equal(0);
                        done();
                      });
                    });
                  });
                });
              });
            });
          });
        });
    });

    describe('Specifying options.recorderId will be considered as a contineous recording', function() {
      it('Contineous recording should succeed if portal.subscriptionControl succeeds', function(done) {
        mockPortal.subscriptionControl = sinon.stub();
        mockPortal.subscriptionControl.resolves('ok');

        return joinFirstly()
          .then(function(result) {
            var options = {audioStreamId: 'targetStreamId-01', videoStreamId: 'targetStreamId-02', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('success');
              expect(data.recorderId).to.be.a('string');
              var recorder_id = data.recorderId;
              var options = {recorderId: recorder_id, audioStreamId: 'stream1'};
              client.emit('startRecorder', options, function(status, data) {
                expect(status).to.equal('success');
                expect(data.recorderId).to.equal(recorder_id);
                expect(mockPortal.subscriptionControl.getCall(0).args).to.deep.equal([client.id, recorder_id, {operation: 'update', data: {audio: {from: 'stream1'}}}]);

                var options = {recorderId: recorder_id, videoStreamId: 'stream2'};
                client.emit('startRecorder', options, function(status, data) {
                  expect(status).to.equal('success');
                  expect(data.recorderId).to.equal(recorder_id);
                  expect(mockPortal.subscriptionControl.getCall(1).args).to.deep.equal([client.id, recorder_id, {operation: 'update', data: {video: {from: 'stream2'}}}]);
                  done();
                });
              });
            });
          });
      });

      it('Contineous recording should fail if portal.subscriptionControl fails', function() {
        mockPortal.subscriptionControl = sinon.stub();
        mockPortal.subscriptionControl.rejects('some-error');

        var recorder_id = 'recorderId';
        return joinFirstly()
          .then(function(result) {
            expect(result).to.equal('ok');
            var options = {recorderId: recorder_id, audioStreamId: 'stream1'};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('some-error');
              done();
            });
          });
      });

      it('Contineous recording should fail if neither audioStreamId nor videoStreamId are specified', function() {
        mockPortal.subscriptionControl = sinon.stub();
        mockPortal.subscriptionControl.rejects('some-error');

        var recorder_id = 'recorderId';
        return joinFirstly()
          .then(function(result) {
            expect(result).to.equal('ok');
            var options = {recorderId: recorder_id, audioCodec: 'aac'};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Neither audio.from nor video.from was specified');
              expect(mockPortal.subscriptionControl.callCount).to.equal(0);
              done();
            });
          });
      });
    });
  });

  describe('on: stopRecorder', function() {
    it('Stopping recording without specifying a valid id should fail.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {recorderId: ''};
          client.emit('stopRecorder', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid recorder id');
            var options = {recorderId: 7878878};
            client.emit('stopRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid recorder id');
              expect(mockPortal.unsubscribe.callCount).to.equal(0);
              done();
            });
          });
        });
    });

    it('Stopping recording should succeed if portal.unsubscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {audioStreamId: 'targetStreamId-01', videoStreamId: 'targetStreamId-02', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.be.a('string');
            var recorder_id = data.recorderId;
            var options = {recorderId: recorder_id};
            client.emit('stopRecorder', options, function(status, data) {
              expect(status).to.equal('success');
              expect(data.recorderId).to.equal(recorder_id);
              expect(data.host).to.equal('unknown');
              expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal([client.id, recorder_id]);
              done();
            });
          });
        });
    });

    it('Stopping recording should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('Invalid recorder id');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {audioStreamId: 'targetStreamId-01', videoStreamId: 'targetStreamId-02', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.be.a('string');
            var recorder_id = data.recorderId;
            var options = {recorderId: recorder_id};
            client.emit('stopRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('Invalid recorder id');
              done();
            });
          });
        });
    });
  });

  describe('on: mute, unmute', function() {
    it('mute/unmute without specifying streamId should fail.', function(done) {
      mockPortal.streamControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('mute', { track: 'video' }, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('no stream ID');
            expect(mockPortal.streamControl.callCount).to.equal(0);

            client.emit('unmute', { track: 'video' }, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('no stream ID');
              expect(mockPortal.streamControl.callCount).to.equal(0);
              done();
            });
          });
        });
    });

    it('mute/unmute without specifying track should fail.', function(done) {
      mockPortal.streamControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('mute', { streamId: 'subStreamId' }, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('invalid track undefined');
            expect(mockPortal.streamControl.callCount).to.equal(0);

            client.emit('unmute', { streamId: 'subStreamId' }, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('invalid track undefined');
              expect(mockPortal.streamControl.callCount).to.equal(0);
              done();
            });
          });
        });
    });

    it('mute/unmute should succeed if portal.streamControl succeeds.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('mute', {streamId: 'subStreamId', track: 'video'}, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'pause', data: 'video'}]);

            client.emit('unmute', {streamId: 'subStreamId', track: 'video'}, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'subStreamId', {operation: 'play', data: 'video'}]);
              done();
            });
          });
        });
    });

    it('mute/unmute should fail if portal.streamControl fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('Permission Denied.');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('mute', {streamId: 'subStreamId', track: 'video'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('Permission Denied.');

            client.emit('unmute', {streamId: 'subStreamId', track: 'video'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('Permission Denied.');
              done();
            });
          });
        });
    });
  });

  describe('on: getRegion, setRegion', function() {
    it('Getting/setting region with invalid stream-id or region-id should fail.', function(done) {
      mockPortal.streamControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('getRegion', {id: 1234}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid stream id');
            expect(mockPortal.streamControl.callCount).to.equal(0);

            client.emit('setRegion', {id: {a: 56}, region: 'regionId'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid stream id');
              expect(mockPortal.streamControl.callCount).to.equal(0);

              client.emit('getRegion', {id: ''}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid stream id');
                expect(mockPortal.streamControl.callCount).to.equal(0);

                client.emit('setRegion', {id: 'subStreamId', region: 789}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid region id');
                  expect(mockPortal.streamControl.callCount).to.equal(0);

                  client.emit('setRegion', {id: 'subStreamId', region: ''}, function(status, data) {
                    expect(status).to.equal('error');
                    expect(data).to.equal('Invalid region id');
                    expect(mockPortal.streamControl.callCount).to.equal(0);
                    done();
                  });
                });
              });
            });
          });
        });
    });

    it('Getting/setting region should succeed if portal.streamControl succeeds.', function(done) {
      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          mockPortal.streamControl = sinon.stub();
          mockPortal.streamControl.resolves({region: 'regionId'});
          client.emit('getRegion', {id: 'subStreamId'}, function(status, data) {
            expect(status).to.equal('success');
            expect(data.region).to.equal('regionId');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'get-region', data: 'common'}]);

            mockPortal.streamControl = sinon.stub();
            mockPortal.streamControl.resolves('ok');
            client.emit('setRegion', {id: 'subStreamId', region: '3'}, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'set-region', data: {region: '3', view: 'common'}}]);

              mockPortal.streamControl = sinon.stub();
              mockPortal.streamControl.resolves({region: 'regionId'});
              client.emit('getRegion', {id: 'subStreamId', mixStreamId: 'mix-view1'}, function(status, data) {
                expect(status).to.equal('success');
                expect(data.region).to.equal('regionId');
                expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'get-region', data: 'view1'}]);

                mockPortal.streamControl = sinon.stub();
                mockPortal.streamControl.resolves('ok');
                client.emit('setRegion', {id: 'subStreamId', region: '3', mixStreamId: 'mix-view2'}, function(status, data) {
                  expect(status).to.equal('success');
                  expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'set-region', data: {region: '3', view: 'view2'}}]);
                  done();
                });
              });
            });
          });
        });
    });

    it('Getting/setting region should fail if portal.getRegion/portal.setRegion fails.', function(done) {
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('some error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('getRegion', {id: 'subStreamId'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('some error');
            expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, 'subStreamId', {operation: 'get-region', data: 'common'}]);

            client.emit('setRegion', {id: 'subStreamId', region: '3'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('some error');
              expect(mockPortal.streamControl.getCall(1).args).to.deep.equal([client.id, 'subStreamId', {operation: 'set-region', data: {region: '3', view: 'common'}}]);
              done();
            });
          });
        });
    });
  });

  describe('Receive message from conference', function() {
    it('Should emit event for text message', function(done) {

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.on('custom_message', function (data) {
            expect(data).to.deep.equal({from: "fromUser", to: "toUser", data: "messageContent"});
            done();
          });

          server.notify(client.id, 'text', {from: "fromUser", to: "toUser", message: "messageContent"});
        });
    });
  });

  describe('on: customMessage', function() {
    it('Should fail if message type is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.streamControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 9898, receiver: 'all', data: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid message type');

            client.emit('customMessage', {type: {obj: true}, payload: {streamId: 'streamId', action: 'video-out-off'}}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid message type');

              client.emit('customMessage', {type: 'some-other-message', payload: {streamId: 'streamId', action: 'video-out-off'}}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid message type');
                expect(mockPortal.text.callCount).to.equal(0);
                expect(mockPortal.streamControl.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });

    it('Should fail if message type is #data# and receiver is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.streamControl = sinon.stub();
      mockPortal.subscriptionControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'data', receiver: '', data: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid receiver');

            client.emit('customMessage', {type: 'data', receiver: 54321}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid receiver');

              client.emit('customMessage', {type: 'data', receiver: {obj: true}}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid receiver');
                expect(mockPortal.text.callCount).to.equal(0);
                expect(mockPortal.streamControl.callCount).to.equal(0);
                expect(mockPortal.subscriptionControl.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });

    it('Should fail if message type is #control# and payload is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.streamControl = sinon.stub();
      mockPortal.subscriptionControl = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'control', payload: 'a-string'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid payload');

            client.emit('customMessage', {type: 'control', payload: 98765}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid payload');

              client.emit('customMessage', {type: 'control', payload: {streamId: 246}}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid connection id');

                client.emit('customMessage', {type: 'control', payload: {streamId: ''}}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid connection id');

                  client.emit('customMessage', {type: 'control', payload: {streamId: {obj: true}}}, function(status, data) {
                    expect(status).to.equal('error');
                    expect(data).to.equal('Invalid connection id');

                    client.emit('customMessage', {type: 'control', payload: {streamId: 'connectionId', action: 1357}}, function(status, data) {
                      expect(status).to.equal('error');
                      expect(data).to.equal('Invalid action');

                      client.emit('customMessage', {type: 'control', payload: {streamId: 'connectionId', action: {obj: true}}}, function(status, data) {
                        expect(status).to.equal('error');
                        expect(data).to.equal('Invalid action');

                        client.emit('customMessage', {type: 'control', payload: {streamId: 'connectionId', action: ''}}, function(status, data) {
                          expect(status).to.equal('error');
                          expect(data).to.equal('Invalid action');

                          client.emit('customMessage', {type: 'control', payload: {streamId: 'connectionId', action: 'not-a-normal-action'}}, function(status, data) {
                            expect(status).to.equal('error');
                            expect(data).to.equal('Invalid action');
                            expect(mockPortal.text.callCount).to.equal(0);
                            expect(mockPortal.streamControl.callCount).to.equal(0);
                            expect(mockPortal.subscriptionControl.callCount).to.equal(0);
                            done();
                          });
                        });
                      });
                    });
                  });
                });
              });
            });
          });
        });
    });

    it('Should succeed if message type is #data# and portal.text succeeds.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.text.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'data', receiver: 'all', data: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.text.getCall(0).args).to.deep.equal([client.id, 'all', 'Hi, there!']);
            done();
          });
        });
    });

    it('Should fail if message type is #data# and portal.text fails.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.text.rejects('some-error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'data', receiver: 'all', data: 'Hi, there!'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('some-error');
            done();
          });
        });
    });

    it('Should succeed if message type is #control# and portal.streamControl/portal.subscriptionControl succeeds.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('ok');
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.resolves('ok');
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'streamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            var sub_id = mockPortal.subscribe.getCall(0).args[1];
            client.emit('customMessage', {type: 'control', payload: {streamId: 'streamId', action: 'video-in-off'}}, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.subscriptionControl.getCall(0).args).to.deep.equal([client.id, sub_id, {operation: 'pause', data: 'video'}]);
              expect(mockPortal.streamControl.callCount).to.equal(0);
              var options = {state: 'erizo', audio: true, video: {device: 'camera'}};
              client.emit('publish', options, undefined, function(status, id) {
                expect(id).to.be.a('string');
                var pub_id = id;
                client.emit('customMessage', {type: 'control', payload: {streamId: pub_id, action: 'video-out-off'}}, function(status, data) {
                  expect(status).to.equal('success');
                  expect(mockPortal.streamControl.getCall(0).args).to.deep.equal([client.id, pub_id, {operation: 'pause', data: 'video'}]);
                  done();
                });
              });
            });
          });
        });
    });

    it('Should fail if message type is #control# and portal.streamControl/portal.subscriptionControl fails.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('ok');
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('ok');
      mockPortal.streamControl = sinon.stub();
      mockPortal.streamControl.rejects('some-error');
      mockPortal.subscriptionControl = sinon.stub();
      mockPortal.subscriptionControl.rejects('some-error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'streamId', audio: true, video: {resolution: {width: 640, height: 480}, quality_level: 'standard'}};
          client.emit('subscribe', options, undefined, function(status, id) {
            expect(status).to.equal('initializing');
            expect(id).to.be.a('string');
            client.emit('customMessage', {type: 'control', payload: {streamId: 'streamId', action: 'video-in-off'}}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('some-error');
              var options = {state: 'erizo', audio: true, video: {device: 'camera'}};
              client.emit('publish', options, undefined, function(status, id) {
                expect(id).to.be.a('string');
                var pub_id = id;
                client.emit('customMessage', {type: 'control', payload: {streamId: pub_id, action: 'video-out-off'}}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.have.string('some-error');
                  done();
                });
              });
            });
          });
        });
    });
  });
});
