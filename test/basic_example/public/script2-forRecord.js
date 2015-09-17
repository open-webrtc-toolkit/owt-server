var localStream;
var localStreams = {};
var client1RemoteStreams = [];

var conferenceTest;
var localstreamTest;
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
      console.log("ready");
      callback(req.responseText);
    }
  };
  req.open('POST', url, true);
  req.setRequestHeader('Content-Type', 'application/json');
  req.send(JSON.stringify(body));
}

function getRooms(callback) {
  var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'getRooms/';

  req.onreadystatechange = function() {
    if (req.readyState === 4) {
      if (req.status === 200) {
        callback(req.responseText);
      } else {
        console.log("++++++deubg problem response:" + req.responseText);
      }
    }
  };

  req.open('GET', url, false);
  req.send();
}

function updateRoom(room, options, callback) {
  var req = new XMLHttpRequest(),
    serverUrl = 'http://localhost:3001/',
    url = serverUrl + 'updateRoom/',
    body = {
      room: room,
      options: options
    };

  req.onreadystatechange = function() {
    if (req.readyState === 4) {
      if (req.status === 200) {
        callback(req.responseText);
      } else {
        console.log("++++++deubg problem response:" + req.responseText);
      }
    }
  };
  req.open('POST', url, false);
  req.setRequestHeader('Content-Type', 'application/json');
  req.send(JSON.stringify(body));
}

function displayStream(stream) {
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
  if (window.navigator.appVersion.indexOf('Trident') < 0) {
    stream.show('test' + streamId);
  } else {
    L.Logger.info('displayStream:', stream.id());
    var canvas = document.createElement('canvas');
    if (stream instanceof Woogeen.RemoteStream && stream.isMixed()) {
      canvas.width = 640;
      canvas.height = 480;
    } else {
      canvas.width = 320;
      canvas.height = 240;
    }
    canvas.setAttribute('autoplay', 'autoplay::autoplay');
    div.appendChild(canvas);
    var ieStream = new ieMediaStream(stream.mediaStream.label);
    attachRemoteMediaStream(canvas, ieStream, stream.pcid);
  }
}

function shareScreen(conference) {
   conference.shareScreen({
    resolution: 'hd720p'
  }, function(stream) {
    document.getElementById('myScreen').setAttribute('style', 'width:320px; height: 240px;');
    if(window.navigator.appVersion.indexOf("Trident") < 0){
      stream.show('myScreen');
    }
    if(window.navigator.appVersion.indexOf("Trident") > -1){
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
}

function userPublish(conferenceClient, room, user, role, resolution, codec, mediaUrl) {
  console.log("Connect to room:", room);
  createToken(room, user, role, function(response) {
    var token = response;
    console.log("Token is:", token);
    conferenceClient.join(token, function(resp) {
      console.log("Join room succeed");
      if (typeof mediaUrl === 'string' && mediaUrl !== '') {
        Woogeen.LocalStream.create({
          video: true,
          audio: true,
          url: mediaUrl
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          conferenceClient.publish(localStream, {}, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      } else {
        console.log('localStream resolution is', resolution);
        Woogeen.LocalStream.create({
          video: {
            device: 'camera',
            resolution: resolution,
            framerate: [30, 500]
          },
          audio: true
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          console.log('localStream is true');
          conferenceClient.publish(localStream, {
            maxVideoBW: 300,
            videoCodec: codec
          }, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      }
    });
  });
}

function stopRecorder(client, id) {
  client.stopRecorder({
    recorderId: id
  }, function(info) {
    console.log("stopRecorder succeed for case:", id);
    console.log("recording stop succesful", info);
  }, function(err) {
    console.log("stopRecorder failed for case:", id);
    console.log("recording stop error", err);
  });
}

function singleRecordTemplate(client, streamId, recorderId, recordTime) {
  client.startRecorder({
    streamId: streamId,
    recorderId: recorderId
  }, function(info) {
    console.log("startRecorder succeed for case:", recorderId);
    console.log("recording successful ", info);
    setTimeout(stopRecorder, recordTime, client, recorderId);
  }, function(err) {
    console.log("startRecorder failed for case:", recorderId);
    console.log("recording error", err);
  });
}

function hybridRecordTemplate(client, firstStreamId, secondStreamId, recorderId, recordTime) {
  client.startRecorder({
    streamId: firstStreamId,
    recorderId: recorderId
  }, function(info) {
    console.log("startRecorder succeed for case:", recorderId);
    console.log("recording successful ", info);
    setTimeout(singleRecordTemplate, recordTime, client, secondStreamId, recorderId, recordTime);
  }, function(err) {
    console.log("startRecorder failed for case:", recorderId);
    console.log("recording error", err);
  });
}
/*
//close loscalStream and disconnect server
function closeLocalStream(stream,client) {
  if (localStream) {
    localStream.close();
    if (localStream.channel && typeof localStream.channel.close === 'function') {
      localStream.channel.close();
    }
  }
  client.leave();
}
*/

//get localStream of client
function getLocalStream (client) {
  var stream;
  for(i in client.localStreams){
    if (client.localStreams.hasOwnProperty(i)) {     
      if (client.localStreams[i].id().indexOf("_SCREEN_") < 0) {
        stream = client.localStreams[i];
        console.log('localStream id is ', stream.id());
      };    
    };   
  };
  console.log('*******************localStream  is ',stream);
  return stream;
}

//close loscalStream and disconnect server
function closeLocalStream(streamlocal,client) {
  var stream = getLocalStream(client);

  stream.close();
  if (stream.channel && typeof stream.channel.close === 'function') {
    stream.channel.close();
  }
  client.leave();
}

//unpublish LocalStream of client
function unpublishLocalStream (client){
  var clientId = client.myId;
  var stream = localStreams[clientId];
    console.log('unpulish stream is ', stream);
      client.unpublish(stream,function(){
        console.log("----------------unpublish localStream successful", stream.id());
      },function(err){
        console.log("----------------unpublish localStream ERROR",err);
    });
}
//publish localStream of client
function publishLocalStream (client) {
  var clientId = client.myId;
  var stream = localStreams[clientId];
  client.publish(stream,{
    maxVideoBW: 300
  },function(){
    console.log("++++++++++++++++publish localStream successful", stream.id());
  },function(err){
    console.log("++++++++++++++++publish localStream ERROR",err);
  });
}

function subscribeStream (client, x) {
  var remoteStream = client1RemoteStreams[x];
  client.subscribe(remoteStream,function () {
    console.log('+++++++++++++++++++subscribe stream is ',remoteStream.id());
  },function (err) {
    console.log('subscribe stream failed',err);
  });
}

function unsubscribeStream (client, x) {
  var remoteStream = client1RemoteStreams[x];
  client.unsubscribe(remoteStream,function () {
    console.log('-------------------unsubscribe stream is ',remoteStream.id());
  },function (err) {
    console.log('unsubscribe stream failed',err);
  });
}
//monitor client existed
function monitorClient (client) {
  if (client) {
    console.log('client is ',client);
  }else{
    console.log('There is no client');
  };
}

//get stream id and start recording MCUCaseList
function vgaInputTest(client, room, duration, MCUCases, MCUNums) {
  var mixStreamId, forwardStreamId;

  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  }
  console.log('mixStreamId is',mixStreamId,'forwardStreamId is ',forwardStreamId);


  console.log(MCUCases[0]);
  //start recording mixStream
  singleRecordTemplate(client, mixStreamId, MCUNums[0], duration);

  console.log(MCUCases[1])
  //start recording forwardStream
  singleRecordTemplate(client, forwardStreamId, MCUNums[1], duration);

  console.log(MCUCases[2])
  //start recording mixstream ten secs and then record forwardStream ten secs
  setTimeout(hybridRecordTemplate, 25000, client, mixStreamId, forwardStreamId, MCUNums[2], duration);

  console.log(MCUCases[3])
  //start recording forwardStream ten secs and then record mixStream ten secs
  setTimeout(hybridRecordTemplate, 55000, client, forwardStreamId, mixStreamId, MCUNums[3], duration);

  //monitor client existed
  setTimeout(monitorClient,56000,client);
}


function record(myRoom, resolutionInput, resolutionOutput, recordDuration, MCUCases, MCUNums,client1,client2, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", client1.remoteStreams);

  //client1 join myRoom and create localStream
  setTimeout(userPublish, 1, client1, myRoom, 'user1', 'presenter', resolutionInput, codec);

  //client2 join myRoom and create localStream
  setTimeout(userPublish, 1000, client2, myRoom, 'user2', 'presenter', resolutionInput, codec);

  //start recording in client1
  setTimeout(vgaInputTest, 7000, client1, myRoom, recordDuration, MCUCases, MCUNums);

  //client leave after 90 secs
  setTimeout(closeLocalStream, 90000, localStream, client1);

  //client2 will leave room when client1 call leave method.
  //setTimeout(closeLocalStream, 90000, localStream, client2);

  console.log('record has done ************************************');

  client1.on('server-disconnected', function() {
   console.log('client1 has been disconnect server');
  });
  client2.on('server-disconnected', function() {
   console.log('client2 has been disconnect server');
  });


}

//------------------------------------------------------------------------------------------------------------
//get stream id and start recording MCUCasesFor4User
function vgaInputTest1(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId, forwardStreamId;
  if (client) {
    console.log('client is ',client);
  };
  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  }
  console.log('mixStreamId is ',mixStreamId);
  console.log(MCUCase);
  singleRecordTemplate(client, mixStreamId, MCUNum, duration);

}

function record1(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", clients[0].remoteStreams);

  //client1,client2,client3,and client4 join myRoom and creat localStream
  for (var i = 0; i < clients.length; i++) {
    setTimeout(userPublish, i*1000, clients[i], myRoom,  'user'+i, 'presenter', resolutionInput, codec);
  };

  //start recording in client1
  setTimeout(vgaInputTest1, 7000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);

  /*client1,client2,client3,and client4 leave myRoom
(目前存在bug，一个client调用leave方法，会导致其他所有的client都离开，为避免console的麻烦，这里调用一次leave)*/
   setTimeout(closeLocalStream, 25000, localStream, clients[0]);
  for (var i = 0; i < clients.length; i++) {
    clients[i].on('server-disconnected', function() {
      console.log('client'+(i+1)+' has been disconnect server');
    });
  };

  console.log('recod1 has done ************************************');
}


//-------------------------------------------------------------------------------------------------------------
//vagInputTest2 and record2 for recording MCUCasesForDeff4User
function vgaInputTest2(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId, forwardStreamId;
  if (client) {
    console.log('client is ',client);
  };
  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  }
  console.log('mixStreamId is ',mixStreamId);
  console.log(MCUCase);
  singleRecordTemplate(client, mixStreamId, MCUNum, duration);
}

function record2(myRoom, two_resolution, resolutionOutput, recordDuration, clients, MCUCase, MCUNum, codec) {
 /*目前调用同一个camera创建localstream，分辨率只能从大到小。对case并无影响*/
  var two_resolution = ['hd1080p', 'hd720p', 'vga', 'sif'];
 //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', two_resolution, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", clients[0].remoteStreams);
  console.log('client1 is ',clients[0]);

  //client1,client2,client3,client4 join myRoom and create localStream
  for (var j = 0; j < clients.length; j++) {
    setTimeout(userPublish, j*1000, clients[j], myRoom, 'user'+j, 'presenter',two_resolution[j] , codec);
  };

  if (MCUNum=='MCU-1224') {
    //for recording MCU-1224
    setTimeout(vgaInputTest3, 15000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);

    setTimeout(closeLocalStream, 60000, localStream, clients[0]);
    for (var i = 0; i < clients.length; i++) {
      clients[i].on('server-disconnected', function() {
        console.log('client'+(i+1)+' has been disconnect server');
      });
    };
  }else{
    //record in MCUCasesForDeff4User array
    setTimeout(vgaInputTest2, 15000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);
    setTimeout(closeLocalStream, 35000, localStream, clients[0]);
    for (var i = 0; i < clients.length; i++) {
      clients[i].on('server-disconnected', function() {
        console.log('client'+(i+1)+' has been disconnect server');
      });
    };
  };


  console.log('record2 has done **********************************************');
}


//---------------------------------------------------------------------------------------------------

function singleRecordTemplate3(client, streamId, recorderId) {
  client.startRecorder({
    streamId: streamId,
    recorderId: recorderId
  }, function(info) {
    console.log("startRecorder succeed for case:", recorderId);
    console.log("recording successful ", info);
  }, function(err) {
    console.log("startRecorder failed for case:", recorderId);
    console.log("recording error", err);
  });
}

function vgaInputTest3(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId;
  var forwardStreamIds = [];

  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else if (stream.isScreen())
        shareScreenStreamId = stream.id();
      else
        forwardStreamIds.push(stream.id());
    }
  }
  console.log('All of forwardStreamId are ',forwardStreamIds);

  console.log(MCUCase);
  //前三个视频调用不带stop的方法，并且每个视频录制10s
  for(x = 0; x<3; x++){
    setTimeout(singleRecordTemplate3, x*10000,client, forwardStreamIds[x], MCUNum, duration);
  }
  //最后一个视频调用带有stop的方法
  setTimeout(singleRecordTemplate, 3*10000,client, forwardStreamIds[3], MCUNum, duration);
  //monitor client existed
  setTimeout(monitorClient,31000,client);
}

function record3(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", clients[0].remoteStreams);
  console.log('clients[0] is ',clients[0]);

  //client1,client2,client3 and client4 join and create localStream
  for (var i = 0; i < clients.length; i++) {
    setTimeout(userPublish, i*1000, clients[i], myRoom, 'user'+i, 'presenter', resolutionInput, codec);
  };

  //start recording in client1
  setTimeout(vgaInputTest3, 15000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);

  //client1,client2,client3 and client4 leave myRoom
  setTimeout(closeLocalStream, 65000, localStream, clients[0]);
  for (var j = 0; j < clients.length; j++) {
    clients[j].on('server-disconnected', function() {
     console.log('client'+(j+1)+' has been disconnect server');
    });
  };

  console.log('record3 has done *********************************');
}

//--------------------------------------------------------------------------------------------------

//record three streams in one file
function record3Hybrid (streamId1,streamId2,streamId3,client,MCUNum,recordTime) {
  setTimeout(singleRecordTemplate3, 0, client, streamId1, MCUNum, recordTime);
  setTimeout(singleRecordTemplate3, 1*10000,client, streamId2, MCUNum, recordTime);
  setTimeout(singleRecordTemplate, 2*10000,client, streamId3, MCUNum, recordTime);
}

//record three streams in difference file at the same time
function recordSameTime (streamId1,streamId2,streamId3,client,MCUNum,recordTime) {
  if (streamId1) {
    singleRecordTemplate(client, streamId1, MCUNum+'-1', recordTime);
  };

    if (streamId2) {
    singleRecordTemplate(client, streamId2, MCUNum+'-2', recordTime);
  };

    if (streamId3) {
    singleRecordTemplate(client, streamId3, MCUNum+'-3', recordTime);
  };
}


function vgaInputTest4(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId;
  var forwardStreamId;

  //get mixStream shareScreen and forwardStream id
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else if (stream.isScreen())
        shareScreenStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  };

  //数组MCUCase中第六个scenario
  if (MCUNum[5]) {
    if (MCUNum[5]=='MCU-1140') {
      console.log(MCUNum[5]);
      setTimeout(recordSameTime,160000,mixStreamId,forwardStreamId,null,client,MCUNum[5],duration);
    };
    if (MCUNum[5]=='MCU-1139') {
      console.log(MCUNum[5]);
      setTimeout(recordSameTime,160000,forwardStreamId,shareScreenStreamId,null,client,MCUNum[5],duration);
    };
    if (MCUNum[5]=='MCU-1138') {
      console.log(MCUNum[5]);
      setTimeout(recordSameTime,160000,mixStreamId,forwardStreamId,shareScreenStreamId,client,MCUNum[5],duration);
    };
  };

  //数组MCUCase中第五个scenario
  if (MCUNum[4]=='MCU-1274') {
    console.log(MCUCase[4]);
    setTimeout(record3Hybrid,120000,shareScreenStreamId,forwardStreamId,mixStreamId,client,MCUNum[4],duration);
  };

  if (MCUNum[4]=='MCU-1276') {
    console.log(MCUCase[4]);
    setTimeout(record3Hybrid,120000,forwardStreamId,shareScreenStreamId,mixStreamId,client,MCUNum[4],duration);
  };

  if (MCUNum[4]=='MCU-1275') {
    console.log(MCUCase[4]);
    setTimeout(record3Hybrid,120000,mixStreamId,forwardStreamId,shareScreenStreamId,client,MCUNum[4],duration);
  };

  if (MCUNum[4]=='MCU-1135') {
    console.log(MCUCase[4]);
    setTimeout(singleRecordTemplate,120000,client, shareScreenStreamId, MCUNum[4], duration);
  };

  //数组MCUCase中有六个scenario，此为前4个
  console.log('All of forwardStreamId are ',forwardStreamId,'mixStreamId is ',mixStreamId,'shareScreen is ',shareScreenStreamId);
  console.log(MCUCase[0]);
  setTimeout(hybridRecordTemplate, 0, client, mixStreamId, shareScreenStreamId, MCUNum[0], duration);
  console.log(MCUCase[1]);
  setTimeout(hybridRecordTemplate,30000,client,forwardStreamId,shareScreenStreamId,MCUNum[1],duration);
  console.log(MCUCase[2]);
  setTimeout(hybridRecordTemplate,60000,client,shareScreenStreamId,mixStreamId,MCUNum[2],duration);
  console.log(MCUCase[3]);
  setTimeout(hybridRecordTemplate,90000,client,shareScreenStreamId,forwardStreamId,MCUNum[3],duration);

  //monitor client existed
  setTimeout(monitorClient,161000,client);
}

function record4(myRoom, resolutionInput, resolutionOutput, recordDuration, client1, client2, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", client1.remoteStreams);
  console.log('client1 is ',client1);
  //client2 join myRoom and create localstream
  setTimeout(userPublish, 0, client2, myRoom, 'user1', 'presenter', resolutionInput, codec);
  //client1 join myRoom and create localstream
  setTimeout(userPublish, 1000, client1, myRoom, 'user2', 'presenter', resolutionInput, codec);
  //client2 sharescreen
  setTimeout(function(){
    shareScreen(client2);
  },2000);

  //start recording in client1
  setTimeout(vgaInputTest4, 25000, client1, myRoom, recordDuration, MCUCase, MCUNum);

  //client1 and client2 leave myRoom
  setTimeout(closeLocalStream, 200000, localStream, client1);

 // setTimeout(closeLocalStream, 200000, localStream, client2);
  client1.on('server-disconnected', function() {
    console.log('client1 has been disconnect server');
  });
  client2.on('server-disconnected', function() {
    console.log('client2 has been disconnect server');
  });

  console.log('record5 has done ********************************');
}

//------------------------------------------------------------------------------------------------------------------
//userPublish5 can create localSteam only video or only audio
function userPublish5(conferenceClient, room, user, role, resolution, codec, mediaUrl, flagvideo, flagaudio) {
  console.log("Connect to room:", room);
  createToken(room, user, role, function(response) {
    var token = response;
    console.log("Token is:", token);
    conferenceClient.join(token, function(resp) {
      console.log("Join room succeed");
      if (typeof mediaUrl === 'string' && mediaUrl !== '') {
        Woogeen.LocalStream.create({
          video: true,
          audio: true,
          url: mediaUrl
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          conferenceClient.publish(localStream, {}, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      } else {
        console.log('localStream resolution is', resolution);
        Woogeen.LocalStream.create({
          video: {
            device: 'camera',
            resolution: resolution,
            framerate: [30, 500]
          },
          audio: true
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          console.log('localStream is true');
          conferenceClient.publish(localStream, {
            video: flagvideo,
            audio: flagaudio,
            maxVideoBW: 300,
            videoCodec: codec
          }, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      }
    });
  });
}

function vgaInputTest5(client, room, duration, MCUCases, MCUNums) {
  var mixStreamId, forwardStreamId;

  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  }
  console.log('mixStreamId is',mixStreamId,'forwardStreamId is ',forwardStreamId);

  console.log(MCUCases[0]);
  singleRecordTemplate(client, forwardStreamId, MCUNums[0], duration);

  console.log(MCUCases[1])
  singleRecordTemplate(client, mixStreamId, MCUNums[1], duration);

}


function record5(myRoom, resolutionInput, resolutionOutput, recordDuration, MCUCases, MCUNums,client1,client2, flag, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", client1.remoteStreams);

  //client1 join myRoom and create localStream only video
  setTimeout(userPublish5, 1, client1, myRoom, 'user1', 'presenter', resolutionInput, codec, null, flag[0], flag[1]);
  //client2 join myRoom and create localStream only audio
  setTimeout(userPublish5, 1000, client2, myRoom, 'user2', 'presenter', resolutionInput, codec, null, flag[0], flag[1]);

  //start recording in client1
  setTimeout(vgaInputTest5, 12000, client1, myRoom, recordDuration, MCUCases, MCUNums);


  setTimeout(closeLocalStream, 30000, localStream, client1);
 // setTimeout(closeLocalStream, 30000, localStream, client2);
  console.log('record5 has done **********************************');


  client1.on('server-disconnected', function() {
    console.log('client1 has been disconnect server');
  });
  client2.on('server-disconnected', function() {
    console.log('client2 has been disconnect server');
  });
}
//----------------------------------------------------------------------------------------------------------------------------
function vgaInputTest6(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId;
  var forwardStreamIds=[];

  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else if (stream.isScreen())
        shareScreenStreamId = stream.id();
      else
        forwardStreamIds.push(stream.id());
    }
  };

  console.log('All of forwardStreamId are ',forwardStreamIds,'mixStreamId is ',mixStreamId,'shareScreen is ',shareScreenStreamId);
  console.log(MCUCase[0]);
  setTimeout(singleRecordTemplate3, 0, client, shareScreenStreamId, MCUNum[0]);
  console.log(MCUCase[1]);
  setTimeout(singleRecordTemplate3, 0, client, forwardStreamIds[0],MCUNum[1]);
  console.log(MCUCase[2]);
  setTimeout(singleRecordTemplate3, 0, client, mixStreamId,MCUNum[2]);

}

function record6(myRoom, resolutionInput, resolutionOutput, recordDuration, client1, client2, client3, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", client1.remoteStreams);
  console.log('client1 is ',client1);
  //client2 join myRoom and create localStream
  setTimeout(userPublish, 0, client2, myRoom,'user1' ,'presenter', resolutionInput, codec);
  //client1 join myRoom and create localStream
  setTimeout(userPublish, 1000, client1, myRoom,'user2' , 'presenter', resolutionInput, codec);
  //client3 join myRoom and create localStream
  setTimeout(userPublish, 2000, client3, myRoom,'user3' , 'presenter', resolutionInput, codec);
  //client2 shareScreen
  setTimeout(function(){
    shareScreen(client2);
  },2500);

  //start recording in client1
  setTimeout(vgaInputTest6, 15000, client1, myRoom, recordDuration, MCUCase, MCUNum);



  //client3 leave first
  setTimeout(closeLocalStream, 25000, localStream, client3);
  //client2 leave secend
  setTimeout(closeLocalStream, 35000, localStream, client2);
  //client1 leave last
  setTimeout(closeLocalStream, 45000, localStream, client1);

  client1.on('server-disconnected', function(event) {
    console.log('client1 has been disconnect server',event);
    client1.clearEventListener("server-disconnected");
  });
  client2.on('server-disconnected', function(event) {
    console.log('client2 has been disconnect server',event);
    client2.clearEventListener("server-disconnected");
  });
  client3.on('server-disconnected', function(event) {
    console.log('client3 has been disconnect server',event.user);
    client3.clearEventListener("server-disconnected");
  });

  console.log('record6 has done ***************************');
}

//-----------------------------------------------------------------------------------------------------------------

//增加获取与client对应的localStream，并将client.myId与localStream以键值形式存储到localStreams中，方便unpublish，publish方法的调用
function userPublish7(conferenceClient, room, user, role, resolution, codec, mediaUrl) {
  console.log("Connect to room:", room);
  createToken(room, user, role, function(response) {
    var token = response;
    console.log("Token is:", token);
    conferenceClient.join(token, function(resp) {
      console.log("Join room succeed");
      if (typeof mediaUrl === 'string' && mediaUrl !== '') {
        Woogeen.LocalStream.create({
          video: true,
          audio: true,
          url: mediaUrl
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;
          conferenceClient.publish(localStream, {}, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      } else {
        console.log('localStream resolution is', resolution);
        Woogeen.LocalStream.create({
          video: {
            device: 'camera',
            resolution: resolution,
            framerate: [30, 500]
          },
          audio: true
        }, function(err, stream) {
          if (err) {
            return L.Logger.error('create LocalStream failed:', err);
          }
          localStream = stream;

          var clientId = conferenceClient.myId;
          localStreams[clientId] = localStream;    
          console.log("localStreams is ", localStreams);

          console.log('localStream is true');
          conferenceClient.publish(localStream, {
            maxVideoBW: 300,
            videoCodec: codec
          }, function(st) {
            L.Logger.info('stream published:', st.id());
          }, function(err) {
            L.Logger.error('publish failed:', err);
          });
        });
      }
    });
  });
}

function vgaInputTest7(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId;
  var forwardStreamId;

  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else if (stream.isScreen())
        shareScreenStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  };

  console.log('All of forwardStreamId are ',forwardStreamId,'mixStreamId is ',mixStreamId,'shareScreen is ',shareScreenStreamId);
  console.log(MCUCase[0]);
  setTimeout(singleRecordTemplate3, 0, client, forwardStreamId, MCUNum[0]);
  console.log(MCUCase[1]);
  setTimeout(singleRecordTemplate, 0, client, mixStreamId, MCUNum[1], 35000);
  console.log(MCUCase[2]);
  setTimeout(singleRecordTemplate, 40000, client, mixStreamId,MCUNum[2], 25000);
  console.log(MCUCase[3]);
  setTimeout(singleRecordTemplate, 70000, client, mixStreamId,MCUNum[3], 30000);
}

function record7(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCase, MCUNum, codec) {

  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", clients[0].remoteStreams);
  console.log('client1 is ',clients[0]);

  //client1 client2 and client3 join myRoom and create localStream
  for (var i = 0; i < 3; i++) {
    setTimeout(userPublish7, i*1000, clients[i], myRoom,'user'+i ,'presenter', resolutionInput, codec);
  };

  //start recording in client1
  setTimeout(vgaInputTest7, 12000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);
  
  //client2 call unpulish method after recording ten secs
  setTimeout(unpublishLocalStream, 22000, clients[2]);

  //client3 call unpulish method after recording 5 secs
  setTimeout(unpublishLocalStream, 27000, clients[1]);

  //client1 call unpulish method after recording 5 secs
  setTimeout(unpublishLocalStream, 32000, clients[0]);


  //publish localStream of client1 
  setTimeout(publishLocalStream, 55000, clients[0]);
  //publish localStream of client2 
  setTimeout(publishLocalStream, 60000, clients[1]);
  //publish localStream of client3 
  setTimeout(publishLocalStream, 65000, clients[2]);



 setTimeout(unpublishLocalStream, 85000, clients[0]);

  setTimeout(publishLocalStream, 90000, clients[0]);

  setTimeout(unpublishLocalStream, 95000, clients[2]);

  setTimeout(unpublishLocalStream, 100000, clients[1]);

  setTimeout(publishLocalStream, 105000, clients[2]);

  setTimeout(publishLocalStream, 110000, clients[1]);

  //client1 ,client2 and client3 leave myRoom
  setTimeout(closeLocalStream, 120000, localStream, clients[0]);
  // setTimeout(closeLocalStream, 100000, localStream, client2);
  // setTimeout(closeLocalStream, 100000, localStream, client3);


  clients[0].on('server-disconnected', function(event) {
    console.log('client1 has been disconnect server',event);
    clients[0].clearEventListener("server-disconnected");
  });
  clients[1].on('server-disconnected', function(event) {
    console.log('client2 has been disconnect server',event);
    clients[1].clearEventListener("server-disconnected");
  });
  clients[2].on('server-disconnected', function(event) {
    console.log('client3 has been disconnect server',event);
    clients[2].clearEventListener("server-disconnected");
  });

  console.log('record7 has done ******************************');
}

//--------------------------------------------------------------------------------------------------------------
function vgaInputTest8(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId;
  var forwardStreamIds;

  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed()){
        mixStreamId = stream.id();
	//mixStream作为远程流存储到client1RemoteStreams
        client1RemoteStreams.push(stream);
      }else if (stream.isScreen()){
        shareScreenStreamId = stream.id();
      }else{
        forwardStreamId = stream.id();
	/*将clien1端的除了shareScreen的远程流存储到client1RemoteStreams中，循环结束client1RemoteStreams中有4个流，
	0角标对应mixStream，2，3角标对应client2和client3的forwardStream*/
        client1RemoteStreams.push(stream);
      }
    }
  };

  console.log('All of forwardStreamId are ',forwardStreamId,'mixStreamId is ',mixStreamId,'shareScreen is ',shareScreenStreamId ,'client1RemoteStreams is ',client1RemoteStreams);
  console.log(MCUCase[0]);
  setTimeout(singleRecordTemplate, 0, client, forwardStreamId, MCUNum[0], 100000);
  console.log(MCUCase[1]);
  setTimeout(singleRecordTemplate, 0, client, mixStreamId, MCUNum[1], 20000);
  console.log(MCUCase[2]);
  setTimeout(singleRecordTemplate, 40000, client, mixStreamId,MCUNum[2], 40000);
  console.log(MCUCase[3]);
  setTimeout(singleRecordTemplate, 100000, client, mixStreamId,MCUNum[3], 40000);
}
/*
房间中3个user，user1在录制其中一个forward stream ，然后unsubscribe这个forward stream，mix stream视频的过程中，
*/
function record8(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", clients[0].remoteStreams);
  console.log('client1 is ',clients[0]);

  //client1 client2 and client3 join myRoom and create localStream
  for (var i = 0; i < 3; i++) {
    setTimeout(userPublish, i*1000, clients[i], myRoom,'user'+i ,'presenter', resolutionInput, codec);
  };

  //start recording in client1
  setTimeout(vgaInputTest8, 12000, clients[0], myRoom, recordDuration, MCUCase, MCUNum);


  //client2 call unsubscribe method after recording ten secs
  setTimeout(unsubscribeStream, 22000, clients[0], 3);


  //client3 call unsubscribe method after recording 5 secs
  setTimeout(unsubscribeStream, 27000, clients[0], 2);




  //publish localStream of client1 
  setTimeout(unsubscribeStream, 70000, clients[0], 0);
  //publish localStream of client2 
  setTimeout(subscribeStream, 75000, clients[0], 0);
  //publish localStream of client3 
  setTimeout(unsubscribeStream, 85000, clients[0], 0);

  setTimeout(subscribeStream, 90000, clients[0], 0);




  setTimeout(subscribeStream, 110000, clients[0], 2);

  setTimeout(subscribeStream, 115000, clients[0], 3);

  setTimeout(unsubscribeStream, 120000, clients[0], 3);

  setTimeout(unsubscribeStream, 125000, clients[0], 2);

  setTimeout(subscribeStream, 130000, clients[0], 3);

  setTimeout(unsubscribeStream, 135000, clients[0], 3);

  setTimeout(subscribeStream, 140000, clients[0], 3);
  //client1 ,client2 and client3 leave myRoom
  setTimeout(closeLocalStream, 160000, localStream, clients[0]);
  // setTimeout(closeLocalStream, 100000, localStream, client2);
  // setTimeout(closeLocalStream, 100000, localStream, client3);


  clients[0].on('server-disconnected', function(event) {
    console.log('client1 has been disconnect server',event);
    clients[0].clearEventListener("server-disconnected");
  });
  clients[1].on('server-disconnected', function(event) {
    console.log('client2 has been disconnect server',event);
    clients[1].clearEventListener("server-disconnected");
  });
  clients[2].on('server-disconnected', function(event) {
    console.log('client3 has been disconnect server',event);
    clients[2].clearEventListener("server-disconnected");
  });

  console.log('record8 has done ******************************');
}
//-----------------------------------------------------------------------------------------------

function vgaInputTest9(client, room, duration, MCUCases, MCUNums) {
  var mixStreamId, forwardStreamId;

  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else
        forwardStreamId = stream.id();
    }
  }
  console.log('mixStreamId is',mixStreamId,'forwardStreamId is ',forwardStreamId);


  console.log(MCUCases);
  //start recording mixStream
  singleRecordTemplate(client, mixStreamId, MCUNums, duration);

}

/*record9方法,这个方法模拟一个正常的会议。
  思路：
	1,在会议中有4个人中的基础上随机的再加入0-12个人。每次运行此方法myRoom中随机有4-16个人存在。
	2,client1在所有client都加入后，开始录制mixStream，在会议中，随机的出现人员的退出，或者加入，目前leave方法存在bug，都以unpublish来操作
	3,在会议中人员的退出或者加入也是随机的。
*/
function record9(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCases, MCUNums, codec) {
  //随机产生一个0-12之间的正整数
  var x = Math.floor(Math.random()*13);

 //录制视频持续时间
  var timeDuration = parseInt(document.getElementById('timeDuration').value);

  var minute = 60000;

  //创建随机产生的‘x’个client，并添加到clients数组中， 
  for (var i = 0; i < x; i++) {
    clients.push(Woogeen.ConferenceClient.create({}));
  };
  console.log('amount of users is ', clients.length);
  //create arr and length = x+4
  var arr = [];
  //创建一个和clients数组等长的角标数组
  for (var j = 0; j < x+4; j++) {
    arr[j] = j;
  };
  //对arr进行随机的排序，
  arr.sort(function  () {
    return 0.5 - Math.random()
  });
  console.log('arr is ',arr);
  //对arr进行随机的截取，此时arr中的元素就作为clients中的角标。
  arr = arr.slice(Math.floor(Math.random()*arr.length), arr.length);
  console.log('arr is ',arr);
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });


  console.log("remote streams are:", clients[0].remoteStreams);
  //client数组中的用户都加入同一个房间，并且在创建本地流成功后，记录client.myId与自身的localstream的对应关系，方便publish，unpublish的调用。
  for (var p = 0; p < clients.length; p++) {
    setTimeout(userPublish7, p*1000, clients[p], myRoom, 'user'+p, 'presenter', resolutionInput, codec);
  };

  //保证所有的用户都加入房间，并在publish成功后，client1开始录制mixStream视频。
  setTimeout(vgaInputTest9, clients.length*1000+7000, clients[0], myRoom, timeDuration*minute, MCUCases, MCUNums);

  //保证视频正常录制开始后，在录制的过程中，‘随机的人员’在视频开录后的‘随机的时间’离开‘随机时间’。比如：‘用户2’在视频开录后的‘5分钟’离开‘20分钟’。
  setTimeout(function  () {
  for (var y = 0; y < arr.length; y++) {
    //arr[y]作为clients的角标，获取会议中随机离开的人员。
    if (clients[arr[y]] != undefined) {

	      //获取y代表的用户开始unpublish的时间。
        var z = Math.floor(Math.random()*timeDuration);
        setTimeout(unpublishLocalStream, z*minute, clients[arr[y]]);
	      //获取y代表的用户开始publish的时间就是(z+o)，并且(z+o)只有在小于录制时间时publish才有意义。
        var o = Math.floor(Math.random()*(timeDuration / 3));
        if ((z+o) < timeDuration) {
	        //为保证publish在unpublish之后至少1分钟执行。
          setTimeout(publishLocalStream, (z+1)*minute+o, clients[arr[y]]);
        };

    };
    console.log('user=', arr[y], 'arr.length=', arr.length, 'z = ', z, 'o =', o, 'timeDuration =', timeDuration);
  };
      
  },clients.length*1000+7000+1000);

  setTimeout(closeLocalStream, (timeDuration+1)*minute, localStream, clients[0]);


  console.log('record9 has done ************************************');

  for (var n = 0; n < clients.length; n++) {
    clients[n].on('server-disconnected', function() {
      console.log('client'+(n+1)+' has been disconnect server');
    });
  };



}
//-------------------------------------------------------------------------------------------------------------
function vgaInputTest10(client, room, duration, MCUCases, MCUNums) {
  var mixStreamId, forwardStreamId;
  var forwardStreamIds = [];
  /*Get mix and forward stream id*/
  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      forwardStreamIds.push(stream.id());
    }
  }
  console.log('all of remoteStreams id is ' , forwardStreamIds);

  forwardStreamIds.sort(function () {
    return 0.5 - Math.random()
  });

  console.log('all of remoteStreams id after randomly sort is ' , forwardStreamIds);
  //start recording 
  if(MCUNums === 'MCU-1284'){
    console.log(MCUCases);
    for (var j = 0; j < forwardStreamIds.length; j++) {
      if (j < (forwardStreamIds.length - 1)) {
        setTimeout(singleRecordTemplate3, j*10000 , client, forwardStreamIds[j], MCUNums);
      }else{
        setTimeout(singleRecordTemplate, j*10000 , client, forwardStreamIds[j], MCUNums, duration);
      };
      
    };
  }
 
  if(MCUNums === 'MCU-1283'){
    console.log(MCUCases);
    for (var k = 0; k < forwardStreamIds.length; k++) {
      singleRecordTemplate(client, forwardStreamIds[k], MCUNums+'-'+k, 30000);
    };
  }
}

/*
record10方法,这个方法模拟一个16个人的正常会议。
1 房间中有16个人，其中3个用户shareScreen，client1用户按照随机的顺序，录制所有的远程流到同一个文件中。
2 和1同样的条件下，client1录制所有的远程流在各自的文件。
*/
function record10(myRoom, resolutionInput, resolutionOutput, recordDuration, clients, MCUCases, MCUNums, codec) {

  for (var i = 0; i < 12; i++) {
    clients.push(Woogeen.ConferenceClient.create({}));
  };

  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });


  console.log("remote streams are:", clients[0].remoteStreams);

  for (var p = 0; p < clients.length; p++) {
    setTimeout(userPublish, p*1000, clients[p], myRoom, 'user'+p, 'presenter', resolutionInput, codec);
  };
  setTimeout(shareScreen, 3*1000, clients[2]);
  setTimeout(shareScreen, 3*1000, clients[1]);
  setTimeout(shareScreen, 3*1000, clients[8]);
  //保证所有的用户都加入房间，并在publish成功后，开始录制
  setTimeout(vgaInputTest10, 16*1000+15000, clients[0], myRoom, recordDuration, MCUCases, MCUNums);


  setTimeout(closeLocalStream, 300000, localStream, clients[0]);


  console.log('record10 has done ************************************');

  for (var n = 0; n < clients.length; n++) {
    clients[n].on('server-disconnected', function() {
      console.log('client'+(n+1)+' has been disconnect server');
    });
  };
}
//-----------------------------------------------------------------------------------------------------------

function vgaInputTest11(client, room, duration, MCUCase, MCUNum) {
  var mixStreamId , shareScreenStreamId , shareScreen;
  var forwardStreamIds=[];

  for (var i in client.remoteStreams) {
    if (client.remoteStreams.hasOwnProperty(i)) {
      var stream = client.remoteStreams[i];
      if (stream.isMixed())
        mixStreamId = stream.id();
      else if (stream.isScreen()){
        shareScreenStreamId = stream.id();
        shareScreen = stream;
      }else
        forwardStreamIds.push(stream.id());
    }
  };

  console.log('All of forwardStreamId are ',forwardStreamIds,'mixStreamId is ',mixStreamId,'shareScreen is ',shareScreenStreamId);
  console.log(MCUCase);
  setTimeout(singleRecordTemplate3, 0, client, shareScreenStreamId, MCUNum);
  for (var j = 0; j < 6; j++) {
    setTimeout(function () {
      client.unsubscribe(shareScreen,function () {
        console.log('unsubscribe shareScreen sucessful ',shareScreen.id());
      },function (err) {
        console.log('unsubscribe shareScreen failed ',err);
      })
    },j*10000+5000);

    setTimeout(function () {
      client.subscribe(shareScreen,function () {
        console.log('subscribe shareScreen sucessful ',shareScreen.id());
      },function (err) {
        console.log('subscribe shareScreen failed ',err);
      })
    },j*10000+10000);
  };
}
/*
三个用户，user2分享视频，user1录制shareScreen，在录制的过程中user1频繁的unsubscribe/subscribe shareSreen。
*/
function record11(myRoom, resolutionInput, resolutionOutput, recordDuration, client1, client2, client3, MCUCase, MCUNum, codec) {
  //update output resolution
  updateRoom(myRoom, {
    'mediaMixing': {
      'video': {
        'resolution': resolutionOutput
      }
    }
  }, function(req) {
    upRoom = JSON.parse(req);
    console.log('room is updated', upRoom);
    console.log('input resolution is ', resolutionInput, '-----output resolution is ', resolutionOutput);
  });

  console.log("remote streams are:", client1.remoteStreams);
  console.log('client1 is ',client1);
  //client2 join myRoom and create localStream
  setTimeout(userPublish, 0, client2, myRoom,'user1' ,'presenter', resolutionInput, codec);
  //client1 join myRoom and create localStream
  setTimeout(userPublish, 1000, client1, myRoom,'user2' , 'presenter', resolutionInput, codec);
  //client3 join myRoom and create localStream
  setTimeout(userPublish, 2000, client3, myRoom,'user3' , 'presenter', resolutionInput, codec);
  //client2 shareScreen
  setTimeout(function(){
    shareScreen(client2);
  },2500);

  //start recording in client1
  setTimeout(vgaInputTest11, 15000, client1, myRoom, recordDuration, MCUCase, MCUNum);


  setTimeout(closeLocalStream, 90000, localStream, client1);
  
  client1.on('server-disconnected', function(event) {
    console.log('client1 has been disconnect server',event);
    client1.clearEventListener("server-disconnected");
  });
  client2.on('server-disconnected', function(event) {
    console.log('client2 has been disconnect server',event);
    client2.clearEventListener("server-disconnected");
  });
  client3.on('server-disconnected', function(event) {
    console.log('client3 has been disconnect server',event.user);
    client3.clearEventListener("server-disconnected");
  });

  console.log('record11 has done ***************************');
}


/*用于自动化测试不同类型的scenario*/
function automatedTest (type) {
  L.Logger.setLogLevel(L.Logger.INFO);
  var rooms, myRoom;
  var recordDuration = 10000;

  getRooms(function(_room) {
    rooms = JSON.parse(_room);
    myRoom = rooms[0]._id;
    console.log("room id is:", myRoom);
  });

  //case中常用的resolution
  var resolutions = ['hd1080p', 'hd720p', 'vga', 'sif'];
  //记录record函数调用次数
  var conunt = 0;

  var client1 = Woogeen.ConferenceClient.create({});
  var client2 = Woogeen.ConferenceClient.create({});
  var client3 = Woogeen.ConferenceClient.create({});
  var client4 = Woogeen.ConferenceClient.create({});
  var clients = [client1,client2,client3,client4];

  //  scenario in testlink (record方法的参数，二维数组，共64个scenario，其type='testBasic')
  var MCUCaseList = [
    ['Case-1187 Record mix stream-001 with the same input/output resolution(input:240p,output:240p)', 'Case-1129 Record forward stream with different input resolution (240p)', 'Case-1127 Record hybrid: mix stream first and then record forward stream in one file(input:240p,output:240p)', 'Case-1240 Record hybrid: Forward stream first and then record mix stream in one file(input:240p,output:240p)'],
    ['Case-1191 Record mix stream combining different input/output resolution-001(input:240p,output:480p)', 'Case-1305 Record forward stream combining different input/output resolution-001(input:240p,output:480p)', 'Case-1228 Record hybrid: mix stream first and then record forward stream in one file(input:240p,output:480p)', 'Case-1245 Record hybrid: Forward stream first and then record mix stream in one file(input:240p,output:480p)'],
    ['Case-1192 Record mix stream combining different input/output resolution-002(input:240p,output:720p)', 'Case-1306  Record forward stream with input resolution different as output stream(input:240p,output:720p)', 'Case-1229 Record hybrid: mix stream first and then record forward stream in one file(input:240p,output:720p)', 'Case-1246 Record hybrid: Forward stream first and then record mix stream in one file(input:240p,output:720p)'],
    ['Case-1193 Record mix stream combining different input/output resolution-003(input:240p,output:1080p)', 'Case-1307  Record forward stream with input resolution different as output stream(input:240p,output:1080p)', 'Case-1230 Record hybrid: mix stream first and then record forward stream in one file(input:240p,output:1080p)', 'Case-1247 Record hybrid: Forward stream first and then record mix stream in one file(input:240p,output:1080p)'],
    ['Case-1194 Record mix stream combining different input/output resolution-004(input:480p,output:240p)', 'Case-1308  Record forward stream with input resolution different as output stream(input:480p,output:240p)', 'Case-1231 Record hybrid: mix stream first and then record forward stream in one file(input:480p,output:240p)', 'Case-1248 Record hybrid: Forward stream first and then record mix stream in one file(input:480p,output:240p)'],
    ['Case-1189 Record mix stream-003with the same input/output resolution(input:480p,output:480p)', 'Case-1184 Record forward stream with different input resolution (480p)', 'Case-1225 Record hybrid: mix stream first and then record forward stream in one file(input:480p,output:480p)', 'Case-1242 Record hybrid: Forward stream first and then record mix stream in one file(input:480p,output:480p)'],
    ['Case-1195 Record mix stream combining different input/output resolution-005(input:480p,output:720p)', 'Case-1311  Record forward stream with input resolution different as output stream(input:480p,output:720p)', 'Case-1232 Record hybrid: mix stream first and then record forward stream in one file(input:480p,output:720p)', 'Case-1249 Record hybrid: Forward stream first and then record mix stream in one file(input:480p,output:720p)'],
    ['Case-1196 Record mix stream combining different input/output resolution-006(input:480p,output:1080p)', 'Case-1312  Record forward stream with input resolution different as output stream(input:480p,output:1080p)', 'Case-1233 Record hybrid: mix stream first and then record forward stream in one file(input:480p,output:1080p)', 'Case-1250 Record hybrid: Forward stream first and then record mix stream in one file(input:480p,output:1080p)'],
    ['Case-1197 Record mix stream combining different input/output resolution-007(input:720p,output:240p)', 'Case-1313  Record forward stream with input resolution different as output stream(input:720p,output:240p)', 'Case-1234 Record hybrid: mix stream first and then record forward stream in one file(input:720p,output:240p)', 'Case-1251 Record hybrid: Forward stream first and then record mix stream in one file(input:720p,output:240p)'],
    ['Case-1198 Record mix stream combining different input/output resolution-008(input:720p,output:480p)', 'Case-1314 Record forward stream with input resolution different as output stream(input:720p,output:480p)', 'Case-1235 Record hybrid: mix stream first and then record forward stream in one file(input:720p,output:480p)', 'Case-1253 Record hybrid: Forward stream first and then record mix stream in one file(input:720p,output:480p)'],
    ['Case-1188 Record mix stream-002 with the same input/output resolution(input:720p,output:720p)', 'Case-1185 Record forward stream with different input resolution (720p)', 'Case-1226 Record hybrid:mix stream first and then record forward stream in one file(input:720p,output:720p)', 'Case-1243 Record hybrid: Forward stream first and then record mix stream in one file(input:720p,output:720p)'],
    ['Case-1199 Record mix stream combining different input/output resolution-009(input:720p,output:1080p)', 'Case-1315  Record forward stream with input resolution different as output stream(input:720p,output:1080p)', 'Case-1236 Record hybrid: mix stream first and then record forward stream in one file(input:720p,output:1080p)', 'Case-1254 Record hybrid: Forward stream first and then record mix stream in one file(input:720p,output:1080p)'],
    ['Case-1200 Record mix stream combining different input/output resolution-010(input:1080p,output:240p)', 'Case-1316  Record forward stream with input resolution different as output stream(input:1080p,output:240p)', 'Case-1237 Record hybrid: mix stream first and then record forward stream in one file(input:1080p,output:240p)', 'Case-1255 Record hybrid: Forward stream first and then record mix stream in one file(input:1080p,output:240p)'],
    ['Case-1201 Record mix stream combining different input/output resolution-011(input:1080p,output:480)', 'Case-1317  Record forward stream with input resolution different as output stream(input:1080p,output:480p)', 'Case-1238 Record hybrid: mix stream first and then record forward stream in one file(input:1080p,output:480p)', 'Case-1256 Record hybrid: Forward stream first and then record mix stream in one file(input:1080p,output:480p)'],
    ['Case-1202 Record mix stream combining different input/output resolution-012(input:1080p,output:720p)', 'Case-1318  Record forward stream with input resolution different as output stream(input:1080p,output:720p)', 'Case-1239 Record hybrid: mix stream first and then record forward stream in one file(input:1080p,output:720p)', 'Case-1257 Record hybrid: Forward stream first and then record mix stream in one file(input:1080p,output:720p)'],
    ['Case-1190 Record mix stream-004with the same input/output resolution(input:1080p,output:1080p)', 'Case-1186 Record forward stream with different input resolution (1080p)', 'Case-1227 Record hybrid:mix stream first and then record forward stream in one file(input:1080p,output:1080p)', 'Case-1244 Record hybrid: Forward stream first and then record mix stream in one file(input:1080p,output:1080p)']
  ];
  //recordId,视频后缀名
  var MCUNumList = [
    ['Case-1187', 'Case-1129', 'Case-1127', 'Case-1240'],
    ['Case-1191', 'Case-1305', 'Case-1228', 'Case-1245'],
    ['Case-1192', 'Case-1306', 'Case-1229', 'Case-1246'],
    ['Case-1193', 'Case-1307', 'Case-1230', 'Case-1247'],
    ['Case-1194', 'Case-1308', 'Case-1231', 'Case-1248'],
    ['Case-1189', 'Case-1184', 'Case-1225', 'Case-1242'],
    ['Case-1195', 'Case-1311', 'Case-1232', 'Case-1249'],
    ['Case-1196', 'Case-1312', 'Case-1233', 'Case-1250'],
    ['Case-1197', 'Case-1313', 'Case-1234', 'Case-1251'],
    ['Case-1198', 'Case-1314', 'Case-1235', 'Case-1253'],
    ['Case-1188', 'Case-1185', 'Case-1226', 'Case-1243'],
    ['Case-1199', 'Case-1315', 'Case-1236', 'Case-1254'],
    ['Case-1200', 'Case-1316', 'Case-1237', 'Case-1255'],
    ['Case-1201', 'Case-1317', 'Case-1238', 'Case-1256'],
    ['Case-1202', 'Case-1318', 'Case-1239', 'Case-1257'],
    ['Case-1190', 'Case-1186', 'Case-1227', 'Case-1244']
  ];

  //scenario in testlink(MCU-1027 ~ MCU-1219)(record1方法的参数，共4个scenario)
  var MCUCasesFor4User = ['MCU-1207:Record mix stream when four users in one room(resolution:A:240p,B:240p,C:240p,D:240p,output:240p)',
  'MCU-1208:Record mix stream when four users in one room(resolution:A:480p,B:480p,C:480p,D:480p,output:480p)',
  'MCU-1209:Record mix stream when four users in one room(resolution:A:720p,B:720p,C:720p,D:720p,output:720p)',
  'MCU-1219:Record mix stream when four users in one room(resolution:A:1080p,B:1080p,C:1080p,D:1080p,output:1080'];
  var MCUNumFor4User = ['MCU-1219','MCU-1209','MCU-1208','MCU-1207'];

  //scenario in testlink(MCU-1281 ~ MCU-1215)(record2方法的参数，共4个scenario)
  var MCUCasesForDeff4User = ['MCU-1215:Record mix stream when four users in one room(resolution:A:480p,B:720p,C:1080p,D:240p,output:1080p)',
  'MCU-1217:Record mix stream when four users in one room(resolution:A:1080p,B:480p,C:240p,D:720p,output:720p)',
  'MCU-1216:Record mix stream when four users in one room(resolution:A:720p,B:240p,C:1080p,D:480p,output:480p)',
  'MCU-1218:Record mix stream when four users in one room(resolution:A:240p,B:480p,C:720p,D:1080p,output:240p)'];
  var MCUNumForDeff4User = ['MCU-1215','MCU-1217','MCU-1216','MCU-1218'];
  var two_resolutions = [
  ['vga','hd720p','hd1080p','sif'],
  ['hd1080p','vga','sif','hd720p'],
  ['hd720p','sif','hd1080p','vga'],
  ['sif','vga','hd720p','hd1080p']
  ];

  //scenario in testlink(MCU-1120 ~ MCU-1223)(record3方法的参数，共4个scenario)
  var MCUCasesForwardFor4User=['MCU-1223:Record ForwardStream when four users in one room(resolution:A:1080p,B:1080p,C:1080p,D:1080p)',
  'MCU-1222:Record ForwardStream when four users in one room(resolution:A:720p,B:720p,C:720p,D:720p)',
  'MCU-1221:Record ForwardStream when four users in one room(resolution:A:480p,B:480p,C:480p,D:480p)',
  'MCU-1220:Record ForwardStream when four users in one room(resolution:A:240p,B:240p,C:240p,D:240p)'];
  var MCUNumsForwardFor4User = ['MCU-1223','MCU-1222','MCU-1221','MCU-1220'];

  //scenario in testlink(MCU-1120 ~ MCU-1223)(record4方法的参数，共4个scenario)
  var MCUCasesForShareHybird = [
  ['MCU-1261:Record hybrid: mix stream first and then record share screen in one file(output:1080p)','MCU-1265:Record hybrid: Forward stream first and then record share screen in one file(input:1080p)','MCU-1268:Record hybrid: Share screen first and then record mix stream in one file(output:1080p)','MCU-1273:Record hybrid: Share screen first and then record forward stream in one file(input:1080p)','MCU-1274:Record hybrid: Share screen first then forward stream and then record mix stream in one file',null],
  ['MCU-1260:Record hybrid: mix stream first and then record share screen in one file(output:720p)','MCU-1264:Record hybrid: Forward stream first and then record share screen in one file(input:720p)','MCU-1264:Record hybrid: Forward stream first and then record share screen in one file(input:720p)','MCU-1272:Record hybrid: Share screen first and then record forward stream in one file(input:720p)','MCU-1276:Record hybrid: Forward stream first then share screen and then record share screen in one file','MCU-1140:Record share screen stream, mix and forward stream in different file at the same time'],
  ['MCU-1259:Record hybrid: mix stream first and then record share screen in one file(output:480p)','MCU-1263:Record hybrid: Forward stream first and then record share screen in one file(input:480p)','MCU-1267:Record hybrid: Share screen first and then record mix stream in one file(output:480p)','MCU-1271:Record hybrid: Share screen first and then record forward stream in one file(input:480p)','MCU-1275:Record hybrid: mix stream first then forward stream and then record share screen in one file','MCU-1139:Record share screen stream forward stream in different file at the same time'],
  ['MCU-1258:Record hybrid: mix stream first and then record share screen in one file(output:240p)','MCU-1262:Record hybrid: Forward stream first and then record share screen in one file(input:240p)','MCU-1266:Record hybrid: Share screen first and then record mix stream in one file(output:240p)','MCU-1270:Record hybrid: Share screen first and then record forward stream in one file(input:240p)','MCU-1135:Record share screen stream','MCU-1138:Record mix stream forward stream in different file at the same time']
  ];
  var MCUNumsForShareHybird = [
  ['MCU-1261','MCU-1265','MCU-1268','MCU-1273','MCU-1274',null],
  ['MCU-1260','MCU-1264','MCU-1264','MCU-1272','MCU-1276','MCU-1140'],
  ['MCU-1259','MCU-1264','MCU-1267','MCU-1271','MCU-1275','MCU-1139'],
  ['MCU-1258','MCU-1262','MCU-1266','MCU-1270','MCU-1135','MCU-1138']
  ];

  //scenario in testlink(MCU-1286 ~ MCU-1145)(record5方法的参数，共4个scenario)
  var MCUCasesForSingleTrack=[['MCU-1144:Record audio only forward stream','MCU-1145:Record audio only mix stream'],
  ['MCU-1286:Record video only forward stream','MCU-1287:Record video only mix stream']
  ];
  var MCUNumsForSingleTrack=[['MCU-1144','MCU-1145'],['MCU-1286','MCU-1287']];
  var flags=[[true,false],[false,true]];

  //scenario in testlink(MCU-1279 ~ MCU-1285)(record6方法的参数，共3个scenario)
  var MCUCasesForDisconnect=['MCU-1279:Record share screen and then disconnect this stream during recording process','MCU-1141:Record forward stream and then disconnect this stream','MCU-1285:Record mix stream until all of them disconnect MCU(three users in room)'];
  var MCUNumsForDisconnect=['MCU-1279','MCU-1141','MCU-1285'];

  //scenario in testlink(MCU-1142 ~ MCU-1278)(record7方法的参数，共4个scenario)
  var MCUCasesForUnpublish=['MCU-1142:Record forward stream and then unpublish this stream','MCU-1278:MCU-1278:Record mix stream and then unpublish a forwardstream one by one(three users in room).','MCU-1350:Record mix stream and then publish forwardstreams one by one(three users in room)','MCU-1351:Record mix stream and then publish/unpublish random forwardstream (three users in room)'];
  var MCUNumsForUnpublish=['MCU-1142','MCU-1278','MCU-1350','MCU-1351'];

  //scenario in testlink(MCU-1142 ~ MCU-1278)(record8方法的参数，共4个scenario)
  var MCUCasesForSubscribe=['MCU-1356:Record forward stream and then unsubscribe/subscribe the forwardStream (three users in room)','MCU-1352:Record mix stream and then unsubscribe forwardstream one by one(three users in room)','MCU-1357:Record mix stream and then unsubscribe/subscribe the mixStream (three user in room)','MCU-1358:Record mix stream and then unsubscribe/subscribe one forwardStream (three user in room)'];
  var MCUNumsForSubscribe=['MCU-1356','MCU-1352','MCU-1357','MCU-1358'];

  MCUCasesFor4User = MCUCasesFor4User.reverse();
  MCUCaseList=MCUCaseList.reverse();
  MCUNumList=MCUNumList.reverse();

  //自动化测试数组MCUCaseList中的scenario
  if (type === 'testBasic') { 
  //record MCU-1187 ~ MCU-1244(call record)
    for (var i = 0; i < resolutions.length; i++) {
      for (var j = 0; j < resolutions.length; j++) {
        //调用record方法，每次内循环录制4个视频,循环结束共录制64个视频
        setTimeout(record, 100000 * conunt, myRoom, resolutions[i], resolutions[j], recordDuration, MCUCaseList[conunt], MCUNumList[conunt],client1,client2,'vp8');
        conunt++;
      };
    };
  };

  //自动化测试数组MCUCasesFor4User，MCUCasesForDeff4User，MCUCasesForwardFor4User，MCUCasesForSingleTrack，4个数组中scenario
  if (type === 'testDeep') {
    
    //record MCUCasesFor4User(use record1)
    for (var x = 0; x < resolutions.length; x++) {
      //call record1 after 'record method' done
      setTimeout(record1,x*30000,myRoom,resolutions[x],resolutions[x],recordDuration,clients,MCUCasesFor4User[x],MCUNumFor4User[x],'vp8');
    };

    //record MCUCasesForDeff4User(use record2)
    for (var y = 0; y < resolutions.length; y++) {
      //call record2 after 'record1 method' done
      setTimeout(record2,5*30000+y*40000, myRoom, two_resolutions[y], resolutions[y], recordDuration, clients, MCUCasesForDeff4User[y], MCUNumForDeff4User[y],'vp8');
    };

    //record MCUCasesForwardFor4User(use record3)
    for (var z = 0; z < resolutions.length; z++) {
      //call record3 after 'record2 method' done
      setTimeout(record3,5*40000+5*40000+z*70000, myRoom, resolutions[z], resolutions[z], recordDuration, clients, MCUCasesForwardFor4User[z], MCUNumsForwardFor4User[z],'vp8');
    };

    //call record2 for recording MCU-1244(use record2)
    setTimeout(record2,5*40000+5*40000+5*70000, myRoom, two_resolutions, 'hd720p', recordDuration, clients, 'MCU-1224:Record ForwardStream when four users in one room(resolution:A:240p,B:480p,C:720p,D:1080p)', 'MCU-1224','vp8');

    //record MCUCasesForSingleTrack(use record5)
    for (var p = 0; p < 2; p++) {
      //call record5 after above test
      setTimeout(record5,5*40000+5*40000+5*70000+70000+p*35000, myRoom, 'vga', 'vga', recordDuration, MCUCasesForSingleTrack[p], MCUNumsForSingleTrack[p],client1,client2, flags[p], 'vp8');
    };
  };

  //用于自动化测试MCUCasesForUnpublish，MCUCasesForSubscribe，数组中的scenario
  if (type === 'unpublish_unsubscribe') {
    //record MCUCasesForUnpublish(use record7)
    setTimeout(record7, 0, myRoom, 'vga', 'vga', recordDuration, clients, MCUCasesForUnpublish, MCUNumsForUnpublish, 'vp8');
    //record MCUCasesForSubscribe(use record8)
    setTimeout(record8, 180000, myRoom, 'vga', 'vga', recordDuration, clients, MCUCasesForSubscribe, MCUNumsForSubscribe, 'vp8');
    //record MCU-1362
    setTimeout(record9, /*36000*/0, myRoom, 'vga', 'vga', recordDuration, clients, 'MCU-1362:Record mixStream when random users unpublish/publish (4-16 users in one room)', 'MCU-1362', 'vp8');
  };

  //用于半自动化测试MCUCasesForShareHybird，MCUCasesForDisconnect，数组中的scenario（scenario中要用到shareScreen功能）
  if (type === 'shareScreen') {
    
    //record MCUCasesForShareHybird(use record4)
    for (var o = 0; o < resolutions.length; o++) {
      setTimeout(record4,205000*o,myRoom,resolutions[o],resolutions[o],recordDuration,client1,client2,MCUCasesForShareHybird[o],MCUNumsForShareHybird[o],'vp8');
    };

    //record MCUCasesForDisconnect(use record6)
    setTimeout(record6, 4*210000, myRoom, 'vga', 'vga', recordDuration, client1, client2, client3, MCUCasesForDisconnect, MCUNumsForDisconnect, 'vp8');

    setTimeout(record10, 4*210000, myRoom, 'vga', 'vga', recordDuration, clients, 'MCU-1284:Record 16 streams in one file', 'MCU-1284', 'vp8');
    setTimeout(record10, 4*210000 + 300000, myRoom, 'vga', 'vga', recordDuration, clients, 'MCU-1283:Record 16 streams in different file at the same time', 'MCU-1283', 'vp8');

    setTimeout(record11, 4*210000 + 300000 + 300000, myRoom, 'vga', 'vga', recordDuration, client1, client2, client3, ' MCU-1360:Record sharescreen stream and then unsubscribe/subscribe sharescreen (three user in', 'MCU-1360', 'vp8');

  };

  
}


window.onload = function() {
  conferenceTest = Woogeen.ConferenceClient.create({});

};




window.onbeforeunload = function() {
  if (localStream) {
    localStream.close();
    if (localStream.channel && typeof localStream.channel.close === 'function') {
      localStream.channel.close();
    }
  }
  for (var i in client1.remoteStreams) {
    if (client1.remoteStreams.hasOwnProperty(i)) {
      var stream = client1.remoteStreams[i];
      stream.close();
      if (stream.channel && typeof stream.channel.close === 'function') {
        stream.channel.close();
      }
      delete client1.remoteStreams[i];
    }
  }
};
