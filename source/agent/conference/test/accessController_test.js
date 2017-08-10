'use strict';
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');
var path = require('path');

var AccessController = require('../accessController');
var rpcReq = require('../rpcRequest');

const testParticipantId = '/#gpMeAEuPeMMDT0daAAAA';
const testRoom = '573eab78111478bb3526421a';

const testAccessControllerSpec = {
  clusterName: 'woogeen-cluster',
  selfRpcId: 'selfRpcId',
  inRoom: testRoom,
  mediaIn: {
    audio: [{
      codec: 'opus',
      sampleRate: 48000,
      channelNum: 2
    }, {
      codec: 'pcmu'
    }, {
      codec: 'pcma'
    }, {
      codec: 'aac'
    }, {
      codec: 'ac3'
    }, {
      codec: 'nellymoser'
    }],
    video: [{
      codec: 'h264',
      profile: 'high'
    }, {
      codec: 'vp8'
    }, {
      codec: 'h265'
    }, {
      codec: 'vp9'
    }]
  },
  mediaOut: {
    audio: [{
      codec: 'opus',
      sampleRate: 48000,
      channelNum: 2
    }, {
      codec: 'pcmu'
    }, {
      codec: 'pcma'
    }, {
      codec: 'aac',
      samepleRate: 48000,
      channelNum: 2
    }, {
      codec: 'ac3',
      samepleRate: 48000,
      channelNum: 2
    }, {
      codec: 'nellymoser',
      samepleRate: 48000,
      channelNum: 2
    }],
    video: [{
      codec: 'h264',
      profile: 'constrained-baseline'
    }, {
      codec: 'h264',
      profile: 'high'
    }, {
      codec: 'vp8'
    }, {
      codec: 'h265'
    }, {
      codec: 'vp9'
    }]
  }
};

describe('accessController.initiate/accessController.terminate: for publishing', function() {
  describe('With proper options should proceed.', function() {
    var mockrpcReq = sinon.createStubInstance(rpcReq);
    var spyOnSessionEstablished = sinon.spy();
    var spyOnSessionAborted = sinon.spy();
    var spyOnSessionSignaling = sinon.spy();
    var accessController = AccessController.create(testAccessControllerSpec, mockrpcReq, spyOnSessionEstablished, spyOnSessionAborted, spyOnSessionSignaling);

    afterEach(function() {
      mockrpcReq.recycleWorkerNode = sinon.stub();
      mockrpcReq.recycleWorkerNode.resolves('ok');
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.terminate.resolves('ok');
      accessController.participantLeave(testParticipantId).catch(function(error) {
        // Handle the reject leave Promise.
      });
    });

    it('Initiating a webrtc -in session with both audio and video should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.onSessionSignaling = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.onSessionSignaling.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();
      spyOnSessionSignaling.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         },
                                         attributes: {aaa: 'bbb'}
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.getWorkerNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {room: testRoom, task: session_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.initiate.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.initiate.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.initiate.getCall(0).args[1]).to.equal(session_id);
          expect(mockrpcReq.initiate.getCall(0).args[2]).to.equal('webrtc');
          expect(mockrpcReq.initiate.getCall(0).args[3]).to.equal('in');
          expect(mockrpcReq.initiate.getCall(0).args[4]).to.deep.equal({
            controller: 'selfRpcId',
            media: {
              audio: {source: 'mic'},
              video: {source: 'camera'}
            }
          });
          return accessController.onSessionSignaling(session_id, {type: 'offer', sdp: 'offerSDPString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onSessionSignaling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return accessController.onSessionSignaling(session_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onSessionSignaling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', session_id, {type: 'candidate', sdp: 'candidateString'}]);
          return accessController.onSessionStatus(session_id, {type: 'answer', sdp: 'answerSDPString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('ok');
          expect(spyOnSessionSignaling.getCall(0).args).to.deep.equal([testParticipantId, session_id, {type: 'answer', sdp: 'answerSDPString'}]);
          return accessController.onSessionStatus(session_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('ok');
          expect(spyOnSessionSignaling.getCall(1).args).to.deep.equal([testParticipantId, session_id, {type: 'candidate', sdp: 'candidateString'}]);
          return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'opus', sampleRate: 48000, channelNum: 2}, video: {codec: 'h264', profile: 'high'}});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(spyOnSessionEstablished.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in',  {locality: {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, media: {
            audio: {codec: 'opus', sampleRate: 48000, channelNum: 2, source: 'mic'},
            video: {codec: 'h264', profile: 'high', source: 'camera'}
          }, info: {owner: testParticipantId, type: 'webrtc', attributes: {aaa: 'bbb'}}}]);
        });
    });

    it('Initiating a webrtc -out session with both audio and video should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.onSessionSignaling = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.onSessionSignaling.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();
      spyOnSessionSignaling.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'out',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {from: 'stream1'},
                                           video: {from: 'stream2'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.getWorkerNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'webrtc', {room: testRoom, task: session_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.initiate.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.initiate.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.initiate.getCall(0).args[1]).to.equal(session_id);
          expect(mockrpcReq.initiate.getCall(0).args[2]).to.equal('webrtc');
          expect(mockrpcReq.initiate.getCall(0).args[3]).to.equal('out');
          expect(mockrpcReq.initiate.getCall(0).args[4]).to.deep.equal({
            controller: 'selfRpcId',
            media: {
              audio: {from: 'stream1'},
              video: {from: 'stream2'}
            }
          });
          return accessController.onSessionSignaling(session_id, {type: 'offer', sdp: 'offerSDPString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onSessionSignaling.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, {type: 'offer', sdp: 'offerSDPString'}]);
          return accessController.onSessionSignaling(session_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(signalingResult) {
          expect(signalingResult).to.equal('ok');
          expect(mockrpcReq.onSessionSignaling.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', session_id, {type: 'candidate', sdp: 'candidateString'}]);
          return accessController.onSessionStatus(session_id, {type: 'answer', sdp: 'answerSDPString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('ok');
          expect(spyOnSessionSignaling.getCall(0).args).to.deep.equal([testParticipantId, session_id, {type: 'answer', sdp: 'answerSDPString'}]);
          return accessController.onSessionStatus(session_id, {type: 'candidate', sdp: 'candidateString'});
        })
        .then(function(statusResult) {
          expect(statusResult).to.equal('ok');
          expect(spyOnSessionSignaling.getCall(1).args).to.deep.equal([testParticipantId, session_id, {type: 'candidate', sdp: 'candidateString'}]);
          return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'opus', sampleRate: 48000, channelNum: 2}, video: {codec: 'h264'}});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(spyOnSessionEstablished.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'out',  {locality: {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, media: {
            audio: {codec: 'opus', sampleRate: 48000, channelNum: 2, from: 'stream1'},
            video: {codec: 'h264', from: 'stream2'}
          }, info: {owner: testParticipantId, type: 'webrtc'}}]);
        });
    });

    it('Initiating a streaming-in session with both audio and video should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'streaming',
                                         connection: {
                                           url: 'URLofSomeIPCamera',
                                           transportProtocol: 'tcp',
                                           bufferSize: 2048,
                                           origin: {isp: 'isp', region: 'region'}
                                         },
                                         media: {
                                           audio: 'auto',
                                           video: true
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.getWorkerNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'streaming', {room: testRoom, task: session_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.initiate.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.initiate.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.initiate.getCall(0).args[1]).to.equal(session_id);
          expect(mockrpcReq.initiate.getCall(0).args[2]).to.equal('streaming');
          expect(mockrpcReq.initiate.getCall(0).args[3]).to.equal('in');
          expect(mockrpcReq.initiate.getCall(0).args[4]).to.deep.equal({
            controller: 'selfRpcId',
            connection: {
              url: 'URLofSomeIPCamera',
              transportProtocol: 'tcp',
              bufferSize: 2048,
              origin: {isp: 'isp', region: 'region'}
            },
            media: {
              audio: 'auto',
              video: true
            }
          });
          expect(accessController.getSessionState(session_id)).to.equal('connecting');
          return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'pcmu'}, video: {codec: 'h264', resolution: {width: 1920, height: 1080}}});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(accessController.getSessionState(session_id)).to.equal('connected');
          expect(spyOnSessionEstablished.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in',  {locality: {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, media: {audio: {codec: 'pcmu'}, video: {codec: 'h264', resolution: {width: 1920, height: 1080}}}, info: {owner: testParticipantId, type: 'streaming'}}]);
        });
    });

    it('Initiating a streaming-out session with both audio and video should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'out',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'streaming',
                                         connection: {
                                           url: 'rtmp://user:pwd@1.1.1.1:9000/url-of-avstream',
                                           origin: {isp: 'isp', region: 'region'}
                                         },
                                         media: {
                                           audio: {from: 'stream1', spec: {codec: 'aac', sampleRate: 48000, channelNum: 2}},
                                           video: {from: 'stream2', spec: {codec: 'h264'}}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.getWorkerNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'streaming', {room: testRoom, task: session_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.initiate.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.initiate.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.initiate.getCall(0).args[1]).to.equal(session_id);
          expect(mockrpcReq.initiate.getCall(0).args[2]).to.equal('streaming');
          expect(mockrpcReq.initiate.getCall(0).args[3]).to.equal('out');
          expect(mockrpcReq.initiate.getCall(0).args[4]).to.deep.equal({
            controller: 'selfRpcId',
            connection: {
              url: 'rtmp://user:pwd@1.1.1.1:9000/url-of-avstream',
              origin: {isp: 'isp', region: 'region'}
            },
            media: {
              audio: {from: 'stream1', spec: {codec: 'aac', sampleRate: 48000, channelNum: 2}},
              video: {from: 'stream2', spec: {codec: 'h264'}}
            }
          });
          expect(accessController.getSessionState(session_id)).to.equal('connecting');
          return accessController.onSessionStatus(session_id, {type: 'ready'});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(accessController.getSessionState(session_id)).to.equal('connected');
          expect(spyOnSessionEstablished.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'out',  {locality: {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, media: {audio: {from: 'stream1', spec: {codec: 'aac', sampleRate: 48000, channelNum: 2}}, video: {from: 'stream2', spec: {codec: 'h264'}}}, info: {owner: testParticipantId, type: 'streaming'}}]);
        });
    });

    it('Initiating a recording-out session with both audio and video should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'out',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'recording',
                                         connection: {
                                           container: 'mkv',
                                           origin: {isp: 'isp', region: 'region'}
                                         },
                                         media: {
                                           audio: {from: 'stream1', spec: {codec: 'opus', sampleRate: 48000, channelNum: 2}},
                                           video: {from: 'stream2', spec: {codec: 'h264'}}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.getWorkerNode.getCall(0).args).to.deep.equal(['woogeen-cluster', 'recording', {room: testRoom, task: session_id}, {isp: 'isp', region: 'region'}]);
          expect(mockrpcReq.initiate.getCall(0).args).to.have.lengthOf(5);
          expect(mockrpcReq.initiate.getCall(0).args[0]).to.deep.equal('rpcIdOfAccessNode');
          expect(mockrpcReq.initiate.getCall(0).args[1]).to.equal(session_id);
          expect(mockrpcReq.initiate.getCall(0).args[2]).to.equal('recording');
          expect(mockrpcReq.initiate.getCall(0).args[3]).to.equal('out');
          expect(mockrpcReq.initiate.getCall(0).args[4]).to.deep.equal({
            controller: 'selfRpcId',
            connection: {
              container: 'mkv',
              origin: {isp: 'isp', region: 'region'}
            },
            media: {
              audio: {from: 'stream1', spec: {codec: 'opus', sampleRate: 48000, channelNum: 2}},
              video: {from: 'stream2', spec: {codec: 'h264'}}
            }
          });
          expect(accessController.getSessionState(session_id)).to.equal('connecting');
          return accessController.onSessionStatus(session_id, {type: 'ready'});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(accessController.getSessionState(session_id)).to.equal('connected');
          expect(spyOnSessionEstablished.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'out',  {locality: {agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'}, media: {audio: {from: 'stream1', spec: {codec: 'opus', sampleRate: 48000, channelNum: 2}}, video: {from: 'stream2', spec: {codec: 'h264'}}}, info: {owner: testParticipantId, type: 'recording'}}]);
        });
    });

    it('Initiation should be aborted if exceptions occur.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'streaming',
                                         connection: {
                                           url: 'URLofSomeIPCamera',
                                           transportProtocol: 'tcp',
                                           bufferSize: 2048,
                                           origin: {isp: 'isp', region: 'region'}
                                         },
                                         media: {
                                           audio: 'auto',
                                           video: true
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onSessionStatus(session_id, {type: 'failed', reason: 'network error'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.equal('network error');
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
          expect(spyOnSessionEstablished.callCount).to.equal(0);
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'network error']);
        });
    });

    it('Initiation should be aborted if the access node fault is detected.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'streaming',
                                         connection: {
                                           url: 'URLofSomeIPCamera',
                                           transportProtocol: 'tcp',
                                           bufferSize: 2048,
                                           origin: {isp: 'isp', region: 'region'}
                                         },
                                         media: {
                                           audio: 'auto',
                                           video: true
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'pcmu'}, video: {codec: 'h264', resolution: {width: 1920, height: 1080}}});
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onFaultDetected('node', 'rpcIdOfAccessNode');
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
          expect(spyOnSessionEstablished.callCount).to.equal(1);
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'Access node exited unexpectedly']);
        });
    });

    it('rpcReq.getWorkerNode timeout or error should fail initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.getWorkerNode.rejects('getWorkerNode timeout or error.');

      return expect(accessController.initiate(testParticipantId,
                                              'sessionId',
                                              'in',
                                              {isp: 'isp', region: 'region'},
                                              {
                                                type: 'webrtc',
                                                media: {
                                                  audio: {source: 'mic'},
                                                  video: {source: 'camera'}
                                                }
                                              }))
        .to.be.rejectedWith('getWorkerNode timeout or error.');
    });

    it('rpcReq.initiate timeout or error should fail initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.rejects('connect timeout or error.');

      return expect(accessController.initiate(testParticipantId,
                                              'sessionId',
                                              'in',
                                              {isp: 'isp', region: 'region'},
                                              {
                                                type: 'webrtc',
                                                media: {
                                                  audio: {source: 'mic'},
                                                  video: {source: 'camera'}
                                                }
                                              }))
        .to.be.rejectedWith('connect timeout or error.');
    });

    it('rpcReq.onSessionSignaling timeout or error should fail initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();
      mockrpcReq.onSessionSignaling = sinon.stub();


      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');
      mockrpcReq.onSessionSignaling.rejects('timeout or error');

      var session_id = 'sessionId';

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onSessionSignaling(session_id, {type: 'candidate', sdp: 'candidateString'});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.equal('timeout or error');
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
        });
    });

    it('No proper audio/video codec during negotiation should fail initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok.');
      mockrpcReq.terminate.resolves('ok.');
      mockrpcReq.recycleWorkerNode.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'opus', sampleRate: 48000, channelNum: 2}, video: false});
        }).then(function(runInHere) {
          expect(runInHere).to.be.false;
        }, function(err) {
          expect(err).to.equal('No proper video codec');
          expect(spyOnSessionEstablished.callCount).to.equal(0);
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'No proper video codec']);
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);

          var session_id1 = 'sessionId1';
          return accessController.initiate(testParticipantId,
                                           session_id1,
                                           'in',
                                           {isp: 'isp', region: 'region'},
                                           {
                                             type: 'webrtc',
                                             media: {
                                               audio: {source: 'mic'},
                                               video: {source: 'camera'}
                                             }
                                           })
            .then(function(result) {
              expect(result).to.equal('ok');
              return accessController.onSessionStatus(session_id1, {type: 'ready', audio: false, video: {codec: 'h264'}});
            }).then(function(runInHere) {
              expect(runInHere).to.be.false;
            }, function(err) {
              expect(err).to.equal('No proper audio codec');
              expect(spyOnSessionEstablished.callCount).to.equal(0);
              expect(spyOnSessionAborted.getCall(1).args).to.deep.equal([testParticipantId, session_id1, 'in', 'No proper audio codec']);
              expect(mockrpcReq.terminate.getCall(1).args).to.deep.equal(['rpcIdOfAccessNode', session_id1, 'in']);
              expect(mockrpcReq.recycleWorkerNode.getCall(1).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id1}]);
            });
        });
    });

    it('The session report with a status.type of #failed# should abort initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onSessionStatus(session_id, {type: 'failed', reason: 'some reason'});
        })
        .then(function(statusResult) {
          expect('run into here').to.be.false;
        }, function(err) {
          expect(err).to.equal('some reason');
          expect(spyOnSessionEstablished.callCount).to.equal(0);
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'some reason']);
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
        });
    });

    it('Participant early leaving should abort initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();
      mockrpcReq.leave = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');
      mockrpcReq.leave.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.participantLeave(testParticipantId);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'Participant leave']);
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
          return expect(accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'opus', sampleRate: 48000, channelNum: 2}, video: {codec: 'h264', profile: 'high'}}))
            .to.be.rejectedWith('Session does NOT exist');
        });
    });

    it('Fault being detected on the access agent should abort initiation.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.terminate = sinon.stub();
      mockrpcReq.recycleWorkerNode = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.terminate.resolves('ok');
      mockrpcReq.recycleWorkerNode.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.onFaultDetected('node', 'rpcIdOfAccessNode');
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'Access node exited unexpectedly']);
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
        });
    });

    it('Termination before initiation should fail.', function() {
      return expect(accessController.terminate('sessionId'))
        .to.be.rejectedWith('Session does NOT exist');
    });

    it('Termination after initiation should succeed.', function() {
      mockrpcReq.getWorkerNode = sinon.stub();
      mockrpcReq.initiate = sinon.stub();
      mockrpcReq.onSessionSignaling = sinon.stub();

      mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
      mockrpcReq.initiate.resolves('ok');
      mockrpcReq.onSessionSignaling.resolves('ok');

      var session_id = 'sessionId';
      spyOnSessionEstablished.reset();
      spyOnSessionAborted.reset();
      spyOnSessionSignaling.reset();

      return accessController.initiate(testParticipantId,
                                       session_id,
                                       'in',
                                       {isp: 'isp', region: 'region'},
                                       {
                                         type: 'webrtc',
                                         media: {
                                           audio: {source: 'mic'},
                                           video: {source: 'camera'}
                                         }
                                       })
        .then(function(result) {
          expect(result).to.equal('ok');
          return accessController.terminate(session_id);
        })
        .then(function(result) {
          expect(result).to.equal('ok');
          expect(spyOnSessionAborted.getCall(0).args).to.deep.equal([testParticipantId, session_id, 'in', 'Participant terminate']);
          expect(mockrpcReq.terminate.getCall(0).args).to.deep.equal(['rpcIdOfAccessNode', session_id, 'in']);
          expect(mockrpcReq.recycleWorkerNode.getCall(0).args).to.deep.equal(['rpcIdOfAccessAgent', 'rpcIdOfAccessNode', {room: testRoom, task: session_id}]);
        });
    });
  });
});

describe('accessController.setMute', function() {
  var mockrpcReq = sinon.createStubInstance(rpcReq);
  var spyOnSessionEstablished = sinon.spy();
  var spyOnSessionAborted = sinon.spy();
  var spyOnSessionSignaling = sinon.spy();
  var accessController = AccessController.create(testAccessControllerSpec, mockrpcReq, spyOnSessionEstablished, spyOnSessionAborted, spyOnSessionSignaling);

  afterEach(function() {
    mockrpcReq.recycleWorkerNode = sinon.stub();
    mockrpcReq.recycleWorkerNode.resolves('ok');
    mockrpcReq.terminate = sinon.stub();
    mockrpcReq.terminate.resolves('ok');
    accessController.participantLeave(testParticipantId).catch(function(error) {
      // Handle the reject leave Promise.
    });
  });

  it('Should fail before initiating', function() {
    return expect(accessController.setMute('sessionId', 'av', true))
      .to.be.rejectedWith('Session does NOT exist');
  });

  it('Should fail on non-webrtc sessions', function() {
    mockrpcReq.getWorkerNode = sinon.stub();
    mockrpcReq.initiate = sinon.stub();
    mockrpcReq.terminate = sinon.stub();
    mockrpcReq.recycleWorkerNode = sinon.stub();

    mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
    mockrpcReq.initiate.resolves('ok');
    mockrpcReq.terminate.resolves('ok');
    mockrpcReq.recycleWorkerNode.resolves('ok');

    var session_id = 'sessionId';
    spyOnSessionEstablished.reset();
    spyOnSessionAborted.reset();

    return accessController.initiate(testParticipantId,
                                     session_id,
                                     'in',
                                     {isp: 'isp', region: 'region'},
                                     {
                                       type: 'streaming',
                                       connection: {
                                         url: 'URLofSomeIPCamera',
                                         transportProtocol: 'tcp',
                                         bufferSize: 2048,
                                         origin: {isp: 'isp', region: 'region'}
                                       },
                                       media: {
                                         audio: 'auto',
                                         video: true
                                       }
                                     })
      .then(function(result) {
        expect(result).to.equal('ok');
        return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'pcmu'}, video: {codec: 'h264', resolution: {width: 1920, height: 1080}}});
      })
      .then(function(result) {
        expect(result).to.equal('ok');
        return expect(accessController.setMute(session_id, 'video', true)).to.be.rejectedWith('Session does NOT support muting');
      });
  });

  it('Should succeed if rpcRequest.mediaOnOff succeeds, and fail if rpcRequest.meidaOnOff fails', function() {
    mockrpcReq.getWorkerNode = sinon.stub();
    mockrpcReq.initiate = sinon.stub();

    mockrpcReq.getWorkerNode.resolves({agent: 'rpcIdOfAccessAgent', node: 'rpcIdOfAccessNode'});
    mockrpcReq.initiate.resolves('ok');

    var session_id = 'sessionId';
    spyOnSessionEstablished.reset();
    spyOnSessionAborted.reset();
    spyOnSessionSignaling.reset();

    return accessController.initiate(testParticipantId,
                                     session_id,
                                     'in',
                                     {isp: 'isp', region: 'region'},
                                     {
                                       type: 'webrtc',
                                       media: {
                                         audio: {source: 'mic'},
                                         video: {source: 'camera'}
                                       }
                                     })
      .then(function(result) {
        expect(result).to.equal('ok');
        return accessController.onSessionStatus(session_id, {type: 'ready', audio: {codec: 'opus', sampleRate: 48000, channelNum: 2}, video: {codec: 'h264', profile: 'high'}});
      })
      .then(function(result) {
        expect(result).to.equal('ok');
        mockrpcReq.mediaOnOff = sinon.stub();
        mockrpcReq.mediaOnOff.resolves('ok');
        return accessController.setMute(session_id, 'video', true);
      })
      .then(function(result) {
        expect(result).to.equal('ok');
        mockrpcReq.mediaOnOff = sinon.stub();
        mockrpcReq.mediaOnOff.rejects('No audio track');
        return expect(accessController.setMute(session_id, 'audio', true)).to.be.rejectedWith('No audio track');
      });
  });
});

