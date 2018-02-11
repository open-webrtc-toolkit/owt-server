'use strict';
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
  token: {
  }
};
mockery.registerMock("./data_access", dbAccess);

after(function(){
    mockery.disable();
});

var Portal = require('../portal');
var rpcReq = require('../rpcRequest');

const testTokenKey = '6d96729ffd07202e7a65ac81278c43a2c87fc6367736431e614607e9d3ceee201f9680cdfcb88df508829e9810c46aaf02c9cc8dcf46369de122ee5b22ec963c';
const testSelfRpcId = 'portal-38A87euk9kfh';
const testClusterName = 'woogeen-cluster';
const testPortalSpec = {tokenKey: testTokenKey,
                        clusterName: testClusterName,
                        selfRpcId: testSelfRpcId
                       };
const testToken = {tokenId: '573eab88111478bb3526421b',
                   host: '10.239.158.161:8080',
                   secure: true,
                   signature: 'ZTIwZTI0YzYyMDFmMTQ1OTc3ZTBkZTdhMDMyZTJhYTkwMTJiZjNhY2E5OTZjNzA1Y2ZiZGU1ODhjMDY3MmFkYw=='
                  };
const testParticipantId = '/#gpMeAEuPeMMDT0daAAAA';
const testRoom = '573eab78111478bb3526421a';

const delete_token_result = {code: 'tokenCode', user: 'user-id', role: 'presenter', origin: {isp: 'isp', region: 'region'}, room: testRoom};
const join_room_result = {
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
            status: 'active',
            source: 'mic',
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
            status: 'active',
            source: 'camera',
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
};

describe('portal.updateTokenKey: update the token key.', function() {
  it('Joining with a valid token will fail if the token key is out of time, and success when token key is updated', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    var portalSpecWitpauseTokenKey = {tokenKey: 'OldTokenKey',
                                     clusterName: testClusterName,
                                     selfRpcId: testSelfRpcId};

    var portal = Portal(portalSpecWitpauseTokenKey, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves(join_room_result);

    var final_result = {tokenCode: delete_token_result.code, data: {user: delete_token_result.user, role: delete_token_result.role, permission: join_room_result.permission, room: join_room_result.room}};

    return portal.join(testParticipantId, testToken).then(function(runInHere) {
        expect(runInHere).to.be.false;
      }, function(reason) {
        expect(reason).to.have.string('Invalid token signature');
        portal.updateTokenKey(testTokenKey);
        return portal.join(testParticipantId, testToken).then(function(result) {
          expect(result).to.deep.equal(final_result);
        });
      });
  });
});

describe('portal.join: Participants join.', function() {
  it('Joining with valid token should succeed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves(join_room_result);

    var final_result = {tokenCode: delete_token_result.code, data: {user: delete_token_result.user, role: delete_token_result.role, permission: join_room_result.permission, room: join_room_result.room}};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      expect(mockrpcReq.getController.getCall(0).args).to.deep.equal(['woogeen-cluster', testRoom]);
      expect(mockrpcReq.join.getCall(0).args).to.deep.equal(['rpcIdOfController', testRoom, {id: testParticipantId, user: delete_token_result.user, role: delete_token_result.role, portal: testSelfRpcId, origin: delete_token_result.origin}]);
    });
  });

  it('The same participant re-joining should succeed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();

    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves(join_room_result);

    var final_result = {tokenCode: delete_token_result.code, data: {user: delete_token_result.user, role: delete_token_result.role, permission: join_room_result.permission, room: join_room_result.room}};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('Participant already in room');
    });
  });

  it('Tokens with incorect signature should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    var tokenWithInvalidSignature = { tokenId: '573eab88111478bb3526421b',
                                      host: '10.239.158.161:8080',
                                      secure: true,
                                      signature: 'AStringAsInvalidSignature' };
    return expect(portal.join(testParticipantId, tokenWithInvalidSignature)).to.be.rejectedWith('Invalid token signature');//Note: The error message is not strictly equality checked, instead it will be fullfiled when the actual error message includes the given string here. That is the inherint behavor of 'rejectedWith'.
  });

  it('token.delete timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcReq.getController timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);

    mockrpcReq.getController.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });

  it('rpcReq.join timeout or error should fail joining.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);

    mockrpcReq.getController.resolves('theRpcIdOfController');

    mockrpcReq.join.rejects('timeout or error');

    return expect(portal.join(testParticipantId, testToken)).to.be.rejectedWith('timeout or error');
  });
});

describe('portal.leave: Participants leave.', function() {
  it('Before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    mockrpcReq.leave = sinon.stub();

    return portal.leave(testParticipantId)
      .then(function(runInHere) {
        expect(runInHere).to.be.false;
      }, function(err) {
        expect(err).to.equal('Participant has NOT joined');
        expect(mockrpcReq.leave.callCount).to.equal(0);
      });
  });

  it('Should succeed if rpcRequest succeeds.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    mockrpcReq.leave = sinon.stub();

    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves(join_room_result);
    mockrpcReq.leave.resolves('ok');

    var final_result = {tokenCode: delete_token_result.code, data: {user: delete_token_result.user, role: delete_token_result.role, permission: join_room_result.permission, room: join_room_result.room}};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      return portal.leave(testParticipantId);
    }).then((result) => {
      expect(result).to.equal('ok');
      expect(mockrpcReq.leave.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId]);
    });
  });

  it('Should succeed even if rpcRequest fails.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    dbAccess.token.delete = sinon.stub();
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.join = sinon.stub();
    mockrpcReq.leave = sinon.stub();

    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join.resolves(join_room_result);
    mockrpcReq.leave.rejects('some error');

    var final_result = {tokenCode: delete_token_result.code, data: {user: delete_token_result.user, role: delete_token_result.role, permission: join_room_result.permission, room: join_room_result.room}};

    return portal.join(testParticipantId, testToken).then(function(result) {
      expect(result).to.deep.equal(final_result);
      return portal.leave(testParticipantId);
    }).then((result) => {
      expect(result).to.equal('ok');
      expect(mockrpcReq.leave.getCall(0).args).to.deep.equal(['rpcIdOfController', testParticipantId]);
    });
  });
});

describe('portal.publish/portal.unpublish/portal.streamControl/portal.onSessionSignaling/portal.subscribe/portal.unsubscribe/portal.updateSubscription', function() {
  it('Before logining should fail.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    var pub = portal.publish(testParticipantId, 'streamId', {type: 'webrtc', media: {audio: {source: 'mic'}, video: {source: 'camera'}}});
    var unpub = portal.unpublish(testParticipantId, 'streamId');
    var sub = portal.subscribe(testParticipantId, 'subscriptionId', {type: 'webrtc', media: {audio: {from: 'stream1'}, video: {from: 'stream2'}}});
    var unsub = portal.unsubscribe(testParticipantId, 'subscriptionId');
    var streamCtrl = portal.streamControl(testParticipantId, 'streamId', {operation: 'mix', data: 'common'});
    var subscriptionCtrl = portal.subscriptionControl(testParticipantId, 'subscriptionId', {operation: 'update', data: {audio: {from: 'stream3'}, video: {from: 'stream4'}}});
    var onSessionSignaling = portal.onSessionSignaling(testParticipantId, 'sessionId', {type: 'offer', sdp: 'offerSDPString'});
    var text = portal.text(testParticipantId, 'anotherParticipnatId', 'Hello');

    return Promise.all([
        expect(pub).to.be.rejectedWith('Participant has NOT joined'),
        expect(unpub).to.be.rejectedWith('Participant has NOT joined'),
        expect(sub).to.be.rejectedWith('Participant has NOT joined'),
        expect(unsub).to.be.rejectedWith('Participant has NOT joined'),
        expect(streamCtrl).to.be.rejectedWith('Participant has NOT joined'),
        expect(subscriptionCtrl).to.be.rejectedWith('Participant has NOT joined'),
        expect(onSessionSignaling).to.be.rejectedWith('Participant has NOT joined'),
        expect(text).to.be.rejectedWith('Participant has NOT joined')
      ]);
  });

  it('Should fail if rpcRequest fails.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete = sinon.stub();
    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join = sinon.stub();
    mockrpcReq.join.resolves(join_room_result);

    mockrpcReq.publish = sinon.stub();
    mockrpcReq.publish.rejects('some error reason');
    mockrpcReq.unpublish = sinon.stub();
    mockrpcReq.unpublish.rejects('some error reason');
    mockrpcReq.subscribe = sinon.stub();
    mockrpcReq.subscribe.rejects('some error reason');
    mockrpcReq.unsubscribe = sinon.stub();
    mockrpcReq.unsubscribe.rejects('some error reason');
    mockrpcReq.streamControl = sinon.stub();
    mockrpcReq.streamControl.rejects('some error reason');
    mockrpcReq.subscriptionControl = sinon.stub();
    mockrpcReq.subscriptionControl.rejects('some error reason');
    mockrpcReq.onSessionSignaling = sinon.stub();
    mockrpcReq.onSessionSignaling.rejects('some error reason');
    mockrpcReq.text = sinon.stub();
    mockrpcReq.text.rejects('some error reason');

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        var pub = portal.publish(testParticipantId, 'streamId', {type: 'webrtc', media: {audio: {source: 'mic'}, video: {source: 'camera'}}});
        var unpub = portal.unpublish(testParticipantId, 'streamId');
        var sub = portal.subscribe(testParticipantId, 'subscriptionId', {type: 'webrtc', media: {audio: {from: 'stream1'}, video: {from: 'stream2'}}});
        var unsub = portal.unsubscribe(testParticipantId, 'subscriptionId');
        var streamCtrl = portal.streamControl(testParticipantId, 'streamId', {operation: 'mix', data: 'common'});
        var subscriptionCtrl = portal.subscriptionControl(testParticipantId, 'subscriptionId', {operation: 'update', data: {audio: {from: 'stream3'}, video: {from: 'stream4'}}});
        var onSessionSignaling = portal.onSessionSignaling(testParticipantId, 'sessionId', {type: 'offer', sdp: 'offerSDPString'});
        var text = portal.text(testParticipantId, 'anotherParticipnatId', 'Hello');
        return Promise.all([
            expect(pub).to.be.rejectedWith('some error reason'),
            expect(unpub).to.be.rejectedWith('some error reason'),
            expect(sub).to.be.rejectedWith('some error reason'),
            expect(unsub).to.be.rejectedWith('some error reason'),
            expect(streamCtrl).to.be.rejectedWith('some error reason'),
            expect(subscriptionCtrl).to.be.rejectedWith('some error reason'),
            expect(onSessionSignaling).to.be.rejectedWith('some error reason'),
            expect(text).to.be.rejectedWith('some error reason')
          ]);
      });
  });

  it('Should succeed if rpcRequest succeeds.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var portal = Portal(testPortalSpec, mockrpcReq);

    dbAccess.token.delete = sinon.stub();
    dbAccess.token.delete.resolves(delete_token_result);
    mockrpcReq.getController = sinon.stub();
    mockrpcReq.getController.resolves('rpcIdOfController');
    mockrpcReq.join = sinon.stub();
    mockrpcReq.join.resolves(join_room_result);

    mockrpcReq.publish = sinon.stub();
    mockrpcReq.publish.resolves('ok');
    mockrpcReq.unpublish = sinon.stub();
    mockrpcReq.unpublish.resolves('ok');
    mockrpcReq.subscribe = sinon.stub();
    mockrpcReq.subscribe.resolves('ok');
    mockrpcReq.unsubscribe = sinon.stub();
    mockrpcReq.unsubscribe.resolves('ok');
    mockrpcReq.streamControl = sinon.stub();
    mockrpcReq.streamControl.resolves('ok');
    mockrpcReq.subscriptionControl = sinon.stub();
    mockrpcReq.subscriptionControl.resolves('ok');
    mockrpcReq.onSessionSignaling = sinon.stub();
    mockrpcReq.onSessionSignaling.resolves('ok');
    mockrpcReq.text = sinon.stub();
    mockrpcReq.text.resolves('ok');

    return portal.join(testParticipantId, testToken)
      .then(function(joinResult) {
        var pub = portal.publish(testParticipantId, 'streamId', {type: 'webrtc', media: {audio: {source: 'mic'}, video: {source: 'camera'}}});
        var unpub = portal.unpublish(testParticipantId, 'streamId');
        var sub = portal.subscribe(testParticipantId, 'subscriptionId', {type: 'webrtc', media: {audio: {from: 'stream1'}, video: {from: 'stream2'}}});
        var unsub = portal.unsubscribe(testParticipantId, 'subscriptionId');
        var streamCtrl = portal.streamControl(testParticipantId, 'streamId', {operation: 'mix', data: 'common'});
        var subscriptionCtrl = portal.subscriptionControl(testParticipantId, 'subscriptionId', {operation: 'update', data: {audio: {from: 'stream3'}, video: {from: 'stream4'}}});
        var onSessionSignaling = portal.onSessionSignaling(testParticipantId, 'sessionId', {type: 'offer', sdp: 'offerSDPString'});
        var text = portal.text(testParticipantId, 'anotherParticipnatId', 'Hello');
        return Promise.all([
            expect(pub).to.become('ok'),
            expect(unpub).to.become('ok'),
            expect(sub).to.become('ok'),
            expect(unsub).to.become('ok'),
            expect(streamCtrl).to.become('ok'),
            expect(subscriptionCtrl).to.become('ok'),
            expect(onSessionSignaling).to.become('ok'),
            expect(text).to.become('ok')
          ]);
      });
  });
});


