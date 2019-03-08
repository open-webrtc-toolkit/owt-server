// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var rpcRequest = require('../rpcRequest');
var rpcChannel = require('../rpcChannel');

describe('rpcRequest.getWorkerNode', function() {
  it('Should succeed if scheduling agent and requiring node succeed.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.resolve({id: 'rpcIdOfWorkerAgent', info: {ip: 'x.x.x.x', purpose: 'purpose', state: 2, max_load: 0.85}}));
    mockRpcChannel.makeRPC.onCall(1).returns(Promise.resolve('rpcIdOfWorkerNode'));

    var req = rpcRequest(mockRpcChannel);

    return req.getWorkerNode('owt-cluster', 'purpose', {room: 'Session', task: 'sessionId'}, {isp: 'isp', region: 'region'}).then(function(result) {
      expect(result).to.deep.equal({agent:'rpcIdOfWorkerAgent', node: 'rpcIdOfWorkerNode'});
      expect(mockRpcChannel.makeRPC.getCall(0).args).to.deep.equal(['owt-cluster', 'schedule', ['purpose', 'sessionId', {isp: 'isp', region: 'region'}, 30000]]);
      expect(mockRpcChannel.makeRPC.getCall(1).args).to.deep.equal(['rpcIdOfWorkerAgent', 'getNode', [{room: 'Session', task: 'sessionId'}]]);
    });
  });

  it('Should fail if scheduling agent timeout or error occurs.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.reject('timeout or error while getting agent'));

    var req = rpcRequest(mockRpcChannel);

    return expect(req.getWorkerNode('owt-cluster', 'purpose', {room: 'Session', task: 'sessionId'})).to.be.rejectedWith('timeout or error while getting agent');
  });

  it('Should fail if requiring node timeout or error occurs.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.resolve('rpcIdOfWorkerAgent'));
    mockRpcChannel.makeRPC.onCall(1).returns(Promise.reject('timeout or error'));

    var req = rpcRequest(mockRpcChannel);

    return expect(req.getWorkerNode('owt-cluster', 'purpose', {room: 'Session', task: 'sessionId'})).to.be.rejectedWith('timeout or error');
  });
});

describe('rpcRequest.initiate', function() {
  it('Should succeed if rpcChannel.makeRPC succeeds.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.resolves('ok');

    var onStatus = sinon.spy();

    return req.initiate('rpcIdOfController',
                        'sessionId',
                        'webrtc',
                        'in',
                        {media: 'mediaOptionsObj', connection: 'connectionOptionsObj'})
      .then(function(result) {
        expect(result).to.deep.equal('ok');
        expect(mockRpcChannel.makeRPC.getCall(0).args[0]).to.equal('rpcIdOfController');
        expect(mockRpcChannel.makeRPC.getCall(0).args[1]).to.equal('publish');
        expect(mockRpcChannel.makeRPC.getCall(0).args[2][0]).to.equal('sessionId');
        expect(mockRpcChannel.makeRPC.getCall(0).args[2][1]).to.equal('webrtc');
        expect(mockRpcChannel.makeRPC.getCall(0).args[2][2]).to.deep.equal({media: 'mediaOptionsObj', connection: 'connectionOptionsObj'});
        return req.initiate('rpcIdOfController',
                            'sessionId',
                            'webrtc',
                            'out',
                            {media: 'mediaOptionsObj', connection: 'connectionOptionsObj'})
          .then(function(result) {
            expect(result).to.deep.equal('ok');
            expect(mockRpcChannel.makeRPC.getCall(1).args[0]).to.equal('rpcIdOfController');
            expect(mockRpcChannel.makeRPC.getCall(1).args[1]).to.equal('subscribe');
            expect(mockRpcChannel.makeRPC.getCall(1).args[2][0]).to.equal('sessionId');
            expect(mockRpcChannel.makeRPC.getCall(1).args[2][1]).to.equal('webrtc');
            expect(mockRpcChannel.makeRPC.getCall(1).args[2][2]).to.deep.equal({media: 'mediaOptionsObj', connection: 'connectionOptionsObj'});

          });
      });
  });

  it('Should fail if rpcChannel.makeRPC timeout or error occurs.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.rejects('timeout or error');

    return Promise.all([
      expect(req.initiate('rpcIdOfController',
                          'sessionId',
                          'webrtc',
                          'in',
                          {media: 'mediaOptionsObj', connection: 'connectionOptionsObj'})).to.be.rejectedWith('timeout or error'),
      expect(req.initiate('rpcIdOfController',
                          'sessionId',
                          'webrtc',
                          'out',
                          {media: 'mediaOptionsObj', connection: 'connectionOptionsObj'})).to.be.rejectedWith('timeout or error')
    ]);
  });
});

describe('rpcRequest.terminate', function() {
  it('Should succeed if rpcChannel.makeRPC succeeds.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.resolves('ok');

    expect(req.terminate('rpcIdOfWorkerNode', 'sessionId', 'out')).to.become('ok');
    expect(req.terminate('rpcIdOfWorkerNode', 'sessionId', 'in')).to.become('ok');
    expect(req.recycleWorkerNode('rpcIdOfWorkerAgent', 'rpcIdOfWorkerNode', {room: 'Session', task: 'sessionId'})).to.become('ok');
  });

  it('Should still succeed if rpcChannel.makeRPC fails.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.rejects('timeout or error');

    expect(req.terminate('rpcIdOfWorkerNode', 'sessionId', 'in')).to.become('ok'),
    expect(req.terminate('rpcIdOfWorkerNode', 'sessionId', 'out')).to.become('ok'),
    expect(req.recycleWorkerNode('rpcIdOfWorkerAgent', 'rpcIdOfWorkerNode', {room: 'Session', task: 'sessionId'})).to.become('ok')
   });
});

describe('rpcRequest.getRoomConfig/onSessionSignaling/mediaOnOff/sendMsg/dropUser', function() {
  it('Should succeed if rpcChannel.makeRPC succeeds.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.resolves('ok-or-data');
    var getRoomConfig = req.getRoomConfig('config-server', 'roomId');
    var mediaOnOff = req.mediaOnOff('rpcIdOfWorkerNode', 'sessionId', 'video', 'in', 'off');
    var onSessionSignaling = req.onSessionSignaling('rpcIdOfWorkerNode', 'sessionId', 'soacObj');
    var sendMsg = req.sendMsg('rpcIdOfPortal', 'participantId', 'event-name', 'msgObj');
    var dropUser = req.dropUser('rpcIdOfPortal', 'participantId');


    return Promise.all([
      expect(getRoomConfig).to.become('ok-or-data'),
      expect(mediaOnOff).to.become('ok-or-data'),
      expect(onSessionSignaling).to.become('ok-or-data'),
      expect(sendMsg).to.become('ok-or-data'),
      expect(dropUser).to.become('ok-or-data')
      ])
      .then(function() {
        expect(mockRpcChannel.makeRPC.callCount).to.equal(5);
      });
  });

  it('Should fail if rpcChannel.makeRPC fails with reason of timeout or error.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.rejects('timeout-or-error');
    var getRoomConfig = req.getRoomConfig('config-server', 'roomId');
    var mediaOnOff = req.mediaOnOff('rpcIdOfWorkerNode', 'sessionId', 'video', 'in', 'off');
    var onSessionSignaling = req.onSessionSignaling('rpcIdOfWorkerNode', 'sessionId', 'soacObj');
    var sendMsg = req.sendMsg('rpcIdOfPortal', 'participantId', 'event-name', 'msgObj');
    var dropUser = req.dropUser('rpcIdOfPortal', 'participantId');

    return Promise.all([
      expect(getRoomConfig).to.be.rejectedWith('timeout-or-error'),
      expect(mediaOnOff).to.be.rejectedWith('timeout-or-error'),
      expect(onSessionSignaling).to.be.rejectedWith('timeout-or-error'),
      expect(sendMsg).to.be.rejectedWith('timeout-or-error'),
      expect(dropUser).to.be.rejectedWith('timeout-or-error')
      ])
      .then(function() {
        expect(mockRpcChannel.makeRPC.callCount).to.equal(5);
      });
  });
});

