
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var restServer = require('../restServer');
var portal = require('../portal');

var testRoom = '573eab78111478bb3526421a';

process.env.NODE_TLS_REJECT_UNAUTHORIZED = "0";

describe('Responding to https requests.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var server = restServer({port: 3003, ssl: true, keystorePath: './cert/certificate.pfx'}, mockPortal);
  var testQueryInterval = 60 * 1000;
  var request = require('request');

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
  });

  it('Joining with valid parameters should succeed.', function(done) {
    mockPortal.join = sinon.stub();

    var join_result = {user: 'Jack',
                       role: 'presenter',
                       session_id: testRoom,
                       participants: [],
                       streams: [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: ['vag', 'hd720p']}, from: ''}]};
    mockPortal.join.resolves(join_result);

    var transformed_streams = [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: [{width: 640, height: 480}, {width: 1280, height: 720}]}, from: ''}];
    var options = {uri: 'https://localhost:3003/clients',
                   method: 'POST',
                   json: {token: 'someValidToken', queryInterval: testQueryInterval}};
    request(options, function(error, response, body) {
      expect(error).to.equal(null);
      expect(response.statusCode).to.equal(200);
      var client_id = mockPortal.join.getCall(0).args[0];
      expect(client_id).to.be.a('string');
      expect(mockPortal.join.getCall(0).args[1]).to.equal('someValidToken');
      expect(body).to.deep.equal({id: client_id, streams: transformed_streams, clients: join_result.participants});
      done();
    });
  });

});

describe('Responding to http requests.', function() {
  var mockPortal = sinon.createStubInstance(portal);
  var server = restServer({port: 3002, ssl: false}, mockPortal);
  var testQueryInterval = 60 * 1000;
  var request = require('request');

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
  });

  function joinFirstly(queryInterval) {
    return new Promise(function(resolve, reject) {
      mockPortal.join = sinon.stub();

      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: []};
      mockPortal.join.resolves(join_result);

      var options = {uri: 'http://localhost:3002/clients',
                     method: 'POST',
                     json: {token: 'someValidToken', queryInterval: queryInterval || testQueryInterval}};
      request(options, function(error, response, body) {
        expect(response.statusCode).to.equal(200);
        resolve(body.id);
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

  describe('User joins.', function() {
    it('Joining with valid parameters should succeed.', function(done) {
      mockPortal.join = sinon.stub();

      var join_result = {user: 'Jack',
                         role: 'presenter',
                         session_id: testRoom,
                         participants: [],
                         streams: [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: ['vag', 'hd720p']}, from: ''}]};
      mockPortal.join.resolves(join_result);

      var transformed_streams = [{id: testRoom, audio: true, video: {device: 'mcu', resolutions: [{width: 640, height: 480}, {width: 1280, height: 720}]}, from: ''}];

      var options = {uri: 'http://localhost:3002/clients',
                     method: 'POST',
                     json: {token: 'someValidToken', queryInterval: testQueryInterval}};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(200);
        var client_id = mockPortal.join.getCall(0).args[0];
        expect(client_id).to.be.a('string');
        expect(mockPortal.join.getCall(0).args[1]).to.equal('someValidToken');
        expect(body).to.deep.equal({id: client_id, streams: transformed_streams, clients: join_result.participants});
        done();
      });
    });

    it('Portal rejection should cause joining failure.', function(done) {
      mockPortal.join = sinon.stub();
      mockPortal.join.rejects('whatever reason');

      var options = {uri: 'http://localhost:3002/clients',
                     method: 'POST',
                     json: {token: 'someValidOrInvalidToken', queryInterval: testQueryInterval}};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'whatever reason'});
        done();
      });
    });
  });

  describe('User queries.', function() {
    it('Quering before joining should fail.', function(done) {
      var options = {uri: 'http://localhost:3002/clients/' + 'some-unjoined-client-id',
                     method: 'GET',
                     json: true};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Quering a joined client before query-interval expired should succeed.', function(done) {
      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/clients/' + clientId,
                         method: 'GET',
                         json: true};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);
            expect(body).to.deep.equal({published: {}, subscribed: {}, notifications: []});
            done();
          });
        });
    });

    it('Quering a joined client after query-interval expired should fail.', function(done) {
      return joinFirstly(10)
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          setTimeout(function() {
            var options = {uri: 'http://localhost:3002/clients/' + clientId,
                           method: 'GET',
                           json: true};
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(404);
              expect(body).to.deep.equal({reason: 'Client absent'});
              done();
            });
          }, 60);
        });
    });
  });

  describe('user leaves.', function() {
    it('Leaving before joining should fail.', function(done) {
      var options = {uri: 'http://localhost:3002/clients/' + 'some-unjoined-client-id',
                     method: 'DELETE',
                     json: true};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Leaving after joining should succeed.', function(done) {
      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/clients/' + clientId,
                         method: 'DELETE',
                         json: true};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);
            done();
          });
        });
    });
  });

  describe('Publishing streams.', function() {
    it('Publishing before joining should fail.', function(done) {
      var options = {uri: 'http://localhost:3002/pub/' + 'some-unjoined-client-id',
                     method: 'POST',
                     json: {type: 'webrtc',
                            options: {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}}}};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Publishing RTSP/RTMP streams after joining should succeed if portal succeeds.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves({agent: 'agentId', node: 'nodeId'});

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          simulateStubResponse(mockPortal.publish, 0, 4, {type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264']});
          var options = {uri: 'http://localhost:3002/pub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'urlOfRtspOrRtmpSource', transport: 'tcp', buffer_size: 2048}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);
            expect(body.id).to.be.a('string');
            expect(mockPortal.publish.getCall(0).args[0]).to.equal(clientId);
            expect(mockPortal.publish.getCall(0).args[1]).to.equal(body.id);
            expect(mockPortal.publish.getCall(0).args[2]).to.equal('avstream');
            expect(mockPortal.publish.getCall(0).args[3]).to.deep.equal({audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'urlOfRtspOrRtmpSource', transport: 'tcp', bufferSize: 2048});
            expect(mockPortal.publish.getCall(0).args[4]).to.be.a('function');
            expect(mockPortal.publish.getCall(0).args[5]).to.be.false;

            var options = {uri: 'http://localhost:3002/clients/' + clientId,
                           method: 'GET',
                           json: true};

            var stream_id = body.id;

            setTimeout(function() {
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(200);
              expect(body.published[stream_id]).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264']}]);

              request(options, function(error, response, body) {
                expect(error).to.equal(null);
                expect(response.statusCode).to.equal(200);
                expect(body.published[stream_id]).to.deep.equal([]);
                done();
              });
            });
            }, 20);
          });
        });
    });

    it('Publishing after joining should fail if portal fails.', function(done) {
      var testStreamId = 'streamId';
      mockPortal.publish = sinon.stub();
      mockPortal.publish.rejects('whatever reason');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/pub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'urlOfRtspOrRtmpSource', transport: 'tcp', buffer_size: 2048}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(404);
            expect(body).to.deep.equal({reason: 'whatever reason'});
            done();
          });
        });
    });
  });

  describe('Unpublishing streams.', function() {
    it('Unpublishing before joining should fail.', function(done) {
      var options = {uri: 'http://localhost:3002/pub/' + 'some-unjoined-client-id' + '/' + 'stream-id',
                     method: 'DELETE',
                     json: true};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Unpublishing should succeed if portal succeeds.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves('streamId');
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.resolves('ok');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/pub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'urlOfRtspOrRtmpSource', transport: 'tcp', buffer_size: 2048}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);

            var options = {uri: 'http://localhost:3002/pub/' + clientId + '/' + body.id,
                           method: 'DELETE',
                           json: true};
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(200);
              done();
            });
          });
        });
    });

    it('Unpublishing should fail if portal fails.', function(done) {
      mockPortal.publish = sinon.stub();
      mockPortal.publish.resolves({agent: 'agentId', node: 'nodeId'});
      mockPortal.unpublish = sinon.stub();
      mockPortal.unpublish.rejects('whatever reason');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/pub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'urlOfRtspOrRtmpSource', transport: 'tcp', buffer_size: 2048}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);

            var options = {uri: 'http://localhost:3002/pub/' + clientId + '/' + body.id,

                           method: 'DELETE',
                           json: true};
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(404);
              expect(body).to.deep.equal({reason: 'whatever reason'});
              done();
            });
          });
        });
    });
  });

  describe('Subscribing streams.', function() {
    it('Subscribing before joining should fail.', function(done) {
      var subscribed_stream = testRoom;
      var options = {uri: 'http://localhost:3002/sub/' + 'some-unjoined-client-id',
                     method: 'POST',
                     json: {type: 'url',
                            options: {url: 'rtmp://url-of-rtsp-or-rtmp-destination',
                                      audio: {from: subscribed_stream, codecs: ['aac']},
                                      video: {from: subscribed_stream, codecs: ['h264'], resolution: {width: 800, height: 600}}}}};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Subscribing to an RTSP/RTMP destination should succeed if portal succeeds.', function(done) {
      var subscribed_stream = testRoom;
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          simulateStubResponse(mockPortal.subscribe, 0, 4, {type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
          var options = {uri: 'http://localhost:3002/sub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'rtmp://url-of-rtsp-or-rtmp-destination',
                                          audio: {from: subscribed_stream, codecs: ['aac']},
                                          video: {from: subscribed_stream, codecs: ['h264'], resolution: {width: 640, height: 480}}}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);
            expect(body.id).to.be.a('string');
            expect(mockPortal.subscribe.getCall(0).args[0]).to.equal(clientId);
            expect(mockPortal.subscribe.getCall(0).args[1]).to.equal(body.id);
            expect(mockPortal.subscribe.getCall(0).args[2]).to.equal('avstream');
            expect(mockPortal.subscribe.getCall(0).args[3]).to.deep.equal({audio: {fromStream: subscribed_stream, codecs: ['aac']}, video: {fromStream: subscribed_stream, codecs: ['h264'], resolution: 'vga'}, url: 'rtmp://url-of-rtsp-or-rtmp-destination'});
            expect(mockPortal.subscribe.getCall(0).args[4]).to.be.a('function');

            var options = {uri: 'http://localhost:3002/clients/' + clientId,
                           method: 'GET',
                           json: true};
            var subscription_id = body.id;

            setTimeout(function() {
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(200);
              expect(body.subscribed[subscription_id]).to.deep.equal([{type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']}]);

              request(options, function(error, response, body) {
                expect(error).to.equal(null);
                expect(response.statusCode).to.equal(200);
                expect(body.subscribed[subscription_id]).to.deep.equal([]);
                done();
              });
            });
            }, 20);
          });
        });
    });

    it('Subscribing should fail if portal fails.', function(done) {
      var subscribed_stream = testRoom;
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.rejects('whatever reason');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/sub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'rtmp://url-of-rtsp-or-rtmp-destination',
                                          audio: {from: subscribed_stream, codecs: ['aac']},
                                          video: {from: subscribed_stream, codecs: ['h264'], resolution: {width: 800, height: 600}}}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(404);
            expect(body).to.deep.equal({reason: 'whatever reason'});
            done();
          });
        });
    });
  });

  describe('Unsubscribing streams.', function() {
    it('Unsubscribing before joining should fail.', function(done) {
      var options = {uri: 'http://localhost:3002/sub/' + 'some-unjoined-client-id' + '/' + 'subscription-id',
                     method: 'DELETE',
                     json: true};
      request(options, function(error, response, body) {
        expect(error).to.equal(null);
        expect(response.statusCode).to.equal(404);
        expect(body).to.deep.equal({reason: 'Client absent'});
        done();
      });
    });

    it('Unsubscribing should succeed if portal succeeds.', function(done) {
      var subscribed_stream = testRoom;
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.resolves('ok');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/sub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'rtmp://url-of-rtsp-or-rtmp-destination',
                                          audio: {from: subscribed_stream, codecs: ['aac']},
                                          video: {from: subscribed_stream, codecs: ['h264'], resolution: {width: 640, height: 480}}}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);

            var options = {uri: 'http://localhost:3002/sub/' + clientId + '/' + body.id,
                           method: 'DELETE',
                           json: true};
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(200);
              done();
            });
          });
        });
    });

    it('Unsubscribing should fail if portal fails.', function(done) {
      var subscribed_stream = testRoom;
      mockPortal.subscribe = sinon.stub();
      mockPortal.subscribe.resolves({agent: 'agentId', node: 'nodeId'});
      mockPortal.unsubscribe = sinon.stub();
      mockPortal.unsubscribe.rejects('whatever reason');

      return joinFirstly()
        .then(function(clientId) {
          expect(clientId).to.be.a('string');

          var options = {uri: 'http://localhost:3002/sub/' + clientId,
                         method: 'POST',
                         json: {type: 'url',
                                options: {url: 'rtmp://url-of-rtsp-or-rtmp-destination',
                                          audio: {from: subscribed_stream, codecs: ['aac']},
                                          video: {from: subscribed_stream, codecs: ['h264'], resolution: {width: 640, height: 480}}}}};
          request(options, function(error, response, body) {
            expect(error).to.equal(null);
            expect(response.statusCode).to.equal(200);

            var options = {uri: 'http://localhost:3002/sub/' + clientId + '/' + body.id,
                           method: 'DELETE',
                           json: true};
            request(options, function(error, response, body) {
              expect(error).to.equal(null);
              expect(response.statusCode).to.equal(404);
              expect(body).to.deep.equal({reason: 'whatever reason'});
              done();
            });
          });
        });
    });
  });
});
