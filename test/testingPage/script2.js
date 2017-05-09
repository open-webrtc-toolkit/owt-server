var conference = Woogeen.ConferenceClient.create();
var conference2 = Woogeen.ConferenceClient.create();
var localStream, remoteStream = [], mixStream, mixStreams = [],
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
var maxVideoBW;
var resolutionName2Value = {
    'cif': {width: 352, height: 288},
    'vga': {width: 640, height: 480},
    'svga': {width: 800, height: 600},
    'xga': {width: 1024, height: 768},
    'r640x360': {width: 640, height: 360},
    'hd720p': {width: 1280, height: 720},
    'sif': {width: 320, height: 240},
    'hvga': {width: 480, height: 320},
    'r480x360': {width: 480, height: 360},
    'qcif': {width: 176, height: 144},
    'r192x144': {width: 192, height: 144},
    'hd1080p': {width: 1920, height: 1080},
    'uhd_4k': {width: 3840, height: 2160},
    'r360x360': {width: 360, height: 360},
    'r480x480': {width: 480, height: 480},
    'r720x720': {width: 720, height: 720}
};
function recordActionResulte(result, where) {
  var innerText = resultRecorder.value;
  var stepResult = where ? where : "";
  stepResult = stepResult + '-' + result;
  if (innerText) {
    resultRecorder.value = innerText + "," + stepResult;
  } else {
    resultRecorder.value = stepResult;
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
  var mixS, forwardS, screenS;
  for (var i in conference.remoteStreams) {
    var stream = conference.remoteStreams[i];
    if (stream.id() !== localStream.id()) {
      if (stream.isMixed()) {
        mixS = stream;
      } else if (stream.isScreen()) {
        screenS = stream;
      } else {
        forwardS = stream;
      };
    }
  }
  if (name === 'mix') {
    return mixS;
  } else if (name === 'forward') {
    return forwardS;
  } else {
    return screenS;
  };
  return undefined;
}

function setWH(ele, percent, parentEleId) {
  var w = document.getElementById(parentEleId).offsetWidth * percent;
  ele.style.width = w + 'px';
  ele.style.height = 0.75 * w + 'px';
  setInterval(function() {
    if (ele && ele.parentNode && w != ele.parentNode.offsetWidth * percent) {
      w = ele.parentNode.offsetWidth * percent;
      ele.style.width = w + 'px';
      ele.style.height = 0.75 * w + 'px';
    }
  }, 100);
}

function displayStream(stream) {
  var div
  var streamId = stream.id();
  console.log("displayStream in");
  console.log("stream id is " + stream.id());
  if (!document.getElementById('test' + stream.id())) {
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
  } else {
    div = document.getElementById('test' + stream.id());
    div.style.display = "block";
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
function hiddenStream(id){
  var streamId = id;
  console.log('stream id is : ', id);
  var videoView = document.getElementById('test' + streamId);
  videoView.style.display = "none";
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
  var localStreams = conference.localStreams;
  for (var i in localStreams) {
    if (conference.remoteStreams[i].isScreen()) {
      conference.playVideo(localStreams[i], function() {
        recordActionResulte(true);
        console.log('play screen success');
      }, function(err) {
        recordActionResulte(false);
        console.log("play screen failed", err);
      });
    }
  }
}

function screenpausevideo() {
  var localStreams = conference.localStreams;
  for (var i in localStreams) {
    if (localStreams[i].isScreen()) {
      conference.pauseVideo(localStreams[i], function() {
        recordActionResulte(true);
        console.log('pause screen success');
      }, function(err) {
        recordActionResulte(false);
        console.log("pause screen failed", err);
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
        maxVideoBW: maxVideoBW,
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

function shareScreen(videoCodec, audioCodec) {
  var extensionId = document.getElementById('extensionid').value;
  if(!extensionId){
    console.warn('Please input the extension ID!');
    return
  };
  Woogeen.LocalStream.create({
        audio:true,
        video:{
          device: 'screen',
          resolution:{
            width: screen.width,
            height: screen.height
          },
          extensionId: extensionId
        }
 }, function(err, stream) {
    if(err){
      console.log('share screen failed: ', err);
      recordActionResulte(false);
      return
    }
    conference.publish(stream, {
       audioCodec: audioCodec,
       videoCodec: videoCodec
    }, function(){
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
    }, function(err){
       console.log('Publish screen failed: ', err)
   });
      recordActionResulte(true);
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
  var subMixVideo = document.getElementById('subMixVideo').checked;
  var subMixAudio = document.getElementById('subMixAudio').checked;
  var subForwVideo = document.getElementById('subForwVideo').checked;
  var subForwAudio = document.getElementById('subForwAudio').checked;
  var xResolution = document.getElementById('selectedresolution').innerText;
  var qualityLevel = document.getElementById('selectedqualitylevel').innerText || 'Standard';
  var view = document.getElementById('selectedview').innerText;
  view = view.indexOf('select') != -1 ? 'common': view;
  var wh = [];
  if(xResolution.indexOf('x') != -1){
     subMixVideo = {qualityLevel: qualityLevel};
     wh = xResolution.split('x');
     subMixVideo.resolution = {
       width : parseInt(wh[1]),
       height: parseInt(wh[0])
     }
  }
  console.log('subMixVideo', subMixVideo, 'subMixAudio', subMixAudio, 'subForwVideo', subForwVideo, 'subForwAudio', subForwAudio);
  if (isValid(conference)) {
    if (st === 'mix') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!localStream || localStream.id() !== stream.id()) {
          if (stream.isMixed() && stream.id().match(view)) {

            L.Logger.info('mix is true');
            L.Logger.info('*****************************************************************subscribe API mix with codec:', codec);
            conference.subscribe(stream, {
              video: subMixVideo,
              audio: subMixAudio,
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
        }
      }
    } else if (st == 'forward') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (!(stream.isMixed()) && (!stream.isScreen()) && (stream.id() != localStream.id())) {
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
      L.Logger.info('************************************************************************subscribe API mix with vp8');
      conference.subscribe(stream, {
        videoCodec: 'vp8'
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

function getMixResolutions (){
  var selectedResolution = document.getElementById('selectedresolution');
  var resolutions = mixStream.resolutions();
  var mixResolutionsD = document.getElementById('mixresolutions');
  mixResolutionsD.style.display =  'inline-block';
  var children = mixResolutionsD.children
  while (children[0]){
    mixResolutionsD.removeChild(children[0]);
  }
  for(var j = 0; j < resolutions.length; j++){
    var child = document.createElement('li');
    child.innerText = resolutions[j].height + 'x' + resolutions[j].width;
    mixResolutionsD.appendChild(child);
  }
  var select = function(){
       selectedResolution.innerText = this.innerText;
       mixResolutionsD.style.display = 'none';
       selectedResolution.onclick = getMixResolutions;
     }

   children = mixResolutionsD.children
   for(var i = 0; i < children.length; i++){
     children[i].onclick = select;
   }
   selectedResolution.onclick = select;
}
function getQualityLevels (){
  var selectedQualityLevel = document.getElementById('selectedqualitylevel');
  var qualityLevels = ['BestQuality','BetterQuality', 'Standard', 'BetterSpeed', 'BestSpeed'];
  var qualityLevelsD = document.getElementById('mixqualitylevels');
  qualityLevelsD.style.display =  'inline-block';
  var children = qualityLevelsD.children
  while (children[0]){
    qualityLevelsD.removeChild(children[0]);
  }
  for(var j = 0; j < qualityLevels.length; j++){
    var child = document.createElement('li');
    child.innerText = qualityLevels[j];
    qualityLevelsD.appendChild(child);
  }
   var select =function(){
       selectedQualityLevel.innerText = this.innerText;
       qualityLevelsD.style.display = 'none';
       selectedQualityLevel.onclick = getQualityLevels;
     }

   children = qualityLevelsD.children
   for(var i = 0; i < children.length; i++){
     children[i].onclick = select;
   }
   selectedQualityLevel.onclick = select;
}
function getViews (){
  var selectedView = document.getElementById('selectedview');
  var views = [];
  var remoteStreams = conference.remoteStreams;
  var mixViewsD = document.getElementById('mixviews');
  mixViewsD.style.display =  'inline-block';
  var children = mixViewsD.children;
  for(var streamId in remoteStreams){
     var stream = remoteStreams[streamId];
     if(stream.isMixed()){
       views.push(streamId.split('-')[1]);
     }
  }
  while (children[0]){
    mixViewsD.removeChild(children[0]);
  }
  for(var j = 0; j < views.length; j++){
    var child = document.createElement('li');
    child.innerText = views[j];
    mixViewsD.appendChild(child);
  }
  var select = function(){
       selectedView.innerText = this.innerText;
       mixViewsD.style.display = 'none';
       selectedView.onclick = getViews;
     }
   children = mixViewsD.children
   for(var i = 0; i < children.length; i++){
     children[i].onclick = select;
   }
   selectedView.onclick = select;
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
    } else if (st == 'mix') {
      for (var i in conference.remoteStreams) {
        var stream = conference.remoteStreams[i];
        if (conference.remoteStreams[i].isMixed()) {
         (function(id){
          conference.unsubscribe(stream, function(et) {
            recordActionResulte(true);
            L.Logger.info(id, 'unsubscribe stream');
            hiddenStream(id);
          }, function(err) {
            recordActionResulte(false);
            L.Logger.error(stream.id(), 'unsubscribe failed:', err);
          });
         })(i);
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
        if (!(conference.remoteStreams[i].isMixed()) && !(conference.remoteStreams[i].isScreen()) && stream.id() !== localStream.id()) {
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
  var id = document.getElementById("setvideo").value;

  console.log("=====Set bitrate to:", bitrate);
  var options = {
    id: id,
    bitrate: bitrate
  };

  conference.setVideoBitrate(options, function(result) {
    recordActionResulte(true);
    console.log("Successfully set video bitrate with result:", result);
  }, function(err) {
    recordActionResulte(false);
    console.log("Fail to set video bitrate with err:", err);
  });
}

function setRegion() {
  var region = document.getElementById("regionid").value;
  var clientId = document.getElementById("setvideo").value;
  var regionViewId = document.getElementById("regionviewid").value;

  var options = {
    id: clientId,
    region: region,
    mixedStreamId: regionViewId
  };

  console.log("User id is:", clientId);
  console.log("Region is:", region);
  conference.setRegion(options, function(result) {
    recordActionResulte(true);
    console.log("Successfully set region with result:", result);
  }, function(err) {
    recordActionResulte(false);
    console.log("Fail to set region with err:", err);
  });


}

function getRegion() {
  var clientId = document.getElementById("setvideo").value;
  var regionViewId = document.getElementById("regionviewid").value;
  var options = {
    id: clientId,
    mixedStreamId: regionViewId
  };

  console.log("User id is:", clientId);
  conference.getRegion(options, function(result) {
    recordActionResulte(true);
    console.log("Successfully get region:", result);
  }, function(err) {
    recordActionResulte(false);
    console.log("Fail to get region with err:", err);
  });

}

function getConnectStats(){
   var streamId = document.getElementById("setvideo").value;
   var stream = conference.remoteStreams[streamId];
   conference.getConnectionStats(stream, function(stat){
     console.log('stream ', streamId, 'stat is : ', stat);
   }, function(err){
     console.log('getConnectionstats has err: ', err);
  });
}

function updateChildren(parents, newChildrens){
  oldChildren = [].slice.call(parents.children, 0);
  oldChildren.forEach(function(ele, index){
    parents.removeChild(ele);
  });
  newChildrens.forEach(function(ele, index){
    parents.appendChild(ele);
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



function unmixStreams() {
   var unmixOperation = document.getElementById('unmixoperation');
   var allRemoteStreamsDom = document.getElementById('allremotestreams');
   var allViewsDom = document.getElementById('allviews');
   var unmixRun = document.getElementById('unmixrun');
   var mixRun = document.getElementById('mixrun');
   var mixClose = document.getElementById('mixclose');
   var allRemoteStreamsObj = conference.remoteStreams;
   var mixViewsList = [], ul1NewChildren = [], ul2NewChildren = [], needUnmixStreams = [], fromViews = [];
   mixStreams.forEach(function(ele, index){
     mixViewsList.push(ele.id().slice(ele.id().length-6));
   });

   function liClick (dataArray, domNode){
     return function(){
              var id = domNode.innerText;
              if(dataArray.indexOf(id) == -1){
                dataArray.push(id);
                domNode.style.backgroundColor = "#40ec63";
             }else{
                dataArray.splice(dataArray.indexOf(id), 1);
                domNode.style.backgroundColor = "#777";
             }
           }

   }
   for(var id in allRemoteStreamsObj){
     var liDom = document.createElement('li');
     liDom.innerText = id.slice(id.length-6);
     liDom.onclick = liClick(needUnmixStreams, liDom);
     ul1NewChildren.push(liDom);
   }
   updateChildren(allRemoteStreamsDom, ul1NewChildren);
   mixViewsList.forEach(function(ele, index){
     var liDom = document.createElement('li');
     liDom.innerText = ele;
     liDom.onclick = liClick(fromViews, liDom);
     ul2NewChildren.push(liDom);
   });
   updateChildren(allViewsDom, ul2NewChildren);

   unmixOperation.style.display = "inline-block";

   function getOperationObj(){
     var fromStreams = [], targetStreams = []
     for(var streamId in allRemoteStreamsObj){
        fromViews.forEach(function(ele){
          if(streamId.indexOf(ele) != -1){
             fromStreams.push(allRemoteStreamsObj[streamId]);
          }
        });
     }
     needUnmixStreams.forEach(function(ele){
       for(var id in allRemoteStreamsObj){
         if(id.indexOf(ele) != -1){
           targetStreams.push(allRemoteStreamsObj[id]);
         }
       }
     });
     return {
            from: fromStreams,
            target: targetStreams
          }
   }
   unmixRun.onclick = function(){
     var operationObj = getOperationObj();
     var from = operationObj.from;
     var target = operationObj.target;
     target.forEach(function(stream){
           conference.unmix(stream, from, function(resp){
             console.log('unmix success: ', resp);
           }, function(err){
             console.log('unmix failed: ', err);
           });
     });
   }
   mixRun.onclick = function(){
     var operationObj = getOperationObj();
     var from = operationObj.from;
     var target = operationObj.target;
     target.forEach(function(stream){
           conference.mix(stream, from, function(resp){
             console.log('mix success: ', resp);
           }, function(err){
             console.log('mix failed: ', err);
           });
     });
   }
   mixClose.onclick = function(){
     unmixOperation.style.display = "none";
   }

}

function unmix_mix() {
  var count = parseInt(document.getElementById("cycleCounts").value) || 20;
  var intervals = parseInt(document.getElementById("intervals").value) || 100;
  var x = 0;
  var interval1 = setInterval(function() {
    x++;
    if (conference !== undefined) {
      conference.unmix(localStream, mixStreams, function(str) {
        console.log("unmix local stream succeed:", str);
        conference.mix(localStream,mixStreams, function(str) {
          console.log("mix local stream succeed:", str);
        }, function(err) {
          console.log("mix local stream failed:", err);
        });
      }, function(err) {
        console.log("unmix local stream failed:", err);
      });
    }
    if (x === count) {
      clearInterval(interval1);
    };
  }, intervals)
}

function unpublish_publish() {
  var count = parseInt(document.getElementById("cycleCounts").value) || 20;
  var intervals = parseInt(document.getElementById("intervals").value) || 100;
  var x = 0;
  var interval1 = setInterval(function() {
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
    if (x === count) {
      clearInterval(interval1);
    };
  }, intervals)

}

function unsubscribe_subscribe(name) {
  var count = parseInt(document.getElementById("cycleCounts").value) || 20;
  var intervals = parseInt(document.getElementById("intervals").value) || 100;
  var x = 0;
  var remoteS = getOneStream(name);
  var interval1 = setInterval(function() {
    x++;
    if (conference !== undefined) {
      conference.unsubscribe(remoteS, function(str) {
        console.log("unsubscribe local stream succeed:", str);
        conference.subscribe(remoteS, function(str) {
          displayStream(str);
          console.log("subscribe local stream succeed:", str);
        }, function(err) {
          console.log("subscribe local stream failed:", err);
        });
      }, function(err) {
        console.log("unsubscribe local stream failed:", err);
      });
    }
    if (x === count) {
      clearInterval(interval1);
    };
  }, intervals)

}

//test quality parameter in subscrebe API.
function qualityTest(){
  //the thenable object of subscribe api.
  function subThenable (resolution, quality, codec) {
     return {
              then: function (resolve, reject) {
                conference.subscribe(mixStream, {video:{resolution: resolution, qualityLevel: quality}, videoCodec: codec}, (st) => {
                  console.log('subscribe mix success');
                  displayStream(st)
                  resolve(Promise.resolve());
                }, (err) => {
                  console.log('subscribe mix failed');
                  reject(err);
                });
              }
            }
  }
    // the thenable obejct of unsubscribe api.
  function unsubThenable () {
     return {
              then: function (resolve, reject) {
                conference.unsubscribe(mixStream, (st) => {
                  console.log('unsubscribe mix success');
                  resolve(Promise.resolve());
                }, (err) => {
                  console.log('unsubscribe mix failed');
                  reject(err);
                });
              }
            }
  }
   //the thenable object of getConnectionstats api
  function bitrateThenable () {
     return {
       then: function (resolve, reject) {
         var bytesRcvd = [], results = [];
         function getBitrate(i){
           conference.getConnectionStats(mixStream, (stats) => {
            console.log('mix stream stats is: ', stats);
            bytesRcvd.push(stats[1].stats.bytes_rcvd);
            if(i == 4){
              bytesRcvd.reduce(function (prev, next) {
                results.push(Math.floor((next - prev) * 8 / 1000 / 3));
                return next
              })
              resolve(results.join("  "))
            }
           }, (err) => {
             reject(err);
           });
         }
         for (let i=0; i<5; i++){
          setTimeout(function(){
             getBitrate(i);
           }, 3000*i)
         }
       }
     }
  }
  // transform resolution {} type to string
  var resolutions = mixStream.resolutions();
  function toStringResolution (resolution){
    let stringResolution = "";
    switch (resolution.height){
      case 240:
        stringResolution = "sif";
        break;
      case 360:
        if (resolution.width == 640) {
          stringResolution = "r640x360";
        }else if (resolution.width == 480) {
          stringResolution = "r480x360";
        }else if (resolution.width == 360) {
          stringResolution = "r360x360";
        }
        break;
      case 480:
        if (resolution.width == 640) {
          stringResolution = "vga";
        }else{
          stringResolution = "r480x480";
        }
        break;
      case 600:
        stringResolution = "svga";
        break;
      case 720:
        if(resolution.width == 720){
          stringResolution = "r720x720";
        }else{
          stringResolution = "hd720p";
        }
        break;
      case 768:
        stringResolution = "xga";
        break;
      case 1080:
        stringResolution = "hd1080p";
        break;
      case 2160:
        stringResolution = "uhd_4k";
        break;
    }
    return stringResolution
  }
  function startTest () {
    var codecs = ['vp8', 'vp9', 'h264'];
    var qualitys = ["BestQuality", "BetterQuality", "Standard", "BetterSpeed", "BestSpeed"];
    var resolutionStandard = {
      'sif': 400,
      'r360x360': 491,
      'r640x360': 666,
      'hvga': 533,
      'r480x360': 566,
      'r480x480': 666,
      'vga': 800,
      'r720x720': 1212,
      'svga': 1317,
      'xga': 1736,
      'hd720p': 2000,
      'hd1080p': 4000,
      'uhd_4k': 16000
    }
    let rate = [1.4, 1.2, 1.0, 0.8, 0.6];
    var expectBitrate = {};
    for(let res in resolutionStandard){
      expectBitrate[res] = {};
      for (let i = 0; i < qualitys.length; i++) {
        expectBitrate[res][qualitys[i]] = resolutionStandard[res] * rate[i]
      };
    }
    console.log('resolutions is : ', resolutions, resolutions.length);
    var p = Promise.resolve();
    var results = [];
    for(let i = 0; i<resolutions.length; i++){
       for (let j = 0; j < codecs.length; j++) {
        for (let k = 0; k < qualitys.length; k++) {
          let currentResu;
          p = p.then(function () {
             return Promise.resolve(unsubThenable())
          }).then(function () {
             return Promise.resolve(subThenable(resolutions[i], qualitys[k], codecs[j]))
          }).then(function () {
              return Promise.resolve(bitrateThenable())
          }).then(function (realBitrate) {
             console.log('++++++++++++++++++++results is: ',resolutions[i],toStringResolution(resolutions[i]), qualitys[k]);
                currentResu = {
                codec: codecs[j],
                quality: qualitys[k],
                realBitrates: realBitrate,
                resolution: toStringResolution(resolutions[i]),
                expectBitrate: expectBitrate[toStringResolution(resolutions[i])][qualitys[k]]
              };
             results.push(currentResu)
             console.log('====================results is: ', results);
             return p = Promise.resolve()
          }).catch(function (err) {
             console.log('fffffffffffffff', err);
             return p = Promise.resolve()
          });
        };
      };
    }
  }
  startTest()
}
function startRTMP (url, options){
    url = document.getElementById('rtmpserverurl').value;
    options = {};
    var streamId = document.getElementById('rtmpstreamid').value;
    var resolution = document.getElementById('rtmpresolution').value;

    if(streamId){
        options.streamId = streamId;
    }
    if(resolution){
        options.resolution = resolutionName2Value[resolution];
    }
    conference.addExternalOutput(url, options, function(){
        console.log('start rtmp success.');
    }, function(err){
        console.log('start rtmp failed.', err);
    });
}

function updateRTMP (url, options){
    url = document.getElementById('rtmpserverurl').value;
    options = {};
    var streamId = document.getElementById('rtmpstreamid').value;
    var resolution = document.getElementById('rtmpresolution').value;

    if(streamId){
        options.streamId = streamId;
    }
    if(resolution){
        options.resolution = resolutionName2Value[resolution];
    }
    conference.updateExternalOutput(url, options, function(){
        console.log('update rtmp success.');
    }, function(err){
        console.log('update rtmp failed.', err);
    });
}

function stopRTMP (url, options){
    url = document.getElementById('rtmpserverurl').value;
    options = {};
    var streamId = document.getElementById('rtmpstreamid').value;

    if(streamId){
        options.streamId = streamId;
    }
    conference.removeExternalOutput(url, function(){
        console.log('stop rtmp success.');
    }, function(err){
        console.log('stop rtmp failed.', err);
    });
}

function startRecording(recording) {
  if (conference !== undefined) {
    var video = document.getElementById("videoStreamId").value;
    var audio = document.getElementById("audioStreamId").value;
    var id = document.getElementById("recorderId").value;
    var videoCodec = document.getElementById("videoCodec").value;
    var audioCodec = document.getElementById("audioCodec").value;

    video = video === ""?undefined:video;
    audio = audio === ""?undefined:audio;
    videoCodec = videoCodec === ""?undefined:videoCodec;
    audioCodec = audioCodec === ""?undefined:audioCodec;
    id = id === ""?undefined:id;
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
  recordActionResulte(true, "onMessage");
  L.Logger.info('Message Received:', event.msg);
});

conference.on('server-disconnected', function() {
  recordActionResulte(true, "server_disconnected");
  L.Logger.info('Server disconnected');
});
conference.on('video-ready', function() {
  recordActionResulte(true, "video-ready");
  L.Logger.info('video-ready');
});
conference.on('video-hold', function() {
  recordActionResulte(true, "video-hold");
  L.Logger.info('video-hold');
});
conference.on('audio-ready', function() {
  recordActionResulte(true, "audio-ready");
  L.Logger.info('audio-ready');
});
conference.on('audio-hold', function() {
  recordActionResulte(true, "audio-hold");
  L.Logger.info('audio-hold');
});
conference.on('video-on', function() {
  recordActionResulte(true, "video-on");
  L.Logger.info('Video on');
});
conference.on('video-off', function() {
  recordActionResulte(true, "video-off");
  L.Logger.info('Video off');
});
conference.on('audio-on', function() {
  recordActionResulte(true, "audio-on");
  L.Logger.info('audio on');
});
conference.on('audio-off', function() {
  recordActionResulte(true, "audio-off");
  L.Logger.info('audio off');
});
conference.on('recorder-added', function() {
  recordActionResulte(true, "recorder-added");
  L.Logger.info('recorder-added');
});
conference.on('recorder-continued', function() {
  recordActionResulte(true, "recorder-continued");
  L.Logger.info('recorder-continued');
});
conference.on('recorder-removed', function() {
  recordActionResulte(true, "recorder-removed");
  L.Logger.info('recorder-removed');
});
conference.on('stream-removed', function(event) {
  var stream = event.stream;
  L.Logger.info('stream removed:', stream.id());
  var id = stream.elementId !== undefined ? stream.elementId : "test" + stream.id();
  if (id !== undefined) {
    var element = document.getElementById(id);
    if (element) {
      document.getElementById('container-video').removeChild(element);
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
  var maxVideoBW = getParameterByName("maxVideoBW");
  var maxAudioBW = getParameterByName("maxAudioBW") || 30;
  var subscribeMix = getParameterByName("subscribeMix") || true;
  var subscribeForward = getParameterByName("subscribeForward") || true;
  var videoCodec = getParameterByName("videoCodec") || 'vp8';
  var audioCodec = getParameterByName("audioCodec") || 'opus';
  var mediaUrl = getParameterByName('url');
  var transport = getParameterByName('transport');
  var bufferSize = getParameterByName('bufferSize');
  var video = getParameterByName("video") || 'all';
  var audio = getParameterByName("audio") || 'all';
  var unmix = getParameterByName("unmix") || false;
  var onlyJoin = getParameterByName("onlyJoin") || false;
if(!maxVideoBW){
  switch (resolution){
    case 'sif':
      maxVideoBW = 300;
      break;
    case 'vga':
      maxVideoBW = 800;
      break;
    case 'hd720p':
      maxVideoBW = 2000;
      break;
    case 'hd1080p':
      maxVideoBW = 4000;
      break;
    default:
      maxVideoBW = 1500;
      break;
  }
}
  console.log('myrole is ' + myrole);
  myCodec = videoCodec;
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
    if (localStream) {
      options[0].setAttribute('value', getOneStream('mix') ? getOneStream('mix').id() : null);
      options[1].setAttribute('value', getOneStream('forward') ? getOneStream('forward').id() : null);
      options[2].setAttribute('value', getOneStream('screen') ? getOneStream('screen').id() : null);
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
      mixStreamID = Object.keys(conference.remoteStreams)[0]
      mixStream = conference.remoteStreams[mixStreamID];
      if (typeof mediaUrl === 'string' && mediaUrl !== '') {
        Woogeen.ExternalStream.create({
          video: videoLocal,
          audio: audioLocal,
          url: mediaUrl
        }, function(err, stream) {
          if (err) {
            recordActionResulte(false, "create_rtsp");
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          recordActionResulte(true, "create_rtsp");
          if (!onlyJoin) {
            conference.publish(localStream, {
              transport: transport,
              bufferSize: bufferSize,
              unmix: !!unmix
            }, function(st) {
              recordActionResulte(true, "pub_rtsp");
              L.Logger.info('stream published:', st.id());
            }, function(err) {
              recordActionResulte(false, "pub_rtsp");
              L.Logger.error('publish failed:', err);
            })
          }
        });

      } else if (shareScreen === false) {
        Woogeen.LocalStream.create({
          video: videoLocal ? {
            device: 'camera',
            resolution: resolution,
            framerate: [30, 500]
          } : videoLocal,
          audio: audioLocal
        }, function(err, stream) {
          if (err) {
            recordActionResulte(false, "create_cam");
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          recordActionResulte(true, "create_cam");
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
            if (!onlyJoin) {
              conference.publish(localStream, {
                unmix: !!unmix,
                maxVideoBW: maxVideoBW,
            //    maxAudioBW: maxAudioBW,
                videoCodec: videoCodec,
                audioCodec: audioCodec
              }, function(st) {
                recordActionResulte(true, "pub_cam");
                L.Logger.info('++++++++++++stream published:', st.id());

              }, function(err) {
                recordActionResulte(false, "pub_cam");
                L.Logger.error('publish failed:', err);
              });
            }
          }
        });
      } else if (isHttps) {
        conference.shareScreen({
          resolution: myResolution
        }, function(stream) {
          recordActionResulte(true, "share_screen");
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
          recordActionResulte(false, "share_screen");
          L.Logger.error('share screen failed:', err);
        });
      } else {
        L.Logger.error('Share screen must be done in https enviromnent!');
      }

      var arrD = document.getElementsByClassName('streamFrame'),
        rS, y;
/*
      setInterval(function() {
        rS = conference.remoteStreams;
        y = 2;
        for (var i in rS) {
          var stream = rS[i];
          if (stream !== localStream) {
            if (stream.isMixed()) {
              conference.getConnectionStats(stream, function(o) {
                arrD[0].innerHTML = o[1].stats.framerate_rcvd;
              }, function(err) {

              });
            } else if (stream.isScreen()) {
              conference.getConnectionStats(stream, function(o) {
                arrD[1].innerHTML = o[1].stats.framerate_rcvd;
              }, function(err) {

              });
            } else {
              conference.getConnectionStats(stream, function(o) {
                arrD[y].innerHTML = o[1].stats.framerate_rcvd;
                y++;
              }, function(err) {

              });
            };

          };

        }
      }, 1000);
*/
      recordActionResulte(true, "join room");

      var streams = resp.streams;
      streams.map(function(stream) {
        L.Logger.info('stream in conference:', stream.id());
        L.Logger.info('subscribing:', stream.id());
        if(stream.isMixed()){
          mixStreams.push(stream);
          mixStream = stream;
          stream.on('VideoLayoutChanged', function() {
            L.Logger.info('stream', stream.id(), 'VideoLayoutChanged');
          });
        }
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

        if (isSubscribe == 'true' || isSubscribe == "") {
          subscribe(stream);
        };
      });
    }, function(err) {
      recordActionResulte(false, "join room");
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
  conference.leave();
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
