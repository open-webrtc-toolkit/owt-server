var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');
var path = require('path');
var mockery = require('mockery');

// Mock dataBaseAccess module before require portal
mockery.enable({
  warnOnUnregistered: false
});

var mockDb = {};
var dbAccess;
dbAccess = {
  getRolesOfRoom: function(roomId) {
    return Promise.resolve(testPortalSpec.permissionMap);
  },
  getPermissionOfUser: function(userId, roomId){
    return Promise.resolve(mockDb[userId]);
  },
  savePermissionOfUser: function(userId, roomId, permission) {
    mockDb[userId] = permission;
    return Promise.resolve(mockDb[userId]);
  },
};
mockery.registerMock("./dataBaseAccess", dbAccess);

after(function(){
    mockery.disable();
});

var Portal = require('../portal');
var rpcReq = require('../rpcRequest');

var testTokenKey = '6d96729ffd07202e7a65ac81278c43a2c87fc6367736431e614607e9d3ceee201f9680cdfcb88df508829e9810c46aaf02c9cc8dcf46369de122ee5b22ec963c';
var testSelfRpcId = 'portal-38A87euk9kfh';
var testTokenServer = 'nuve';
var testClusterName = 'woogeen-cluster';
var testPortalSpec = {tokenKey: testTokenKey,
                      tokenServer: testTokenServer,
                      clusterName: testClusterName,
                      selfRpcId: testSelfRpcId,
                      permissionMap: {'admin': {publish: true, subscribe: true, record: true, addExternalOutput: true, manage:true},
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


describe('portal.updateTokenKey: update the token key.', function() {
  it('Joining with a valid token will fail if the token key is out of time, and success when token key is updated', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    var portalSpecWithOldTokenKey = {tokenKey: 'OldTokenKey',
                                     tokenServer: testTokenServer,
                                     clusterName: testClusterName,
                                     selfRpcId: testSelfRpcId,
                                     permissionMap: {'presenter': {publish: true, subscribe: true}}};

    var portal = Portal(portalSpecWithOldTokenKey, mockrpcReq);

    var login_result = {code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession};
    dbAccess.deleteToken.resolves(login_result);

    mockrpcReq.getController.resolves('rpcIdOfController');

    var join_result = {participants: [],
                       streams: []};
    mockrpcReq.join.resolves(join_result);

    var final_result = {tokenCode: login_result.code, user: login_result.userName, role: login_result.role, session_id: login_result.room, participants: join_result.participants, streams: join_result.streams};

    return portal.join(testParticipantId, testToken).then(function(runInHere) {
        expect(runInHere).to.be.false;
      }, function(reason) {
        expect(reason).to.have.string('Invalid token signature');
        portal.updateTokenKey(testTokenKey);
        return portal.join(testParticipantId, testToken).then(function(result) {
          expect(result).to.deep.equal(final_result);
          expect(mockrpcReq.getController.getCall(0).args).to.deep.equal(['woogeen-cluster', testSession]);
          expect(mockrpcReq.join.getCall(0).args).to.deep.equal(['rpcIdOfController', testSession, {id: testParticipantId, name: 'Jack', role: 'presenter', portal: testSelfRpcId}]);
        });
      });
  });
});

describe('portal.join: Participants join.', function() {
  it('Joining with valid token should succeed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    var portal = Portal(testPortalSpec, mockrpcReq);

    var login_result = {code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession};
    dbAccess.deleteToken.resolves(login_result);

    mockrpcReq.getController.resolves('rpcIdOfController');

    var join_result = {participants: [],
                       streams: []};
    mockrpcReq.join.resolves(join_result);

    var final_result = {tokenCode: login_result.code, user: login_result.userName, role: login_result.role, session_id: login_result.room, participants: join_result.participants, streams: join_result.streams};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      expect(mockrpcReq.getController.getCall(0).args).to.deep.equal(['woogeen-cluster', testSession]);
      expect(mockrpcReq.join.getCall(0).args).to.deep.equal(['rpcIdOfController', testSession, {id: testParticipantId, name: 'Jack', role: 'presenter', portal: testSelfRpcId}]);
    });
  });

  it('Tokens with incorect signature should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    var tokenWithInvalidSignature = { tokenId: '573eab88111478bb3526421b',
                                      host: '10.239.158.161:8080',
                                      secure: true,
                                      signature: 'AStringAsInvalidSignature' };
    return expect(portal.join(testParticipantId, tokenWithInvalidSignature)).to.be.rejectedWith('Invalid token signature');//Note: The error message is not strictly equality checked, instead it will be fullfiled when the actual error message includes the given string here. That is the inherint behavor of 'rejectedWith'.
  });

  it('deleteToken timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcReq.getController timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    var login_result = {code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession};
    dbAccess.deleteToken.resolves(login_result);

    mockrpcReq.getController.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcReq.join timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    var login_result = {code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession};
    dbAccess.deleteToken.resolves(login_result);

    var controller_result = 'theRpcIdOfController';
    mockrpcReq.getController.resolves(controller_result);

    mockrpcReq.join.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });
});

describe('portal.leave: Participants leave.', function() {
  it('Leaving without logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    mockrpcReq.unpublish = sinon.stub();
    mockrpcReq.unsubscribe = sinon.stub();
    mockrpcReq.recycleAccessNode = sinon.stub();
    mockrpcReq.unpub2Session = sinon.stub();
    mockrpcReq.unsub2Session = sinon.stub();
    mockrpcReq.leave = sinon.stub();

    return portal.leave(testParticipantId)
      .then(function(runInHere) {
        expect(runInHere).to.be.false;
      }, function(err) {
        expect(err).to.equal('Participant ' + testParticipantId + ' does NOT exist.');
        expect(mockrpcReq.unpublish.callCount).to.equal(0);
        expect(mockrpcReq.unsubscribe.callCount).to.equal(0);
        expect(mockrpcReq.recycleAccessNode.callCount).to.equal(0);
        expect(mockrpcReq.unpub2Session.callCount).to.equal(0);
        expect(mockrpcReq.unsub2Session.callCount).to.equal(0);
        expect(mockrpcReq.leave.callCount).to.equal(0);
      });
  });

  describe('Leaving should cause releasing the media connections and unpublishing/unsubscribing.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();

      var login_result = {code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession};
      dbAccess.deleteToken.resolves(login_result);
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({
                                   participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(loginResult) {
        done();
      });
    });

    it('Leaving after successfully logining should succeed.', function() {
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');

      return portal.leave(testParticipantId)
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.leave.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId]);
        });
    });

    it('Leaving after successfully publishing should cause disconnecting and unpublishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();
      mockrpcReq.unpub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');
      mockrpcReq.unpub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(locality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
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
          expect(mockrpcReq.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
        });
     });

    it('Leaving after successfully subscribing should cause disconnecting and unsubscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(result) {
          return portal.leave(testParticipantId);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
        });
     });
  });
});

describe('portal.publish/portal.unpublish: Participants publish/unpublish streams', function() {
  it('Publishing before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var spyConnectionObserver = sinon.spy();

    return expect(portal.publish(testParticipantId,
                                 'streamId',
                                 'webrtc',
                                 {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                 spyConnectionObserver,
                                 false))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Publishing without permission(publish: undefined | false) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.publish(testParticipantId,
                                     'streamId',
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Publishing without permission(publish: {audio: true} but stream contains video) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer_pub_a', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.publish(testParticipantId,
                                     'streamId',
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Publishing without permission(publish: {video: true} but stream contains video) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer_pub_v', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.publish(testParticipantId,
                                     'streamId',
                                     'webrtc',
                                     {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                     spyConnectionObserver,
                                     false))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Unpublishing before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return expect(portal.unpublish(testParticipantId, 'streamId'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  describe('Publishing with proper permission should proceed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();

      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession});
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        done();
      });
    });

    afterEach(function() {
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.unpub2Session = sinon.stub();
      mockrpcReq.unpub2Session.resolves('ok');
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');
      portal.leave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
    });

    it('Publishing an rtsp/rtmp stream with both audio and video should succeed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'avstream',
                            {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera', transport: 'tcp', bufferSize: 2048},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          expect(mockrpcReq.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'avstream', {session: testSession, task: stream_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.publish.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.publish.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.publish.getCall(0).args[1]).to.equal(stream_id);
          expect(mockrpcReq.publish.getCall(0).args[2]).to.equal('avstream');
          expect(mockrpcReq.publish.getCall(0).args[3]).to.deep.equal({controller: testSelfRpcId, audio: true, video: {resolution: 'unknown'}, url: 'URIofSomeIPCamera', transport: 'tcp', buffer_size: 2048});
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
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
          expect(mockrpcReq.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'hd1080p', device: 'unknown', codec: 'h264'}, type: 'avstream'}, false]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264'], video_resolution: 'hd1080p'}]);
        });
    });

    it('Publishing rtsp/rtmp streams should be aborted if exceptions occur.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.unpub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');
      mockrpcReq.unpub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'avstream',
                            {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera', transport: 'tcp', buffer_size: 2048},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
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
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
          expect(mockrpcReq.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
        });
    });

    it('Publishing rtsp/rtmp streams should be aborted if the access node fault is detected.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.unpub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');
      mockrpcReq.unpub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'avstream',
                            {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera', transport: 'tcp', buffer_size: 2048},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['h264'], video_resolution: 'hd1080p'});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return portal.onFaultDetected({purpose: 'avstream', type: 'node', id: 'rpcIdOfAccessNode'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          expect(reason).to.have.string('access node exited unexpectedly');
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
          expect(mockrpcReq.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
        });
    });

    it('Publishing a webrtc stream with both audio and video should succeed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          expect(mockrpcReq.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {session: testSession, task: stream_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.publish.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.publish.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.publish.getCall(0).args[1]).to.equal(stream_id);
          expect(mockrpcReq.publish.getCall(0).args[2]).to.equal('webrtc');
          expect(mockrpcReq.publish.getCall(0).args[3]).to.deep.equal({controller: testSelfRpcId, audio: true, video: {resolution: 'vga'}});
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
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
          expect(mockrpcReq.onConnectionSignalling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return portal.onConnectionSignalling(testParticipantId, stream_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onConnectionSignalling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', stream_id, {type: 'candidate', sdp: 'candidateString'}]);
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
          expect(mockrpcReq.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'vga', framerate: 30, device: 'camera', codec: 'vp8'}, type: 'webrtc'}, false]);
          expect(spyConnectionObserver.getCall(3).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']}]);
        });
    });

    it('rpcReq.getAccessNode timeout or error should fail publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.getAccessNode.rejects('getAccessNode timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.publish(testParticipantId,
                            'streamId',
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false))
        .to.be.rejectedWith('getAccessNode timeout or error.');
    });

    it('rpcReq.publish timeout or error should fail publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.rejects('connect timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.publish(testParticipantId,
                            'streamId',
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false))
        .to.be.rejectedWith('connect timeout or error.');
    });

    it('rpcReq.onConnectionSignalling timeout or error should fail publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();


      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.rejects('timeout or error');

      var stream_id = 'streamId';
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          return portal.onConnectionSignalling(testParticipantId, stream_id, {type: 'candidate', sdp: 'candidateString'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
        });
    });

    it('No proper audio/video codec during negotiation should fail publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();


      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok.');
      mockrpcReq.unpublish.resolves('ok.');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.rejects('timeout or error');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: [], video_codecs: ['vp8', 'h264']});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.equal('No proper audio codec');
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);

          var stream_id1 = 'streamId1', on_connection_status1;
          var spyConnectionObserver1 = sinon.spy();
          return portal.publish(testParticipantId,
                                stream_id1,
                                'webrtc',
                                {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                spyConnectionObserver1,
                                false)
            .then(function(connectionLocality) {
              on_connection_status1 = mockrpcReq.publish.getCall(1).args[4];
              return on_connection_status1({type: 'ready', audio_codecs: ['opus'], video_codecs: []});
            }).then(function(runInHere) {
              expect(runInHere).to.be.false;
            }, function(err) {
              expect(err).to.equal('No proper video codec');
              expect(mockrpcReq.unpublish.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', stream_id1]);
              expect(mockrpcReq.recycleAccessNode.getCall(1).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id1}]);
              var stream_id2 = 'streamId2', on_connection_status2;
              var spyConnectionObserver2 = sinon.spy();
              return portal.publish(testParticipantId,
                                    stream_id2,
                                    'avstream',
                                    {audio: true, video: {resolution: 'unknown', device: 'unknown'}, url: 'URIofSomeIPCamera'},
                                    spyConnectionObserver2,
                                    false)
                .then(function(connectionLocality) {
                  on_connection_status2 = mockrpcReq.publish.getCall(2).args[4];
                  return on_connection_status2({type: 'ready', audio_codecs: undefined, video_codecs: ['h264']});
                }).then(function(runInHere) {
                  expect(runInHere).to.be.false;
                }, function(err) {
                  expect(err).to.equal('No proper audio codec');
                  expect(mockrpcReq.unpublish.getCall(2).args).to.deep.equal(['rpcIdOfAccessNode', stream_id2]);
                  expect(mockrpcReq.recycleAccessNode.getCall(2).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id2}]);
                });
            });
        });
    });

    it('rpcReq.pub2Session(to controller) timeout or error should fail publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.pub2Session.rejects('timeout or error');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            true)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockrpcReq.pub2Session.callCount).to.equal(1);
          expect(mockrpcReq.pub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {codec: 'pcmu'}, video: {resolution: 'vga', framerate: 30, device: 'camera', codec: 'vp8'}, type: 'webrtc'}, true]);
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'timeout or error'}]);
        });
    });

    it('The connection reporting a status with type of #failed# should abort publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'failed', reason: 'reasonString'});
        })
        .then(function(statusResult) {
          expect('run into here').to.be.false;
        }, function(err) {
          expect(err).to.equal('reasonString');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'reasonString'}]);
          expect(mockrpcReq.pub2Session.callCount).to.equal(0);
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
        });
    });

    it('Participant early leaving should abort publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();
      mockrpcReq.unpub2Session = sinon.stub();
      mockrpcReq.leave = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.leave.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          return portal.leave(testParticipantId);
        })
        .then(function(leaveResult) {
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
          expect(mockrpcReq.pub2Session.callCount).to.equal(0);
          expect(mockrpcReq.unpub2Session.callCount).to.equal(0);
          return expect(on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']}))
            .to.be.rejectedWith('Participant ' + testParticipantId + ' has left when the connection gets ready.');
        });
    });

    it('Fault being detected on the access agent should abort publishing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return portal.onFaultDetected({purpose: 'webrtc', type: 'worker', id: 'rpcIdOfAccessAgent'});
        })
        .then(function(statusResult) {
          expect('run into here').to.be.false;
        }, function(err) {
          expect(err).to.equal('access node exited unexpectedly');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'access node exited unexpectedly'}]);
          expect(mockrpcReq.pub2Session.callCount).to.equal(0);
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
        });
    });

    it('Unpublishing before publishing should fail.', function() {
      return expect(portal.unpublish(testParticipantId, 'streamId'))
        .to.be.rejectedWith('stream does not exist');
    });

    it('Unpublishing a half published stream(connected but not published to controller) should cause disconnect(to access node) and return ok.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');

      var stream_id = 'streamId';
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          return portal.unpublish(testParticipantId, stream_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
        });
    });

    it('Unpublishing a successfully published stream should cause disconnect(to access node) and unpublish(to controller) and return ok.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();
      mockrpcReq.unpub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');
      mockrpcReq.unpub2Session.resolves('ok');

      var stream_id = 'streamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.publish(testParticipantId,
                            stream_id,
                            'webrtc',
                            {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                            spyConnectionObserver,
                            false)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockrpcReq.pub2Session.callCount).to.equal(1);
          return portal.unpublish(testParticipantId, stream_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unpublish.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', stream_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: stream_id}]);
          expect(mockrpcReq.unpub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, stream_id]);
        });
    });
  });
});

describe('portal.setMute/portal.mix/portal.unmix: Administrators manipulate streams that published.', function() {
  it('Should fail before joining', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var mix = portal.mix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);
    var unmix = portal.unmix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);
    var setMute = portal.setMute(testParticipantId, 'streamId', true);

    return Promise.all([
      expect(mix).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(unmix).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(setMute).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
    ]);
  });

  it('Should fail without manage permission', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
    .then(function(joinResult) {
      var setMute = portal.setMute(testParticipantId, 'streamId', true);
      var mix = portal.mix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);
      var unmix = portal.unmix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);

      return Promise.all([
        expect(mix).to.be.rejectedWith('Mix Permission Denied'),
        expect(unmix).to.be.rejectedWith('Unmix Permission Denied'),
        expect(setMute).to.be.rejectedWith('Mute/Unmute Permission Denied'),
      ]);
    });
  });

  it('Should success after login with manage permission', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    mockrpcReq.mix = sinon.stub();
    mockrpcReq.unmix = sinon.stub();
    mockrpcReq.setMute = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});
    mockrpcReq.mix.resolves('ok');
    mockrpcReq.unmix.resolves('ok');
    mockrpcReq.setMute.resolves('ok');


    return portal.join(testParticipantId, testToken)
    .then(function(joinResult) {
      var mix = portal.mix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);
      var unmix = portal.unmix(testParticipantId, 'streamId', ['mixStream1', 'mixStream2']);
      var setMute = portal.setMute(testParticipantId, 'streamId', true);

      return Promise.all([
        expect(mix).to.become('ok'),
        expect(unmix).to.become('ok'),
        expect(setMute).to.become('ok'),
      ]);
    });
  });
});

describe('portal.setPermission: Administrators update user permission.', function() {
  it('Should fail before joining', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var setPermission = portal.setPermission(testParticipantId, 'targetUserID', 'publish', true);

    return Promise.all([
      expect(setPermission).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.')
    ]);
  });

  it('Should fail with invalid target user', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
    .then(function(joinResult) {
      var setPermission = portal.setPermission(testParticipantId, 'invalidUserID', 'publish', true);

      return Promise.all([
        expect(setPermission).to.be.rejectedWith('Target ' + 'invalidUserID' + ' does NOT exist.')
      ]);
    });
  });

  it('Should fail without manage permission', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [{id: 'targetUserID', name: 'targetUserName', role: 'presenter'}],
                                 streams: []});

    return portal.join('targetUserID', testToken)
    .then(function(targetJoin) {
      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Tom', role: 'viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
      return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        var setPermission = portal.setPermission(testParticipantId, 'targetUserID', 'publish', false);

        return Promise.all([
          expect(setPermission).to.be.rejectedWith('setPermission Permission Denied')
        ]);
      });
    });
  });

  it('Should success after login with manage permission', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [{id: 'targetUserID', name: 'targetUserName', role: 'presenter'}],
                                 streams: []});

    return portal.join('targetUserID', testToken)
    .then(function(targetJoin) {
      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Tom', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
      return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        var setPermission = portal.setPermission(testParticipantId, 'targetUserID', 'publish', false)
          .then(function(result) {
            expect(mockDb['targetUserID'].customize).deep.equal({ 'publish': false });
            return result;
          });

        return Promise.all([
          expect(setPermission).to.become('ok'),
        ]);
      });
    });
  });
});

describe('portal.setVideoBitrate/portal.mediaOnOff: Participants manipulate streams they published.', function() {
  it('Should fail before joining', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var setVideoBitrate = portal.setVideoBitrate(testParticipantId, 'streamId', 500),
        mediaOnOff = portal.mediaOnOff(testParticipantId, 'streamId', 'video', 'in', 'off');

    return Promise.all([
      expect(setVideoBitrate).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(mediaOnOff).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.')
    ]);
  });

  it('Should fail before publishing.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        var setVideoBitrate = portal.setVideoBitrate(testParticipantId, 'streamId', 500),
            mediaOnOff = portal.mediaOnOff(testParticipantId, 'streamId', 'video', 'in', 'off');

        return Promise.all([
          expect(setVideoBitrate).to.be.rejectedWith('stream does not exist'),
          expect(mediaOnOff).to.be.rejectedWith('connection does not exist')
        ]);
      });
  });

  describe('Manipulating streams after publishing.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);
    var testStreamId = 'streamId';

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.publish = sinon.stub();
      mockrpcReq.pub2Session = sinon.stub();

      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({participants: [],
                                   streams: []});
      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.publish.resolves('ok');
      mockrpcReq.pub2Session.resolves('ok');

      var spyConnectionObserver = sinon.spy(),
        on_connection_status;

      return portal.join(testParticipantId, testToken)
        .then(function(joinResult) {
          return portal.publish(testParticipantId,
                                testStreamId,
                                'webrtc',
                                {audio: true, video: {resolution: 'vga', framerate: 30, device: 'camera'}},
                                spyConnectionObserver,
                                false);
        })
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.publish.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockrpcReq.pub2Session.callCount).to.equal(1);
          done();
        });
    });

    afterEach(function() {
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.unpub2Session = sinon.stub();
      mockrpcReq.unpub2Session.resolves('ok');
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');
      portal.leave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
      testStreamId = undefined;
    });

    it('Should succeed if rpcReq.setVideoBitrate/rpcReq.mediaOnOff succeeds.', function() {
      mockrpcReq.setVideoBitrate = sinon.stub();
      mockrpcReq.mediaOnOff = sinon.stub();
      mockrpcReq.updateStream = sinon.stub();

      mockrpcReq.setVideoBitrate.resolves('ok');
      mockrpcReq.updateStream.resolves('ok');
      mockrpcReq.mediaOnOff.resolves('ok');

      var setVideoBitrate = portal.setVideoBitrate(testParticipantId, testStreamId, 500),
          mediaOnOff = portal.mediaOnOff(testParticipantId, testStreamId, 'video', 'in', 'off');

      return Promise.all([
        expect(setVideoBitrate).to.become('ok'),
        expect(mediaOnOff).to.become('ok')
        ])
        .then(function() {
          expect(mockrpcReq.setVideoBitrate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testStreamId, 500]);
          expect(mockrpcReq.mediaOnOff.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testStreamId, 'video', 'in', 'off']);
        });
    });

    it('Should fail if rpcReq.setVideoBitrate/rpcReq.mediaOnOff fails.', function() {
      mockrpcReq.setVideoBitrate = sinon.stub();
      mockrpcReq.mediaOnOff = sinon.stub();
      mockrpcReq.updateStream = sinon.stub();

      mockrpcReq.updateStream.rejects('timeout or error');
      mockrpcReq.setVideoBitrate.rejects('timeout or error');
      mockrpcReq.mediaOnOff.rejects('timeout or error');

      var setVideoBitrate = portal.setVideoBitrate(testParticipantId, testStreamId, 500),
          mediaOnOff = portal.mediaOnOff(testParticipantId, testStreamId, 'video', 'in', 'off');

      return Promise.all([
        expect(setVideoBitrate).to.be.rejectedWith('timeout or error'),
        expect(mediaOnOff).to.be.rejectedWith('timeout or error')
        ]);
    });
  });
});

describe('portal.subscribe/portal.unsubscribe/portal.mediaOnOff: Participants subscribe/unsubscribe streams and switch on/off the media tracks of the subscribed streams.', function() {
  it('Subscribing before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var spyConnectionObserver = sinon.spy();

    return expect(portal.subscribe(testParticipantId,
                                  'subscriptionId',
                                  'webrtc',
                                  {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga', quality_level: 'standard'}},
                                   spyConnectionObserver))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Subscribing to webrtc clients without permission(subscribe: undefined | false) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'guest', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.subscribe(testParticipantId,
                                       'subscriptionId',
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga', quality_level: 'standard'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to webrtc clients without permission(subscribe: {audio: true} and require video as well) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'a_viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.subscribe(testParticipantId,
                                       'subscriptionId',
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga', quality_level: 'standard'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to webrtc clients without permission(subscribe: {video: true} and require audio as well) should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'v_viewer', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.subscribe(testParticipantId,
                                       'subscriptionId',
                                       'webrtc',
                                       {streamId: 'targetStreamId', audio: true, video: {resolution: 'vga', quality_level: 'standard'}},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Subscribing to recording files without permission should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    var spyConnectionObserver = sinon.spy();

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.subscribe(testParticipantId,
                                       'subscriptionId',
                                       'recording',
                                       {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                                       spyConnectionObserver))
          .to.be.rejectedWith('unauthorized');
      });
  });

  it('Unsubscribing before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return expect(portal.unsubscribe(testParticipantId, 'subscriptionId'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Switching on/off the media tracks before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return expect(portal.mediaOnOff(testParticipantId, 'subscriptionId', 'video', 'out', 'off'))
        .to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  describe('Subscribing after successfully logining should proceed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();

      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        done();
      });
    });

    afterEach(function() {
      mockrpcReq.unpublish = sinon.stub();
      mockrpcReq.unpublish.resolves('ok');
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.unsub2Session = sinon.stub();
      mockrpcReq.unsub2Session.resolves('ok');
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');
      portal.leave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
    });

    it('Subscribing an already-subscribed stream should fail.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var subscription_id, on_connection_status;
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                             'subscriptionId',
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                             spyConnectionObserver)
        .then(function(subscriptionId) {
          subscription_id = subscriptionId;
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockrpcReq.sub2Session.callCount).to.equal(1);
          return expect(portal.subscribe(testParticipantId,
                                         'subscriptionId',
                                         'webrtc',
                                         {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                                         spyConnectionObserver1)).to.be.rejectedWith('Not allowed to subscribe an already-subscribed stream');
        });
    });

    it('Subscribing a stream with both audio and video to a webrtc client should succeed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');

      var subscription_id = testParticipantId + '-sub-' + 'targetStreamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                             'webrtc',
                             {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          expect(mockrpcReq.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {session: testSession, task: subscription_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockrpcReq.subscribe.getCall(0).args[2]).to.equal('webrtc');
          expect(mockrpcReq.subscribe.getCall(0).args[3]).to.deep.equal({controller: testSelfRpcId, audio: true, video: {resolution: 'vga', quality_level: 'standard'}});
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
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
          expect(mockrpcReq.onConnectionSignalling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onConnectionSignalling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, {type: 'candidate', sdp: 'candidateString'}]);
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
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8', 'h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId', codecs: ['vp8', 'h264'], resolution: 'vga', quality_level: 'standard'}, type: 'webrtc'}]);
          expect(spyConnectionObserver.getCall(3).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8', 'h264']}]);
        });
    });

    it('Subscribing a stream to an rtsp/rtmp url should succeed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                             'avstream',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['aac']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, url: 'rtmp://user:pwd@1.1.1.1:9000/url-of-avstream'},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          expect(mockrpcReq.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'avstream', {session: testSession, task: subscription_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockrpcReq.subscribe.getCall(0).args[2]).to.equal('avstream');
          expect(mockrpcReq.subscribe.getCall(0).args[3]).to.deep.equal({controller: testSelfRpcId, audio: {codecs: ['aac']}, video: {codecs: ['h264'], resolution: 'vga'}, url: 'rtmp://user:pwd@1.1.1.1:9000/url-of-avstream'});
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId1', codecs: ['pcm_raw']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, type: 'avstream'}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']}]);
        });
    });

    it('Subscribing streams to an rtsp/rtmp url should abort when exceptions occur.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                             'avstream',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['aac']}, video: {fromStream: 'targetStreamId2', codecs: ['h264'], resolution: 'vga'}, url: 'rtmp://user:pwd@1.1.1.1:9000/url-of-avstream'},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.sub2Session.callCount).to.equal(1);
          return on_connection_status({type: 'failed', reason: 'network error'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          console.log(reason);
          expect(reason).to.equal('network error');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
          expect(mockrpcReq.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'failed', reason: 'network error'}]);
        });
    });

    it('Subscribing streams to a recording file should succeed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: 1000},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          expect(connectionLocality).to.deep.equal({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
          expect(mockrpcReq.getAccessNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'recording', {session: testSession, task: subscription_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.subscribe.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.subscribe.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.subscribe.getCall(0).args[1]).to.equal(subscription_id);
          expect(mockrpcReq.subscribe.getCall(0).args[2]).to.equal('recording');
          expect(mockrpcReq.subscribe.getCall(0).args[3]).to.deep.equal({controller: testSelfRpcId, audio: {codecs: ['pcmu']}, video: {codecs: ['vp8']}, path: 'path-of-recording', filename: 'room_' + testSession + '-' + subscription_id + '.mkv', interval: 1000});
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          expect(on_connection_status).to.be.a('function');
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.sub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, type: 'recording'}]);
          expect(spyConnectionObserver.getCall(1).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}]);
        });
    });

    it('Subscribing streams with the same subscription-id with the previous one and with type of recording should continue.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var on_connection_status;
      var testRecorderId = '2016082415322905';
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                              testRecorderId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return portal.subscribe(testParticipantId,
                                  testRecorderId,
                                 'recording',
                                 {audio: {fromStream: 'targetStreamId3', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId4', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                                 spyConnectionObserver1);
        })
        .then(function(connectionLocality) {
          return new Promise(function(resolve, reject) {setTimeout(function() {resolve('ok');}, 30);});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, testRecorderId]);
          expect(mockrpcReq.sub2Session.getCall(1).args).to.deep.equal(['rpcIdOfController', testParticipantId, testRecorderId, {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, {audio: {fromStream: 'targetStreamId3', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId4', codecs: ['vp8']}, type: 'recording'}]);
          expect(spyConnectionObserver1.getCall(0).args).to.deep.equal([{type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}]);
        });
    });

    it('Subscribing streams with the same subscription-id but different audio codec with the previous one and with type of recording should fail.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var on_connection_status;
      var testRecorderId = '2016082415322905';
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                              testRecorderId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return portal.subscribe(testParticipantId,
                                  testRecorderId,
                                 'recording',
                                 {audio: {fromStream: 'targetStreamId3', codecs: ['opus']}, video: {fromStream: 'targetStreamId4', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                                 spyConnectionObserver1);
        })
        .then(function(runToHere) {
          expect(runToHere).to.be.false;
        }, function(reason) {
          expect(reason).to.equal('Can not continue the current recording with a different audio codec.');
        });
    });

    it('Subscribing streams with the same subscription-id but different video codec with the previous one and with type of recording should fail.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var on_connection_status;
      var testRecorderId = '2016082415322905';
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                              testRecorderId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return portal.subscribe(testParticipantId,
                                  testRecorderId,
                                 'recording',
                                 {audio: {fromStream: 'targetStreamId3', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId4', codecs: ['h264']}, path: 'path-of-recording', interval: -1},
                                 spyConnectionObserver1);
        })
        .then(function(runToHere) {
          expect(runToHere).to.be.false;
        }, function(reason) {
          expect(reason).to.equal('Can not continue the current recording with a different video codec.');
        });
    });

    it('Subscribing streams with the same subscription-id with the previous one but not with type of recording should fail.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var on_connection_status;
      var targetURL = 'rtmp://xxx.yyy.zzz/live/1935';
      var spyConnectionObserver = sinon.spy();
      var spyConnectionObserver1 = sinon.spy();

      return portal.subscribe(testParticipantId,
                              targetURL,
                             'avstream',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['aac']}, video: {fromStream: 'targetStreamId2', codecs: ['h264']}, url: targetURL},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcm_raw'], video_codecs: ['h264']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return portal.subscribe(testParticipantId,
                                  targetURL,
                                 'avstream',
                                 {audio: {fromStream: 'targetStreamId3', codecs: ['aac']}, video: {fromStream: 'targetStreamId4', codecs: ['h264']}, url: targetURL},
                                 spyConnectionObserver1);
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        },function(reason) {
          expect(reason).to.be.equal('Not allowed to subscribe an already-subscribed stream');
        });
    });

    it('Subscribing streams to a recording file should abort when exceptions occur.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');


      var testRecorderId = '2016061416024671';
      var spyConnectionObserver = sinon.spy();
      var on_connection_status;

      return portal.subscribe(testParticipantId,
                              testRecorderId,
                             'recording',
                             {audio: {fromStream: 'targetStreamId1', codecs: ['pcmu']}, video: {fromStream: 'targetStreamId2', codecs: ['vp8']}, path: 'path-of-recording', interval: -1},
                             spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('initializing');
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.sub2Session.callCount).to.equal(1);
          return on_connection_status({type: 'failed', reason: 'write file error'});
        })
        .then(function(runHere) {
          expect(runHere).to.be.false;
        }, function(reason) {
          console.log(reason);
          expect(reason).to.equal('write file error');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', testRecorderId]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: testRecorderId}]);
          expect(mockrpcReq.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, testRecorderId]);
          expect(spyConnectionObserver.getCall(2).args).to.deep.equal([{type: 'failed', reason: 'write file error'}]);
        });
    });

    it('rpcReq.getAccessNode timeout or error should fail subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.getAccessNode.rejects('getAccessNode timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.subscribe(testParticipantId,
                                     'subscriptionId',
                                     'webrtc',
                                     {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                                     spyConnectionObserver))
        .to.be.rejectedWith('getAccessNode timeout or error.');
    });

    it('rpcReq.subscribe timeout or error should fail subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.rejects('connect timeout or error.');

      var spyConnectionObserver = sinon.spy();

      return expect(portal.subscribe(testParticipantId,
                                     'subscriptionId',
                                     'webrtc',
                                     {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                                     spyConnectionObserver))
        .to.be.rejectedWith('connect timeout or error.');
    });

    it('rpcReq.onConnectionSignalling timeout or error should fail subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();


      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.rejects('timeout or error');

      var subscription_id = testParticipantId + '-sub-' + 'targetStreamId';
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          return portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', sdp: 'candidateString'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
        });
    });

    it('rpcReq.sub2Session(to controller) timeout or error should cause fail subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.sub2Session.rejects('timeout or error');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.be.an('error');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'timeout or error'}]);
        });
    });

    it('The connection reporting a status with type of #failed# should abort subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.onConnectionSignalling = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.onConnectionSignalling.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'failed', reason: 'reasonString'});
        })
        .then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.have.string('reasonString');
          expect(spyConnectionObserver.getCall(0).args).to.deep.equal([{type: 'failed', reason: 'reasonString'}]);
          expect(mockrpcReq.sub2Session.callCount).to.equal(0);
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
          return expect(portal.onConnectionSignalling(testParticipantId, 'targetStreamId', {type: 'candidate', candidate: 'candidateString'}))
            .to.be.rejectedWith('Connection does NOT exist when receiving a signaling.');
        });
    });

    it('Participant early leaving should abort subscribing.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();
      mockrpcReq.leave = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.leave.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'initializing'});
        })
        .then(function(statusResult) {
          return portal.leave(testParticipantId);
        })
        .then(function(leaveResult) {
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
          expect(mockrpcReq.sub2Session.callCount).to.equal(0);
          expect(mockrpcReq.unsub2Session.callCount).to.equal(0);
          return expect(on_connection_status({type: 'ready', audio_codecs: ['pcmu'], video_codecs: ['vp8']}))
            .to.be.rejectedWith('Participant ' + testParticipantId + ' has left when the connection gets ready.');
        });
    });

    it('Unsubscribing before subscribing should fail.', function() {
      return expect(portal.unsubscribe(testParticipantId, 'subscriptionId'))
        .to.be.rejectedWith('subscription does not exist');
    });

    it('Unsubscribing a half subscribed stream(connected but not subscribed from controller) should cause disconnect(to access node) and return ok.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');

      var subscription_id = 'subscriptionId';
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return portal.unsubscribe(testParticipantId, subscription_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
        });
    });

    it('Unsubscribing a successfully subscribed stream should cause disconnect(to access node) and unsubscribe(to controller) and return ok.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.unsub2Session.resolves('ok');

      var subscription_id = 'subscriptionId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockrpcReq.sub2Session.callCount).to.equal(1);
          return portal.unsubscribe(testParticipantId, subscription_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.unsubscribe.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id]);
          expect(mockrpcReq.recycleAccessNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {session: testSession, task: subscription_id}]);
          expect(mockrpcReq.unsub2Session.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, subscription_id]);
        });
    });

    it('Switching on/off the media tracks before subscribing should fail.', function() {
      return expect(portal.mediaOnOff(testParticipantId, 'subscriptionId', 'video', 'out', 'off'))
          .to.be.rejectedWith('connection does not exist');
    });

    it('Switching on/off the media tracks after subscribing should proceed.', function() {
      mockrpcReq.getAccessNode = sinon.stub();
      mockrpcReq.subscribe = sinon.stub();
      mockrpcReq.unsubscribe = sinon.stub();
      mockrpcReq.recycleAccessNode = sinon.stub();
      mockrpcReq.sub2Session = sinon.stub();
      mockrpcReq.mediaOnOff = sinon.stub();
      mockrpcReq.unsub2Session = sinon.stub();

      mockrpcReq.getAccessNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.subscribe.resolves('ok');
      mockrpcReq.unsubscribe.resolves('ok');
      mockrpcReq.recycleAccessNode.resolves('ok');
      mockrpcReq.sub2Session.resolves('ok');
      mockrpcReq.mediaOnOff.onCall(0).returns(Promise.resolve('ok'));
      mockrpcReq.mediaOnOff.onCall(1).returns(Promise.reject('timeout or error'));
      mockrpcReq.unsub2Session.resolves('ok');

      var subscription_id = testParticipantId + '-sub-' + 'targetStreamId', on_connection_status;
      var spyConnectionObserver = sinon.spy();

      return portal.subscribe(testParticipantId,
                              subscription_id,
                              'webrtc',
                              {audio: {fromStream: 'targetStreamId'}, video: {fromStream: 'targetStreamId', resolution: 'vga', quality_level: 'standard'}},
                              spyConnectionObserver)
        .then(function(connectionLocality) {
          on_connection_status = mockrpcReq.subscribe.getCall(0).args[4];
          return on_connection_status({type: 'ready', audio_codecs: ['pcmu', 'opus'], video_codecs: ['vp8', 'h264']});
        })
        .then(function() {
          expect(mockrpcReq.sub2Session.callCount).to.equal(1);
          return portal.mediaOnOff(testParticipantId, 'targetStreamId', 'video', 'out', 'off');
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.mediaOnOff.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', subscription_id, 'video', 'out', 'off']);
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
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return Promise.all([
      expect(portal.getRegion(testParticipantId, 'subStreamId', 'mixStreamId')).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.'),
      expect(portal.setRegion(testParticipantId, 'subStreamId', 500, 'mixStreamId')).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.')
      ]);
  });

  describe('Getting/setting region after successfully logining should proceed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();

      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'admin', origin: {isp: 'isp', region: 'region'}, room: testSession});
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        done();
      });
    });

    afterEach(function() {
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');
      portal.leave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
    });

    it('Getting region should succeed if rpcReq.getRegion succeeds.', function() {
      mockrpcReq.getRegion = sinon.stub();
      mockrpcReq.getRegion.resolves('regionId');

      return portal.getRegion(testParticipantId, 'subStreamId', 'mixStreamId')
        .then(function(result) {
          expect(result).to.equal('regionId');
          expect(mockrpcReq.getRegion.getCall(0).args).to.deep.equal(['rpcIdOfController', 'subStreamId', 'mixStreamId']);
        });
    });

    it('Getting region should fail if rpcReq.getRegion fails.', function() {
      mockrpcReq.getRegion = sinon.stub();
      mockrpcReq.getRegion.rejects('no such a stream');

      return expect(portal.getRegion(testParticipantId, 'subStreamId', 'mixStreamId')).to.be.rejectedWith('no such a stream');
    });

    it('Setting region should succeed if rpcReq.setRegion succeeds.', function() {
      mockrpcReq.setRegion = sinon.stub();
      mockrpcReq.setRegion.resolves('ok');

      return portal.setRegion(testParticipantId, 'subStreamId', 500, 'mixStreamId')
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.setRegion.getCall(0).args).to.deep.equal(['rpcIdOfController', 'subStreamId', 500, 'mixStreamId']);
        });
    });

    it('Setting region should fail if rpcReq.setRegion fails.', function() {
      mockrpcReq.setRegion = sinon.stub();
      mockrpcReq.setRegion.rejects('failed-for-some-reason');

      return expect(portal.setRegion(testParticipantId, 'subStreamId', 500, 'mixStreamId')).to.be.rejectedWith('failed-for-some-reason');
    });
  });
});

describe('portal.text', function() {
  it('Sending text before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return expect(portal.text(testParticipantId, 'all', 'Hi, there!')).to.be.rejectedWith('Participant ' + testParticipantId + ' does NOT exist.');
  });

  it('Sending text without permission should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer_no_text', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.text(testParticipantId, 'all', 'Hi, there!')).to.be.rejectedWith('unauthorized');
      });
  });

  describe('Sending text after successfully logining should proceed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    beforeEach(function(done) {
      dbAccess.deleteToken = sinon.stub();
      mockrpcReq.getController = sinon.stub();
      mockrpcReq.join = sinon.stub();

      dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testSession});
      mockrpcReq.getController.resolves('rpcIdOfController');
      mockrpcReq.join.resolves({participants: [],
                                   streams: []});

      portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        done();
      });
    });

    afterEach(function() {
      mockrpcReq.leave = sinon.stub();
      mockrpcReq.leave.resolves('ok');
      portal.leave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
    });

    it('Should succeed if rpcReq.text succeeds.', function() {
      mockrpcReq.text = sinon.stub();
      mockrpcReq.text.resolves('ok');

      return portal.text(testParticipantId, 'all', 'Hi, there!')
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.text.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId, 'all', 'Hi, there!']);
        });
    });

    it('Should fail if rpcReq.text fails.', function() {
      mockrpcReq.text = sinon.stub();
      mockrpcReq.text.rejects('no such a receiver');

      return expect(portal.text(testParticipantId, 'a-non-existed-receiver', 'some-message')).to.be.rejectedWith('no such a receiver');
    });
  });
});

describe('portal.getParticipantsByController', function() {
  it('Getting participants should return empty list if no participant has joined.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    return expect(portal.getParticipantsByController('node', 'rpcIdOfController')).to.become([]);
  });

  it('Sending text without permission should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.deleteToken = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    dbAccess.deleteToken.resolves({code: 'tokenCode', userName: 'Jack', role: 'viewer_no_text', origin: {isp: 'isp', region: 'region'}, room: testSession});
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves({participants: [],
                                 streams: []});

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        return expect(portal.getParticipantsByController('node', 'rpcIdOfController')).to.become([testParticipantId]);
      });
  });
});

