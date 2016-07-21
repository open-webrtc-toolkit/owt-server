
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var socketIOServer = require('../socketIOServer');
var sioClient = require('socket.io-client');
var portal = require('../portal');

var testRoom = '573eab78111478bb3526421a';

describe('Clients connect to socket.io server.', function() {
  it('Connecting to an insecure socket.io server should succeed.', function(done) {
    var server = socketIOServer({port: 3001, ssl: false});

    return server.start()
      .then(function(result) {
        expect(result).to.equal('ok');

        var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});
        client.on('connect', function() {
          expect(true).to.be.true;
          server.stop();
          done();
        });
      });
  });

  it('Connecting to a secured socket.io server should succeed.', function(done) {
    var server = socketIOServer({port: 3004, ssl: true, keystorePath: './cert/certificate.pfx'});

    return server.start()
      .then(function(result) {
        expect(result).to.equal('ok');

        var https = require('https');
        https.globalAgent.options.rejectUnauthorized = false;
        var client = sioClient.connect('https://localhost:3004', {reconnect: false, secure: true, 'force new connection': true, agent: https.globalAgent});
        client.on('connect', function() {
          expect(true).to.be.true;
          server.stop();
          done();
        });
      })
      .catch(function(err) {
        done();
      });
  });
});

describe('Notifying events to clients.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var server = socketIOServer({port: 3001, ssl: false}, mockPortal);
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
    return expect(server.notify('/#112233445566', 'event', 'data')).to.be.rejectedWith('participant does not exist');
  });

  it('Notifying the joined clients should succeed.', function(done) {
    var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});

    client.on('add_stream', function(data) {
      expect(data).to.deep.equal({audio: true, video: {resolution: 'vga', device: 'camera'}});
      done();
    });

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      client.emit('token', 'someValidToken', function(status, resp) {
        expect(status).to.equal('success');
        return expect(server.notify('/#' + client.id, 'add_stream', {audio: true, video: {resolution: 'vga', device: 'camera'}})).to.become('ok');
      });
    });
  });
});

describe('Drop users from sessions.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  mockPortal.leave = sinon.stub();
  mockPortal.leave.resolves('ok');

  var server = socketIOServer({port: 3001, ssl: false}, mockPortal);
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

  it('Dropping unconnected users should fail.', function() {
    return expect(server.drop('unconnectedUser', 'anyRoom')).to.be.rejectedWith('user not in room');
  });

  it('Dropping users from rooms they are not in should fail.', function(done) {
    var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      mockPortal.join.rejects('user not in room');

      client.emit('token', 'someValidToken', function(status, resp) {
        return server.drop('Jack', 'someOtherEoomJackNotIn')
          .then(function(runInHere) {
            expect(runInHere).to.be.false;
          }, function(err) {
            expect(err).to.have.string('user not in room');
            done();
          });
      });
    });
  });

  it('Dropping users from rooms they are in should succeed.', function(done) {
    var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      client.emit('token', 'someValidToken', function(status, resp) {
        return server.drop('/#' + client.id, testRoom)
          .then(function(result) {
            expect(result).to.equal('ok');
            done();
          });
      });
    });
  });

  it('Dropping users from all rooms they are in should succeed.', function(done) {
    var client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});

    client.on('connect', function() {
      mockPortal.join = sinon.stub();
      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      client.emit('token', 'someValidToken', function(status, resp) {
        return server.drop('/#' + client.id)
          .then(function(result) {
            expect(result).to.equal('ok');
            done();
          });
      });
    });
  });
});

describe('Responding to clients.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var server = socketIOServer({port: 3001, ssl: false}, mockPortal);
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
    client = sioClient.connect('http://localhost:3001', {reconnect: false, secure: false, 'force new connection': true});
    client.on('connect', function() {
      done();
    });
  });

  afterEach('Clients disconnect.', function() {
    mockPortal.leave = sinon.stub();
    mockPortal.leave.resolves('ok');
    client.close();
  });

  function joinFirstly() {
    return new Promise(function(resolve, reject) {
      mockPortal.join = sinon.stub();

      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      client.emit('token', 'someValidToken', function(status, resp) {
        expect(status).to.equal('success');
        resolve('ok');
      });
    });
  }

  function simulateStubResponse(stubbedMethod, callCount, responderPosition, response) {
    var count = 0, interval = setInterval(function() {
      var temp = stubbedMethod.getCall(callCount);
      if (temp && temp.args && (typeof temp.args[responderPosition] === 'function')) {
        temp.args[responderPosition](response);
        clearInterval(interval);
      } else if (count > 300) {
        clearInterval(interval);
      } else {
        count += 1;
      }
    }, 5);
  }

  describe('on: token', function() {
    it('Joining with valid tokens should succeed.', function(done) {
      mockPortal.join = sinon.stub();

      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: ['vag', 'hd720p']}, from: '', socket: ''}]};
      mockPortal.join.resolves(join_result);

      var transformed_streams = [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: [{width: 640, height: 480}, {width: 1280, height: 720}]}, from: '', socket: ''}];

      client.emit('token', 'someValidToken', function(status, resp) {
        expect(status).to.equal('success');
        expect(mockPortal.join.getCall(0).args).to.deep.equal(['/#' + client.id, 'someValidToken']);
        expect(resp).to.deep.equal({id: join_result.session_id, clientId: '/#' + client.id, streams: transformed_streams, users: join_result.participants});
        done();
      });
    });

    it('Joining with invalid tokens should fail and cause disconnection.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.rejects('invalid token');

      client.on('disconnect', function() {
        done();
      });

      client.emit('token', 'someInvalidToken', function(status, resp) {
        expect(status).to.equal('error');
        expect(resp).to.have.string('invalid token');
        //done();
      });
    });
  });

  describe('on: publish, signaling_message', function() {
    it('Publishing streams without joining should fail.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('streamId');

      client.emit('publish', {}, undefined, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        done();
      });
    });

    it('Publishing webrtc streams after joining should succeed.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.onConnectionSignalling = sinon.stub();

      mockPortal.publish.resolves('streamId');
      mockPortal.onConnectionSignalling.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {state: 'erizo', audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}};
          client.emit('publish', options, undefined, function(status, id) {
            var observer;
            if (status === 'initializing') {
               expect(id).to.equal('streamId');
               expect(mockPortal.publish.getCall(0).args[0]).to.equal('/#' + client.id);
               expect(mockPortal.publish.getCall(0).args[1]).to.equal('webrtc');
               expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}});
               observer = mockPortal.publish.getCall(0).args[3];
               expect(observer).to.be.a('function');
               expect(mockPortal.publish.getCall(0).args[4]).to.be.false;
               client.emit('signaling_message', {streamId: 'streamId', msg: {type: 'offer', sdp: 'offerSDPString'}}, undefined, function() {
                 expect(mockPortal.onConnectionSignalling.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId', {type: 'offer', sdp: 'offerSDPString'}]);
                 client.emit('signaling_message', {streamId: 'streamId', msg: {type: 'candidate', sdp: 'candidateString'}}, undefined, function() {
                   expect(mockPortal.onConnectionSignalling.getCall(1).args).to.deep.equal(['/#' + client.id, 'streamId', {type: 'candidate', sdp: 'candidateString'}]);
                   observer({type: 'ready', audio_codecs: ['opus'], video_codecs: ['vp8']});
                 });
               });
            } else if (status === 'error') {
              done();
            }
          });

          client.on('signaling_message_erizo', function(arg) {
            if (arg.mess.type === 'ready') {
              done();
            }
          });
        });
    });

    it('Publishing rtsp/rtmp streams after joining should succeed.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('streamId');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.publish, 0, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264']});
          var options = {state: 'url', audio: true, video: true, transport: 'tcp', bufferSize: 2048, unmix: true};
          client.emit('publish', options, 'urlOfRtspOrRtmpSource', function(status, id) {
            expect(mockPortal.publish.getCall(0).args[0]).to.equal('/#' + client.id);
            expect(mockPortal.publish.getCall(0).args[1]).to.equal('avstream');
            expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'urlOfRtspOrRtmpSource', transport: 'tcp', bufferSize: 2048});
            expect(mockPortal.publish.getCall(0).args[3]).to.be.a('function');
            expect(mockPortal.publish.getCall(0).args[4]).to.be.true;
            if (status === 'success') {
              expect(id).to.equal('streamId');
              done();
            }
          });
        });
    });

    it('The stream description should be adjusted if not in expected format while publishing.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.onConnectionSignalling = sinon.stub();

      mockPortal.publish.resolves('streamId');
      mockPortal.onConnectionSignalling.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options1 = {state: 'erizo'};
          client.emit('publish', options1, undefined, function(status, id) {
            if (status === 'initializing') {
               expect(mockPortal.publish.getCall(0).args[2]).to.deep.equal({audio: true, video: {resolution: 'unknown', device: 'unknown'}});

               var options2 = {state: 'erizo', audio: false, video: true, unmix: true};
               client.emit('publish', options2, undefined, function(status, id) {
                 if (status === 'initializing') {
                    expect(mockPortal.publish.getCall(1).args[2]).to.deep.equal({audio: false, video: {resolution: 'unknown', device: 'unknown'}});
                    expect(mockPortal.publish.getCall(1).args[4]).to.be.true;

                    var options3 = {state: 'erizo', audio: true, video: false};
                    client.emit('publish', options3, undefined, function(status, id) {
                      if (status === 'initializing') {
                         expect(mockPortal.publish.getCall(2).args[2]).to.deep.equal({audio: true, video: false});

                         var options4 = {state: 'erizo', audio: false, video: {resolution: 'vga', device: 'screen'}};
                         client.emit('publish', options4, undefined, function(status, id) {
                           if (status === 'initializing') {
                              expect(mockPortal.publish.getCall(3).args[2]).to.deep.equal({audio: false, video: {resolution: 'vga', device: 'screen'}});
                              expect(mockPortal.publish.getCall(3).args[4]).to.be.true;

                              var options5 = {state: 'erizo', audio: true, video: {resolution: '', device: 'camera'}};
                              client.emit('publish', options5, undefined, function(status, id) {
                                if (status === 'initializing') {
                                   expect(mockPortal.publish.getCall(4).args[2]).to.deep.equal({audio: true, video: {resolution: 'unknown', device: 'camera'}});
                                   expect(mockPortal.publish.getCall(4).args[4]).to.be.false;
                                   done();
                                 }
                              });
                            }
                         });
                      }
                    });
                 }
               });
            }
          });
        });
    });
  });

  describe('on: unpublish', function() {
    it('Unpublishing streams without joining should fail.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.resolves('ok');

      client.emit('unpublish', 'streamId', function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.unpublish.callCount).to.equal(0);
        done();
      });
    });

    it('Unpublishing streams should succeed if portal.unpublish succeeds.', function(done) {
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unpublish', 'previouslyPublishedStreamId', function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal(['/#' + client.id, 'previouslyPublishedStreamId']);
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
            expect(mockPortal.unpublish.getCall(0).args).to.deep.equal(['/#' + client.id, 'non-ExistStreamId']);
            done();
          });
        });
    });
  });

  describe('on: setVideoBitrate', function() {
    it('Setting video bitrate without joining should fail.', function(done) {
      mockPortal.setVideoBitrate = sinon.stub();
      mockPortal.setVideoBitrate.resolves('ok');

      var options = {id: 'streamId', bitrate: 500};
      client.emit('setVideoBitrate', options, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.setVideoBitrate.callCount).to.equal(0);
        done();
      });
    });

    it('Setting video bitrate should succeed if portal.unpublish succeeds.', function(done) {
      mockPortal.setVideoBitrate = sinon.stub();
      mockPortal.setVideoBitrate.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {id: 'streamId', bitrate: 500};
          client.emit('setVideoBitrate', options, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.setVideoBitrate.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId', 500]);
            done();
          });
        });
    });

    it('Setting video bitrate should fail if portal.unpublish fails.', function(done) {
      mockPortal.setVideoBitrate = sinon.stub();
      mockPortal.setVideoBitrate.rejects('stream does not exist');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {id: 'streamId', bitrate: 500};
          client.emit('setVideoBitrate', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('stream does not exist');
            expect(mockPortal.setVideoBitrate.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId', 500]);
            done();
          });
        });
    });
  });

  describe('on: addToMixer, removeFromMixer', function() {
    it('Mixing/unmixing streams without joining should fail.', function(done) {
      mockPortal.mix = sinon.stub();
      mockPortal.unmix = sinon.stub();

      client.emit('addToMixer', 'streamId', function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.mix.callCount).to.equal(0);

        client.emit('removeFromMixer', 'streamId', function(status, data) {
          expect(status).to.equal('error');
          expect(data).to.equal('unauthorized');
          expect(mockPortal.unmix.callCount).to.equal(0);
          done();
        });
      });
    });

    it('Mixing/unmixing streams should succeed if portal.mix/portal.unmix suceeds.', function(done) {
      mockPortal.mix = sinon.stub();
      mockPortal.unmix = sinon.stub();

      mockPortal.mix.resolves('ok');
      mockPortal.unmix.resolves('ok');

      return joinFirstly()
        .then(function() {
          client.emit('addToMixer', 'streamId', function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.mix.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId']);

            client.emit('removeFromMixer', 'streamId', function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.unmix.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId']);
              done();
            });
          });
      });
    });

    it('Mixing/unmixing streams should fail if portal.mix/portal.unmix fails.', function(done) {
      mockPortal.mix = sinon.stub();
      mockPortal.unmix = sinon.stub();

      mockPortal.mix.rejects('stream does not exist');
      mockPortal.unmix.rejects('stream does not exist');

      return joinFirstly()
        .then(function() {
          client.emit('addToMixer', 'streamId', function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('stream does not exist');
            expect(mockPortal.mix.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId']);

            client.emit('removeFromMixer', 'streamId', function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('stream does not exist');
              expect(mockPortal.unmix.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId']);
              done();
            });
          });
      });
    });
  });

  describe('on: subscribe, signaling_message', function() {
    it('Subscribing without joining should fail.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('subscriptionId');

      client.emit('subscribe', {}, undefined, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        done();
      });
    });

    it('Subscribing after joining should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.onConnectionSignalling = sinon.stub();

      mockPortal.subscribe.resolves('subscriptionId');
      mockPortal.onConnectionSignalling.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}}};
          client.emit('subscribe', options, undefined, function(status, id) {
            var observer;
            if (status === 'initializing') {
               expect(id).to.equal('subscriptionId');
               expect(mockPortal.subscribe.getCall(0).args[0]).to.equal('/#' + client.id);
               expect(mockPortal.subscribe.getCall(0).args[1]).to.equal('webrtc');
               expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}});
               observer = mockPortal.subscribe.getCall(0).args[3];
               expect(observer).to.be.a('function');
               client.emit('signaling_message', {streamId: 'subscriptionId', msg: {type: 'offer', sdp: 'offerSDPString'}}, undefined, function() {
                 expect(mockPortal.onConnectionSignalling.getCall(0).args).to.deep.equal(['/#' + client.id, 'subscriptionId', {type: 'offer', sdp: 'offerSDPString'}]);
                 client.emit('signaling_message', {streamId: 'subscriptionId', msg: {type: 'candidate', sdp: 'candidateString'}}, undefined, function() {
                   expect(mockPortal.onConnectionSignalling.getCall(1).args).to.deep.equal(['/#' + client.id, 'subscriptionId', {type: 'candidate', sdp: 'candidateString'}]);
                   observer({type: 'ready', audio_codecs: ['opus'], video_codecs: ['vp8']});
                 });
               });
            } else if (status === 'error') {
              done();
            }
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
      mockPortal.onConnectionSignalling = sinon.stub();

      mockPortal.subscribe.resolves('subscriptionId');
      mockPortal.onConnectionSignalling.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {streamId: 'targetStreamId', audio: true, video: {resolution: {width: 640, height: 480}}};
          client.emit('subscribe', options, undefined, function(status, id) {
            if (status === 'initializing') {
              expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}});
              var options1 = {streamId: 'targetStreamId1', video: {resolution: {width: 640, height: 480}}};
              client.emit('subscribe', options1, undefined, function(status, id) {
                if (status === 'initializing') {
                  expect(mockPortal.subscribe.getCall(1).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId1'}, video: {fromStream: 'targetStreamId1', resolution: 'vga'}});
                  done();
                }
              });
            }
          });
        });
    });
  });

  describe('on: unsubscribe', function() {
    it('Unsubscribing without joining should fail.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      client.emit('unsubscribe', 'subscriptionId', function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.unsubscribe.callCount).to.equal(0);
        done();
      });
    });

    it('Unsubscribing should succeed if portal.unsubscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unsubscribe', 'previouslySubscribedSubscriptionId', function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, '/#' + client.id + '-sub-' + 'previouslySubscribedSubscriptionId']);
            done();
          });
        });
    });

    it('Unsubscribing should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('subscription does not exist');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('unsubscribe', 'non-ExistSubscriptionId', function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('subscription does not exist');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, '/#' + client.id + '-sub-' + 'non-ExistSubscriptionId']);
            done();
          });
        });
    });
  });

  describe('on: addExternalOutput', function() {
    it('Starting pulling out rtsp/rtmp streams should fail before joining.', function(done) {
      mockPortal.subscribe = sinon.stub();

      client.emit('addExternalOutput', {}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.subscribe.callCount).to.equal(0);
        done();
      });
    });

    it('Starting pulling out rtsp/rtmp streams with proper options after joining should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('subscriptionId');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.subscribe, 0, 3, {type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
          var options = {streamId: 'targetStreamId', resolution: {width: 640, height: 480}, url: 'rtsp://target.host'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal('/#' + client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.equal('avstream');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId', codecs: ['aac']}, video: {fromStream: 'targetStreamId', codecs: ['h264'], resolution: 'vga'}, url: 'rtsp://target.host'});
            done();
          });
        });
    });

    it('The rtsp/rtmp subscribing description should be adjusted if not in expected format.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('subscriptionId');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.subscribe, 0, 3, {type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
          var options = {url: 'rtsp://target.host'};
          client.emit('addExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: testRoom, codecs: ['aac']}, video: {fromStream: testRoom, codecs: ['h264']}, url: 'rtsp://target.host'});
            done();
          });
        });
    });

    it('Starting pulling out rtsp/rtmp streams should fail if the url is invalid.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('subscriptionId');

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
              var options2 = {url: 'a invalid url'};
              client.emit('addExternalOutput', options2, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid RTSP/RTMP server url');
                done();
              });
            });
          });
        });
    });
  });

  describe('on: updateExternalOutput', function() {
    it('Updating out rtsp/rtmp streams should fail before joining.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      client.emit('updateExternalOutput', {}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.unsubscribe.callCount).to.equal(0);
        done();
      });
    });

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

    it('Updating pulling out rtsp/rtmp streams should fail if no previous stream on the specfied url.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('subscription does not exist');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('updateExternalOutput', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('subscription does not exist');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'rtsp://target.host'])
            done();
          });
        });
    });

    it('Updating pulling out rtsp/rtmp streams should succeed if portal.unsubscribe/subscribe succeeds.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {url: 'rtsp://target.host'};
          client.emit('updateExternalOutput', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'rtsp://target.host'])
            done();
          });
        });
    });

  });

  describe('on: removeExternalOutput', function() {
    it('Stopping pulling out rtsp/rtmp streams should fail before joining.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      client.emit('removeExternalOutput', {}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.unsubscribe.callCount).to.equal(0);
        done();
      });
    });

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
            expect(status).to.equal('success');
            expect(data.url).to.equal('rtsp://target.host');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'rtsp://target.host'])
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
            expect(data).to.equal('Invalid RTSP/RTMP server url');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'rtsp://target.host'])
            done();
          });
        });
    });
  });

  describe('on: startRecorder', function() {
    it('Starting recording should fail before joining.', function(done) {
      mockPortal.subscribe = sinon.stub();

      client.emit('startRecorder', {}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.subscribe.callCount).to.equal(0);
        done();
      });
    });

    it('Starting recording after joining with proper options should succeed.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('yyyyMMddhhmmssSS');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.subscribe, 0, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
          var options = {audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId2', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.equal('yyyyMMddhhmmssSS');
            expect(data.path).to.equal('/tmp/room_' + testRoom + '-yyyyMMddhhmmssSS.mkv');
            expect(data.host).to.equal('unknown');
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal('/#' + client.id);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.equal('recording');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: '/tmp', interval: 1000});
            done();
          });
        });
    });

    it('Starting recording after joining with inproper options should fail.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('yyyyMMddhhmmssSS');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {audioStreamId: '', videoStreamId: 'targetStreamId', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid audio stream id');

            var options = {audioStreamId: 'targetStreamId', videoStreamId: '', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid video stream id');

              var options = {audioStreamId: 'targetStreamId', videoStreamId: 'targetStreamId', audioCodec: '', videoCodec: 'vp8', path: '/tmp', interval: 1000};
              client.emit('startRecorder', options, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid audio codec');

                var options = {audioStreamId: 'targetStreamId', videoStreamId: 'targetStreamId', audioCodec: 'opus', videoCodec: '', path: '/tmp', interval: 1000};
                client.emit('startRecorder', options, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid video codec');
                  done();
                });
              });
            });
          });
        });
    });

    it('Exceptions should cause aborting recording and notifying clients.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('yyyyMMddhhmmssSS');
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      client.on('remove_recorder', function(data) {
        expect(data.id).to.equal('yyyyMMddhhmmssSS');
        done();
      });

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.subscribe, 0, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
          var observer;
          var options = {audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId2', audioCodec: 'pcmu', videoCodec: 'vp8', path: '/tmp', interval: 1000};
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.equal('yyyyMMddhhmmssSS');
            observer = mockPortal.subscribe.getCall(0).args[3];
            expect(observer).to.be.a('function');
            observer({type: 'failed', reason: 'write header error'});
          });
        });
    });

    it('The recording subscribing description should be adjusted if not in expected format.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('yyyyMMddhhmmssSS');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          simulateStubResponse(mockPortal.subscribe, 0, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
          var options = {}; // unspecified both audio and video stream-ids.
          client.emit('startRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.subscribe.getCall(0).args[2]).to.deep.equal({audio: {fromStream: testRoom, codecs: ['pcmu']}, video: {fromStream: testRoom, codecs: ['vp8']}, interval: -1});

            simulateStubResponse(mockPortal.subscribe, 1, 3, {type: 'ready', audio_codecs: ['opus_48000_2'], video_codecs: ['vp8']});
            var options = {audioStreamId: 'targetStreamId1', audioCodec: 'opus'}; //unspecified video stream-id, audio codec is 'opus'.
            client.emit('startRecorder', options, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.subscribe.getCall(1).args[2]).to.deep.equal({audio: {fromStream: 'targetStreamId1', codecs: ['opus_48000_2']}, video: false, interval: -1});

              simulateStubResponse(mockPortal.subscribe, 2, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
              var options = {videoStreamId: 'targetStreamId2', videoCodec: 'h264', interval: 2000}; //unspecified audio stream-id.
              client.emit('startRecorder', options, function(status, data) {
                expect(status).to.equal('success');
                expect(mockPortal.subscribe.getCall(2).args[2]).to.deep.equal({audio: false, video: {fromStream: 'targetStreamId2', codecs: ['h264']}, interval: 2000});

                simulateStubResponse(mockPortal.subscribe, 3, 3, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
                var options = {videoCodec: 'h264', recorderId: '2016060418215098', interval: -6}; //invalid interval.
                client.emit('startRecorder', options, function(status, data) {
                  expect(status).to.equal('success');
                  expect(mockPortal.subscribe.getCall(3).args[2]).to.deep.equal({audio: {fromStream: testRoom, codecs: ['pcmu']}, video: {fromStream: testRoom, codecs: ['h264']}, recorderId: '2016060418215098', interval: -1});
                  done();
                });
              });
            });
          });
        });
    });

    it('Starting recording with invalid recorderId or codec-names specified should fail.', function(done) {
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves('yyyyMMddhhmmssSS');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {recorderId: 987654321};
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

                var options = {recorderId: 'aaaa', audioStreamId: 'targetStreamId1', videoStreamId: 'targetStreamId1', audioCodec: '', videoCodec: ''}; //
                client.emit('startRecorder', options, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid audio codec');
                  expect(mockPortal.subscribe.callCount).to.equal(0);
                  done();
                });
              });
            });
          });
        });
    });
  });

  describe('on: stopRecorder', function() {
    it('Stopping recording should fail before joining.', function(done) {
      mockPortal.unsubscribe = sinon.stub();

      client.emit('stopRecorder', {}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.unsubscribe.callCount).to.equal(0);
        done();
      });
    });

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
          var options = {recorderId: 'recorder-id'};
          client.emit('stopRecorder', options, function(status, data) {
            expect(status).to.equal('success');
            expect(data.recorderId).to.equal('recorder-id');
            expect(data.host).to.equal('unknown');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'recorder-id'])
            done();
          });
        });
    });

    it('Stopping recording should fail if portal.unsubscribe fails.', function(done) {
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('Invalid recorder id');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          var options = {recorderId: 'recorder-id'};
          client.emit('stopRecorder', options, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('Invalid recorder id');
            expect(mockPortal.unsubscribe.getCall(0).args).to.deep.equal(['/#' + client.id, 'recorder-id'])
            done();
          });
        });
    });
  });

  describe('on: getRegion, setRegion', function() {
    it('Getting/setting region should fail before joining.', function(done) {
      mockPortal.getRegion = sinon.stub();
      mockPortal.setRegion = sinon.stub();

      client.emit('getRegion', {id: 'subStreamId'}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');
        expect(mockPortal.getRegion.callCount).to.equal(0);

        client.emit('setRegion', {id: 'subStreamId', region: 'regionId'}, function(status, data) {
          expect(status).to.equal('error');
          expect(data).to.equal('unauthorized');
          expect(mockPortal.setRegion.callCount).to.equal(0);
          done();
        });
      });
    });

    it('Getting/setting region with invalid stream-id or region-id should fail.', function(done) {
      mockPortal.getRegion = sinon.stub();
      mockPortal.setRegion = sinon.stub();

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('getRegion', {id: 1234}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.equal('Invalid stream id');
            expect(mockPortal.getRegion.callCount).to.equal(0);

            client.emit('setRegion', {id: {a: 56}, region: 'regionId'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.equal('Invalid stream id');
              expect(mockPortal.setRegion.callCount).to.equal(0);

              client.emit('getRegion', {id: ''}, function(status, data) {
                expect(status).to.equal('error');
                expect(data).to.equal('Invalid stream id');
                expect(mockPortal.getRegion.callCount).to.equal(0);

                client.emit('setRegion', {id: 'subStreamId', region: 789}, function(status, data) {
                  expect(status).to.equal('error');
                  expect(data).to.equal('Invalid region id');
                  expect(mockPortal.setRegion.callCount).to.equal(0);

                  client.emit('setRegion', {id: 'subStreamId', region: ''}, function(status, data) {
                    expect(status).to.equal('error');
                    expect(data).to.equal('Invalid region id');
                    expect(mockPortal.setRegion.callCount).to.equal(0);
                    done();
                  });
                });
              });
            });
          });
        });
    });

    it('Getting/setting region should succeed if portal.getRegion/portal.setRegion succeeds.', function(done) {
      mockPortal.getRegion = sinon.stub();
      mockPortal.getRegion.resolves('regionId');
      mockPortal.setRegion = sinon.stub();
      mockPortal.setRegion.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('getRegion', {id: 'subStreamId'}, function(status, data) {
            expect(status).to.equal('success');
            expect(data.region).to.equal('regionId');
            expect(mockPortal.getRegion.getCall(0).args).to.deep.equal(['/#' + client.id, 'subStreamId']);

            client.emit('setRegion', {id: 'subStreamId', region: '3'}, function(status, data) {
              expect(status).to.equal('success');
              expect(mockPortal.setRegion.getCall(0).args).to.deep.equal(['/#' + client.id, 'subStreamId', '3']);
              done();
            });
          });
        });
    });

    it('Getting/setting region should fail if portal.getRegion/portal.setRegion fails.', function(done) {
      mockPortal.getRegion = sinon.stub();
      mockPortal.getRegion.rejects('Invalid stream id');
      mockPortal.setRegion = sinon.stub();
      mockPortal.setRegion.rejects('another error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('getRegion', {id: 'subStreamId'}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('Invalid stream id');
            expect(mockPortal.getRegion.getCall(0).args).to.deep.equal(['/#' + client.id, 'subStreamId']);

            client.emit('setRegion', {id: 'subStreamId', region: '3'}, function(status, data) {
              expect(status).to.equal('error');
              expect(data).to.have.string('another error');
              expect(mockPortal.setRegion.getCall(0).args).to.deep.equal(['/#' + client.id, 'subStreamId', '3']);
              done();
            });
          });
        });
    });
  });

  describe('on: customMessage', function() {
    it('Should fail before joining.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.mediaOnOff = sinon.stub();

      client.emit('customMessage', {type: 'data', receiver: 'all', data: 'Hi, there!'}, function(status, data) {
        expect(status).to.equal('error');
        expect(data).to.equal('unauthorized');

        client.emit('customMessage', {type: 'control', payload: {streamId: 'streamId', action: 'video-out-off'}}, function(status, data) {
          expect(status).to.equal('error');
          expect(data).to.equal('unauthorized');
          expect(mockPortal.text.callCount).to.equal(0);
          expect(mockPortal.mediaOnOff.callCount).to.equal(0);
          done();
        });
      });
    });

    it('Should fail if message type is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.mediaOnOff = sinon.stub();

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
                expect(mockPortal.mediaOnOff.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });

    it('Should fail if message type is #data# and receiver is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.mediaOnOff = sinon.stub();

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
                expect(mockPortal.mediaOnOff.callCount).to.equal(0);
                done();
              });
            });
          });
        });
    });

    it('Should fail if message type is #control# and payload is invalid.', function(done) {
      mockPortal.text = sinon.stub();
      mockPortal.mediaOnOff = sinon.stub();

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

                client.emit('customMessage', {type: 'control', payload: {streamId: {obj: true}}}, function(status, data) {
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
                            expect(mockPortal.mediaOnOff.callCount).to.equal(0);
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
            expect(mockPortal.text.getCall(0).args).to.deep.equal(['/#' + client.id, 'all', 'Hi, there!']);
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

    it('Should succeed if message type is #control# and portal.mediaOnOff succeeds.', function(done) {
      mockPortal.mediaOnOff = sinon.stub();
      mockPortal.mediaOnOff.resolves('ok');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'control', payload: {streamId: 'streamId', action: 'video-out-off'}}, function(status, data) {
            expect(status).to.equal('success');
            expect(mockPortal.mediaOnOff.getCall(0).args).to.deep.equal(['/#' + client.id, 'streamId', 'video', 'in', 'off']);
            done();
          });
        });
    });

    it('Should fail if message type is #control# and portal.mediaOnOff fails.', function(done) {
      mockPortal.mediaOnOff = sinon.stub();
      mockPortal.mediaOnOff.rejects('some-error');

      return joinFirstly()
        .then(function(result) {
          expect(result).to.equal('ok');
          client.emit('customMessage', {type: 'control', payload: {streamId: 'streamId', action: 'video-out-off'}}, function(status, data) {
            expect(status).to.equal('error');
            expect(data).to.have.string('some-error');
            done();
          });
        });
    });
  });

  describe('on: disconnect', function() {
    it('Disconnecting after joining should cause participant leaving.', function(done) {
      mockPortal.leave = sinon.stub();
      mockPortal.leave.resolves('ok');

      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      var participant_id = '/#' + client.id;
      client.on('disconnect', function() {
        //NOTE: the detection method here is not acurate.
        setTimeout(function() {
          expect(mockPortal.leave.getCall(0).args).to.deep.equal([participant_id]);
          done();
        }, 20);
      });

      client.emit('token', 'someValidToken', function(status, resp) {
        expect(status).to.equal('success');
        client.disconnect();
      });
    });

    it('Disconnecting before joining should not cause participant leaving.', function(done) {
      mockPortal.leave = sinon.stub();

      client.on('disconnect', function() {
        //NOTE: the detection method here is not acurate.
        setTimeout(function() {
          expect(mockPortal.leave.callCount).to.equal(0);
          done();
        }, 20);
      });

      client.disconnect();
    });
  });
});

