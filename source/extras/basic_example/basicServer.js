/*global require, __dirname, console, process*/
'use strict';

var express = require('express'),
    bodyParser = require('body-parser'),
    errorhandler = require('errorhandler'),
    morgan = require('morgan'),
    fs = require('fs'),
    https = require('https'),
    N = require('./nuve');

var app = express();

// app.configure ya no existe
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
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS, DELETE');
    res.header('Access-Control-Allow-Headers', 'origin, content-type');
    if (req.method == 'OPTIONS') {
        res.send(200);
    } else {
        next();
    }
});

//app.set('views', __dirname + '/../views/');
//disable layout
//app.set("view options", {layout: false});

N.API.init('_auto_generated_ID_', '_auto_generated_KEY_', 'http://localhost:3000/');

var myRoom;
var p2pRoom;

N.API.getRooms(function(roomlist) {
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


app.get('/getUsers/:room', function(req, res) {
    var room = req.params.room;
    N.API.getUsers(room, function(users) {
        res.send(users);
    }, function(err) {
        res.send(err);
    });
});

app.post('/createToken/', function(req, res) {
    var room = req.body.room || myRoom,
        username = req.body.username,
        role = req.body.role;
    if (room === 'p2p') {
        room = p2pRoom;
    }
    N.API.createToken(room, username, role, function(token) {
        res.send(token);
    },function (err){
        res.send(err);
    });
});


app.post('/createRoom/', function (req, res) {
    'use strict';
    var name = req.body.name;
    N.API.createRoom(name, function (response) {
        res.send(response);
    },function (err) {
        res.send(err);
    });
});
app.get('/getRooms/', function (req, res) {
    'use strict';
    N.API.getRooms(function (rooms) {
        res.send(rooms);
    },function (err) {
        res.send(err);
    });
});

app.get('/getRoom/:room', function (req, res) {
    'use strict';
    var room = req.params.room;
    N.API.getRoom(room,function (rooms) {
        res.send(rooms);
    },function (err) {
        res.send(err);
    });
});
app.get('/getUsers/:room', function (req, res) {
    'use strict';
    var room = req.params.room;
    N.API.getUsers(room, function (users) {
        res.send(users);
    },function (err) {
        res.send(err);
    });
});

app.get('/room/:room/user/:user', function (req, res) {
    'use strict';
    var room = req.params.room;
    var uid = req.params.user;
    N.API.getUser(room, uid, function (user) {
        res.send(user);
    },function (err) {
        res.send(err);
    });
});

app.delete('/room/:room/user/:user', function (req, res) {
    'use strict';
    var room = req.params.room;
    var uid = req.params.user;
    N.API.deleteUser(room, uid, function (result) {
        res.send(result);
    },function (err) {
        res.send(err);
    });
})

app.delete('/room/:room', function (req, res) {
    'use strict';
    var room = req.params.room;
    N.API.deleteRoom(room, function (result) {
        res.send(result);
    },function (err) {
        res.send(err);
    });
})

app.listen(3001);

var cipher = require('../../common/cipher');
cipher.unlock(cipher.k, '../../cert/.woogeen.keystore', function cb (err, obj) {
    if (!err) {
        try {
            https.createServer({
                pfx: fs.readFileSync('../../cert/certificate.pfx'),
                passphrase: obj.sample
            }, app).listen(3004);
        } catch (e) {
            err = e;
        }
    }
    if (err) {
        console.error('Failed to setup secured server:', err);
        return process.exit();
    }
});
