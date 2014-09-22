var serverUrl = '/';
var localStream, client;
var allStreams = {};
var errHandler;

// ooVoo: custom message spec
// request format
// {
//   "payload": {
//     "id": "158",
//     "SourceID": "4",
//     "TargetID": "3",
//     "Buffer": "test567, incall message"
//   },
//   "type": "message"
// }
// response format
// {
//   "id":"158",
//   "SourceID":"4",
//   "TargetID":"3",
//   "Buffer":"test567, incall message"
// }

function mkmessage(message) {
  var srcId = window.myClientId;
  var dstId = $("input#msg-receiver").val();
  return {
    "payload": {
      "id": "158",
      "SourceID": srcId,
      "TargetID": dstId,
      "Buffer": message
      },
    "type": "message"
  };
}

function oovooInfo (uid, callback) {
  $.ajax({
    url: '/oovooid/'+uid,
    type: 'GET'
  }).done(function (res) {
    if (res === 'error') {
      callback('error');
      return;
    }
    callback(null, res);
  }).fail(function (res) {
    callback('error', res);
  });
}

function getOneStream() {
  for (var i in allStreams) {
    if (allStreams[i].getId() !== localStream.getId()) {
      return allStreams[i];
    }
  }
  return undefined;
}

var _ = function (handle) {
  var __ = {
    unpublish: function () {
      client.unpublish(localStream, function (err) {
        L.Logger.error(err);
      });
    },
    unsubscribe: function () {
      for (var j in allStreams) {
        if (j !== 'local' && allStreams[j]) {
          client.unsubscribe(allStreams[j], function (err) {
            L.Logger.error(err);
          });
        }
      }
    },
    sendmessage: function () {
      if (typeof window.myClientId === 'string' && window.myClientId !== '') {
        client.send(mkmessage('test incall message'), function () {
          L.Logger.info('sendmessage: success');
        }, function (err) {
          L.Logger.error('sendmessage:', err);
        });
      } else {
        L.Logger.error('My Client Id not ready.');
      }
    },
    broadcast: function () {
      if (typeof window.myClientId === 'string' && window.myClientId !== '') {
        client.send({
          "payload": {
            "id": "158",
            "SourceID": window.myClientId,
            "TargetID": "0",
            "Buffer": "Broadcasting @ "+new Date()
            },
          "type": "message"
        }, function () {
          L.Logger.info('broadcast: success');
        }, function (err) {
          L.Logger.error('broadcast:', err);
        });
      } else {
        L.Logger.error('My Client Id not ready.');
      }
    },
    publish: function () {
      client.publish(localStream, function (err) {
        $(function(){
          var notice = new PNotify({
            title: 'publish failed',
            text: err,
            type: 'error',
            hide: false
          });
          notice.get().click(function() {
            notice.remove();
          });
        });
      });
    },
    subscribe: function () {
      for (var j in allStreams) {
        if (j !== 'local' && allStreams[j]) {
          client.subscribe(allStreams[j], function (err) {
            $(function(){
              var notice = new PNotify({
                title: 'subscribe failed',
                text: err,
                type: 'error',
                hide: false
              });
              notice.get().click(function() {
                notice.remove();
              });
            });
          });
        }
      }
    },
    connect: function () {
      client.connect(function(err) {
        L.Logger.error(err);
      });
    },
    disconnect: function () {
      client.disconnect();
    },
    stream_close: function () {
      localStream.close();
      localStream.hide();
    },
    stream_init: function () {
      localStream.init(errHandler);
    },
    stream_show: function () {
      localStream.show('localVideo');
    },
    stream_hide: function () {
      localStream.hide();
    },
    "video-out-on": function () {
      console.log('video-out-on')
      localStream.playVideo();
    },
    "video-out-off": function () {
      console.log('video-out-off')
      localStream.pauseVideo();
    },
    "audio-out-on": function () {
      console.log('audio-out-on')
      localStream.playAudio();
    },
    "audio-out-off": function () {
      console.log('audio-out-off')
      localStream.pauseAudio();
    },
    "video-in-on": function () {
      var lastRemtoeStream = getOneStream();
      if (lastRemtoeStream) {
        lastRemtoeStream.playVideo();
      }
    },
    "video-in-off": function () {
      var lastRemtoeStream = getOneStream();
      if (lastRemtoeStream) {
        lastRemtoeStream.pauseVideo();
      }
    },
    "audio-in-on": function () {
      var lastRemtoeStream = getOneStream();
      if (lastRemtoeStream) {
        lastRemtoeStream.playAudio();
      }
    },
    "audio-in-off": function () {
      var lastRemtoeStream = getOneStream();
      if (lastRemtoeStream) {
        lastRemtoeStream.pauseAudio();
      }
    }
  };
  return __[handle] || function(){ L.Logger.warning('no such handler:', handle); };
}

var chat_controller = function (event) {
  event.preventDefault();
  var id = event.currentTarget.id;
  _(id)();
};

function getParameterByName(name) {
  name = name.replace(/[\[]/, '\\\[').replace(/[\]]/, '\\\]');
  var regex = new RegExp('[\\?&]' + name + '=([^&#]*)'),
      results = regex.exec(location.search);
  return results == null ? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
}

function bindPlayer(stream) {
  stream.showing = false;
  stream.show = function (elementID, options) {
    if (stream.showing) {
      return;
    }
    options = options || {};
    stream.elementID = elementID;
    if (stream.hasVideo() || stream.hasScreen()) {
      // Draw on HTML
      if (elementID !== undefined) {
        stream.player = new Erizo.VideoPlayer({id: stream.getId(), stream: stream, elementID: elementID, options: options});
        stream.showing = true;
      }
    } else if (stream.hasAudio) {
      stream.player = new Erizo.AudioPlayer({id: stream.getId(), stream: stream, elementID: elementID, options: options});
      stream.showing = true;
    }
  };
  stream.stop = function () {
    if (stream.showing) {
      if (stream.player !== undefined) {
        stream.player.destroy();
        stream.showing = false;
      }
    }
  };
  stream.play = stream.show;
  stream.hide = stream.stop;
  stream.onRemove = stream.stop;
}


var makeAuth = function (targetId, callback) {
  var authMsg = {
    "payload": {
      "avs_ip": "shwde9302.ccr.corp.intel.com",
      "avs_port": "443",
      "app_id": "",
      "app_token": "",
      "Carrier": "",
      "ClientSubType": "",
      "ClientType": "0",
      "DeviceModel": "",
      "Manufacturer": "",
      "ParticipantInfo": "",
      "Conference_ID": "webrtc_test",
      "conf_key": "TEST_SESSION_1",
      "Os": "",
      "ParticipantID": client.socket.socket.sessionid,
      "SdkVersion": "",
      "CodecInfo": "vc=vp8;r=all;dv=0;ac=opus,g711"
    },
    "type": "login"
  };

  if (typeof targetId === 'string' && targetId !== '') {
    oovooInfo(targetId, function (err, resp) {
      if (err) {
        L.Logger.error(err);
        return;
      }
      authMsg.payload.avs_ip = resp.avs_ip;
      authMsg.payload.conf_key = resp.conf_id;
      callback(authMsg);
    });
  } else {
    callback(authMsg);
  }
};

function joinConference (gateway, targetId, gWidth, resolution) {
  var screen = getParameterByName('screen');
  var vWidth = gWidth-20-16;

  function setWidth() {
    var width = Object.keys(allStreams).length > 1 ? vWidth/2-20 : vWidth;
    var root = $('div#gVideo').children('div');
    root.width(width);
    root.height(width/4*3);
    root.children('div').width(width);
    root.children('div').height(width/4*3);
  }

  localStream = Erizo.Stream({audio: true, video: true, data: true, screen: screen});
  bindPlayer(localStream);

  if (typeof resolution === 'string') {
    resolution = resolution.toLowerCase();
    localStream.setVideo(resolution, [15, 60]);
  }

  var gateway_host = gateway || location.hostname;
  var isSecured = window.location.protocol === 'https:';
  if (isSecured) {
    gateway_host += ':443';
  } else {
    gateway_host += ':80';
  }

  client = Erizo.Client({
    username: 'foobar',
    role: 'presenter',
    host: gateway_host,
    secure: isSecured,
    type: 'oovoo'
  });

  client.on('client-connected', function (evt) {
    $(function(){
      new PNotify({
        title: 'Gateway connected',
        text: 'Trying logging on avs',
        type: 'success',
        delay: 5000
      });
    });
    if (targetId) L.Logger.info('targetId:', targetId);
    makeAuth(targetId, function (auth) {
      var inputAvsIp = $('#avs_ip').val();
      if (inputAvsIp !== '') {
        auth.payload.avs_ip = inputAvsIp;
      }
      client.send(auth, function (msg) {
        $(function(){
          new PNotify({
            title: 'Session Info',
            text: msg || 'UNKNOWN',
            type: 'info',
            delay: 5000
          });
        });
      });
    });
  });

  client.on('client-id', function (evt) {
    if (typeof client.ClientId === 'string' && client.ClientId !== '') {
      window.myClientId = client.ClientId;
      $(function(){
        new PNotify({
          title: 'My Client Id',
          text: client.ClientId,
          type: 'info',
          delay: 5000
        });
      });
    }
  });

  client.on('client-disconnected', function (evt) {
    $(function(){
      new PNotify({
        title: false,
        text: 'Gateway disconnected',
        type: 'notice',
        delay: 5000
      });
    });
    delete window.myClientId;
  });

  client.onMessage(function (evt) {
    $(function(){
      new PNotify({
        title: 'Message Received',
        text: evt.attr,
        type: 'info'
      });
    });
  });

  client.on('stream-published', function (evt) {
    var stream = evt.stream;
    $(function(){
      new PNotify({
        title: 'Stream published',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
  });

  client.on('stream-subscribed', function(evt) {
    var stream = evt.stream;
    $(function(){
      new PNotify({
        title: 'Stream subscribed',
        text: 'Stream Id: '+stream.getId(),
        type: 'success',
        delay: 5000
      });
    });
    allStreams[stream.getId()] = stream;
    if ($('div#gVideo #remoteVideo'+stream.getId()).length === 0) {
      $('div#gVideo').append('<div class="col-md-1 column vivid sample-video-elem"><div id="remoteVideo'+stream.getId()+'"></div></div>');
    }
    setWidth();
    stream.show('remoteVideo' + stream.getId());
    // Pause video playing because we're not sure if the remote stream will
    // send out video tracks immediately, otherwise the player will wait for the
    // first frame of the video track before it can play the audio track.
    stream.disableVideo();
  });

  client.on('video-hold', function (evt) {
    var stream = evt.stream;
    stream.disableVideo();
    $(function(){
      new PNotify({
        title: 'Video paused',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
  });

  client.on('video-ready', function (evt) {
    var stream = evt.stream;
    $(function(){
      new PNotify({
        title: 'Video playing',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
    stream.enableVideo();
  });

  client.on('audio-hold', function (evt) {
    var stream = evt.stream;
    stream.disableAudio();
    $(function(){
      new PNotify({
        title: 'Audio paused',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
  });

  client.on('audio-ready', function (evt) {
    var stream = evt.stream;
    $(function(){
      new PNotify({
        title: 'Audio playing',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
    stream.enableAudio();
  });

  client.on('video-on', function (evt) {
    L.Logger.info(evt);
    localStream.enableVideo();
  });

  client.on('video-off', function (evt) {
    L.Logger.info(evt);
    localStream.disableVideo();
  });

  client.on('audio-on', function (evt) {
    L.Logger.info(evt);
    localStream.enableAudio();
  });

  client.on('audio-off', function (evt) {
    L.Logger.info(evt);
    localStream.disableAudio();
  });

  client.on('stream-added', function (evt) {
    var stream = evt.stream;
    bindPlayer(stream);
    $(function(){
      new PNotify({
        title: 'Stream added',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
    client.subscribe(stream, function(err) {
      $(function(){
        var notice = new PNotify({
          title: 'subscribe failed',
          text: err,
          type: 'error',
          hide: false
        });
        notice.get().click(function() {
          notice.remove();
        });
      });
    });
  });

  client.on('stream-removed', function (evt) {
    var stream = evt.stream;
    $(function(){
      new PNotify({
        title: 'Stream removed',
        text: 'Stream Id: '+stream.getId(),
        type: 'info',
        delay: 5000
      });
    });
    delete allStreams[stream.getId()];
    if (stream.elementID !== undefined) {
      $('#'+stream.elementID).parent().remove();
    }
    setWidth();
  });

  client.on('client-joined', function (evt) {
    $(function(){
      new PNotify({
        title: 'Client joined',
        text: evt.user,
        type: 'info',
        delay: 5000
      });
    });
    $('input#msg-receiver').val(evt.user.userId); // store last joined userid
  });

  client.on('client-left', function (evt) {
    $(function(){
      new PNotify({
        title: 'Client left',
        text: evt.user,
        type: 'info',
        delay: 5000
      });
    });
  });

  client.connect(function(err) {
    $(function(){
      new PNotify({
        title: 'Connection ERROR',
        text: err,
        type: 'error',
        delay: 5000
      });
    });
  });

  localStream.on('access-accepted', function () {
    allStreams['local'] = localStream;
    $('div#gVideo #localVideo').parent().show();
    setWidth(vWidth);
    localStream.show('localVideo');
    (function publish () {
      if (typeof client.ClientId === 'string' && client.ClientId !== '') {
        client.publish(localStream, function(err) {
          $(function(){
            var notice = new PNotify({
              title: 'publish failed',
              text: err,
              type: 'error',
              hide: false
            });
            notice.get().click(function() {
              notice.remove();
            });
          });
        });
      } else {
        setTimeout(publish, 100);
      }
    })();
  });

  errHandler = function (err) {
    $(function() {
      new PNotify({
        title: 'Media Access Failed',
        text: err.msg,
        type: err.type
      })
    });
  }

  localStream.init(errHandler);
};
