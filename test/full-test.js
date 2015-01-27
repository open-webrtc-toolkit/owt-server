describe('Full test', function() {
    "use strict";
  var TIMEOUT=10000;
  var roomid;
  var codec="vp8";
  var createRoom, updateRoom, createToken, createService, getRooms, getServices,getService,deleteService,getRoom, getUsers, getUser,deleteRoom,deleteUser,subscribeStreams;
  var debug = function(msg) {
        console.log('++++ debug message:' + msg);
      };
beforeEach(function() {
  //L.Logger.setLogLevel(L.Logger.INFO);
    createRoom = function (name, callback, options) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'createRoom/',
    body = {name: name, options: options};

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };
    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body))
    };

    updateRoom = function(room, options, callback) {
    var req = new XMLHttpRequest(),
    serverUrl = 'http://localhost:3001/',
    url = serverUrl + 'updateRoom/',
    body = {room: room, options: options};

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };
    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body));
    }

    createToken = function (room, userName, role, callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'createToken/',
    body = {room: room, username: userName, role: role};
   // body = {username: userName, role: role};

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
      //  if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
     // }
    };
    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body));
    };

  createService = function (name, key, callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'createService/',
    body = {name:name, key:key};

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body));
  };
  getRooms = function (callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'getRooms/';

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };
  getRoom = function (_room,callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    room = _room,
    url = serverUrl + 'getRoom/' + room;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };

deleteRoom = function (_room, callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    room = _room,
    url = serverUrl + 'room/' + room;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('DELETE', url, true);
    req.send();
  };

  getUsers = function (_room,callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    room = _room,
    url = serverUrl + 'getUsers/' + room;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };

  getUser = function (_room, _user,callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    room = _room,
    user = _user,
    url = serverUrl + 'room/' + room +'/user/' + user;
    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };


  deleteUser = function (_room, _user,callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    room = _room,
    user = _user,
    url = serverUrl + 'room/' + room + '/user/' +user;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('DELETE', url, true);
    req.send();
  };

  getServices = function (callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    url = serverUrl + 'getServices/';

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };
  getService = function (_service,callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    service = _service,
    url = serverUrl + 'getService/' + service;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('GET', url, true);
    req.send();
  };

  deleteService = function (_service, _force, callback) {
    var req = new XMLHttpRequest(),
    serverUrl = "http://localhost:3001/",
    service = _service,
    url = serverUrl + 'deleteService/' + service;

    req.onreadystatechange = function () {
      if (req.readyState === 4) {
        if (req.status === 200) {
          callback(req.responseText);
        } else {
          console.log("++++++deubg problem response:" + req.responseText);
        }
      }
    };

    req.open('DELETE', url, true);
    req.send();
  };


});
//beforeEach end

afterEach(function() {
  /*
  deleteRoom(roomid, function(_room) {
      callback();
      room1 = JSON.parse(_room);
      msg = room1.msg;
    });
  */
});



describe('management-test', function () {
  "use strict";
  var room, roomname;
  var serviceResponse, response, msg, token;

  it('CreateRoom test should be successful', function() {
    var callback = jasmine.createSpy("createRoom");
    createRoom("test_1", function(_room) {
      callback();
      room = JSON.parse(_room);
      roomid = room._id;
      console.log("==========room id is:" + roomid);
      roomname = room.name;
    });

    waitsFor(function () {
      return callback.callCount > 0;
      // debug
    });

    runs(function () {
      expect(roomname).toEqual('test_1');
    });

  });

  xit('updateRoom test should be successful', function() {
    var callback = jasmine.createSpy('updateRoom'), respText;
    runs(function() {
      updateRoom(roomid, {name:"test_1",mode:"plain", quality:"480*320",limit_in:3, limit_all:16, bitRate:700}, function(data) {
        console.log('++++ updateRoom response' + data);
        respText = data;
        callback();
      });
    });

    waitsFor(function() {
      return callback.callCount > 0;
    });

    runs(function() {
      expect(callback).toHaveBeenCalled();
      expect(respText).not.toMatch(/Error/);
    });
  });
  xit('getRoom update room information should be correct', function() {
    debug('getRoom');
    var callback = jasmine.createSpy("getRoom");
    var room_limit_in, room_limit_all, room_mode, room_bitRate, room_quality;
    getRoom(roomid, function(_room) {
      callback();
      room = JSON.parse(_room);
      roomname = room.name;
      room_limit_in = room.limit_in;
      room_limit_all = room.limit_all;
      room_bitRate = room.bitRate;
      room_quality = room.quality;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(roomname).toEqual('test_1');
      expect(room_limit_in).toEqual(3);
      expect(room_limit_all).toEqual(16);
      expect(room_bitRate).toEqual(700);
      expect(room_quality).toEqual('480*320');
    });

 });

  xit('updateRoom test should be failed if input wrong parameters', function() {
    var callback = jasmine.createSpy('updateRoom'), respText;
    runs(function() {
      updateRoom(roomid, {limit: 3}, function(data) {
        console.log('++++ updateRoom response if set error parameters' + data);
        respText = data;
        callback();
      });
    });

    waitsFor(function() {
      return callback.callCount > 0;
    });

    runs(function() {
      expect(callback).toHaveBeenCalled();
      expect(respText).toMatch(/Error/);
    });

    runs(function() {
      updateRoom(roomid, {quality:3, name:"*)&", limit_in:""}, function(data) {
        console.log('++++ updateRoom response if set wrong values' + data);
        respText = data;
        callback();
      });
    });

    waitsFor(function() {
      return callback.callCount > 0;
    });

    runs(function() {
      expect(callback).toHaveBeenCalled();
      expect(respText).toMatch(/Error/);
    });

    runs(function() {
      updateRoom(roomid, {name:"test_1",mode:"plain", quality:"480*320",limit_in:3, limit_all:16, bitRate:700}, function(data) {
        console.log('++++ updateRoom response' + data);
        respText = data;
        callback();
      });
    });

    waitsFor(function() {
      return callback.callCount > 0;
    });

    runs(function() {
      expect(callback).toHaveBeenCalled();
      expect(respText).not.toMatch(/Error/);
    });
  });
  it('getRooms should be succesful', function() {
        debug('getRooms');
   var callback = jasmine.createSpy("getRooms");
   var rooms ;
   getRooms(function(_room) {
    rooms = JSON.parse(_room);
    callback();
   });
    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(rooms.length).toBeGreaterThan(0);
    });
  });
  xit('CreateService testService should be successful', function() {
      debug('CreateService testService should be successful');
    var callback = jasmine.createSpy("createService");
    createService("testService","1234", function(_response) {
      callback();
      serviceResponse = JSON.parse(_response);
      serviceResponse = serviceResponse.msg;
      var reg = new RegExp('"',"g");
      serviceResponse = serviceResponse.replace(reg,"");
      console.log('serviceResponse is' + serviceResponse);

    });

    waitsFor(function () {
      return callback.callCount > 0;
    },"createService Callback", TIMEOUT);

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });

  });

  xit('getServices should be successful', function() {
    debug('getServices should be successful');
    var callback = jasmine.createSpy("getServices");
    getServices(function(_service) {
      callback();
    });

    waitsFor(function () {
      return callback.callCount > 0;
    },"createService Callback", TIMEOUT);

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });

  });

  xit('getService testService should be successful', function() {
    var callback = jasmine.createSpy("getService");
    getService(serviceResponse, function(_response) {
      callback();
    },"getService Callback", TIMEOUT);

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });

  });

  xit('getService no-existence service should return error message', function() {
    var callback = jasmine.createSpy("getService");
    getService("aaa", function(_response) {
      callback();
      response = JSON.parse(_response);
      msg = response.msg;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    },"get no existence Callback", TIMEOUT);

    runs(function () {
      expect(msg).toEqual('Service not found');
    });

  });

  xit('deleteService testService should be successful', function() {
    var callback = jasmine.createSpy("deleteService");
   //deleteService(serviceResponse, true, function(_response) {
    deleteService(serviceResponse, function(_response) {
      callback();
      response = JSON.parse(_response);
      msg = response.msg;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    },"deletServcice Callback", TIMEOUT);

    runs(function () {
      expect(msg).toEqual('Service deleted');
    });

  });

  xit('deleteService no-existence service should return error message', function() {
    var callback = jasmine.createSpy("deleteService");
    deleteService(serviceResponse, function(_response) {
      callback();
      response = JSON.parse(_response);
      msg = response.msg;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    },"delete no-existence Servcice Callback", TIMEOUT);

    runs(function () {
      expect(msg).toEqual('Service not found');
    });

  });

  xit('getUsers should be correct', function() {
    var tokenCallback = jasmine.createSpy("token");
    var mediaCallback = jasmine.createSpy("getusermedia");
    var errorMediaCallback = jasmine.createSpy("getusermediaerror");
    var callbackConnect = jasmine.createSpy("connectroom");
    var publishCallback = jasmine.createSpy("publishCallback");
    var publishErrorCallback = jasmine.createSpy("publishErrorCallback");
    var callback = jasmine.createSpy("getUsers");
    var roomid = "549097ad7a982f41474a20b0";
    createToken(roomid, "user", "presenter", function(_token) {
      callback();
      token = _token;
      console.log("==========token is:" + token);
    });
    waitsFor(function () {
      return tokenCallback.callCount > 0;
    });

    runs(function () {
      expect(tokenCallback).toHaveBeenCalled();
    });
    Woogeen.LocalStream.create({
           video: {
             device: 'camera',
             resolution: 'vga'
           },
           audio: true
         }, function (err, stream) {
           if (err) {
               errorMediaCallback();
           }else{
               mediaCallback();
           }
         });

    waitsFor(function () {
      return mediaCallback.callCount > 0;
    });

    runs(function () {
      expect(mediaCallback).toHaveBeenCalled();
      expect(errorMediaCallback.callCount).toEqual(0);
    });
    runs(function () {
      room = Woogeen.ConferenceClient.create({});
      room.join(token, function (resp) {
       console.log("join 1 " + resp);
        callback();
        remoteStream = resp.streams;
        room.publish(localStream, {maxVideoBW: 300, videoCodec: codec}, function(st) {
         console.log('stream published:', st.id());
            publishCallback();
        },function (err) {
          console.log('publish failed:', err);
             publishErrorCallback();
           });
      }, function(err){
          console.log("join failed " + err);
          errorCallback();
      });
    });
    waitsFor(function () {
      return publishCallback.callCount > 0;
    });
 runs(function () {
  console.log("roomid is " + roomid);
  var roomid = "549097ad7a982f41474a20b0";
    getUsers(roomid,function(_users) {
      callback();
      var users = _users;
      debug(users);
    });
  });
    waitsFor(function () {
      return callback.callCount > 0;
    });
    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  xit('getUser should be correct', function() {
    var callback = jasmine.createSpy("getUser");
    var roomid = "549097ad7a982f41474a20b0";
    console.log("roomid is " + roomid);
    getUser(roomid,'user',function(_users) {
      callback();
    });
    waitsFor(function () {
      return callback.callCount > 0;
    });
    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  xit('deleteUser should be correct', function() {
    var callback = jasmine.createSpy("deletUser");
    var roomid = "549097ad7a982f41474a20b0";
    deleteUser(roomid,'user',function(_users) {
      callback();
      users = JSON.parse(_users);
      msg = users.msg;
    });
    waitsFor(function () {
      return callback.callCount > 0;
    });
    runs(function () {
      expect(msg).toMatch(/Success/);
    });
  });

  it('delete room test which still have user should be successful', function() {
    var callback = jasmine.createSpy("deleteRoom");
    var room1;
    console.log("delete roomid is " + roomid);
    deleteRoom(roomid, function(_room) {
      callback();
     // room1 = JSON.parse(_room);
     // msg = room1.msg;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
     // expect(msg).toEqual('Room deleted');
      expect(callback).toHaveBeenCalled();
    });

  });

  it('CreateRoom test after delete with same name should be successful', function() {
    var callback = jasmine.createSpy("createRoom");
      console.log("create roomid is " + roomid);
    createRoom("test", function(_room) {
      callback();
      room = JSON.parse(_room);
      roomid = room._id;
       console.log("create roomid is " + roomid);
      console.log("==========room id is:" + roomid);
      roomname = room.name;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(roomname).toEqual('test');
    });

  });

  it('delete inactive room test should be successful', function() {
    var callback = jasmine.createSpy("deleteRoom");
      console.log("delete roomid is " + roomid);
    deleteRoom(roomid, function(_room) {
      debug("delete roomid is " + roomid);
      callback();
     // room1 = JSON.parse(_room);
     // msg = room1.msg;
    });

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
     // expect(msg).toEqual('Inactive room deleted');
      expect(callback).toHaveBeenCalled();
    });

  });

  it('deleteRoom unexistent room should be get error message', function() {
    var callback = jasmine.createSpy("deleteRoom");
    debug("delete roomid is " + roomid);
    deleteRoom(roomid, function(_room) {
      callback();
      //room1 = JSON.parse(_room);
     // msg = room1.msg;
    });
    // no msg at current design//
    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      //expect(msg).toEqual('Room does not exist');
      expect(callback).toHaveBeenCalled();
    });
  });

  it('getRooms should be succesful', function() {
   var callback = jasmine.createSpy("getRooms");
   var roomlist;
   getRooms(function(_room) {
    callback();
    roomlist = JSON.parse(_room);
    debug('roomlist' + roomlist);
    debug('roomlist' +roomlist[0]._id);
   });
    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(roomlist.length).toBeGreaterThan(0);
    });
  });
});

  describe('API-test', function () {
    "use strict";
  var roomid, response, msg, room, room_2, room1, token, token_2, localStream, localStream_2, remoteStream, remoteStream_2, localStreamID, remoteStreamID,localV,serviceResponse,streamtest,streamtest_mix, users, token1, token2, user1Stream, user2Stream, user1Room, user2Room, user1RemoteStreams, user2RemoteStreams;
  var canvas, context, bitmap, bitmapURL, width, height;

  it('create Token', function () {
    var callback = jasmine.createSpy("token");
    var getRoomsCallback = jasmine.createSpy("createRoom");
    var serverroom;
    getRooms(function(_room) {
    getRoomsCallback();
    var rooms = JSON.parse(_room);
    console.log(rooms.length);
      for (var i=0; i < rooms.length; i++){
              if (rooms[i].name === 'myRoom') {
                 roomid = rooms[i]._id;
                 console.log('room Id:', myRoom);
             }
         }
   });

    waitsFor(function () {
      return getRoomsCallback.callCount > 0;
    });

    runs(function () {
      expect(getRoomsCallback).toHaveBeenCalled();
    });
    createToken(roomid, "user_1", "presenter", function(_token) {
      callback();
      token = _token;
      console.log("==========token is:" + token);
    });
    createToken(roomid, "user_2", "presenter", function(_token) {
     // createToken("user_2", "presenter", function(_token) {
      callback();
      token_2 = _token;
      console.log("==========token_2 is:" + token_2);
    });
    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });


  it('connect to room twice with same token', function() {
    var callback = jasmine.createSpy("connectroom");
    var errorCallback = jasmine.createSpy("connectroomerror");
    var callback2 = jasmine.createSpy("connectroom");
    var errorCallback2 = jasmine.createSpy("connectroomerror");
    var publishErrorCallback = jasmine.createSpy("publishErrorCallback");
    var publishCallback = jasmine.createSpy("publishCallback");
    room = Woogeen.ConferenceClient.create({});
    room_2 = Woogeen.ConferenceClient.create({});
    console.log("token is " + token);
    room.join(token, function (resp) {
       console.log("join 1 " + resp);
        callback();
    }, function(err){
        console.log("join failed " + err);
        errorCallback();
    });
     room_2.join(token, function (resp) {
        console.log("join 2 " + resp);
         callback2();
     },function(err) {
         errorCallback2();
     });
    waits(1000);
    runs(function () {
      expect(callback.callCount).toEqual(1);
      expect(callback2.callCount).toEqual(0);
      expect(errorCallback.callCount).toEqual(0);
      expect(errorCallback2.callCount).toEqual(0);
    });
  });

  it('Disconnect room and create Token again', function () {
    var callback = jasmine.createSpy("token");
    var getRoomsCallback = jasmine.createSpy("createRoom");
    var roomdisconnectedCallback = jasmine.createSpy("roomdisconnected");
    var serverroom;
   /*
    * createRoom("test", function(_room) {
      createRoomCallback();
      serverroom = JSON.parse(_room);
      roomid = serverroom._id;
      console.log("==========room id is:" + roomid);
      roomname = room.name;
    });

    waitsFor(function () {
      return createRoomCallback.callCount > 0;
      // debug
    },"CreateRoom", TIMEOUT);

    runs(function () {
      expect(roomname).toEqual('test');
    });
*/

    runs(function() {
      room.addEventListener('server-disconnected', function(){
        roomdisconnectedCallback();
      });
      room_2.addEventListener('server-disconnected', function(){
        roomdisconnectedCallback();
      });
      room.leave();
       room_2.leave();
    }, 'room disconnected', TIMEOUT);

    waitsFor(function () {
      return roomdisconnectedCallback.callCount > 1;
    });

    runs(function() {
      getRooms(function(_room) {
        getRoomsCallback();
        var roomlist = JSON.parse(_room);
        console.log(roomlist.length);
        roomid = roomlist[0]._id;
        debug('roomid is' + roomid);
      });
    });


    waitsFor(function () {
      return getRoomsCallback.callCount > 0;
    });

    runs(function () {
      expect(getRoomsCallback).toHaveBeenCalled();
    });
    createToken(roomid, "user_1", "presenter", function(_token) {
      callback();
      token = _token;
      console.log("==========token is:" + token);
    });
    createToken(roomid, "user_2", "presenter", function(_token) {
      callback();
      token_2 = _token;
      console.log("==========token_2 is:" + token_2);
    });
    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  it('create localStream by createLocalStream', function () {
    var callback = jasmine.createSpy("createLocalStream");
    var errorCallback = jasmine.createSpy("createLocalStreamError");
    Woogeen.LocalStream.create({
           video: {
              device: 'camera',
              resolution : 'vga'
            },
           audio: true
         }, function (err, stream) {
           if (err) {
               errorCallback();
           }else{
            callback();
           localStream = stream;
           }
         });
      Woogeen.LocalStream.create({
           video: {
             device: 'camera',
             resolution: 'vga'
           },
           audio: true
         }, function (err, stream) {
           if (err) {
               errorCallback();
           }else{
               callback();
           localStream_2 = stream;
           }
         });

    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {

      expect(callback).toHaveBeenCalled();
    });
  });

  it('close should be defined', function() {
    expect(localStream.close).toBeDefined();
  });
  it('show should be defined', function() {
    expect(localStream.show).toBeDefined();
  });
  it('hide should be defined', function() {
    expect(localStream.hide).toBeDefined();
  });
  it('id should be defined', function() {
    expect(localStream.id).toBeDefined();
  });
  it('attributes should be defined', function() {
    expect(localStream.attributes).toBeDefined();
  });
  it('attr should be defined', function() {
    expect(localStream.attr).toBeDefined();
  });
  it('isMixed should be defined', function() {
    expect(localStream.isMixed).toBeUndefined();
  });
  it('isScreen should be defined', function() {
    expect(localStream.isScreen).toBeDefined();
  });
  /*
  it('playVideo should be defined', function() {
    expect(localStream.playVideo).toBeDefined();
  });
  it('pauseVideo should be defined', function() {
    expect(localStream.pauseVideo).toBeDefined();
  });
  it('playAudio should be defined', function() {
    expect(localStream.playAudio).toBeDefined();
  });
  it('pauseAudio should be defined', function() {
    expect(localStream.pauseAudio).toBeDefined();
  });
*/
  it('enableAudio should be defined', function() {
    expect(localStream.enableAudio).toBeDefined();
  });
  it('disableAudio should be defined', function() {
    expect(localStream.disableAudio).toBeDefined();
  });
  it('enableVideo should be defined', function() {
    expect(localStream.enableVideo).toBeDefined();
  });
  it('diableVideo should be defined', function() {
    expect(localStream.disableVideo).toBeDefined();
  });
  it('localStream hasAudio() should return true', function () {
    runs(function () {
      expect(localStream.hasAudio()).toEqual(true);
    });
  });
  it('localStream hasVideo() should return true', function () {
    runs(function () {
      expect(localStream.hasVideo()).toEqual(true);
    });
  });
  it('localStream isScreen should return false', function () {
    runs(function () {
      expect(localStream.isScreen()).toEqual(false);
    });
  });
  it('localStream id should be undefined before publish', function () {
    runs(function () {
      expect(localStream.id()).toEqual(undefined);
    });
  });
  it('attr and attributes should be work', function () {
    runs(function () {
      localStream.attr("name","test");
    });
    runs(function () {
      expect(localStream.attributes().name).toEqual("test");
    });
  });

  it('show() should call correct', function(){
    var localdiv = document.createElement("div");
    var Dectection;
    var IsPlaying;
    localdiv.setAttribute("id", "localDiv");
    localdiv.setAttribute("style", "width :320px; height: 240px;");
    document.body.appendChild(localdiv);
    localStream.show('localDiv');
    waits(500);
    runs(function() {
        console.log("localStream.id is " + localStream.id());
     var video = document.getElementById('stream' + localStream.id());
     expect(video.src).toMatch(/blob/);
   });
   runs(function() {
       var localId = 'stream' + localStream.id();
       Dectection = startDetection(localId,"320","240");
   })

   runs(function() {
        expect(Dectection).toEqual(true);
    });
    waits(10000);
    runs(function() {
       IsPlaying = isVideoPlaying()});
    runs(function() {
        expect(IsPlaying).toEqual(true);
    });
  });

  it('mouse over should show bar', function () {
    $(document.getElementById('subbar_'+localStream.id())).trigger('mouseover');
    var bar = document.getElementById('bar_'+localStream.id());
    runs(function() {
      var style = document.getElementById('bar_'+localStream.id()).style.display;
      expect(style).toBe('block');

    });
  });

  it('mouse out should display bar', function () {
    $(document.getElementById('subbar_'+localStream.id())).trigger('mouseout');
    waits(10000);
    runs(function() {
      var style = document.getElementById('bar_'+localStream.id()).style.display;
      expect(style).toBe('none');

    });
  });

  it('click volume should mute voice', function () {
    localStreamID = localStream.id();
    $(document.getElementById('volume_'+localStreamID)).click();
    runs(function() {
      var style = document.getElementById('volume_'+localStreamID).src;
      expect(style).toMatch(/iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAQAAAD9CzEMAAAABGdBTUEAALGOfPtRk/);
    });
  });

  it('click volume again should umute voice', function () {
    localStreamID = localStream.id();
    $(document.getElementById('volume_'+localStreamID)).click();
    runs(function() {
      var style = document.getElementById('volume_'+localStreamID).src;
      expect(style).toMatch(/iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAQAAAD9CzEMAAAAB/);
    });
  });

  it('hide() should call correct', function(){
    localStream.hide();
    runs(function() {
      var video = document.getElementById('stream' + localStream.id());
      expect(video).toBeNull();
    });
  });

  it('close() should call correct', function(){
    localStream.close();
    runs(function() {
      var video = document.getElementById('stream' + localStream.id);
      expect(video).toBeNull();
      expect(localStream.stream).not.toBeDefined();
    });
  });
  it('Can create local media by createLocalMedia after closed', function () {
    var callback = jasmine.createSpy("getusermedia");
    var errorCallback = jasmine.createSpy("getusermedia");
    Woogeen.LocalStream.create({
           video: {
             device: 'camera',
             resolution: 'vga'
           },
           audio: true
         }, function (err, stream) {
           if (err) {
               errorCallback();
           }
           else{
               callback();
           localStream = stream;
           }
    });
    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(errorCallback.callCount).toEqual(0);
    });
  });



  it('two clients connect to room and publish local stream ', function () {
    var callback = jasmine.createSpy("connectroom");
    var errorCallback = jasmine.createSpy("connectroomerror");
    var publishErrorCallback = jasmine.createSpy("publishErrorCallback");
    var publishCallback = jasmine.createSpy("publishCallback");
    var peerJoinedCallback = jasmine.createSpy("publishErrorCallback");
    room = Woogeen.ConferenceClient.create({});
    room_2 = Woogeen.ConferenceClient.create({});
    console.log("token is " + token);
    runs(function() {
     room.addEventListener("peer-joined", function(roomEvent) {
      peerJoinedCallback();
    });
     room_2.addEventListener("peer-joined", function(roomEvent) {
      peerJoinedCallback();
     });
    });
    runs(function() {
    room.join(token, function (resp) {
       console.log("join 1 " + resp);
        callback();
        remoteStream = resp.streams;
        room.publish(localStream, {maxVideoBW: 300, videCodec: codec}, function(st) {
         console.log('stream published:', st.id());
            publishCallback();
        },function (err) {
          console.log('publish failed:', err);
             publishErrorCallback();
           });
    }, function(err){
        console.log("join failed " + err);
        errorCallback();
    });
   });

    waitsFor(function () {
      return callback.callCount > 0;
    },"user 1 should join room correctly ", TIMEOUT);
    waitsFor(function () {
      return publishCallback.callCount > 0;
    }, "publishCallback should be called for the first join user ",TIMEOUT);
    runs(function() {
     room_2.join(token_2, function (resp) {
        console.log("join 2 " + resp);
         callback();
         remoteStream_2 = resp.streams;
         room_2.publish(localStream_2, {maxVideoBW: 300, videoCodec: codec}, function(st) {
          console.log('stream published:', st.id());
             publishCallback();
         },function (err) {
          console.log("pulish failed " + err);
             publishErrorCallback();
            });
     },function(err) {
         errorCallback();
     });
  });

    waitsFor(function () {
      return callback.callCount > 1;
    },"two user should join room correctly ", TIMEOUT);
    waitsFor(function () {
      return publishCallback.callCount > 1;
    }, "publishCallback should be called twice ",TIMEOUT);

    runs(function () {
      expect(callback.callCount).toEqual(2);
      expect(publishCallback.callCount).toEqual(2);
      expect(errorCallback.callCount).toEqual(0);
      expect(publishErrorCallback.callCount).toEqual(0);
    });
  });

  it('stream added event will be monitor ', function () {
    var callback = jasmine.createSpy("publishroom");
    waits(30000);
    room.addEventListener("stream-added", function(msg) {
      debug("stream-added");
     // remoteStream.push(msg.stream);
      debug(msg.stream.id());
      callback();
    });
    room_2.addEventListener("stream-added", function(msg) {
      debug("stream-added2");
      debug(msg.stream.id());
      callback();
    });

    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(callback.callCount).toEqual(6);
    });
  });

  it('no stream-added event if publish error stream', function (){
    var pubishErrorCallback = jasmine.createSpy("publisherrorstream");
    var publishCallback = jasmine.createSpy("publishstream");
    var callback = jasmine.createSpy("stream-added");
    room.publish("");
    room.publish("", {maxVideoBW: 300}, function(st) {
        publishCallback();
    },function (err) {
        pubishErrorCallback();
    });
    room.publish('*&*&',{maxVideoBW: 300}, function(st) {
        publishCallback();
    },function (err) {
        pubishErrorCallback();
    });
    room.publish('90',{maxVideoBW: 300}, function(st) {
        publishCallback();
    },function (err) {
        pubishErrorCallback();
    });;

    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      room.publish(stream,{maxVideoBW: 300}, function(st) {
          publishCallback();
      },function (err) {
          pubishErrorCallback();
    });
    }
    waits(10000);
    room_2.addEventListener("stream-added", function(msg) {
      callback();
    });
    runs(function() {
      expect(publishCallback.callCount).toEqual(0);
      expect(callback.callCount).toEqual(0);
      expect(pubishErrorCallback.callCount).toEqual(6);
     });
   });

  it('no stream-added event if publish stream already published', function (){
    var pubishErrorCallback = jasmine.createSpy("publisherrorstream");
    var publishCallback = jasmine.createSpy("publishstream");
    var callback = jasmine.createSpy("stream-added");
    room.publish(localStream,{maxVideoBW: 300, videoCodec: codec}, function(st) {
        publishCallback();
    },function (err) {
        pubishErrorCallback();
    });
    waits(10000);
    room_2.addEventListener("stream-added", function(msg) {
      callback();
    });

    runs(function() {
      expect(callback.callCount).toEqual(0);
     },"stream-added event", TIMEOUT);
   });

  it('room.localStreams should be defined as Object', function() {
    expect(room.localStreams).toBeDefined();
    expect(room.localStreams).toEqual(jasmine.any(Object));
  });

  it('room.remoteStreams should be defined as Object', function() {
    expect(room.remoteStreams).toBeDefined();
    expect(room.remoteStreams).toEqual(jasmine.any(Object));
  });
  /*
  it('playVideo for remoteStream should be defined', function() {
    runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.playVideo).toBeDefined();
        }
      });
  });
  it('pauseVideo for remoteStream should be defined', function() {
    runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.pauseVideo).toBeDefined();
        }
      });
  });
  it('playAudio for remoteStream should be defined', function() {
     runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.playAudio).toBeDefined();
        }
      });
  });
  it('pauseAudio for remoteStream should be defined', function() {
     runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.pauseAudio).toBeDefined();
        }
      });
  });
*/
  it('enableAudio for remoteStream should be defined', function() {
     runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.enableAudio).toBeDefined();
      }
     });
  });
  it('disableAudio for remoteStream should be defined', function() {
     runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.disableAudio).toBeDefined();
      }
     });
  });
  it('enableVideo for remoteStream should be defined', function() {
     runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.enableVideo).toBeDefined();
      }
     });
  });
  it('disableVideo for remoteStream should be defined', function() {
      runs(function() {
      for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        expect(stream.disableVideo).toBeDefined();
      }
      });
  });
  /*
it('enableVideo /disableVideo for local stream will trigger video-off and video-on', function() {
    var videoOnCallback = jasmine.createSpy("video-on");
    var videoOffCallback = jasmine.createSpy("video-off");
    runs(function() {
      room.addEventListener("video-on", function(){
        videoOnCallback();
      })
      room.addEventListener("video-off", function(){
        videoOffCallback();
      })
    });
     runs(function() {
      localStream.pauseVideo();
    });
     waits(1000);
     runs(function() {
      localStream.playVideo();
    });
    waitsFor(function () {
      return videoOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(videoOnCallback.callCount).toEqual(1);
        expect(videoOffCallback.callCount).toEqual(1);
    });
  });
it('pauseAudio /playAudio for local stream will trigger audio-off and audio-on', function() {
    var audioOnCallback = jasmine.createSpy("audio-on");
    var audioOffCallback = jasmine.createSpy("audio-off");
    runs(function() {
      room.addEventListener("audio-on", function(){
        audioOnCallback();
      })
      room.addEventListener("audio-off", function(){
        audioOffCallback();
      })
    });
     runs(function() {
      localStream.pauseAudio();
    });
     waits(1000);
     runs(function() {
      localStream.playAudio();
    });
    waitsFor(function () {
      return audioOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(audioOnCallback.callCount).toEqual(1);
        expect(aduioOffCallback.callCount).toEqual(1);
    });
  });
*/
/*
it('enableVideo /disableVideo for local stream will trigger video-off and video-on', function() {
    var videoOnCallback = jasmine.createSpy("video-on");
    var videoOffCallback = jasmine.createSpy("video-off");
    runs(function() {
      room.addEventListener("video-on", function(){
        videoOnCallback();
      })
      room.addEventListener("video-off", function(){
        videoOffCallback();
      })
    });
     runs(function() {
      localStream.pauseVideo();
    });
     waits(1000);
     runs(function() {
      localStream.playVideo();
    });
    waitsFor(function () {
      return videoOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(videoOnCallback.callCount).toEqual(1);
        expect(videoOffCallback.callCount).toEqual(1);
    });
  });
it('enableAudio /disableAudio for local stream will trigger audio-off and audio-on', function() {
    var audioOnCallback = jasmine.createSpy("audio-on");
    var audioOffCallback = jasmine.createSpy("audio-off");
    runs(function() {
      room.addEventListener("audio-on", function(){
        audioOnCallback();
      })
      room.addEventListener("audio-off", function(){
        audioOffCallback();
      })
    });
     runs(function() {
      localStream.enableAudio();
    });
     waits(1000);
     runs(function() {
      localStream.disableAudio();
    });
    waitsFor(function () {
      return audioOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(audioOnCallback.callCount).toEqual(1);
        expect(aduioOffCallback.callCount).toEqual(1);
    });
  });
 /// video ready/hold
*/
/*
 it('pauseVideo /playVideo for remote stream will trigger video-ready and video-hold', function() {
    var videoReadyCallback = jasmine.createSpy("video-ready");
    var videoHoldCallback = jasmine.createSpy("video-off");
    runs(function() {
      room.addEventListener("video-ready", function(){
        videoReadyCallback();
      })
      room.addEventListener("video-hold", function(){
        videoHoldCallback();
      })
    });
     runs(function() {
         for(var i in room.remoteStreams){
             var stream = room.remoteStreams[i];
            stream.pauseVideo();
         }
    });
     waits(1000);
     runs(function() {
         for(var i in room.remoteStreams){
             var stream = room.remoteStreams[i];
            stream.playVideo();
         }
    });
    waitsFor(function () {
      return videoReadyCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(videoReadyCallback.callCount).toEqual(1);
        expect(videoHoldCallback.callCount).toEqual(1);
    });
  });
it('pauseAudio /playAudio for remote stream will trigger audio-ready and audio-hold', function() {
    var audioReadyCallback = jasmine.createSpy("audio-ready");
    var audioHoldCallback = jasmine.createSpy("audio-hold");
    runs(function() {
      room.addEventListener("audio-on", function(){
        audioReadyCallback();
      })
      room.addEventListener("audio-off", function(){
        audioHoldCallback();
      })
    });
     runs(function() {
         for(var i in room.remoteStreams){
             var stream = room.remoteStreams[i];
            stream.pauseAudio();
         }
    });
     waits(1000);
     runs(function() {
         for(var i in room.remoteStreams){
             var stream = room.remoteStreams[i];
            stream.playAudio();
         }
    });
waitsFor(function () {
      return audioHoldCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(audioHoldCallback.callCount).toEqual(1);
        expect(aduioReadyCallback.callCount).toEqual(1);
    });
  });
/*
it('enableVideo /disableVideo for local stream will trigger video-off and video-on', function() {
    var videoOnCallback = jasmine.createSpy("video-on");
    var videoOffCallback = jasmine.createSpy("video-off");
    runs(function() {
      room.addEventListener("video-on", function(){
        videoOnCallback();
      })
      room.addEventListener("video-off", function(){
        videoOffCallback();
      })
    });
     runs(function() {
      localStream.pauseVideo();
    });
     waits(1000);
     runs(function() {
      localStream.playVideo();
    });
waitsFor(function () {
      return videoOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(videoOnCallback.callCount).toEqual(1);
        expect(videoOffCallback.callCount).toEqual(1);
    });
  });

    var audioOnCallback = jasmine.createSpy("audio-on");
    var audioOffCallback = jasmine.createSpy("audio-off");
    runs(function() {
      room.addEventListener("audio-on", function(){
        audioOnCallback();
      })
      room.addEventListener("audio-off", function(){
        audioOffCallback();
      })
    });
     runs(function() {
      localStream.enableAudio();
    });
     waits(1000);
     runs(function() {
      localStream.disableAudio();
    });
    waitsFor(function () {
      return audioOnCallback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
        expect(audioOnCallback.callCount).toEqual(1);
        expect(aduioOffCallback.callCount).toEqual(1);
    });
  });*/

  it('room.state should return 2', function() {
    expect(room.state).toEqual(2);
  });

  it('remoteStream should have three streams', function () {
    var callback = jasmine.createSpy("subscribe");
    for (var index in room.remoteStreams) {
    var stream = room.remoteStreams[index];
    callback();
    }

    runs(function () {
        expect(callback.callCount).toEqual(3);
    });
  });


 // Susbscribe forward stream
  it('susbscribe forward stream and check up it is playing ', function () {
    var callback = jasmine.createSpy("subscribe");
    var errorCallback = jasmine.createSpy("errorSubscribe");
    var Dectection;
    var IsPlaying;
    for (var index in room.remoteStreams){
    var stream = room.remoteStreams[index];
      debug(stream.id());
      debug("localstreamid" + localStream.id())
      if((localStream.id() != stream.id())&&(!stream.isMixed())&&(!stream.isScreen())){
        debug("subscribeStream + " + stream.id());
        var remoteId = "stream" + stream.id();
        room.subscribe(stream, {videoCodec: codec}, function () {
            callback();
           var divg = document.createElement("div");
           divg.setAttribute("id", "peerID");
           divg.setAttribute("style", "width :320px; height: 240px;");
           document.body.appendChild(divg);
           stream.show('peerID');
           waits(500);
           debug("remoteId is +" +remoteId);
           runs(function() {
            var video = document.getElementById(remoteId);
            expect(video.src).toMatch(/blob/);
          });
         runs(function() {
          debug("remoteId is +" +remoteId);
          Dectection = startDetection(remoteId ,"320","240");
        })
         runs(function() {
          expect(Dectection).toEqual(true);
        });
         waits(10000);
         runs(function() {
          IsPlaying = isVideoPlaying();
        });
         runs(function() {
          expect(IsPlaying).toEqual(true);
         });
        }, function(err){
            errorCallback();
     });
    }
    }

    waitsFor(function () {
      return callback.callCount > 0;
    },"subscribe successful callback should trigged", 50000);

    runs(function () {
      expect(callback).toHaveBeenCalled();
       expect(errorCallback.callCount).toEqual(0);
       expect(Dectection).toEqual(true);
        expect(IsPlaying).toEqual(true);
    });
  });

it('check up state included iceConnectionState, iceGatheringState, hasVideo, hasaudio for remote forward stream', function() {
     var streamcallback = jasmine.createSpy("stats for remoteStream");
     var iceConnectionStateCallback = jasmine.createSpy("iceConnectionState");
     var iceGatheringStateCallback = jasmine.createSpy("iceGatheringState");
     var streamCallback = jasmine.createSpy("stream");
     var hasVideoCallback = jasmine.createSpy("hasvideo");
     var hasAudioCallback = jasmine.createSpy("hasaudio");
     for (var index in room.remoteStreams) {
         var stream = room.remoteStreams[index];
         //((!stream.isMix()&&(!stream.isScreen()))){
         if((localStream.id() != stream.id())&&(!stream.isMixed())&&(!stream.isScreen())){
             if(stream.channel.peerConnection.iceConnectionState == "completed"){
                 iceConnectionStateCallback();
             }
             if(stream.channel.peerConnection.iceGatheringState == "complete"){
                 iceGatheringStateCallback();
            }
             if(stream.hasVideo() == true){
                 hasVideoCallback();
            }
             if(stream.hasAudio() == true){
                 hasAudioCallback();
             }

         }
     }
     runs(function() {
         expect(iceConnectionStateCallback.callCount).toEqual(1);
         expect(iceGatheringStateCallback.callCount).toEqual(1);
         expect(hasVideoCallback.callCount).toEqual(1);
         expect(hasAudioCallback.callCount).toEqual(1);
     });
 });


  it('subscribe error stream will not succesful', function(){
    var callback = jasmine.createSpy("subscribe");
    var errorCallback = jasmine.createSpy("errorSubscribe");
    room.subscribe("",function (resp) {
        callback();},
        function(err){
        errorCallback();
        });
    room.subscribe("*(&(&^",function (resp) {
        callback();
        },function(err){
            errorCallback();
        });
    room.subscribe('90',function (resp) {
        callback();
        },function(err){
            errorCallback();
        });
    runs(function() {
       expect(callback.callCount).toEqual(0);
      expect(errorCallback.callCount).toEqual(3);
    });
  });
  it('subscribe the stream already subscribed will not succesful', function(){
    var callback = jasmine.createSpy("subscribe");
    var errorCallback = jasmine.createSpy("errorSubscribe");
    for (var index in remoteStream) {
      var stream = remoteStream[index];
    if((localStream.id() != stream.id())&&(!stream.isMixed()))
    {
        room.subscribe(stream, {videoCodec: codec}, function (resp) {
        callback();}, function(err){
        errorCallback();
        });
    }
     };
    runs(function () {
      expect(callback.callCount).toEqual(0);
      expect(errorCallback.callCount).toEqual(0);
    });
  });

  //subscribe mix stream
    it('subscribe mix stream and check up mix stream is playing ', function () {
    var callback = jasmine.createSpy("subscribe");
    var errorCallback = jasmine.createSpy("errorSubscribe");
    var Dectection;
    var IsPlaying;
    var streamtest_mix;
    var mixId;
    var remoteId;
    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      debug(stream.id());
      debug("remotestreamid" + stream.id())
      if((stream.isMixed())){
        debug("aa subscribeStream  " + stream.id());
        mixId = stream.id();
        streamtest_mix = stream;
         remoteId = "stream"+mixId;
        room.subscribe(streamtest_mix, {videoCodec: codec}, function () {
          callback();
          debug("aaaaaaaaaaaaaaa subscribeStream  " + mixId);
          var divg = document.createElement("div");
          divg.setAttribute("id", "peerID_mix");
          divg.setAttribute("style", "width :640px; height: 480px;");
          document.body.appendChild(divg);
          streamtest_mix.show('peerID_mix');
          debug("remoteId mix id tag is " + remoteId);
          waits(500);
          runs(function() {
            var video = document.getElementById(remoteId);
            expect(video.src).toMatch(/blob/);
          });
          runs(function() {
            Dectection = startDetection(remoteId ,"320","240");
          })
          runs(function() {
            expect(Dectection).toEqual(true);
          });
          waits(10000);
          runs(function() {
            IsPlaying = isVideoPlaying();
          });
          runs(function() {
            expect(IsPlaying).toEqual(true);
          });
        }, function(err) {
          errorCallback();
        });
        }
    }

    waitsFor(function () {
      return callback.callCount > 0;
    },"subscribe mix successful callback should trigged", 50000);

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(errorCallback.callCount).toEqual(0);
      expect(Dectection).toEqual(true);
      expect(IsPlaying).toEqual(true);
    });
  });

it('check up state inclucded iceConnectionState, iceGatheringState, hasVideo, hasaudio for remote mix stream', function() {
     var streamcallback = jasmine.createSpy("stats for remoteStream");
     var iceConnectionStateCallback = jasmine.createSpy("iceConnectionState");
     var iceGatheringStateCallback = jasmine.createSpy("iceGatheringState");
     var streamCallback = jasmine.createSpy("stream");
     var hasVideoCallback = jasmine.createSpy("hasvideo");
     var hasAudioCallback = jasmine.createSpy("hasaudio");
     for (var index in room.remoteStreams) {
         var stream = room.remoteStreams[index];
         if(stream.isMixed()){
             if(stream.channel.peerConnection.iceConnectionState == "completed"){
                 iceConnectionStateCallback();
             }
             if(stream.channel.peerConnection.iceGatheringState == "complete"){
                 iceGatheringStateCallback();
            }
             if(stream.hasVideo() == true){
                 hasVideoCallback();
            }
             if(stream.hasAudio() == true){
                 hasAudioCallback();
             }
         }
     }
     runs(function() {
         expect(iceConnectionStateCallback.callCount).toEqual(1);
         expect(iceGatheringStateCallback.callCount).toEqual(1);
         expect(hasVideoCallback.callCount).toEqual(1);
         expect(hasAudioCallback.callCount).toEqual(1);
     });
 });

  it('mouse over should show volume bar for remoteStream', function () {
    for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        if(stream.isMixed()){
            remoteStreamID = stream.id();
            $(document.getElementById('picker_'+remoteStreamID)).trigger('mouseover');
      }
   }
   runs(function() {
    var style = document.getElementById('picker_'+remoteStreamID).style.display;
    expect(style).toBe('block');
    });

  });

  it('mouseout should not show volume bar', function () {
    for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        if(stream.isMixed()){
            remoteStreamID = stream.id();
            $(document.getElementById('picker_'+remoteStreamID)).trigger('mouseout');
        }
    }
    runs(function() {
      var style = document.getElementById('picker_'+remoteStreamID).style.display;
      expect(style).toBe('none');
    });
  });

  it('change volume via volume bar', function () {
    for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        if(stream.isMixed()){
            remoteStreamID = stream.id();
            $(document.getElementById('picker_'+remoteStreamID)).val(100);
     }
    }
    runs(function() {
      waits(500);
      var value = $(document.getElementById('picker_'+remoteStreamID)).val();
      expect(value).toBe('100');
    });
  });

  it('click volume should mute voice', function () {
    for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        if(stream.isMixed()){
            remoteStreamID = stream.id();
            $(document.getElementById('volume_'+remoteStreamID)).click();
        }
    }
    runs(function() {
      var style = document.getElementById('volume_'+remoteStreamID).src;
      expect(style).toMatch(/iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAQAAAD9CzEMAAAABGdBTUEAALGOfPtRk/);
    });
  });

  it('click volume again should umute voice', function () {
    for (var index in room.remoteStreams) {
        var stream = room.remoteStreams[index];
        if(stream.isMixed()){
            remoteStreamID = stream.id();
            $(document.getElementById('volume_'+remoteStreamID)).click();
        }
    }
    runs(function() {
      var style = document.getElementById('volume_'+remoteStreamID).src;
      expect(style).toMatch(/iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAQAAAD9CzEMAAAABGdBTUEAALGOfPtRk/);
    });
  });


//send/
  it('should not trigger stream-data event on the sender peer side', function(){
    var callback = jasmine.createSpy("senddata");
    var errorCallback = jasmine.createSpy("senddataerror");
    var messageCallback = jasmine.createSpy("onMessage");
    room.onMessage(function (event) {
      messageCallback();
    });
    room_2.send("data",0,function(){
      callback();
    }, function(err){
      errorCallback();
    });

    waitsFor(function () {
        return callback.callCount > 0;
      },"send data call back", TIMEOUT);
    waitsFor(function () {
        return messageCallback.callCount > 0;
      },"send data on message callback", TIMEOUT);

    runs(function () {
      expect(errorCallback).not.toHaveBeenCalled();
      expect(callback).toHaveBeenCalled();
      expect(messageCallback).toHaveBeenCalled();
    });
  });


  it('unsubscribe stream should empty HTML element', function () {
    var callback = jasmine.createSpy('unsubscribe'),
    streamCount = 0;
    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      room.unsubscribe(stream);
      streamCount++;
    }

    runs(function () {
      var video = document.getElementById('stream' +remoteStreamID);
      expect(video.getAttribute("url")).toBeNull;

    });
   });

  it('should unpublish stream in room', function () {
    var callback = jasmine.createSpy("publishroom");
    var Dectection;
    var IsPlaying;
    room_2.unpublish(localStream_2);
    room.addEventListener("stream-removed", function(msg) {
      callback();
    });
     room.addEventListener("stream-changed", function(msg) {
      callback();
    });
    waitsFor(function () {
      return callback.callCount > 0;
    });
    runs(function() {
        for (var index in room.remoteStreams){
            var stream = room.remoteStreams[index];
            if((localStream.id() != stream.id())&&(stream.id() != 0)){
                var remoteId = "stream"+index;
                console.log(remoteId);
                Dectection = startDetection(remoteId,"640","480");
                //     IsPlaying = isVideoPlaying();
                }
        }
    });
    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
    runs(function() {
        expect(Dectection).toEqual("novideo");
     //   expect(IsPlaying).toEqual("video-not-playing");
    });
  });


  it('id should return undefined after unpublish', function() {
    var callback = jasmine.createSpy("unpublish");
    runs(function(){
      room.unpublish(localStream,function(){
        callback();
      }, function(err){
      });
    });
    waitsFor(function () {
      return callback.callCount > 0;
    },"unpublish", TIMEOUT);
    runs(function() {
      expect(localStream.id()).toBeNull();
    });
  });


  it('Can publish stream in room with perfs', function () {
    room.publish(localStream,{videoCodec: codec, width: 320, height: 240, framerate: 30, audiocodec: "ISAC", maxVideoBW: 500})
    var callback = jasmine.createSpy("publishroom");
    waits(30000);
    room.addEventListener("stream-added", function(msg) {
      remoteStream = [];
      remoteStream.push(msg.stream);
      callback();
    });

    waitsFor(function () {
      return callback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });


  it('Disconnect from room can trigger server-disconnected event', function () {
    var callback = jasmine.createSpy("connectroom");
    room.addEventListener("server-disconnected", callback);
    room_2.addEventListener("server-disconnected", callback);
    room.leave();
    room_2.leave();
    waitsFor(function () {
      return callback.callCount > 1;
    });
    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  it('connect to room with error token should be failed and report error message ', function () {
    var callback = jasmine.createSpy("connectroom");
    var errorCallback = jasmine.createSpy("connectroomerror");
    room.join(token, function (resp) {
            callback();
            debug("join room succesfully", resp);
     }, function (err){
             errorCallback();
             debug('server connection failed:', err);
    });

    runs(function () {
      expect(callback).not.toHaveBeenCalled();
      expect(errorCallback.callCount).toEqual(1);
    });
  });

  it('Create two new token again should be successful', function () {
    var callback = jasmine.createSpy("token");

    createToken(roomid, "user", "presenter", function(_token) {
      callback();
      token = _token;
    });

    createToken(roomid, "user_2", "presenter", function(_token) {
      callback();
      token_2 = _token;
    });
    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  it('It should be failed should get access to user media without video and audio', function () {
    var callback = jasmine.createSpy("getusermedia");
    Woogeen.LocalStream.create({
           video: false,
           audio: false
         }, function (err, stream) {
           if (err) {
               callback();
             debug('create LocalStream failed:', err);
           }
         });
    Woogeen.LocalStream.create({
           video: false,
           audio: false
         }, function (err, stream) {
           if (err) {
              callback();
             debug('create LocalStream failed:', err);
           }
         });

    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });
  it('It should be successful should get access to user media for audio only', function () {
    var errorCallback = jasmine.createSpy("errorgetusermedia");
    var callback = jasmine.createSpy("getusermedia");
    localStream = undefined;
    localStream_2 = undefined;
    remoteStream = undefined;
    Woogeen.LocalStream.create({
           video: false,
           audio: true
         }, function (err, stream) {
           if (err) {
               errorCallback();
           } else{
               callback();
               localStream = stream;}
         });
      Woogeen.LocalStream.create({
           video: false,
           audio: true
         }, function (err, stream) {
           if (err) {
               errorCallback();
           } else{
               callback();
           localStream_2 = stream;}
         });
    waitsFor(function () {
      return callback.callCount > 1;
    });
    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });
  it('two clients connect to room and publish local stream with audio only', function () {
    var callback = jasmine.createSpy("connectroom");
    var errorCallback = jasmine.createSpy("connectroomerror");
    var publishErrorCallback = jasmine.createSpy("publishErrorCallback");
    var publishCallback = jasmine.createSpy("publishCallback");
    var peerJoinedCallback = jasmine.createSpy("publishErrorCallback");
    room = Woogeen.ConferenceClient.create({});
    room_2 = Woogeen.ConferenceClient.create({});
    console.log("token is " + token);
    runs(function() {
     room.addEventListener("peer-joined", function(roomEvent) {
      peerJoinedCallback();
    });
     room_2.addEventListener("peer-joined", function(roomEvent) {
      peerJoinedCallback();
     });
    });
    runs(function() {
    room.join(token, function (resp) {
       console.log("join 1 " + resp);
        callback();
        remoteStream = resp.streams;
    }, function(err){
        console.log("join failed " + err);
        errorCallback();
    });
   });

    waitsFor(function () {
      return callback.callCount > 0;
    },"user 1 should join room correctly ", TIMEOUT);
    runs(function() {
     room_2.join(token_2, function (resp) {
        console.log("join 2 " + resp);
         callback();
         remoteStream_2 = resp.streams;
         room_2.publish(localStream_2, {maxVideoBW: 300}, function(st) {
          console.log('stream published:', st.id());
             publishCallback();
         },function (err) {
          console.log("pulish failed " + err);
             publishErrorCallback();
            });
     },function(err) {
         errorCallback();
     });
  });

    waitsFor(function () {
      return callback.callCount > 1;
    },"two user should join room correctly ", TIMEOUT);
    waitsFor(function () {
      return publishCallback.callCount > 0;
    }, "publishCallback should be called",TIMEOUT);

    runs(function () {
      expect(callback.callCount).toEqual(2);
      expect(publishCallback.callCount).toEqual(1);
      expect(errorCallback.callCount).toEqual(0);
      expect(publishErrorCallback.callCount).toEqual(0);
    });
  });

  it('localStream hasAudio() should return true if localStream only included data', function () {
    runs(function () {
      expect(localStream.hasAudio()).toEqual(true);
    });
  });

  it('localStream hasVideo() should return false if localStream only included data', function () {
    runs(function () {
      expect(localStream.hasVideo()).toEqual(false);
    });
  });


  it('should publish stream in room for localStream which only audio', function () {
    if (room !== undefined && room.state === 2) {
      var callback = jasmine.createSpy("publishroom");
      var publishErrorCallback = jasmine.createSpy("publishError");
      var publishCallback = jasmine.createSpy("publishCallback");
      room.addEventListener("stream-added", function(msg) {
        remoteStream = [];
        remoteStream.push(msg.stream);
        callback();
      });

        room.publish(localStream, {maxVideoBW: 300}, function(st) {
         console.log('stream published:', st.id());
            publishCallback();
        },function (err) {
          console.log('publish failed:', err);
             publishErrorCallback();
           });
      waitsFor(function () {
        return callback.callCount > 0;
      },'stream-add event did not arrive',120000);

      runs(function () {
        expect(callback).toHaveBeenCalled();
        expect(publishCallback).toHaveBeenCalled();
      });
    } else {
      //assert this case as failing directly
      expect(false).toEqual(true);
    }
  });

  it('subscribe to stream in room for localStream which without audio only, no video will display', function () {
    var callback = jasmine.createSpy("subscribe");
    var remoteId;
    var Dectection;
    var IsPlaying;
    var mixId;
    room.addEventListener("stream-subscribed", function(streamEvent) {
      callback();
      streamtest = streamEvent.stream;

    });
    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      debug(stream.id());
      debug("remotestreamid" + stream.id())
      if((stream.isMixed())){
        debug("subscribeStream  " + stream.id());
        mixId = stream.id();
        remoteId = "stream"+stream.id();
        streamtest = stream; //room.subscribe(stream);

        room.subscribe(stream, function () {
            callback();
          var divg = document.createElement("div");
          divg.setAttribute("id", "peerID"+mixId);
          divg.setAttribute("style", "width :640px; height: 480px;");
          document.body.appendChild(divg);
          streamtest.show('peerID'+mixId);
          debug("remoteId mix id tag is " + remoteId);
          waits(500);
          runs(function() {
            var video = document.getElementById(remoteId);
            expect(video.src).toMatch(/blob/);
          });
          runs(function() {
            Dectection = startDetection(remoteId ,"320","240");
          })
          runs(function() {
            expect(Dectection).toEqual('novideo');
          });
          waits(10000);
          runs(function() {
            IsPlaying = isVideoPlaying();
          });
          runs(function() {
            expect(IsPlaying).toEqual('video-not-playing');
          });
        }, function(err) {
          errorCallback();
        });
        }
    }
    waitsFor(function () {
      return callback.callCount > 0;
    },"stream-subscribed should trigged", 20000);

    runs(function () {
      expect(callback).toHaveBeenCalled();
    });
  });

  it('subscribe the stream which already subscribed in room for localStream with audio only will be failed', function () {
    var callback = jasmine.createSpy("subscribe");
    var errorCallback = jasmine.createSpy("subscribe");
    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      if(stream.isMixed()){
      room.subscribe(stream, function(){
        callback();
      },function(err){
         errorCallback();
        });
     }
    }
    waits(30000);
    runs(function () {
      expect(callback).not.toHaveBeenCalled();
      expect(errorCallback).toHaveBeenCalled();
    });
  });


  it('unsubscribe stream should can be called correct with localStream which audio only', function () {
    var callback = jasmine.createSpy('unsubscribe');
    var errorCallback = jasmine.createSpy('unsubscribeError');
    var count = 0;
    for (var index in room.remoteStreams) {
      var stream = room.remoteStreams[index];
      if(room.remoteStreams[index].isMixed()){
      count ++;
      room.unsubscribe(stream, function() {
        callback();
      }, function(err) {
        errorCallback();
          });
    }
    }
    waitsFor(function () {
      return callback.callCount > 0;
    },"stream-unsubscribed should trigged", 20000);
    runs(function () {
      expect(errorCallback).not.toHaveBeenCalled();
      expect(callback).toHaveBeenCalled();
      expect(callback.callCount).toEqual(count);
    });
  });


  it('unpublish stream in room should be called correct with localStream audio only ', function () {
    var removeCallback = jasmine.createSpy("removeCallback");
    var callback = jasmine.createSpy("successCallback");
    var errorCallback = jasmine.createSpy("errorCallback");
    room.unpublish(localStream,function() {
      callback();
    }, function(err) {
      errorCallback();
    });

    room.addEventListener("stream-removed", function(msg) {
      removeCallback();
    });


    waitsFor(function () {
      return removeCallback.callCount > 0;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(removeCallback).toHaveBeenCalled();
    });
  });


  it('disconnect from room should be called correct with localstream with audio only', function () {
    var callback = jasmine.createSpy("connectroom");

    room.addEventListener("server-disconnected", callback);
    room_2.addEventListener("server-disconnected", callback);
    room.leave();
    room_2.leave();

    waitsFor(function () {
      return callback.callCount > 1;
    });

    runs(function () {
      expect(callback).toHaveBeenCalled();
      expect(callback.callCount).toEqual(2);
    });
  });

});//API-Test-Finished

//});
});
