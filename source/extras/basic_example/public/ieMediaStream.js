var pc_id = 0;
var globalLocalView = null;
var globalLocalStream = new Object();

ieMediaStream = function(label) {
  var that = this;
  this.id = label;
  this.attachedPCID = 0;
  this.label = label;
  this.ended = false;
  this.onaddtrack = null;
  this.onended = null;
  this.onremovetrack = null;
  this.audioTracks = [new ieTrack()];
  this.videoTracks = [new ieTrack()];

  this.getTracks = function() {
  }
  this.getAudioTracks = function() {
    return that.audioTracks;
  }
  this.getVideoTracks = function() {
    return that.videoTracks;
  }
  this.addTrack = function(track) {
  }
  this.removeTrack = function(trac) {
  }
  this.stop = function() {
  }
}

ieTrack = function() {
}

ieRTCDataChannel = function(label, constraints) {
  var that = this;
  var pendingMessages = new Array();
  this.activeX = document.getElementById("WebRTC.ActiveX");
  this.label = label;
  this.constraints = constraints;

  this.onmessage = null;
  this.onopen = null;
  this.onerror = null;
  this.onclose = null;
  this.readyState = 'connecting';
  this.activeX.onmessage = function(msg) {
    var evt = new Object();
    evt.data = msg;
    pendingMessages.push(evt);
    if (that.onmessage) {
      for (var event in pendingMessages) {
        that.onmessage(event);
      }
      pendingMessages = new Array();
    }
  }
  this.activeX.onopen = function(msg) {
    var evt = new Object();
    evt.target = that;
    that.readyState = 'open';
    if (that.open) {
      that.onopen(evt);
    }
  }
  this.activeX.onerror = function() {
    that.onerror();
  }
  this.activeX.onclose = function() {
    var evt = new Object();
    evt.target = that;
    that.readyState = 'closed'
    if (that.onclose) {
      that.onclose(evt);
    }
  }
  this.close = function() {
    if(that.activeX){
      that.activeX.closeDataChannel(that.label);
    }
  }
  this.send = function(data) {
    if(that.activeX){
      that.activeX.send(data);
    }
  }
}

function ieRTCPeerConnection(config, constraints) {
  var that = this;
  this.myId = pc_id+1;
  pc_id = pc_id +1;
  var plugin = document.createElement("OBJECT");
  plugin.setAttribute("ID", "WebRTC.ActiveX"+that.myId);
  plugin.setAttribute("height", "0");
  plugin.setAttribute("width", "0");
  plugin.setAttribute("CLASSID", "CLSID:1D117433-FD6F-48D2-BF76-26E2DC5390FC");
  document.body.appendChild(plugin);
  that.activeX=document.getElementById("WebRTC.ActiveX"+that.myId);

    // new ieRTCPeerConnection
  if (config != null) {
    config = config["iceServers"];
    this.config = JSON.stringify(config);
  }
  if (constraints != null) {
    this.constraints = JSON.stringify(constraints);
  }
  this.activeX.initializePeerConnection(this.config, this.constraints);

  this.createAnswer = function(success, failure, constraints) {
    that.activeX.createAnswer(function (sdp) {
      success(JSON.parse(sdp));
    }, failure, JSON.stringify(constraints));
  }
  this.setLocalDescription = function(sdp, success, failure) {
    that.activeX.setLocalDescription(sdp, function () {
      success();
      that.localDescription = sdp;
    }, failure);
  }
  this.getLocalDescription = function(success) {
    if(!that.activeX){return;}
    that.activeX.getLocalDescription(function (sdp) {
      success(sdp);
    });
  }
  this.setRemoteDescription = function(sdp, success, failure) {
    that.activeX.setRemoteDescription(sdp, success, failure);
  }
  this.addStream = function(stream) {
      that.activeX.getUserMedia(stream.constraints,  function(label) {
        var ieStream = new ieMediaStream(label);
        var ctx = globalLocalView.getContext("2d");
        var img = new Image();
        that.activeX.attachMediaStream(ieStream.label, function (data) {
          img.src = data;
          ctx.drawImage(img, 0, 0, globalLocalView.width, globalLocalView.height);
        });

        that.activeX.addStream(ieStream);
      }, stream.onfailure);
  }
  // Peer connection methods
  this.createOffer = function(success, failure, constraints) {
    if(!that.activeX){return;}
    that.activeX.createOffer(function (sdp) {
      success(JSON.parse(sdp));
    }, failure, JSON.stringify(constraints));
  }
  this.removeStream = function(stream) {
    if(!that.activeX){return;}
    that.activeX.removeStream(stream);
  }
  this.addIceCandidate = function(candidate, success, failure) {
    if(!that.activeX){return;}
    that.activeX.addIceCandidate(candidate, success, failure);
  }
  this.close = function() {
    if(that.activeX){
      that.activeX.close();
      delete that.activeX;
      that.activeX = undefined;
    }
    var plugin = document.getElementById("WebRTC.ActiveX" + that.myId);
    if(plugin){
      document.body.removeChild(plugin);
    }
  }
  this.createDataChannel = function(label, constraints) {
    if(that.activeX){
      that.activeX.createDataChannel(label, constraints);
    }
    var dc = new ieRTCDataChannel(label, constraints);
    return dc;
  }

  // Peer connection events
  this.onicecandidate = null;
  this.onaddstream = null;
  this.onremovestream = null;
  this.oniceconnectionstatechange = null;
  this.onsignalingstatechange = null;
  this.onnegotiationneeded = null;
  this.ondatachannel = null;
  this.iceGatheringState = null;

  // Bind events to ActiveX
  this.activeX.onicecandidate = function(evt) {
    var obj = JSON.parse(evt);
    if (that.onicecandidate) {
      that.onicecandidate(obj);
    }
  }
  this.activeX.onaddstream = function(label) {
    var evt = new Object();
    var stream = new ieMediaStream(label);
    evt.stream = stream;
    evt.pcid = that.myId;
    if (that.onaddstream) {
      //we need to pass the pc_id back to caller to add stream on corresponding peerconnection instance
      that.onaddstream(evt);
    }
  }
  this.onaddstream = function (label) {
    if(that.activeX){
      that.activeX.onaddstream(label);
    }
  }
  this.activeX.onremovestream = function(label) {
    var evt = new Object();
    var stream = new ieMediaStream(label);
    evt.stream = stream;
    // if (that.onremovestream) {
    //   that.onremovestream(evt);
    // }
  }
  this.onremovestream = function (label) {
    if(that.activeX){
      that.activeX.onremovestream(label);
    }
  }
  this.activeX.oniceconnectionstatechange = function(state) {
    that.iceConnectionState = state;
    that.iceGatheringState = state;
    if (that.oniceconnectionstatechange) {
      that.oniceconnectionstatechange(state);
    }
  }
  this.activeX.onicegatheringstatechange = function (state) {
    if (that.onicegatheringstatechange) {
      that.onicegatheringstatechange(state);
    }
  }
  this.activeX.onsignalingstatechange = function() {
    if(!that.activeX) return;
    that.signalingState = that.activeX.signalingState;
    if (that.onsignalingstatechange) {
      that.onsignalingstatechange();
    }
  }
  this.activeX.onnegotiationneeded = function() {
    if (that.onnegotiationneeded) {
      that.onnegotiationneeded();
    }
  }
  this.activeX.ondatachannel = function(label) {
    var evt = new Object();
    var dc = new ieRTCDataChannel(label, null);
    evt.channel = dc;
    if (that.ondatachannel) {
      that.ondatachannel(evt);
    }
  }
  this.ondatachannel = function (label) {
    if(that.activeX){
      that.activeX.ondatachannel(label);
    }
  }

  // Peer connection properties
  this.iceConnectionState = "new";
  this.signalingState = "stable";
}
