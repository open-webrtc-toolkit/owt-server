/*global require, __dirname, console*/
var express = require('express'),
    bodyParser = require('body-parser'),
    errorhandler = require('errorhandler'),
    morgan = require('morgan'),
    net = require('net'),
    N = require('./nuve'),
    fs = require("fs"),
    https = require("https"),
    config = require('./../../etc/woogeen_config');

var options = {
    key: fs.readFileSync('cert/key.pem').toString(),
    cert: fs.readFileSync('cert/cert.pem').toString()
};

var app = express();

// app.configure ya no existe
"use strict";
app.use(errorhandler({
    dumpExceptions: true,
    showStack: true
}));
app.use(morgan('dev'));
app.use(express.static(__dirname + '/public'));

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
    extended: true
}));

app.use(function (req, res, next) {
  "use strict";
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE');
  res.header('Access-Control-Allow-Headers', 'origin, content-type');
  if (req.method == 'OPTIONS') {
    res.send(200);
  }
  else {
    next();
  }
});

//app.set('views', __dirname + '/../views/');
//disable layout
//app.set("view options", {layout: false});

N.API.init(config.nuve.superserviceID, config.nuve.superserviceKey, 'http://localhost:3000/');

var myRoom;
var p2pRoom;

N.API.getRooms(function(roomlist) {
    "use strict";
    var rooms = JSON.parse(roomlist);
    console.log(rooms.length +' rooms in this service.');
    for (var i = 0; i < rooms.length; i++) {
        if (myRoom === undefined && rooms[i].name === 'myRoom') {
            myRoom = rooms[i]._id;
            console.log('MyRoom Id:', myRoom);
        }
        if (p2pRoom === undefined && rooms[i].mode === 'p2p') {
            p2pRoom = rooms[i]._id;
            console.log('P2PRoom Id:', p2pRoom);
        }
        if (myRoom !== undefined && p2pRoom !== undefined) {
            break;
        }
    }
    var tryCreate = function (room, callback) {
        N.API.createRoom(room.name, function (roomID) {
            console.log('Created room:', roomID._id);
            callback(roomID._id);
        }, function (status, err) {
            console.log('Error in creating room:', err, '[Retry]');
            setTimeout(function(){
                tryCreate(room, callback);
            }, 100);
        }, room);
    };

    var room;
    if (!myRoom) {
        room = {name:'myRoom'};
        tryCreate(room, function (Id) {
            myRoom = Id;
            console.log('myRoom Id:', myRoom);
        });
    }
    if (!p2pRoom) {
        room = {name:'P2P Room', mode:'p2p'};
        tryCreate(room, function (Id) {
            p2pRoom = Id;
            console.log('P2PRoom Id:', p2pRoom);
        });
    }
});


app.get('/getRooms/', function(req, res) {
    "use strict";
    N.API.getRooms(function(rooms) {
        res.send(rooms);
    });
});

app.get('/getUsers/:room', function(req, res) {
    "use strict";
    var room = req.params.room;
    N.API.getUsers(room, function(users) {
        res.send(users);
    });
});


app.post('/createToken/', function(req, res) {
    "use strict";
    var room = req.body.room || myRoom,
        username = req.body.username,
        role = req.body.role;
    if (room === 'p2p') {
        room = p2pRoom;
    }
    N.API.createToken(room, username, role, function(token) {
        console.log(token);
        res.send(token);
    });
});


app.use(function(req, res, next) {
    "use strict";
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE');
    res.header('Access-Control-Allow-Headers', 'origin, content-type');
    if (req.method == 'OPTIONS') {
        res.send(200);
    } else {
        next();
    }
});



app.listen(3001);

var server = https.createServer(options, app);
server.listen(3004);
