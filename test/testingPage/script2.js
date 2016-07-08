var conference = Woogeen.ConferenceClient.create();
var conference2 = Woogeen.ConferenceClient.create();
var localStream, remoteStream = [],
  localStream2;
var Mix = false;
var Forward = true;
var myCodec = "";
var ClientId;
var currentRoom;
var newToken;
var videoLocal = true;
var audioLocal = true;
var options = document.getElementById('streamIds').children;
var resultRecorder = document.getElementById('resultRecorder');

function recordActionResulte (result) {
  var innerText = resultRecorder.value;
  if (innerText) {
    resultRecorder.value = innerText + "," +result;
  }else {
    resultRecorder.value = result;
  };
}

function getParameterByName(name) {
  name = name.replace(/[\[]/, "\\\[").replace(/[\]]/, "\\\]");
  var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
    results = regex.exec(location.search);
  return results == null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
}

function isValid(obj) {
  return (typeof obj === 'object' && obj !== null);
}

function getOneStream(name) {
  var mixS , forwardS, screenS;
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      if (stream.isMixed()) {
        mixS = stream;
      }else if(stream.isScreen()){
        screenS = stream;
      }else {
        forwardS = stream;
      };
    }
  }
  if (name === 'mix') {
    return mixS;
  }else if(name === 'forward'){
    return forwardS;
  }else {
    return screenS;
  };
  return undefined;
}

function setWH (ele, percent, parentEleId) {
     var w = document.getElementById(parentEleId).offsetWidth * percent;
     ele.style.width = w + 'px';
     ele.style.height = 0.75 * w + 'px';
     setInterval(function(){
       if(w != ele.parentNode.offsetWidth * percent){
            w = ele.parentNode.offsetWidth * percent;
            ele.style.width = w + 'px';
            ele.style.height = 0.75*w + 'px';
        }
     }, 100);
}

function displayStream(stream) {
  var div
  var streamId = stream.id();
  console.log("displayStream in");
  console.log("stream id is " + stream.id());
  if(!document.getElementById('test' + stream.id())){
    div = document.createElement('div');
    if (stream instanceof Woogeen.RemoteStream && stream.isMixed()) {
       div.setAttribute('style', 'float: left; margin-left:2px; border: 5px solid #670795; padding:1px');
       setWH(div, 0.97, 'container-video');
    } else {
      div.setAttribute('style', 'float: left; margin:3px 0px 3px 3px;');
      setWH(div, 0.49, 'container-video');
    }
    div.setAttribute('id', 'test' + streamId);
    document.getElementById('container-video').appendChild(div);
  }else{
    div = document.getElementById('test' + stream.id());
  }


  if (window.navigator.appVersion.indexOf("Trident") < 0) {
    stream.show('test' + streamId);
  } else {
    L.Logger.info('displayStream:', stream.id());
    var canvas = document.createElement("canvas");
    if (stream instanceof Woogeen.RemoteStream && stream.isMixed()) {
      canvas.width = 640;
      canvas.height = 480;
    } else {
      canvas.width = 320;
      canvas.height = 240;
    }
    canvas.setAttribute("autoplay", "autoplay::autoplay");
    div.appendChild(canvas);
    var ieStream = new ieMediaStream(stream.mediaStream.label);
    attachRemoteMediaStream(canvas, ieStream, stream.pcid);
  }
}

function recreateLocal(hasVideo, hasAudio) {
  console.log('recreateLocal');
  localStream.close();

  console.log('video is ', hasVideo, 'audio is ', hasAudio);
  Woogeen.LocalStream.create({
    video: hasVideo,
    audio: hasAudio,

  }, function(err, stream) {
    if (err) {
      recordActionResulte(false);
      return L.Logger.error('create LocalStream failed:', err);
    }
    localStream = stream;
    recordActionResulte(true);
    if (window.navigator.appVersion.indexOf("Trident") < 0) {
      localStream.show('localVideo');
    }
    if (window.navigator.appVersion.indexOf("Trident") > -1) {
      var canvas = document.createElement("canvas");
      canvas.width = 320;
      canvas.height = 240;
      canvas.setAttribute("autoplay", "autoplay::autoplay");
      document.getElementById("localVideo").appendChild(canvas);
      attachMediaStream(canvas, localStream.mediaStream);
    }

  });
}

function playVideo() {
  console.log('video-out-on');
  conference.playVideo(localStream, function() {
    recordActionResulte(true);
    console.log('play video succesful');
  }, function() {
    recordActionResulte(false);
    console.log('play video failed');
  });
}

function pauseVideo() {
  console.log('video-out-off')
  conference.pauseVideo(localStream, function() {
    recordActionResulte(true);
    console.log('pause video succesful');
  }, function() {
    recordActionResulte(false);
    console.log('pause video failed');
  });
}

function playAudio() {
  console.log('audio-out-on');
  conference.playAudio(localStream, function() {
    recordActionResulte(true);
    console.log('play audio succesful');
  }, function() {
    recordActionResulte(false);
    console.log('play audio failed');
  });
}

function pauseAudio() {
  console.log('audio-out-off');
  conference.pauseAudio(localStream, function() {
    recordActionResulte(true);
    console.log('pause audio succesful');
  }, function() {
    recordActionResulte(false);
    console.log('pause audio failed');
  });
}

function enableVideo() {
  console.log('enableVideo');
  localStream.enableVideo();
}

function disableVideo() {
  console.log('disableVideo');
  localStream.disableVideo();
}

function enableAudio() {
  console.log('enableAudio')
  localStream.enableAudio();
}

function disableAudio() {
  console.log('disableAudio')
  localStream.disableAudio();
}

function lastremoteplayvideo() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    conference.playVideo(lastRemoteStream, function() {
      console.log('play video succesful');
    }, function() {
      console.log('play video failed');
    });
  }
}

function lastremotepausevideo() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    conference.pauseVideo(lastRemoteStream, function() {
      console.log('play video succesful');
    }, function() {
      console.log('play video failed');
    });
  }
}

function lastremoteplayaudio() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    conference.playAudio(lastRemoteStream, function() {
      console.log('play video succesful');
    }, function() {
      console.log('play video failed');
    });
  }
}

function lastremotepauseaudio() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    conference.pauseAudio(lastRemoteStream, function() {
      console.log('play audio succesful');
    }, function() {
      console.log('play audio failed');
    });
  }
}

function lastremoteenablevideo() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    lastRemoteStream.enableVideo();
  }
}

function lastremotedisablevideo() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    lastRemoteStream.disableVideo();
  }
}

function lastremoteenableaudio() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    lastRemoteStream.enableAudio();
  }
}

function lastremotedisableaudio() {
  var lastRemoteStream = getOneStream();
  if (lastRemoteStream) {
    lastRemoteStream.disableAudio();
  }
}

function screenplayvideo() {
  for (var i in conference.remoteStreams) {
    if (conference.remoteStreams[i].isScreen()) {
      conference.playVideo(conference.remoteStreams[i], function() {
        recordActionResulte(true);
        console.log('play remote video success');
      }, function(err) {
        recordActionResulte(false);
        console.log("play remote video failed", err);
      });
    }
  }
}

function screenpausevideo() {
  for (var i in conference.remoteStreams) {
    if (conference.remoteStreams[i].isScreen()) {
      conference.pauseVideo(conference.remoteStreams[i], function() {
        recordActionResulte(true);
        console.log('play remote video success');
      }, function(err) {
        recordActionResulte(false);
        console.log("play remote video failed", err);
      });
    }
  }
}

function remoteplayvideo() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      //      stream.playVideo();
      conference.playVideo(stream, function() {
        recordActionResulte(true);
        console.log('play remote video success');
      }, function(err) {
        recordActionResulte(false);
        console.log("play remote video failed", err);
      });
    }
  }
}

function remoteplayaudio() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      //stream.playAudio();
      conference.playAudio(stream, function() {
        recordActionResulte(true);
        console.log('pause remote audio success');
      }, function(err) {
        recordActionResulte(false);
        console.log("pause remote audio failed", err);
      });
    }
  }
}

function remotepauseaudio() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      //stream.pauseAudio();
      conference.pauseAudio(stream, function() {
        console.log('pause remote audio success');
        recordActionResulte(true);
      }, function(err) {
        recordActionResulte(false);
        console.log("pause remote audio failed", err);
      });
    }
  }
}


function remotepausevideo() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      conference.pauseVideo(stream, function() {
        recordActionResulte(true);
          console.log("yes")
        }),
        function(err) {
          recordActionResulte(false);
          console.log('failed', err)
        };
    }
  }
}

function remoteenablevideo() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      stream.enableVideo();
    }
  }
}

function remotedisablevideo() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      stream.disableVideo();
    }
  }
}

function remoteenableaudio() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      stream.enableAudio();
    }
  }
}

function remotedisableaudio() {
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      stream.disableAudio();
    }
  }
}

function sendd() {
  if (isValid(conference)) {
    console.log("clientid is ", ClientId);
    conference.send("&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________************^&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________***********&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________************^&((((((((((((((((((%^$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*^^^^^)))))))&_********)ULHHHHHHHHHHHHHHHHHHHHHHHHHLLLLLLLGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGLOBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB<UJ&)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))SHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHhhhiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiI*********************_HJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJGGGGGGGGGGGGGGGGGCCCCCCCCCCCCCCCCCCCCCCCJUuuuuuuuuuuuuuuuuuuOYYYYYYYYYYYYYYYYYYSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWIOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO&____________***********", ClientId, function() {
      console.log("send successful");
      recordActionResulte(true);
    }, function(err) {
      recordActionResulte(false);
      console.log("send error", err);
    });
  }
}

function hidev(tag) {
  if (isValid(localStream)) {
    localStream.hide(tag);
  }
}

function showv(tag) {
  if (isValid(localStream) && localStream.showing !== true) {
    localStream.show(tag, {
      muted: 'muted'
    });
  }
}

function createNewToken(role) {
  createToken(currentRoom, 'user', role, function(response) {
    recordActionResulte(true);
    newToken = response;
    console.log("create new token ", newToken);
  });
}

function joinRoom() {
  console.log("join room using new token", newToken);
  conference.join(newToken, function(resp) {
    recordActionResulte(true);
    console.log("join room succesfully", resp);
  }, function(err) {
    recordActionResulte(false);
    console.log("join room err", err);
  });
}

function unpub() {
  if (isValid(localStream) && isValid(conference)) {
    conference.unpublish(localStream, function(et) {
      recordActionResulte(true);
    }, function(err) {
      recordActionResulte(false);
      L.Logger.error('unpublish error ', err);
    });
  };
}

function stoplocal() {
  if (isValid(localStream)) {
    localStream.hide("localVideo");
    localStream.close();
  }
}

function pub(codec) {
  if (isValid(conference)) {
    if (isValid(localStream)) {
      L.Logger.info('@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@publish local with codec:', codec);
      conference.publish(localStream, {
        maxVideoBW: 300,
        videoCodec: codec
      }, function(st) {
        recordActionResulte(true);
        L.Logger.info('stream published:', st.id());
      }, function(err) {
        recordActionResulte(false);
        // if(err) throw err;
        L.Logger.error('publish failed:', err);
      });
    } else {
      L.Logger.info('screenshare:');
    }
  }
}

function shareScreen() {
  //shareScreen will be added here
  conference.shareScreen({
    resolution: 'hd720p'
  }, function(stream) {
    recordActionResulte(true);
    // document.getElementById('screenVideo').setAttribute('style', 'width:320px; height: 240px;');
    if (window.navigator.appVersion.indexOf("Trident") < 0) {
      stream.show('screenVideo');
    }
    if (window.navigator.appVersion.indexOf("Trident") > -1) {
      var canvas = document.createElement("canvas");
      canvas.width = 320;
      canvas.height = 240;
      canvas.setAttribute("autoplay", "autoplay::autoplay");
      document.getElementById("screenVideo").appendChild(canvas);
      attachMediaStream(canvas, stream.mediaStream);
    }
  }, function(err) {
    recordActionResulte(false);
    L.Logger.error('share screen failed:', err);
  });
}

function fullScreen() {
  var el = document.getElementById('conference');
  if (typeof el.webkitRequestFullScreen === 'function') {
    el.webkitRequestFullScreen();
  } else if (typeof el.mozRequestFullScreen === 'function') {
    el.mozRequestFullScreen();
  }
  console.log('fullscreen request!');
}

function subs(st, codec) {
  subMixVideo = document.getElementById('subMixVideo').checked;
  subMixAudio = document.getElementById('subMixAudio').checked;
  subForwVideo = document.getElementById('subForwVideo').checked;
  subForwAudio = document.getElementById('subForwAudio').checked;
  console.log('subMixVideo', subMixVideo, 'subMixAudio', subMixAudio, 'subForwVideo', subForwVideo, 'subForwAudio', subForwAudio);
  if (isValid(conference)) {
    if (st === 'mix') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!localStream || localStream.id() !== stream.id()) {
          if (stream.isMixed()) {
            stream.on('VideoLayoutChanged', function() {
              L.Logger.info('stream', stream.id(), 'VideoLayoutChanged');
            });
            stream.on("VideoEnabled", function() {
              L.Logger.info('@@@@@@@@@@@@@Video enabled event triggered!');
            });

            stream.on("VideoDisabled", function() {
              L.Logger.info('@@@@@@@@@@@@@Video disabled event triggered!');
            });

            stream.on("AudioEnabled", function() {
              L.Logger.info('@@@@@@@@@@@@@Audio enabled event triggered!');
            });

            stream.on("AudioDisabled", function() {
              L.Logger.info('@@@@@@@@@@@@@Audio disabled event triggered!');
            });
            L.Logger.info('mix is true');
            L.Logger.info('*****************************************************************subscribe API mix with codec:', codec);
            conference.subscribe(stream, {
              video: subMixVideo, //subMixVideo,
              audio: subMixAudio,
              videoCodec: codec,
              resolution: {
                'width': 333,
                'height': 222
              }
            }, function(et) {
              recordActionResulte(true);
              L.Logger.info('subscribe stream', et.id());
              displayStream(et);
            }, function(err) {
              recordActionResulte(false);
              L.Logger.error('subscribe failed:', err);
            });
          };
        }
      }
    } else if (st == 'forward') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!(stream.isMixed()) && (!stream.isScreen()) && (stream.id() != localStream.id())) {
          stream.on('VideoLayoutChanged', function() {
            L.Logger.info('stream', stream.id(), 'VideoLayoutChanged');
          });
          stream.on("VideoEnabled", function() {
            L.Logger.info('@@@@@@@@@@@@@Video enabled event triggered!');
          });

          stream.on("VideoDisabled", function() {
            L.Logger.info('@@@@@@@@@@@@@Video disabled event triggered!');
          });

          stream.on("AudioEnabled", function() {
            L.Logger.info('@@@@@@@@@@@@@Audio enabled event triggered!');
          });

          stream.on("AudioDisabled", function() {
            L.Logger.info('@@@@@@@@@@@@@Audio disabled event triggered!');
          });
          L.Logger.info('forward is true');
          L.Logger.info('*******************************************************subscribe API forward with codec:', codec);
          conference.subscribe(stream, {
            video: subForwVideo,
            audio: subForwAudio,
            videoCodec: codec
          }, function(et) {
            recordActionResulte(true);
            L.Logger.info('subscribe stream', et.id());
            displayStream(et);
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error('subscribe failed:', err);
          });
        };
      };
    } else if (st == 'shareScreen') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (stream.isScreen()) {
          L.Logger.info('shareScreen is true');
          L.Logger.info('***************************************************************subscribe API share screen with codec:', codec);
          conference.subscribe(stream, {
            videoCodec: codec
          }, function(et) {
            recordActionResulte(true);
            L.Logger.info('subscribe stream', et.id());
            displayStream(et);
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error('subscribe failed:', err);
          });
        };
      };
    } else if (st == 'all') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        var flag = true;
        for (var x in conference.localStreams) {
          if (conference.localStreams[x].id() == stream.id()) {
            flag = false;
            break
          }
        }
        if (flag) {
          conference.subscribe(stream, {
            videoCodec: codec
          }, function(et) {
            recordActionResulte(true);
            L.Logger.info('subscribe stream', et.id());
            displayStream(et);
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error('subscribe failed:', err);
          });
        }
      };
    } else if (!localStream || st.id() !== localStream.id()) {
      conference.subscribe(stream, {
        videoCodec: codec
      }, function(et) {
        recordActionResulte(true);
        displayStream(stream);
        L.Logger.info(stream.id(), 'subscribe stream');
      }, function(err) {
        recordActionResulte(false);
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
        conference.unsubscribe(stream, function(et) {
          recordActionResulte(true);
          L.Logger.info(stream.id(), 'unsubscribe stream');
        }, function(err) {
          recordActionResulte(false);
          L.Logger.error(stream.id(), 'unsubscribe failed:', err);
        });
      }
    } else if (!localStream || st == 'mix') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (conference.remoteStreams[i].isMixed()) {
          conference.unsubscribe(stream, function(et) {
            recordActionResulte(true);
            L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error(stream.id(), 'unsubscribe failed:', err);
          });
        }
      }
    } else if (st == 'shareScreen') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (conference.remoteStreams[i].isScreen()) {
          conference.unsubscribe(stream, function(et) {
            recordActionResulte(true);
            L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error(stream.id(), 'unsubscribe failed:', err);
          });
        }
      }
    } else {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!(conference.remoteStreams[i].isMixed()) && !(conference.remoteStreams[i].isScreen())) {
          conference.unsubscribe(stream, function(et) {
            recordActionResulte(true);
            L.Logger.info(stream.id(), 'unsubscribe stream');
          }, function(err) {
            recordActionResulte(false);
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
      // localStream.close();
    }
  }
}

function subscribe(stream) {
  console.log('Mix is ' + Mix);
  console.log('Forward is ' + Forward);
  console.log('video codec is ' + myCodec);

  if (!localStream || localStream.id() !== stream.id()) {
    if (Mix == true) {

      L.Logger.info('subscribing:', stream.id());
      var videoOpt = true;
      if (stream.isMixed()) {
        var resolutions = stream.resolutions();
        console.log('resolutions is ---------', resolutions);
        var resolution;
        if (resolutions.length > 1) {
          resolution = resolutions[Math.floor(Math.random() * 10) % resolutions.length];
          console.log('resolution is +++++++++++', resolution);
          videoOpt = {
            resolution: resolution
          };
          L.Logger.info('subscribe stream with option:', resolution);
        }
        L.Logger.info('>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>subscribe stream mix with codec:', myCodec);
        conference.subscribe(stream, {
          video: videoOpt,
          videoCodec: myCodec
        }, function() {
          L.Logger.info('subscribed:', stream.id());
          displayStream(stream, resolution);
        }, function(err) {
          L.Logger.error(stream.id(), 'subscribe failed:', err);
        });
      }
    } else {
      L.Logger.info('won`t subscribe', stream.id());
    }
    if (Forward == true) {

      L.Logger.info('forward is true');
      L.Logger.info('>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>subscribe stream forward with myCodec');
      if (!(stream.isMixed()) /* && !(stream.isScreen())*/ ) {
        L.Logger.info('2forward is true');
        conference.subscribe(stream, {
          video: subForwVideo,
          audio: subForwAudio,
          videoCodec: myCodec
        }, function(et) {
          L.Logger.info('subscribe stream', et.id());
          displayStream(et);
        }, function(err) {
          L.Logger.error('subscribe failed:', err);
        });
      };
    };
  };
}
// var localStream;

function createToken(room, userName, role, callback) {
  var req = new XMLHttpRequest();
  var url = '/createToken/';
  var body = {
    room: room,
    username: userName,
    role: role
  };
  req.onreadystatechange = function() {
    if (req.readyState === 4) {
      callback(req.responseText);
    }
  };
  req.open('POST', url, true);
  req.setRequestHeader('Content-Type', 'application/json');
  req.send(JSON.stringify(body));
}

var recording;

function setVideoBitrate() {
  var bitrate = document.getElementById("bitrate").value;
  var id = document.getElementById("setVideo").value;

  console.log("=====Set bitrate to:", bitrate);
  var options = {
    id: id,
    bitrate: bitrate
  };

  conference.setVideoBitrate(options, function(result) {
    console.log("Successfully set video bitrate with result:", result);
  }, function(err) {
    console.log("Fail to set video bitrate with err:", err);
  });
}

function setRegion() {
  var region = document.getElementById("regionId").value;
  var clientId = document.getElementById("setVideo").value;

  var options = {
    id: clientId,
    region: region
  };

  console.log("User id is:", clientId);
  console.log("Region is:", region);
  conference.setRegion(options, function(result) {
    console.log("Successfully set region with result:", result);
  }, function(err) {
    console.log("Fail to set region with err:", err);
  });


}

function getRegion() {
  var clientId = document.getElementById("setVideo").value;
  var options = {
    id: clientId
  };

  console.log("User id is:", clientId);
  conference.getRegion(options, function(result) {
    console.log("Successfully get region:", result);
  }, function(err) {
    console.log("Fail to get region with err:", err);
  });

}

function mixLocal() {
  if (conference !== undefined) {
    conference.mix(localStream, function(str) {
      recordActionResulte(true);
      console.log("Mix local stream succeed:", str);
    }, function(err) {
      recordActionResulte(false);
      console.log("Mix local stream failed:", err);
    });
  }
}

function unmixLocal() {
  if (conference !== undefined) {
    console.log("local stream is:", localStream);
    console.log("Remote stream is:", conference.remoteStreams);
    conference.unmix(localStream, function(str) {
      recordActionResulte(true);
      console.log("Unmix local stream succeed:", str);
    }, function(err) {
      recordActionResulte(false);
      console.log("Unmix local stream failed:", err);
    });
  }
}

function unmix_mix() {
   var count = parseInt(document.getElementById("cycleCounts").value) || 20;
    var x = 0;
    var interval1 = setInterval(function () {
        x++;
        if (conference !== undefined) {
        conference.unmix(localStream, function(str) {
          console.log("unmix local stream succeed:", str);
          conference.mix(localStream, function(str) {
            console.log("mix local stream succeed:", str);
          }, function(err) {
            console.log("mix local stream failed:", err);
          });
        }, function(err) {
          console.log("unmix local stream failed:", err);
        });
      }
      if (x === count ) {
        clearInterval(interval1);
      };
    }, 100)
}

function unpublish_publish() {
  var count = parseInt(document.getElementById("cycleCounts").value) || 20;
  var x = 0;
  var interval1 = setInterval(function () {
      x++;
      if (conference !== undefined) {
      conference.unpublish(localStream, function(str) {
        console.log("unpublsh local stream succeed:", str);
        conference.publish(localStream, function(str) {
          console.log("publsh local stream succeed:", str);
        }, function(err) {
          console.log("publsh local stream failed:", err);
        });
      }, function(err) {
        console.log("unpublsh local stream failed:", err);
      });
    }
    if (x === count ) {
      clearInterval(interval1);
    };
  }, 100)

}

function unsubscribe_subscribe(name) {
  var count = parseInt(document.getElementById("cycleCounts").value) || 20;
  var x = 0;
  var remoteS = getOneStream(name);
  var interval1 = setInterval(function () {
      x++;
      if (conference !== undefined) {
      conference.unsubscribe(remoteS, function(str) {
        console.log("unsubscribe local stream succeed:", str);
        conference.subscribe(remoteS, {videoCodec: codec}, function(str) {
          displayStream(str);
          console.log("subscribe local stream succeed:", str);
        }, function(err) {
          console.log("subscribe local stream failed:", err);
        });
      }, function(err) {
        console.log("unsubscribe local stream failed:", err);
      });
    }
    if (x === count ) {
      clearInterval(interval1);
    };
  }, 100)

}

function startRecording(recording) {
  if (conference !== undefined) {
    var video = document.getElementById("videoStreamId").value;
    var audio = document.getElementById("audioStreamId").value;
    var id = document.getElementById("recorderId").value;
    var videoCodec = document.getElementById("videoCodec").value;
    var audioCodec = document.getElementById("audioCodec").value;
    console.log('----------------videoCodec is :', videoCodec);
    if (recording) {
      conference.startRecorder({
        videoStreamId: video,
        audioStreamId: audio,
        recorderId: id,
        videoCodec: videoCodec,
        audioCodec: audioCodec
      } /*{streamId: video, recorderId: id}*/ , function(info) {
        recordActionResulte(true);
        console.log("recording successful ", info);
      }, function(err) {
        recordActionResulte(false);
        console.log("recording error", err);
      });
    } else {
      conference.stopRecorder({
        recorderId: id
      }, function(info) {
        recordActionResulte(true);
        console.log("recording stop succesful", info);
      }, function(err) {
        recordActionResulte(false);
        console.log("recording stop error", err);
      });
    }
  }
}

//  var conference = Woogeen.Conference.create({});



conference.onMessage(function(event) {
  L.Logger.info('Message Received:', event.msg);
});

conference.on('server-disconnected', function() {
  L.Logger.info('Server disconnected');
});
conference.on('video-ready', function() {
  L.Logger.info('video-ready');
});
conference.on('video-hold', function() {
  L.Logger.info('video-hold');
});
conference.on('audio-ready', function() {
  L.Logger.info('audio-ready');
});
conference.on('audio-hold', function() {
  L.Logger.info('audio-hold');
});
conference.on('video-on', function() {
  L.Logger.info('Video on');
});
conference.on('video-off', function() {
  L.Logger.info('Video off');
});
conference.on('audio-on', function() {
  L.Logger.info('audio on');
});
conference.on('audio-off', function() {
  L.Logger.info('audio off');
});
conference.on('recorder-added', function() {
  L.Logger.info('recorder-added');
});
conference.on('recorder-continued', function() {
  L.Logger.info('recorder-continued');
});
conference.on('recorder-removed', function() {
  L.Logger.info('recorder-removed');
});
conference.on('stream-removed', function(event) {
  var stream = event.stream;
  for (var i = 0; i < options.length; i++) {
    if(options[i].getAttribute('value') === stream.id()){
      // options[i].setAttribute('value', '');
    }
  };
  L.Logger.info('stream removed:', stream.id());
  var id = stream.elementId !== undefined ? stream.elementId : "test" + stream.id();
  if (id !== undefined) {
    var element = document.getElementById(id);
    if (element) {
      // document.getElementById('container-video').removeChild(element);
    }
  }
});

conference.on('user-joined', function(event) {
  L.Logger.info('user joined:', event.user);
  var ClientId = event.user;
  console.log(ClientId.id);
  conference.send("))))))))))))))))", ClientId.id, function() {
    console.log("send successful");
  }, function(err) {
    console.log("send error", err);
  });
});

conference.on('user-left', function(event) {
  L.Logger.info('user left:', event.user);
  var ClientId = event.user;
  conference.send("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", ClientId.id, function() {
    console.log("send successful");
  }, function(err) {
    console.log("send error", err);
  });
});


window.onload = function() {
  L.Logger.setLogLevel(L.Logger.INFO);
  var shareScreen = getParameterByName('screen') || false;
  var myRoom = getParameterByName('room');
  var isPublish = getParameterByName("publish");
  var isSubscribe = getParameterByName("subscribe");
  var resolution = getParameterByName("resolution") || 'vga';
  var prepareRooms = getParameterByName("preparerooms");
  var myname = getParameterByName('myname') || 'audience_' + Math.round(Math.random() * 10000);
  var myrole = getParameterByName('myrole') || 'presenter';
  var maxVideoBW = getParameterByName("maxVideoBW") || 300;
  var maxAudioBW = getParameterByName("maxAudioBW") || 30;
  var subscribeMix = getParameterByName("subscribeMix") || true;
  var subscribeForward = getParameterByName("subscribeForward") || true;
  var codec = getParameterByName("codec") || 'vp8';
  var mediaUrl = getParameterByName('url');
  var transport = getParameterByName('transport');
  var bufferSize = getParameterByName('bufferSize');
  var video = getParameterByName("video") || 'all';
  var audio = getParameterByName("audio") || 'all';
  var unmix = getParameterByName("unmix") || false;

  console.log('myrole is ' + myrole);
  myCodec = codec;
  currentRoom = myRoom;
  if (subscribeMix == true) {
    Mix = true;
  }
  if (subscribeForward == "false") {
    Forward = false;
  }

  if (video == 'local') {
    console.log("********************************video local is false");
    videoLocal = false;
  }

  if (audio == 'local') {
    console.log("********************************audio local is false");
    audioLocal = false;
  }
  conference.on('stream-changed', function(event) {
    console.log('stream-changed**********************', event);

  })


  conference.on('stream-added', function(event) {
    var stream = event.stream;
    L.Logger.info('stream added:', stream.id());

    if (localStream) {
      options[0].setAttribute('value', getOneStream('mix')?getOneStream('mix').id():null);
      options[1].setAttribute('value', getOneStream('forward')?getOneStream('forward').id():null);
      options[2].setAttribute('value', getOneStream('screen')?getOneStream('screen').id():null);
      // options[2].setAttribute('value', stream.id());
    };

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
    if (isSubscribe == 'true' || isSubscribe == "") {
      subscribe(stream);
    }
  });
  createToken(myRoom, 'user', myrole, function(response) {
    var token = response;

    conference.join(token, function(resp) {
      // var mixStream1 = getOneStream("mix");
      console.log('==============>>>>>>>>>>>', conference.remoteStreams);
      var bytes_recv, diff;
      // setInterval(function () {
        // conference.getConnectionStats(mixStream1, function(stats){
        //   var bytes_recvX = stats[1].stats.bytes_recv;
        //   diff = bytes_recvX - bytes_recv;
        //   bytes_recv = bytes_recvX;
        //   console.log('============>>>>>>>>>>>>>', diff);
        // });
      // }, 100)

      if (typeof mediaUrl === 'string' && mediaUrl !== '') {
        Woogeen.LocalStream.create({
          video: videoLocal,
          audio: audioLocal,
          url: mediaUrl
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          conference.publish(localStream, {
            transport: transport,
            bufferSize: bufferSize,
            videoCodec: codec,
            unmix: !!unmix
          }, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          })
        });
      } else if (shareScreen === false) {
        Woogeen.LocalStream.create({
          video: videoLocal ? {
            device: 'camera',
            resolution: resolution,
            framerate: [30, 500]
          } : videoLocal,
          audio: true
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          console.log('localStream is ', localStream);
          setWH(document.getElementById('localVideo'), 0.49, 'container-video');
          setWH(document.getElementById('screenVideo'), 0.49, 'container-video');
          if (window.navigator.appVersion.indexOf("Trident") < 0) {
            localStream.show('localVideo');
          }
          if (window.navigator.appVersion.indexOf("Trident") > -1) {
            var canvas = document.createElement("canvas");
            canvas.width = 320;
            canvas.height = 240;
            canvas.setAttribute("autoplay", "autoplay::autoplay");
            document.getElementById("localVideo").appendChild(canvas);
            attachMediaStream(canvas, localStream.mediaStream);
          }
          if (isPublish == 'true' || isPublish == "") {
            console.log('+++++++++++publish is ', isPublish);
            conference.publish(localStream, {
              unmix: !!unmix,
              maxVideoBW: maxVideoBW,
              videoCodec: codec
            }, function(st) {
              L.Logger.info('++++++++++++stream published:', st.id());

            }, function(err) {
              L.Logger.error('publish failed:', err);
            });
          }
        });
      } else if (isHttps) {
        conference.shareScreen({
          resolution: myResolution
        }, function(stream) {
          document.getElementById('myScreen').setAttribute('style', 'width:320px; height: 240px;');
          if (window.navigator.appVersion.indexOf("Trident") < 0) {
            stream.show('myScreen');
          }
          if (window.navigator.appVersion.indexOf("Trident") > -1) {
            var canvas = document.createElement("canvas");
            canvas.width = 320;
            canvas.height = 240;
            canvas.setAttribute("autoplay", "autoplay::autoplay");
            document.getElementById("myScreen").appendChild(canvas);
            attachMediaStream(canvas, stream.mediaStream);
          }
        }, function(err) {
          L.Logger.error('share screen failed:', err);
        });
      } else {
        L.Logger.error('Share screen must be done in https enviromnent!');
      }

      var streams = resp.streams;

      streams.map(function(stream) {
        L.Logger.info('stream in conference:', stream.id());
        L.Logger.info('subscribing:', stream.id());
        if (isSubscribe == 'true' || isSubscribe == "") {
          subscribe(stream);
        };
      });
    }, function(err) {
      L.Logger.error('server connection failed:', err);
    });



  });
};

function openWindow() {
  console.log(location.pathname);
  for (var i = 0; i < 4; i++) {

    window.open('/');
  };
}

window.onbeforeunload = function() {
  if (localStream) {
    localStream.close();
    if (localStream.channel && typeof localStream.channel.close === 'function') {
      localStream.channel.close();
    }
  }
  for (var i in conference.remoteStreams) {
    if (conference.remoteStreams.hasOwnProperty(i)) {
      var stream = conference.remoteStreams[i];
      stream.close();
      if (stream.channel && typeof stream.channel.close === 'function') {
        stream.channel.close();
      }
      delete conference.remoteStreams[i];
    }
  }
};
