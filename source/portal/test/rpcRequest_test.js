// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var rpcRequest = require('../rpcRequest');
var rpcChannel = require('../rpcChannel');

describe('rpcRequest.getController', function() {
  it('Should succeed if scheduling agent and requiring node succeed.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.resolve({id: 'RpcIdOfControllerAgent', info: {ip: 'x.x.x.x', purpose: 'conference', state: 2, max_load: 0.85}}));
    mockRpcChannel.makeRPC.onCall(1).returns(Promise.resolve('RpcIdOfController'));

    var req = rpcRequest(mockRpcChannel);

    return req.getController('owt-cluster', 'roomId').then(function(result) {
      expect(result).to.equal('RpcIdOfController');
      expect(mockRpcChannel.makeRPC.getCall(0).args).to.deep.equal(['owt-cluster', 'schedule', ['conference', 'roomId', 'preference', 30000]]);
      expect(mockRpcChannel.makeRPC.getCall(1).args).to.deep.equal(['RpcIdOfControllerAgent', 'getNode', [{room: 'roomId', task: 'roomId'}]]);
    });
  });

  it('Should fail if scheduling agent error or timeout occurs.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.reject('error or timeout while getting agent'));

    var req = rpcRequest(mockRpcChannel);

    return expect(req.getController('owt-cluster', 'roomId')).to.be.rejectedWith('error or timeout while getting agent');
  });

  it('Should fail if requiring node error or timeout occurs.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    mockRpcChannel.makeRPC.onCall(0).returns(Promise.resolve('RpcIdOfControllerAgent'));
    mockRpcChannel.makeRPC.onCall(1).returns(Promise.reject('error or timeout'));

    var req = rpcRequest(mockRpcChannel);

    return expect(req.getController('owt-cluster', 'roomId')).to.be.rejectedWith('error or timeout');
  });
});

describe('rpcRequest.join/leave/publish/unpublish/subscribe/unsubscribe/onSessionSignaling/streamControl/subscriptionControl/text', function() {
  it('Should succeed if rpcChannel.makeRPC succeeds.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.resolves('ok-or-data');

    var join = req.join('rpcIdOfController', 'roomId', {id: 'participantId', user: 'UserId', role: 'UserRole', portal: 'portalRpcId', origin: 'originObj'});
    var leave = req.leave('rpcIdOfController', 'roomId', 'participantId')
    var pub = req.publish('rpcIdOfController', 'participantId', 'streamId', 'pubOptions');
    var unpub = req.unpublish('rpcIdOfController', 'participantId', 'streamId');
    var sub = req.subscribe('rpcIdOfController', 'participantId', 'subscriptionId', 'subOptions');
    var unsub = req.unsubscribe('rpcIdOfController', 'participantId', 'subscriptionId');
    var onSessionSignaling = req.onSessionSignaling('rpcIdOfAccessNode', 'sessionId', {type: 'offer', sdp: 'offerSDPString'});
    var streamControl = req.streamControl('rpcIdOfAccessNode', 'streamId', {operation: 'pause', data: 'video'});
    var subscriptionControl = req.subscriptionControl('rpcIdOfAccessNode', 'subscriptionId', {operation: 'update', data: {video: {from: 'streamxx'}}});
    var text = req.text('rpcIdOfController', 'fromWhom', 'toWhom', 'message body');

    return Promise.all([
      expect(join).to.become('ok-or-data'),
      expect(leave).to.become('ok-or-data'),
      expect(pub).to.become('ok-or-data'),
      expect(unpub).to.become('ok-or-data'),
      expect(sub).to.become('ok-or-data'),
      expect(unsub).to.become('ok-or-data'),
      expect(onSessionSignaling).to.become('ok-or-data'),
      expect(streamControl).to.become('ok-or-data'),
      expect(subscriptionControl).to.become('ok-or-data'),
      expect(text).to.become('ok-or-data'),
      ])
      .then(function() {
        expect(mockRpcChannel.makeRPC.callCount).to.equal(10);
      });
  });

  it('Should fail if rpcChannel.makeRPC fails with reason of error or timeout.', function() {
    var mockRpcChannel = sinon.createStubInstance(rpcChannel);
    mockRpcChannel.makeRPC = sinon.stub();
    var req = rpcRequest(mockRpcChannel);

    mockRpcChannel.makeRPC.rejects('error or timeout');

    var join = req.join('rpcIdOfController', 'roomId', {id: 'participantId', user: 'UserId', role: 'UserRole', portal: 'portalRpcId', origin: 'originObj'});
    var leave = req.leave('rpcIdOfController', 'roomId', 'participantId')
    var pub = req.publish('rpcIdOfController', 'participantId', 'streamId', 'pubOptions');
    var unpub = req.unpublish('rpcIdOfController', 'participantId', 'streamId');
    var sub = req.subscribe('rpcIdOfController', 'participantId', 'subscriptionId', 'subOptions');
    var unsub = req.unsubscribe('rpcIdOfController', 'participantId', 'subscriptionId');
    var onSessionSignaling = req.onSessionSignaling('rpcIdOfAccessNode', 'sessionId', {type: 'offer', sdp: 'offerSDPString'});
    var streamControl = req.streamControl('rpcIdOfAccessNode', 'streamId', {operation: 'pause', data: 'video'});
    var subscriptionControl = req.subscriptionControl('rpcIdOfAccessNode', 'subscriptionId', {operation: 'update', data: {video: {from: 'streamxx'}}});
    var text = req.text('rpcIdOfController', 'fromWhom', 'toWhom', 'message body');

    return Promise.all([
      expect(join).to.be.rejectedWith('error or timeout'),
      expect(leave).to.be.rejectedWith('error or timeout'),
      expect(pub).to.be.rejectedWith('error or timeout'),
      expect(unpub).to.be.rejectedWith('error or timeout'),
      expect(sub).to.be.rejectedWith('error or timeout'),
      expect(unsub).to.be.rejectedWith('error or timeout'),
      expect(onSessionSignaling).to.be.rejectedWith('error or timeout'),
      expect(streamControl).to.be.rejectedWith('error or timeout'),
      expect(subscriptionControl).to.be.rejectedWith('error or timeout'),
      expect(text).to.be.rejectedWith('error or timeout'),
      ])
      .then(function() {
        expect(mockRpcChannel.makeRPC.callCount).to.equal(10);
      });
  });
});

