'use strict';
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');
var sinonAsPromised = require('sinon-as-promised');

var Participant = require('../participant');
var rpcReq = require('../rpcRequest');

describe('participant.isPublishPermitted', function() {
  var participant_spec = {id: 'participantId', user: {id: 'user-id', name: 'user-name'}, role: 'role', portal: 'portalObj', origin: 'originObj'}
  var mockrpcReq = sinon.createStubInstance(rpcReq);

  it('With permission.publish being false.', function() {
    participant_spec.permission = {
      publish: false,
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant = Participant(participant_spec, mockrpcReq);

    expect(participant.isPublishPermitted('audio', 'webrtc')).to.be.false;
    expect(participant.isPublishPermitted('audio', 'streaming')).to.be.false;
    expect(participant.isPublishPermitted('video', 'webrtc')).to.be.false;
    expect(participant.isPublishPermitted('video', 'streaming')).to.be.false;
  });

  it('With permission.publish.media being {audio: true, video: false}.', function() {
    participant_spec.permission = {
      publish: {
        type: ['webrtc', 'streaming'],
        media: {
          audio: true,
          video: false
        }
      },
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant = Participant(participant_spec, mockrpcReq);

    expect(participant.isPublishPermitted('audio', 'webrtc')).to.be.true;
    expect(participant.isPublishPermitted('audio', 'streaming')).to.be.true;
    expect(participant.isPublishPermitted('video', 'webrtc')).to.be.false;
    expect(participant.isPublishPermitted('video', 'streaming')).to.be.false;
  });

  it('With permission.publish.media being {audio: false, video: true}.', function() {
    participant_spec.permission = {
      publish: {
        type: ['webrtc', 'streaming'],
        media: {
          audio: false,
          video: true
        }
      },
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant = Participant(participant_spec, mockrpcReq);

    expect(participant.isPublishPermitted('audio', 'webrtc')).to.be.false;
    expect(participant.isPublishPermitted('audio', 'streaming')).to.be.false;
    expect(participant.isPublishPermitted('video', 'webrtc')).to.be.true;
    expect(participant.isPublishPermitted('video', 'streaming')).to.be.true;
  });

  it('With permission.publish.type not containing a specific type.', function() {
    participant_spec.permission = {
      publish: {
        type: ['streaming'],
        media: {
          audio: true,
          video: true
        }
      },
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant1 = Participant(participant_spec, mockrpcReq);

    expect(participant1.isPublishPermitted('audio', 'webrtc')).to.be.false;
    expect(participant1.isPublishPermitted('audio', 'streaming')).to.be.true;
    expect(participant1.isPublishPermitted('video', 'webrtc')).to.be.false;
    expect(participant1.isPublishPermitted('video', 'streaming')).to.be.true;

    participant_spec.permission = {
      publish: {
        type: ['webrtc'],
        media: {
          audio: true,
          video: true
        }
      },
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant2 = Participant(participant_spec, mockrpcReq);

    expect(participant2.isPublishPermitted('audio', 'webrtc')).to.be.true;
    expect(participant2.isPublishPermitted('audio', 'streaming')).to.be.false;
    expect(participant2.isPublishPermitted('video', 'webrtc')).to.be.true;
    expect(participant2.isPublishPermitted('video', 'streaming')).to.be.false;
  });

  it('With permission.publish.media being {audio: false, video: true} and permission.publish.type contains all types.', function() {
    participant_spec.permission = {
      publish: {
        type: ['webrtc', 'streaming'],
        media: {
          audio: true,
          video: true
        }
      },
      subscribe: {
        type: ['webrtc', 'streaming', 'recording'],
        media: {
          audio: true,
          video: true
        }
      },
      text: 'to-all',
      manage: false
    };
    var participant = Participant(participant_spec, mockrpcReq);

    expect(participant.isPublishPermitted('audio', 'webrtc')).to.be.true;
    expect(participant.isPublishPermitted('audio', 'streaming')).to.be.true;
    expect(participant.isPublishPermitted('video', 'webrtc')).to.be.true;
    expect(participant.isPublishPermitted('video', 'streaming')).to.be.true;
  });
});

