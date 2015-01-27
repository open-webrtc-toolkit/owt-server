var conference = Woogeen.ConferenceClient.create();
var localStream, remoteStream=[];
var Mix = false;
var Forward = false;
var myCodec = "";
var ClientId;
var currentRoom;
var newToken;
function getParameterByName(name) {
  name = name.replace(/[\[]/, "\\\[").replace(/[\]]/, "\\\]");
  var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
  results = regex.exec(location.search);
  return results == null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
}

function isValid( obj ) {
  return (typeof obj === 'object' && obj !== null);
}

function getOneStream() {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      return stream;
    }
  }
  return undefined;
}
function displayStream (stream) {
    var div = document.createElement('div');

    var streamId = stream.id();
        console.log("displayStream in");
        console.log("stream id is " + stream.id());
    if (stream instanceof Woogeen.RemoteStream && stream.isMixed()) {
      div.setAttribute('style', 'width: 640px; height: 480px;');
    } else {
      div.setAttribute('style', 'width: 320px; height: 240px;');
    }
    div.setAttribute('id', 'test' + streamId);
    document.body.appendChild(div);
    stream.show('test' + streamId);
  }

function recreateLocal(video){
      console.log('recreateLocal');
      localStream.close();
      if (video == true) {
      Woogeen.LocalStream.create({
          video: {
              device: 'camera',
              resolution: 'hd720p',
              framerate: [30, 500]
            },
            audio: true
          },function (err, stream) {
          localStream = stream;
          localStream.show('localVideo');
        });
      }
      else {
      Woogeen.LocalStream.create({
          video: false,
          audio: true
          },function (err, stream) {
          localStream = stream;
          localStream.show('localVideo');
        });
      }
}

function playVideo () {
      console.log('video-out-on')
      localStream.playVideo();
    }
function pauseVideo () {
      console.log('video-out-off')
      localStream.pauseVideo();
    }
function playAudio () {
      console.log('audio-out-on')
      localStream.playAudio();
    }
function pauseAudio () {
      console.log('audio-out-off')
      localStream.pauseAudio();
    }
function enableVideo () {
      console.log('enableVideo')
      localStream.enableVideo();
    }
function disableVideo () {
      console.log('disableVideo')
      localStream.disableVideo();
    }
function enableAudio () {
      console.log('enableAudio')
      localStream.enableAudio();
    }
function disableAudio () {
      console.log('disableAudio')
      localStream.disableAudio();
    }
function lastremoteplayvideo () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.playVideo();
      }
    }
function lastremotepausevideo () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.pauseVideo();
      }
    }
function lastremoteplayaudio () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.playAudio();
      }
    }
function lastremotepauseaudio () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.pauseAudio();
      }
    }

function lastremoteenablevideo () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.enableVideo();
      }
}
function lastremotedisablevideo () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.disableVideo();
      }
    }
function lastremoteenableaudio () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.enableAudio();
      }
    }
function lastremotedisableaudio () {
      var lastRemoteStream = getOneStream();
      if (lastRemoteStream) {
        lastRemoteStream.disableAudio();
      }
    }
  function remoteplayvideo () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.playVideo();
    }
  }
 }
function remotepausevideo () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.pauseVideo();
    }
  }
    }
function remoteplayaudio () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.playAudio();
    }
  }
}
function remotepauseaudio () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.pauseAudio();
    }
  }
}

function remoteenablevideo () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.enableVideo();
    }
  }
}
function remotedisablevideo () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.disableVideo();
    }
  }
}
function remoteenableaudio () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.enableAudio();
    }
  }
}
function remotedisableaudio () {
  for (var i in conference.remoteStreams) {
      var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
       stream.disableAudio();
    }
  }
}
function sendd() {
  if (isValid(conference)) {
    /*
      if(ClientId == undefined){
          ClientId =0;
      }
      else{
         ClientId = ClientId.id();
      }
      */
      console.log("clientid is ",ClientId);
    conference.send("&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________************^&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________***********&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________************^&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________***********",ClientId, function(){
        console.log("send successful");
    }, function(err){
        console.log("send error", err);
    });
  }
}
function hidev (tag) {
  if (isValid(localStream)) {
    localStream.hide(tag);
  }
}

function showv (tag) {
  if (isValid(localStream) && localStream.showing !== true) {
    localStream.show(tag, {muted: 'muted'});
  }
}

function createNewToken(role) {
createToken(currentRoom, 'user', role, function (response) {
       newToken = response;
       console.log("create new token ", newToken);
       });
}

function joinRoom(){
    console.log("join room using new token", newToken);
    conference.join(newToken, function (resp) {
        console.log("join room succesfully", resp);
    }, function (err){
        console.log("join room err", err);
    });
}

function unpub () {
  if (isValid(localStream) && isValid(conference)) {
    conference.unpublish(localStream, function(et){
   //L.Logger.info('unpublish stream ', et.id());
    }, function(err){

    L.Logger.error('unpublish error ', err);
    });
  };
}

function stoplocal() {
  if (isValid(localStream)) {
    localStream.hide("localVideo");
  //  alert("b");
    localStream.close();
  }
}

function pub (codec) {
  if (isValid(conference)) {
    if (isValid(localStream)) {
        conference.publish(localStream, {maxVideoBW: 300,videoCodec: codec}, function (st) {
              L.Logger.info('stream published:',st.id());
            }, function (err) {
              L.Logger.error('publish failed:', err);
            });
    } else {
              L.Logger.info('screenshare:');
  }
}
}

function shareScreen () {
//shareScreen will be added here
          conference.shareScreen({resolution:'hd720p'}, function (stream) {
            document.getElementById('myScreen').setAttribute('style', 'width:320px; height: 240px;');
            stream.show('myScreen');
          }, function (err) {
            L.Logger.error('share screen failed:', err);
          });
}

function fullScreen () {
  var el = document.getElementById('conference');
  if (typeof el.webkitRequestFullScreen === 'function') {
    el.webkitRequestFullScreen();
  } else if (typeof el.mozRequestFullScreen === 'function') {
    el.mozRequestFullScreen();
  }
  console.log('fullscreen request!');
}

function subs(st, codec) {
  if (isValid(conference)) {
    if (st === 'mix') {
        for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!localStream || localStream.id() !== stream.id()) {
              if(stream.isMixed()){
                  L.Logger.info( 'mix is true');
                  conference.subscribe(stream, {videoCodec: codec}, function(et){
                  L.Logger.info( 'subscribe stream', et.id());
                  displayStream(et);
                 }, function(err){
                 L.Logger.error('subscribe failed:', err);
                 });
              };
      }
    }
  }
     else if ( st == 'forward'){
      for (var i in conference.remoteStreams) {
          var stream = conference.remoteStreams[i];
              if(!(stream.isMixed()) && (!stream.isScreen())){
                  L.Logger.info( 'forward is true');
                  conference.subscribe(stream, {videoCodec: codec}, function(et){
                      L.Logger.info('subscribe stream', et.id());
                      displayStream(et);
                  }, function(err){
                      L.Logger.error('subscribe failed:', err);
                  });
            };
            };
      }
      else if (st == 'all') {
      for (var i in conference.remoteStreams) {
          var stream = conference.remoteStreams[i];
                conference.subscribe(stream,{videoCodec: codec},  function(et){
                L.Logger.info('subscribe stream', et.id());
                displayStream(et);
                }, function(err){
                 L.Logger.error('subscribe failed:', err);
                 });
            };
    } else if(!localStream || st.id() !== localStream.id()) {
      conference.subscribe(stream, {videoCodec: 'vp8'}, function(et){
        displayStream(stream);
        L.Logger.info(stream.id(), 'subscribe stream');
      }, function(err){
        L.Logger.error(stream.id(), 'subscribe failed:', err);
      });
    };
  };
}

function unsub(st) {
  if (isValid(conference)) {
    if (st === undefined) {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
          conference.unsubscribe(stream,function(et){
              L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err){
              L.Logger.error(stream.id(), 'unsubscribe failed:', err);
      });
      }
    }
    else if(!localStream || st == 'mix') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (conference.remoteStreams[i].isMixed() ) {
          conference.unsubscribe(stream,function(et){
              L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err){
              L.Logger.error(stream.id(), 'unsubscribe failed:', err);
          });
        }
      }
    }
      else {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!(conference.remoteStreams[i].isMixed())&&(conference.remoteStreams[i].isScreen()) ) {
          conference.unsubscribe(stream,function(et){
              L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err){
              L.Logger.error(stream.id(), 'unsubscribe failed:', err);
          });
        }
    }
  }
}
}

function room_disconn() {
  if (isValid(conference)) {
    conference.leave();
    if (localStream) {
      localStream.close();
    }
  }
}

function subscribe(stream) {
        console.log('Mix is ' + Mix);
        console.log('Forward is ' + Forward);
        console.log('video codec is ' + myCodec);
        if (!localStream || localStream.id() !== stream.id()) {
          if(Mix == true) {
              L.Logger.info( 'mix is true');
              if(stream.isMixed()){
                  L.Logger.info( '2mix is true');
                  conference.subscribe(stream,{videoCodec: 'vp8', videoCodec: myCodec },  function(et){
                      L.Logger.info( 'subscribe stream', et.id());
                      displayStream(et);
                  }, function(err){
                      L.Logger.error('subscribe failed:', err);
                  });
              };
            };
            if(Forward == true){
                  L.Logger.info( 'forward is true');
                  if(!(stream.isMixed()) && (!stream.isScreen())){
                      L.Logger.info( '2forward is true');
                      conference.subscribe(stream,{videoCodec: 'vp8', videoCodec: myCodec },  function(et){
                          L.Logger.info('subscribe stream', et.id());
                          displayStream(et);
                      }, function(err){
                          L.Logger.error('subscribe failed:', err);
                      });
            };
            };
        };
}
 // var localStream;

  function createToken (room, userName, role, callback) {
    var req = new XMLHttpRequest();
    var url = '/createToken/';
    var body = {room: room, username: userName, role: role};
    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        callback(req.responseText);
      }
    };
    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body));
  }

   var recording;
   function startRecording (recording) {
     if (conference !== undefined){
       if (recording){
         conference.startRecorder({}, function(info){console.log("recording successful ", info);}, function(err){console.log("recording error", err);});
       } else {
         conference.stopRecorder({}, function(info){console.log("recording stop succesful", info);}, function(err){console.log("recording stop error", err);});
       }
     }
   }

//  var conference = Woogeen.Conference.create({});



  conference.onMessage(function (event) {
    L.Logger.info('Message Received:', event.msg);
  });

  conference.on('server-disconnected', function () {
    L.Logger.info('Server disconnected');
  });
  conference.on('video-ready', function(){
    L.Logger.info('video-ready');
  });
    conference.on('video-hold', function(){
    L.Logger.info('video-hold');
  });
    conference.on('audio-ready', function(){
    L.Logger.info('audio-ready');
  });
    conference.on('audio-hold', function(){
    L.Logger.info('audio-hold');
  });
    conference.on('video-on', function(){
    L.Logger.info('Video on');
  });
    conference.on('video-off', function(){
    L.Logger.info('Video off');
  });
    conference.on('audio-on', function(){
    L.Logger.info('audio on');
  });
    conference.on('audio-off', function(){
    L.Logger.info('audio off');
  });

  conference.on('stream-removed', function (event) {
    var stream = event.stream;
    L.Logger.info('stream removed:' ,stream.id());
    if (stream.elementId !== undefined) {
      var element = document.getElementById(stream.elementId);
      if (element) {document.body.removeChild(element);}
    }
  });

  conference.on('user-joined', function (event) {
    L.Logger.info('user joined:', event.user);
    var ClientId = event.user;
    console.log(ClientId.id);
        conference.send("))))))))))))))))",ClientId.id,function(){
     console.log("send successful");
    }, function(err){
        console.log("send error", err);
    });
  });

  conference.on('user-left', function (event) {
    L.Logger.info('user left:', event.user);
    var ClientId = event.user;
        conference.send("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",ClientId.id,function(){
     console.log("send successful");
    }, function(err){
    console.log("send error", err);
    });
  });

  window.onload = function () {
    L.Logger.setLogLevel(L.Logger.INFO);
    var shareScreen = getParameterByName('screen') || false;
    var myRoom = getParameterByName('room');
    var isPublish = getParameterByName("publish");
    var isSubscribe = getParameterByName("subscribe");
    var resolution = getParameterByName("resolution") || 'vga';
    var prepareRooms = getParameterByName("preparerooms");
    var myname = getParameterByName('myname') || 'audience_'+Math.round(Math.random()*10000);
    var myrole = getParameterByName('myrole') || 'presenter';
    var maxVideoBW = getParameterByName("maxVideoBW") || 300;
    var maxAudioBW = getParameterByName("maxAudioBW") || 30;
    var subscribeMix = getParameterByName("subscribeMix") || true;
    var subscribeForward = getParameterByName("subscribeForward") || true;
    var codec = getParameterByName("codec") || 'vp8';

   console.log('myrole is ' + myrole);
   myCodec = codec;
   currentRoom = myRoom;
      if(subscribeMix == true){
          Mix = true;
      }
      if(subscribeForward == true) {
         Forward = true;
      }
  conference.on('stream-added', function (event) {
    var stream = event.stream;
    L.Logger.info('stream added:', stream.id());
    var fromMe = false;
    for (var i in conference.localStreams) {
      if (conference.localStreams.hasOwnProperty(i)) {
        if (conference.localStreams[i].id() === stream.id()) {
          fromMe = true;
          break;
        }
      }
    }
    if (fromMe) {
      L.Logger.info('stream', stream.id(), 'is from me; will not be subscribed.');
      return;
    }
    L.Logger.info('subscribing:', stream.id());
          if(isSubscribe == 'true' || isSubscribe == ""){
              subscribe(stream);
          }
  });
    createToken(myRoom, 'user', myrole, function (response) {
      var token = response;
      conference.join(token, function (resp) {
        if (shareScreen === false) {
          Woogeen.LocalStream.create({
              video: {
              device: 'camera',
              resolution: resolution,
              framerate: [30, 500]
            },
            audio: true
          }, function (err, stream) {
            if (err) {
                return L.Logger.error('create LocalStream failed:', err);
            }
            localStream = stream;
            localStream.show('localVideo');
            if(isPublish == 'true' || isPublish == ""){
            conference.publish(localStream, {maxVideoBW: maxVideoBW,videoCodec: codec}, function (st) {
              L.Logger.info('stream published:', st.id());
            }, function (err) {
                L.Logger.error('publish failed:', err);
                });
            }
          });
        } else if (isHttps) {
          conference.shareScreen({resolution: myResolution}, function (stream) {
            document.getElementById('myScreen').setAttribute('style', 'width:320px; height: 240px;');
            stream.show('myScreen');
          }, function (err) {
            L.Logger.error('share screen failed:', err);
          });
        } else {
          L.Logger.error('Share screen must be done in https enviromnent!');
        }

        var streams = resp.streams;

        streams.map(function (stream) {
          L.Logger.info('stream in conference:', stream.id());
          L.Logger.info('subscribing:', stream.id());
          if(isSubscribe == 'true' || isSubscribe == ""){
              subscribe(stream);
          };
        });
      }, function (err) {
        L.Logger.error('server connection failed:', err);
      });
    });
  };
