var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var Portal = require('../portal');
var rpcClient = require('../rpcClient');

var testTokenKey = '6d96729ffd07202e7a65ac81278c43a2c87fc6367736431e614607e9d3ceee201f9680cdfcb88df508829e9810c46aaf02c9cc8dcf46369de122ee5b22ec963c';
var testSelfRpcId = 'portal-38A87euk9kfh';
var testTokenServer = 'nuve';
var testClusterName = 'woogeen-cluster';
var testPortalSpec = {tokenKey: testTokenKey,
                      tokenServer: testTokenServer,
                      clusterName: testClusterName,
                      selfRpcId: testSelfRpcId,
                      permissionMap: {'admin': {publish: true, subscribe: true, record: true},
                                      'presenter': {publish: true, subscribe: true},
                                      'viewer': {subscribe: true},
                                      'viewer_no_text': {subscribe: true, text: false},
                                      'a_viewer': {subscribe: {audio: true}},
                                      'v_viewer': {subscribe: {video: true}},
                                      'viewer_pub_a': {subscribe: true, publish: {audio: true}},
                                      'viewer_pub_v': {subscribe: true, publish: {video: true}},
                                      'guest': {}}
                     };
var testToken = {tokenId: '573eab88111478bb3526421b',
                 host: '10.239.158.161:8080',
                 secure: true,
                 signature: 'ZTIwZTI0YzYyMDFmMTQ1OTc3ZTBkZTdhMDMyZTJhYTkwMTJiZjNhY2E5OTZjNzA1Y2ZiZGU1ODhjMDY3MmFkYw=='
                };
var testParticipantId = '/#gpMeAEuPeMMDT0daAAAA';
var testSession = '573eab78111478bb3526421a';

describe('portal.join: Participants join.', function() {
  it('Joining with valid token should succeed.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    var portal = Portal(testPortalSpec, mockRpcClient);

    var login_result = {userName: 'Jack', role: 'presenter', room: testSession};
    mockRpcClient.tokenLogin.resolves(login_result);

    mockRpcClient.getController.resolves('rpcIdOfController');

    var join_result = {participants: [],
                       streams: []};
    mockRpcClient.join.resolves(join_result);

    var final_result = {user: login_result.userName, role: login_result.role, session_id: login_result.room, participants: join_result.participants, streams: join_result.streams};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      expect(mockRpcClient.tokenLogin.getCall(0).args).to.deep.equal(['nuve', testToken.tokenId]);
      expect(mockRpcClient.getController.getCall(0).args).to.deep.equal(['woogeen-cluster', testSession]);
      expect(mockRpcClient.join.getCall(0).args).to.deep.equal(['rpcIdOfController', testSession, {id: testParticipantId, name: 'Jack', role: 'presenter', portal: testSelfRpcId}]);
    });
  });

  it('Tokens with incorect signature should fail joining.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockRpcClient);

    var tokenWithInvalidSignature = { tokenId: '573eab88111478bb3526421b',
                                      host: '10.239.158.161:8080',
                                      secure: true,
                                      signature: 'AStringAsInvalidSignature' };
    return expect(portal.join(testParticipantId, tokenWithInvalidSignature)).to.be.rejectedWith('Invalid token signature');//Note: The error message is not strictly equality checked, instead it will be fullfiled when the actual error message includes the given string here. That is the inherint behavor of 'rejectedWith'.
  });

  it('rpcClient.tokenLogin timeout or error should fail joining.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcClient.getController timeout or error should fail joining.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockRpcClient);

    var login_result = {userName: 'Jack', role: 'presenter', room: testSession};
    mockRpcClient.tokenLogin.resolves(login_result);

    mockRpcClient.getController.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcClient.join timeout or error should fail joining.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockRpcClient);

    var login_result = {userName: 'Jack', role: 'presenter', room: testSession};
    mockRpcClient.tokenLogin.resolves(login_result);

    var controller_result = 'theRpcIdOfController'
    mockRpcClient.getController.resolves(controller_result);

    mockRpcClient.join.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });
});

describe('portal.leave: Participants leave.', function() {
  it('Leaving without logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.unpublish = sinon.stub();
    mockRpcClient.unsubscribe = sinon.stub();
    mockRpcClient.recycleAccessNode = sinon.stub();
    mockRpcClient.unpub2Session = sinon.stub();
    mockRpcClient.unsub2Session = sinon.stub();
    mockRpcClient.leave = sinon.stub();

    return portal.leave(testParticipantId)
      .then(function(runInHere) {
        expect(runInHere).to.be.false;
      }, function(err) {
        expect(err).to.equal('Participant ' + testParticipantId + ' does NOT exist.');
        expect(mockRpcClient.unpublish.callCount).to.equal(0);
        expect(mockRpcClient.unsubscribe.callCount).to.equal(0);
        expect(mockRpcClient.recycleAccessNode.callCount).to.equal(0);
        expect(mockRpcClient.unpub2Session.callCount).to.equal(0);
        expect(mockRpcClient.unsub2Session.callCount).to.equal(0);
        expect(mockRpcClient.leave.callCount).to.equal(0);
      });
  });

  describe('Leaving should cause releasing the media connections and unpublishing/unsubscribing.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({
                                   participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        done();
      });
    });

    it('Leaving after successfully logining should succeed.', function() {
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');

      return portal.leave(testParticipantId)
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.leave.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId]);
        });
    });

    it('Leaving after successfully publishing should cause disconnecting and unpublishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();
      mockRpcClient.unpub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');
      mockRpcClient.unpub2Session.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(result) {
          return portal.leave(testParticipantId);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
        });
     });

    it('Leaving after successfully subscribing should cause disconnecting and unsubscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(result) {
          return portal.leave(testParticipantId);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
        });
     });
  });
});

describe('portal.publish/portal.unpublish: Participants publish/unpublish streams', function() {
  it('Publishing before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    var spyConnectionObserver = sinon.spy();

    return expect(portal.publish(testParticipantId,
                                 'webrtc',
                                 {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                 spyConnectionObserver,
                                 false))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Publishing without permission(publish: undefined | false) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'viewer', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.publish(testParticipantId,
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Publishing without permission(publish: {audio: true} but stream contains video) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'viewer_pub_a', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.publish(testParticipantId,
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Publishing without permission(publish: {video: true} but stream contains video) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'viewer_pub_v', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.publish(testParticipantId,
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Unpublishing before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    return expect(portal.unpublish(testParticipantId, 'streamId'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  describe('Publishing with proper permission should proceed.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        done();
      });
    });

    afterEach(function() {
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.unpub2Session = sinon.stub();
      mockRpcClient.unpub2Session.resolves('ok');
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');
      portal.leave(testParticipantId);
    });

    it('Publishing an rtsp/rtmp stream with both audio and video should succeed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'avstream',
                            {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera'},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          expect(streamId).to.be.a('string');
          expect(mockRpcClient.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'avstream', {session: testSession, consumer: streamId}]);
          expect(mockRpcClient.publish.getCall(0).args).to.have.lengthOf(5);
          expect(mockRpcClient.publish.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockRpcClient.publish.getCall(0).args[1]).to.equal(stream_id);
          expect(mockRpcClient.publish.getCall(0).args[2]).to.equal('avstream');
          expect(mockRpcClient.publish.getCall(0).args[3]).to.deep.equal({audio: true, video: {resolution: 'unknown'}, url: 'URIofSomeIPCamera/room_' + testSession + '-' + stream_id + '.sdp'});
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'initializing'}]);
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264'], video_resolution: 'hd1080p'});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'hd1080p', device: 'unknown', codec: 'h264'}, type: 'avstream'}, false]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264'], video_resolution: 'hd1080p'}]);
        });
    });

    it('Publishing rtsp/rtmp streams should be aborted if exceptions occur.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.unpub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');
      mockRpcClient.unpub2Session.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'avstream',
                            {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera'},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264'], video_resolution: 'hd1080p'});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return on_connection_status({type: 'failed', reason: 'network error'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          expect(reason).to.have.string('network error');
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
          expect(mockRpcClient.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
        });
    });

    it('Publishing a webrtc stream with both audio and video should succeed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          expect(streamId).to.be.a('string');
          expect(mockRpcClient.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {session: testSession, consumer: streamId}]);
          expect(mockRpcClient.publish.getCall(0).args).to.have.lengthOf(5);
          expect(mockRpcClient.publish.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockRpcClient.publish.getCall(0).args[1]).to.equal(stream_id);
          expect(mockRpcClient.publish.getCall(0).args[2]).to.equal('webrtc');
          expect(mockRpcClient.publish.getCall(0).args[3]).to.deep.equal({audio: true, video: {resolution: 'vga'}});
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'initializing'}]);
          return portal.onConnectionSignalling(testParticipantId, stream_id, {type: 'offer', sdp: 'offerSDPString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockRpcClient.onConnectionSignalling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return portal.onConnectionSignalling(testParticipantId, stream_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockRpcClient.onConnectionSignalling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', stream_id, {type: 'candidate', sdp: 'candidateString'}]);
          return on_connection_status({type: 'answer', sdp: 'answerSDPString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('answer');
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'answer', sdp: 'answerSDPString'}]);
          return on_connection_status({type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('candidate');
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'candidate', sdp: 'candidateString'}]);
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'vga', framerate: 30, device: 'camera', codec: 'vp8'}, type: 'webrtc'}, false]);
          expect(spyConnectionObserver.getCall(3).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']}]);
        });
    });

    it('rpcClient.getAccessNode timeout or error should fail publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.getAccessNode.rejects('getAccessNode timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false))
        .to.be.rejectedWith('getAccessNode timeout or error.');
    });

    it('rpcClient.publish timeout or error should fail publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.rejects('connect timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false))
        .to.be.rejectedWith('connect timeout or error.');
    });

    it('rpcClient.onConnectionSignalling timeout or error should fail publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();


      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.onConnectionSignalling.rejects('timeout or error');

      var stream_id;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          return portal.onConnectionSignalling(testParticipantId, stream_id, {type: 'candidate', sdp: 'candidateString'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
        });
    });

    it('No proper audio/video codec during negotiation should fail publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();


      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok.');
      mockRpcClient.unpublish.resolves('ok.');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.onConnectionSignalling.rejects('timeout or error');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: [], video_codecs: ['vp8', 'h264']});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.equal('No proper audio codec');
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);

          var stream_id1, on_connection_status1;
          var spyConnectionObserver1 = sinon.spy();
          return portal.publish(testParticipantId,
                                'webrtc',
                                {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                spyConnectionObserver,
                                false)
            .then(function(streamId) {
              stream_id1 = streamId;
              on_connection_status1 = mockRpcClient.publish.getCall(1).args[4];
              return on_connection_status1({type: 'ready', audio_codecs: ['opus'], video_codecs: []});
            }).then(function(runInHere) {
              expect(runInHere).to.be.false;
            }, function(err) {
              expect(err).to.equal('No proper video codec');
              expect(mockRpcClient.unpublish.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', stream_id1]);
              expect(mockRpcClient.recycleAccessNode.getCall(1).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id1}]);
            });
        });
    });

    it('rpcClient.pub2Session(to controller) timeout or error should fail publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.pub2Session.rejects('timeout or error');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            true)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockRpcClient.pub2Session.callCount).to.equal(1);
          expect(mockRpcClient.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'vga', framerate: 30, device: 'camera', codec: 'vp8'}, type: 'webrtc'}, true]);
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'failed', reason: 'timeout or error'}]);
        });
    });

    it('The connection reporting a status with type of #failed# should abort publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'failed', reason: 'reasonString'});
        })
        .then(function(statusResult) {
          expect('run into here').to.be.false;
        }, function(err) {
          expect(err).to.equal('reasonString');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'reasonString'}]);
          expect(mockRpcClient.pub2Session.callCount).to.equal(0);
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
        });
    });

    it('Participant early leaving should abort publishing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();
      mockRpcClient.unpub2Session = sinon.stub();
      mockRpcClient.leave = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.leave.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          return portal.leave(testParticipantId);
        })
        .then(function(leaveResult) {
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
          expect(mockRpcClient.pub2Session.callCount).to.equal(0);
          expect(mockRpcClient.unpub2Session.callCount).to.equal(0);
          return expect(on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']}))
            .to.be.rejectedWith('Participant ' + testParticipantId + ' has left when the connection gets ready.');
        });
    });

    it('Unpublishing before publishing should fail.', function() {
      return expect(portal.unpublish(testParticipantId, 'streamId'))
        .to.be.rejectedWith('stream does not exist');
    });

    it('Unpublishing a half published stream(connected but not published to controller) should cause disconnect(to access node) and return ok.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');

      var stream_id;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          return portal.unpublish(testParticipantId, streamId);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
        });
    });

    it('Unpublishing a successfully published stream should cause disconnect(to access node) and unpublish(to controller) and return ok.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();
      mockRpcClient.unpub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');
      mockRpcClient.unpub2Session.resolves('ok');

      var stream_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(streamId) {
          stream_id = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockRpcClient.pub2Session.callCount).to.equal(1);
          return portal.unpublish(testParticipantId, stream_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: stream_id}]);
          expect(mockRpcClient.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
        });
    });
  });
});

describe('portal.mix/portal.unmix/portal.setVideoBitrate/portal.mediaOnOff: Participants manipulate streams they published.', function() {
  it('Should fail before joining', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    var mix = portal.mix(testParticipantId, 'streamId'),
        unmix = portal.unmix(testParticipantId, 'streamId'),
        setVideoBitrate = portal.setVideoBitrate(testParticipantId, 'streamId', 500),
        mediaOnOff = portal.mediaOnOff(testParticipantId, 'streamId', 'video', 'in', 'off');

    return Promise.all([
      expect(mix).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(unmix).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(setVideoBitrate).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(mediaOnOff).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.')
      ]);
  });

  it('Should fail before publishing.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        var mix = portal.mix(testParticipantId, 'streamId'),
            unmix = portal.unmix(testParticipantId, 'streamId'),
            setVideoBitrate = portal.setVideoBitrate(testParticipantId, 'streamId', 500),
            mediaOnOff = portal.mediaOnOff(testParticipantId, 'streamId', 'video', 'in', 'off');

        return Promise.all([
          expect(mix).to.be.rejectedWith('stream does not exist'),
          expect(unmix).to.be.rejectedWith('stream does not exist'),
          expect(setVideoBitrate).to.be.rejectedWith('stream does not exist'),
          expect(mediaOnOff).to.be.rejectedWith('connection does not exist')
          ]);
      });
  });

  describe('Manipulating streams after publishing.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);
    var testStreamId;

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.publish = sinon.stub();
      mockRpcClient.pub2Session = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({participants: [],
                                   streams: []});
      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.publish.resolves('ok');
      mockRpcClient.pub2Session.resolves('ok');

      var spyConnectionObserver = sinon.spy(),
          on_connection_status;

      return portal.join(testParticipantId, testToken)
        .then(function(login_result) {
          return portal.publish(testParticipantId,
                                'webrtc',
                                {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                spyConnectionObserver,
                                false);
        })
        .then(function(streamId) {
          testStreamId = streamId;
          on_connection_status = mockRpcClient.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockRpcClient.pub2Session.callCount).to.equal(1);
          done();
        });
    });

    afterEach(function() {
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.unpub2Session = sinon.stub();
      mockRpcClient.unpub2Session.resolves('ok');
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');
      portal.leave(testParticipantId);
      testStreamId = undefined;
    });

    it('Should succeed if rpcClient.mix/rpcClient.unmix/rpcClient.setVideoBitrate/rpcClient.mediaOnOff succeeds.', function() {
      mockRpcClient.mix = sinon.stub();
      mockRpcClient.unmix = sinon.stub();
      mockRpcClient.setVideoBitrate = sinon.stub();
      mockRpcClient.mediaOnOff = sinon.stub();

      mockRpcClient.mix.resolves('ok');
      mockRpcClient.unmix.resolves('ok');
      mockRpcClient.setVideoBitrate.resolves('ok');
      mockRpcClient.mediaOnOff.resolves('ok');

      var mix = portal.mix(testParticipantId, testStreamId),
          unmix = portal.unmix(testParticipantId, testStreamId),
          setVideoBitrate = portal.setVideoBitrate(testParticipantId, testStreamId, 500),
          mediaOnOff = portal.mediaOnOff(testParticipantId, testStreamId, 'video', 'in', 'off');

      return Promise.all([
        expect(mix).to.become('ok'),
        expect(unmix).to.become('ok'),
        expect(setVideoBitrate).to.become('ok'),
        expect(mediaOnOff).to.become('ok')
        ])
        .then(function() {
          expect(mockRpcClient.mix.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, testStreamId]);
          expect(mockRpcClient.unmix.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, testStreamId]);
          expect(mockRpcClient.setVideoBitrate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testStreamId, 500]);
          expect(mockRpcClient.mediaOnOff.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testStreamId, 'video', 'in', 'off']);
        });
    });

    it('Should fail if rpcClient.mix/rpcClient.unmix/rpcClient.setVideoBitrate/rpcClient.mediaOnOff fails.', function() {
      mockRpcClient.mix = sinon.stub();
      mockRpcClient.unmix = sinon.stub();
      mockRpcClient.setVideoBitrate = sinon.stub();
      mockRpcClient.mediaOnOff = sinon.stub();

      mockRpcClient.mix.rejects('timeout or error');
      mockRpcClient.unmix.rejects('timeout or error');
      mockRpcClient.setVideoBitrate.rejects('timeout or error');
      mockRpcClient.mediaOnOff.rejects('timeout or error');

      var mix = portal.mix(testParticipantId, testStreamId),
          unmix = portal.unmix(testParticipantId, testStreamId),
          setVideoBitrate = portal.setVideoBitrate(testParticipantId, testStreamId, 500),
          mediaOnOff = portal.mediaOnOff(testParticipantId, testStreamId, 'video', 'in', 'off');

      return Promise.all([
        expect(mix).to.be.rejectedWith('timeout or error'),
        expect(unmix).to.be.rejectedWith('timeout or error'),
        expect(setVideoBitrate).to.be.rejectedWith('timeout or error'),
        expect(mediaOnOff).to.be.rejectedWith('timeout or error')
        ]);
    });
  });
});

describe('portal.subscribe/portal.unsubscribe/portal.mediaOnOff: Participants subscribe/unsubscribe streams and switch on/off the media tracks of the subscribed streams.', function() {
  it('Subscribing before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    var spyConnectionObserver = sinon.spy();

    return expect(portal.subscribe(testParticipantId,
                                  'webrtc',
                                  {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga'}},
                                   spyConnectionObserver))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Subscribing to webrtc clients without permission(subscribe: undefined | false) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'guest', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.subscribe(testParticipantId,
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to webrtc clients without permission(subscribe: {audio: true} and require video as well) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'a_viewer', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.subscribe(testParticipantId,
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to webrtc clients without permission(subscribe: {video: true} and require audio as well) should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'v_viewer', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.subscribe(testParticipantId,
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to recording files without permission should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.subscribe(testParticipantId,
                                       'recording',
                                       {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Unsubscribing before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    return expect(portal.unsubscribe(testParticipantId, 'subscriptionId'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Switching on/off the media tracks before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    return expect(portal.mediaOnOff(testParticipantId, 'subscriptionId', 'video', 'out', 'off'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  describe('Subscribing after successfully logining should proceed.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'admin', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        done();
      });
    });

    afterEach(function() {
      mockRpcClient.unpublish = sinon.stub();
      mockRpcClient.unpublish.resolves('ok');
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.unsub2Session = sinon.stub();
      mockRpcClient.unsub2Session.resolves('ok');
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');
      portal.leave(testParticipantId);
    });

    it('Subscribing an already-subscribed stream should fail.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockRpcClient.sub2Session.callCount).to.equal(1);
          return expect(portal.subscribe(testParticipantId,
                                         'webrtc',
                                         {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                                         spyConnectionObserver1)).to.be.rejectedWith('Not allowed to subscribe an already-subscribed stream');
        });
    });

    it('Subscribing a stream with both audio and video to a webrtc client should succeed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          expect(subscription_id).to.be.a('string');
          expect(mockRpcClient.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {session: testSession, consumer: subscriptionId}]);
          expect(mockRpcClient.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockRpcClient.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockRpcClient.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockRpcClient.subscribe.getCall(0).args[2]).to.equal('webrtc');
          expect(mockRpcClient.subscribe.getCall(0).args[3]).to.deep.equal({audio: true, video: {resolution: 'vga'}});
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'initializing'}]);
          return portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'offer', sdp: 'offerSDPString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockRpcClient.onConnectionSignalling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockRpcClient.onConnectionSignalling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, {type: 'candidate', sdp: 'candidateString'}]);
          return on_connection_status({type: 'answer', sdp: 'answerSDPString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('answer');
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'answer', sdp: 'answerSDPString'}]);
          return on_connection_status({type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('candidate');
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'candidate', sdp: 'candidateString'}]);
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId', codecs: ['vp8'], resolution: 'vga'}, type: 'webrtc'}]);
          expect(spyConnectionObserver.getCall(3).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}]);
        });
    });

    it('Subscribing a stream to an rtsp/rtmp server should succeed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'avstream',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['aac']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, url: 'url-of-avstream'},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          expect(subscription_id).to.be.a('string');
          expect(subscription_id).to.not.equal('targetStreamId1');
          expect(subscription_id).to.not.equal('targetStreamId2');
          expect(mockRpcClient.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'avstream', {session: testSession, consumer: subscriptionId}]);
          expect(mockRpcClient.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockRpcClient.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockRpcClient.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockRpcClient.subscribe.getCall(0).args[2]).to.equal('avstream');
          expect(mockRpcClient.subscribe.getCall(0).args[3]).to.deep.equal({audio: {codecs: ['aac']}, video: {codecs: ['h264'], resolution: 'vga'}, url: 'url-of-avstream/room_' + testSession + '-' + subscription_id + '.sdp'});
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId1', codecs: ['pcm_raw']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, type: 'avstream'}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']}]);
        });
    });

    it('Subscribing streams to a recording file should abort when exceptions occur.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'avstream',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['aac']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, url: 'url-of-avstream'},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.sub2Session.callCount).to.equal(1);
          return on_connection_status({type: 'failed', reason: 'network error'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          console.log(reason);
          expect(reason).to.equal('network error');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
          expect(mockRpcClient.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'failed', reason: 'network error'}]);
        });
    });

    it('Subscribing streams to a recording file should succeed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: 1000},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          expect(subscription_id).to.be.a('string');
          expect(subscription_id).to.have.lengthOf(16);
          expect(subscription_id).to.not.equal('targetStreamId1');
          expect(subscription_id).to.not.equal('targetStreamId2');
          expect(mockRpcClient.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'recording', {session: testSession, consumer: subscriptionId}]);
          expect(mockRpcClient.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockRpcClient.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockRpcClient.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockRpcClient.subscribe.getCall(0).args[2]).to.equal('recording');
          expect(mockRpcClient.subscribe.getCall(0).args[3]).to.deep.equal({audio: {codecs: ['pcmu']}, video: {codecs: ['vp8']}, path: 'path-of-recording', filename: 'room_' + testSession + '-' + subscription_id + '.mkv', interval: 1000});
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, type: 'recording'}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}]);
        });
    });

    it('Subscribing streams to a recording file should use the specified recorder-id as the subscription id.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');

      var testRecorderId = '2016061416024671';
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', recorderId: testRecorderId, interval: -1},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          expect(subscriptionId).to.equal(testRecorderId);
        });
    });

    it('Subscribing streams to a recording file should abort when exceptions occur.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.unsub2Session.resolves('ok');


      var testRecorderId = '2016061416024671';
      var spyConnectionObserver = sinon.spy();
      var on_connection_status;

      return portal.subscribe(testParticipantId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', recorderId: testRecorderId, interval: -1},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          expect(subscriptionId).to.equal(testRecorderId);
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.sub2Session.callCount).to.equal(1);
          return on_connection_status({type: 'failed', reason: 'write file error'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          console.log(reason);
          expect(reason).to.equal('write file error');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testRecorderId]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: testRecorderId}]);
          expect(mockRpcClient.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, testRecorderId]);
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'failed', reason: 'write file error'}]);
        });
    });

    it('rpcClient.getAccessNode timeout or error should fail subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.getAccessNode.rejects('getAccessNode timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.subscribe(testParticipantId,
                                     'webrtc',
                                     {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                                     spyConnectionObserver))
        .to.be.rejectedWith('getAccessNode timeout or error.');
    });

    it('rpcClient.subscribe timeout or error should fail subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.rejects('connect timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.subscribe(testParticipantId,
                                    'webrtc',
                                    {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                                    spyConnectionObserver))
        .to.be.rejectedWith('connect timeout or error.');
    });

    it('rpcClient.onConnectionSignalling timeout or error should fail subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();


      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.onConnectionSignalling.rejects('timeout or error');

      var subscription_id;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          return portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', sdp: 'candidateString'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
        });
    });

    it('rpcClient.sub2Session(to controller) timeout or error should cause fail subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.sub2Session.rejects('timeout or error');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'failed', reason: 'timeout or error'}]);
        });
    });

    it('The connection reporting a status with type of #failed# should abort subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.onConnectionSignalling = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.onConnectionSignalling.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'failed', reason: 'reasonString'});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.have.string('reasonString');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'reasonString'}]);
          expect(mockRpcClient.sub2Session.callCount).to.equal(0);
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
          return expect(portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', candidate: 'candidateString'}))
            .to.be.rejectedWith('Connection does NOT exist when receiving a signaling.');
        });
    });

    it('Participant early leaving should abort subscribing.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();
      mockRpcClient.leave = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.leave.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          return portal.leave(testParticipantId);
        })
        .then(function(leaveResult) {
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
          expect(mockRpcClient.sub2Session.callCount).to.equal(0);
          expect(mockRpcClient.unsub2Session.callCount).to.equal(0);
          return expect(on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}))
            .to.be.rejectedWith('Participant ' + testParticipantId + ' has left when the connection gets ready.');
        });
    });

    it('Unsubscribing before subscribing should fail.', function() {
      return expect(portal.unsubscribe(testParticipantId, 'subscriptionId'))
        .to.be.rejectedWith('subscription does not exist');
    });

    it('Unsubscribing a half subscribed stream(connected but not subscribed from controller) should cause disconnect(to access node) and return ok.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');

      var subscription_id;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return portal.unsubscribe(testParticipantId, subscription_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
        });
    });

    it('Unsubscribing a successfully subscribed stream should cause disconnect(to access node) and unsubscribe(to controller) and return ok.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockRpcClient.sub2Session.callCount).to.equal(1);
          return portal.unsubscribe(testParticipantId, subscription_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockRpcClient.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, consumer: subscription_id}]);
          expect(mockRpcClient.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
        });
    });

    it('Switching on/off the media tracks before subscribing should fail.', function() {
      return expect(portal.mediaOnOff(testParticipantId, 'subscriptionId', 'video', 'out', 'off'))
          .to.be.rejectedWith('connection does not exist');
    });

    it('Switching on/off the media tracks after subscribing should proceed.', function() {
      mockRpcClient.getAccessNode = sinon.stub();
      mockRpcClient.subscribe = sinon.stub();
      mockRpcClient.unsubscribe = sinon.stub();
      mockRpcClient.recycleAccessNode = sinon.stub();
      mockRpcClient.sub2Session = sinon.stub();
      mockRpcClient.mediaOnOff = sinon.stub();
      mockRpcClient.unsub2Session = sinon.stub();

      mockRpcClient.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockRpcClient.subscribe.resolves('ok');
      mockRpcClient.unsubscribe.resolves('ok');
      mockRpcClient.recycleAccessNode.resolves('ok');
      mockRpcClient.sub2Session.resolves('ok');
      mockRpcClient.mediaOnOff.onCall(0).returns(Promise.resolve('ok'));
      mockRpcClient.mediaOnOff.onCall(1).returns(Promise.reject('timeout or error'));
      mockRpcClient.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockRpcClient.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockRpcClient.sub2Session.callCount).to.equal(1);
          return portal.mediaOnOff(testParticipantId, 'targetStreamId', 'video', 'out', 'off');
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.mediaOnOff.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, 'video', 'out', 'off']);
          return expect(portal.mediaOnOff(testParticipantId, 'targetStreamId', 'video', 'out', 'on')).to.be.rejectedWith('timeout or error');
        })
        .then(function() {
          return portal.leave(testParticipantId);
        });
    });
  });
});

describe('portal.getRegion/portal.setRegion: Manipulate the mixed stream.', function() {
  it('Getting/setting region before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    return Promise.all([
      expect(portal.getRegion(testParticipantId, 'subStreamId')).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(portal.setRegion(testParticipantId, 'subStreamId', 500)).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.')
      ]);
  });

  describe('Getting/setting region after successfully logining should proceed.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        done();
      });
    });

    afterEach(function() {
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');
      portal.leave(testParticipantId);
    });

    it('Getting region should succeed if rpcClient.getRegion succeeds.', function() {
      mockRpcClient.getRegion = sinon.stub();
      mockRpcClient.getRegion.resolves('regionId');

      return portal.getRegion(testParticipantId, 'subStreamId')
        .then(function(result) {
          expect(result).to.equal('regionId');
          expect(mockRpcClient.getRegion.getCall(0).args).to.deep.equal(['rpcIdOfController', 'subStreamId']);
        });
    });

    it('Getting region should fail if rpcClient.getRegion fails.', function() {
      mockRpcClient.getRegion = sinon.stub();
      mockRpcClient.getRegion.rejects('no such a stream');

      return expect(portal.getRegion(testParticipantId, 'subStreamId')).to.be.rejectedWith('no such a stream');
    });

    it('Setting region should succeed if rpcClient.setRegion succeeds.', function() {
      mockRpcClient.setRegion = sinon.stub();
      mockRpcClient.setRegion.resolves('ok');

      return portal.setRegion(testParticipantId, 'subStreamId', 500)
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.setRegion.getCall(0).args).to.deep.equal(['rpcIdOfController', 'subStreamId', 500]);
        });
    });

    it('Setting region should fail if rpcClient.setRegion fails.', function() {
      mockRpcClient.setRegion = sinon.stub();
      mockRpcClient.setRegion.rejects('failed-for-some-reason');

      return expect(portal.setRegion(testParticipantId, 'subStreamId', 500)).to.be.rejectedWith('failed-for-some-reason');
    });
  });
});

describe('portal.text', function() {
  it('Sending text before logining should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    return expect(portal.text(testParticipantId, 'all', 'Hi, there!')).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Sending text without permission should fail.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    mockRpcClient.tokenLogin = sinon.stub();
    mockRpcClient.getController = sinon.stub();
    mockRpcClient.join = sinon.stub();

    mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'no_text_viewer', room: testSession});
    mockRpcClient.getController.resolves('rpcIdOfController');
    mockRpcClient.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        return expect(portal.text(testParticipantId, 'all', 'Hi, there!')).to.be.rejectedWith('unauthorized');
      });
  });

  describe('Sending text after successfully logining should proceed.', function() {
    var mockRpcClient = sinon.createStubInstance(rpcClient);
    var portal = Portal(testPortalSpec, mockRpcClient);

    beforeEach(function(done) {
      mockRpcClient.tokenLogin = sinon.stub();
      mockRpcClient.getController = sinon.stub();
      mockRpcClient.join = sinon.stub();

      mockRpcClient.tokenLogin.resolves({userName: 'Jack', role: 'presenter', room: testSession});
      mockRpcClient.getController.resolves('rpcIdOfController');
      mockRpcClient.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(login_result) {
        done();
      });
    });

    afterEach(function() {
      mockRpcClient.leave = sinon.stub();
      mockRpcClient.leave.resolves('ok');
      portal.leave(testParticipantId);
    });

    it('Should succeed if rpcClient.text succeeds.', function() {
      mockRpcClient.text = sinon.stub();
      mockRpcClient.text.resolves('ok');

      return portal.text(testParticipantId, 'all', 'Hi, there!')
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockRpcClient.text.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, 'all', 'Hi, there!']);
        });
    });

    it('Should fail if rpcClient.text fails.', function() {
      mockRpcClient.text = sinon.stub();
      mockRpcClient.text.rejects('no such a receiver');

      return expect(portal.text(testParticipantId, 'a-non-existed-receiver', 'some-message')).to.be.rejectedWith('no such a receiver');
    });
  });
});

