(function () {
  'use strict';
  var localStream;
  function getParameterByName (name) {
    name = name.replace(/[\[]/, '\\\[').replace(/[\]]/, '\\\]');
    var regex = new RegExp('[\\?&]' + name + '=([^&#]*)'),
        results = regex.exec(location.search);
    return results === null ? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
  }

  var subscribeMix = getParameterByName("mix") || "true";

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

  // var recording;
  // var recordingId;
  // function startRecording () {
  //   if (room !== undefined){
  //     if (!recording){
  //       room.startRecording(localStream, function(id) {
  //         recording = true;
  //         recordingId = id;
  //       });
  //     } else {
  //       room.stopRecording(recordingId);
  //       recording = false;
  //     }
  //   }
  // }

  var conference = Woogeen.ConferenceClient.create({});

  function displayStream (stream) {
    var div = document.createElement('div');
    var streamId = stream.id();
    if (stream instanceof Woogeen.RemoteStream && stream.isMixed()) {
      div.setAttribute('style', 'width: 640px; height: 480px;');
    } else {
      div.setAttribute('style', 'width: 320px; height: 240px;');
    }
    div.setAttribute('id', 'test' + streamId);
    div.setAttribute('title', 'Stream#' + streamId);
    document.body.appendChild(div);
    stream.show('test' + streamId);
  }

  conference.onMessage(function (event) {
    L.Logger.info('Message Received:', event.msg);
  });

  conference.on('server-disconnected', function () {
    L.Logger.info('Server disconnected');
  });

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

    if ((subscribeMix === "true" && (stream.isMixed() || stream.isScreen())) ||
      (subscribeMix !== "true" && !stream.isMixed())) {
      L.Logger.info('subscribing:', stream.id());
      conference.subscribe(stream, function () {
        L.Logger.info('subscribed:', stream.id());
        displayStream(stream);
      }, function (err) {
        L.Logger.error(stream.id(), 'subscribe failed:', err);
      });
    }
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
  });

  conference.on('user-left', function (event) {
    L.Logger.info('user left:', event.user);
  });

  window.onload = function () {
    L.Logger.setLogLevel(L.Logger.INFO);
    var myResolution = getParameterByName('resolution') || 'vga';
    var shareScreen = getParameterByName('screen') || false;
    var myRoom = getParameterByName('room');
    var isHttps = (location.protocol === 'https:');

    if (isHttps) {
      var shareButton = document.getElementById('shareScreen');
      if (shareButton) {
        shareButton.setAttribute('style', 'display:block');
        shareButton.onclick = (function () {
          conference.shareScreen({resolution: myResolution}, function (stream) {
            document.getElementById('myScreen').setAttribute('style', 'width:320px; height: 240px;');
            stream.show('myScreen');
          }, function (err) {
            L.Logger.error('share screen failed:', err);
          });
        });
      }
    }

    createToken(myRoom, 'user', 'presenter', function (response) {
      var token = response;

      conference.join(token, function (resp) {
        if (shareScreen === false) {
          Woogeen.LocalStream.create({
            video: {
              device: 'camera',
              resolution: myResolution
            },
            audio: true
          }, function (err, stream) {
            if (err) {
              return L.Logger.error('create LocalStream failed:', err);
            }
            localStream = stream;
            localStream.show('myVideo');
            conference.publish(localStream, {maxVideoBW: 300}, function (st) {
              L.Logger.info('stream published:', st.id());
            }, function (err) {
              L.Logger.error('publish failed:', err);
            });
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
          if ((subscribeMix === "true" && (stream.isMixed() || stream.isScreen())) ||
            (subscribeMix !== "true" && !stream.isMixed())) {
            L.Logger.info('subscribing:', stream.id());
            conference.subscribe(stream, function () {
              L.Logger.info('subscribed:', stream.id());
              displayStream(stream);
            }, function (err) {
              L.Logger.error(stream.id(), 'subscribe failed:', err);
            });
          }
        });
      }, function (err) {
        L.Logger.error('server connection failed:', err);
      });
    });
  };
}());
