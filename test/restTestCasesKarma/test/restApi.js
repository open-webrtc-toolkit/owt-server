const {
    expect
} = require('chai');
const {
    createLocal,
    join,
    publish,
    publishCase,
    subscribe,
} = require('./jssdk.js');
const {
    ConferenceClient
} = Owt.Conference;
const {
    MediaStreamFactory,
    Resolution,
    StreamSourceInfo,
    LocalStream,
    RemoteStream,
} = Owt.Base;
const config = require('../scripts/config.js');
function send(method, entity, body) {
    return new Promise(function (resolve, reject) {
        let req = new XMLHttpRequest();
        req.onreadystatechange = function () {
            if (req.readyState === 4) {
                console.log('readyState is: ', req.readyState, req.status);
                if (req.status === 200) {
                    try {
                        let resp = JSON.parse(req.responseText);
                        console.log('http request success(resp is JSON): ');
                        resolve(resp);
                    } catch (e) {
                        console.log('http request success(resp is not JSON): ');
                        resolve(req.responseText);
                    }
                } else {

                    reject(req.responseText);
                }
            }
        };
        console.log('send: ', method, config.serverUrl + entity, JSON.stringify(body));
        req.open(method, config.serverUrl + entity, true);
        req.setRequestHeader('Content-Type', 'application/json');
        if (body !== undefined) {
            req.send(JSON.stringify(body));
        } else {
            req.send();
        }
    })
};

function send3000(method, entity, body) {
    return new Promise(function (resolve, reject) {
        let req = new XMLHttpRequest();
        req.onreadystatechange = function () {
            if (req.readyState === 4) {
                console.log('readyState is: ', req.readyState, req.status);
                if (req.status === 200) {
                    try {
                        let resp = JSON.parse(req.responseText);
                        console.log('http request success(resp is JSON): ');
                        resolve(resp);
                    } catch (e) {
                        console.log('http request success(resp is not JSON): ');
                        resolve(req.responseText);
                    }
                } else {

                    reject(req.responseText);
                }
            }
        };
        console.log('send: ', method, config.serverUrl3000 + entity, JSON.stringify(body));
        req.open(method, config.serverUrl3000 + entity, true);
        req.setRequestHeader('Content-Type', 'application/json');
        req.setRequestHeader('authorization', config.authorization);
        if (body !== undefined) {
            req.send(JSON.stringify(body));
        } else {
            req.send();
        }
    })
};

let listRooms = function () {
    return send('GET', '/rooms/');
};

let getRoom = function (room) {
    return send('GET', '/rooms/' + room + '/');
};

let createRoom = function (options) {
    return send('POST', '/rooms/', options);
};

let deleteRoom = function (room) {
    return new Promise(function (resolve, reject) {
        send('DELETE', '/rooms/' + room + '/')
            .then(resp => {
                setTimeout(() => { resolve(resp) }, 5000)

            }, err => {
                console.log("deleroom err", err)
                setTimeout(() => { reject(err) }, 5000)


            })
    });
};

let updateRoom = function (room, config) {
    return send('PUT', '/rooms/' + room + '/', config);
};

let listParticipants = function (room) {
    return send('GET', '/rooms/' + room + '/participants/');
};

let getParticipant = function (room, participant) {
    return send('GET', '/rooms/' + room + '/participants/' + participant + '/');
};

let forbidSub = function (room, participant, jsonPatch) {
    return send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch);
};

let forbidPub = function (room, participant, jsonPatch) {
    return send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch);
};

let dropParticipant = function (room, participant) {
    return send('DELETE', '/rooms/' + room + '/participants/' + participant + '/');
};

let listStreams = function (room) {
    return send('GET', '/rooms/' + room + '/streams/', onerror);
};

let getStream = function (room, stream) {
    return send('GET', '/rooms/' + room + '/streams/' + stream);
};

let mixStream = function (room, stream, view) {
    let jsonPatch = [{
        op: 'add',
        path: '/info/inViews',
        value: view
    }];
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let unmixStream = function (room, stream, view) {
    let jsonPatch = [{
        op: 'remove',
        path: '/info/inViews',
        value: view
    }];
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let setRegion = function (room, stream, region, subStream) {
    let jsonPatch = [{
        op: 'replace',
        path: '/info/layout/0/stream',
        value: subStream
    }];
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let updateLayout = function (room, stream, Layout) {
    let jsonPatch = [{
        op: "replace",
        path: "/info/layout",
        value: Layout
    }]
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let pauseStream = function (room, stream, track) {
    let jsonPatch = [];
    if (track === 'audio' || track === 'av') {
        jsonPatch.push({
            op: 'replace',
            path: '/media/audio/status',
            value: 'inactive'
        });
    }

    if (track === 'video' || track === 'av') {
        jsonPatch.push({
            op: 'replace',
            path: '/media/video/status',
            value: 'inactive'
        });
    }
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let playStream = function (room, stream, track) {
    let jsonPatch = [];
    if (track === 'audio' || track === 'av') {
        jsonPatch.push({
            op: 'replace',
            path: '/media/audio/status',
            value: 'active'
        });
    }

    if (track === 'video' || track === 'av') {
        jsonPatch.push({
            op: 'replace',
            path: '/media/video/status',
            value: 'active'
        });
    }
    return send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch);
};

let dropStream = function (room, stream) {
    return send('DELETE', '/rooms/' + room + '/streams/' + stream);
};

let startStreamingIn = function (room) {
    let options = {
        url: 'rtspUrl',
        media: {
            audio: 'auto',
            video: true
        },
        transport: {
            protocol: 'udp',
            bufferSize: 2048
        }
    };
    options.url = config.rtspUrl;
    return send('POST', '/rooms/' + room + '/streaming-ins', options);
};

let stopStreamingIn = function (room, stream) {
    return send('DELETE', '/rooms/' + room + '/streaming-ins/' + stream);
};

let listRecordings = function (room) {
    return send('GET', '/rooms/' + room + '/recordings/');
};

let startRecording = function (room, options) {
    return send('POST', '/rooms/' + room + '/recordings', options);
};

let stopRecording = function (room, id) {
    return send('DELETE', '/rooms/' + room + '/recordings/' + id);
};

let updateRecording = function (room, id, jsonPatch) {
    /*   let jsonPatch = [{
           op: 'replace',
           path: '/media/audio/from',
           value: audioFrom
       }, {
           op: 'replace',
           path: '/media/video/from',
           value: videoFrom
       }];*/
    return send('PATCH', '/rooms/' + room + '/recordings/' + id, jsonPatch);
};

let listStreamingOuts = function (room) {
    return send3000('GET', '/rooms/' + room + '/streaming-outs/');
};

let startStreamingOut = function (room, url, audioFrom, videoFrom) {
    let options = {
        media: {
            audio: {
                from: audioFrom
            },
            video: {
                from: videoFrom
            }
        },
        url: url
    };
    return send('POST', '/rooms/' + room + '/streaming-outs', options);
};

let stopStreamingOut = function (room, id) {
    return send('DELETE', '/rooms/' + room + '/streaming-outs/' + id);
};

let updateStreamingOut = function (room, id, audioFrom, videoFrom) {
    let jsonPatch = [{
        op: 'replace',
        path: '/media/audio/from',
        value: audioFrom
    }, {
        op: 'replace',
        path: '/media/video/from',
        value: videoFrom
    }];
    return send('PATCH', '/rooms/' + room + '/streaming-outs/' + id, jsonPatch);
};

let listAnalytics = function (room) {
    return send3000('GET', '/v1/rooms/' + room + '/analytics');
};

let startAnalytics = function (room, jsonPost) {
    return send3000('POST', '/v1/rooms/' + room + '/analytics', jsonPost);
};

let stopAnalytics = function (room, analyticId) {
    return send3000('DELETE', '/v1/rooms/' + room + '/analytics/' + analyticId)
};

let listSipcalls = function (room) {
    return send('GET', '/rooms/' + room + '/sipcalls');
};
let startSipcalls = function (room, jsonPost) {
    return send('POST', '/rooms/' + room + '/sipcalls', jsonPost);
};

let updateSipcalls = function (room, sipcallsId, jsonPatch) {
    return send('PATCH', '/rooms/' + room + '/sipcalls/' + sipcallsId, jsonPatch);
}

let stopSipcalls = function (room, sipcallsId) {
    return send('DELETE', '/rooms/' + room + '/sipcalls/' + sipcallsId)
};


let createToken = function (room, user, role) {
    let body = {
        room: room,
        user: user,
        role: role
    };
    console.log('createToken: ', room, user, role);
    return send('POST', '/tokens/', body);
};
var createRoomX = (() => {
    let roomId;
    return createRoom({
        name: 'citest',
        options: {
            views: [{
                name: 'common',
                video: {
                }
            }]
        }
    })
})

var joinX = (() => {
    return createRoom({
        name: 'citest',
        options: {
            views: [{
                name: 'common',
                video: {
                }
            }]
        }
    })
        .then((room) => {
            roomId = room._id;
            return createToken(room._id, 'testuser', 'presenter')
        })
        .then((token) => {
            return join(token)
        })
})

var sipcallsX = (() => {
    return createRoom({
        name: 'citest2',
        options: {
            sip: {
                "sipServer": config.sipServer,
                "username": config.username,
                "password": config.password
            },
            views: [{
                name: 'common',
                video: {
                }
            }]
        }
    })
        .then((room) => {
            roomId1 = room._id;
            return createToken(room._id, 'testuser1', 'presenter')
        })
        .then((token) => {
            return join(token)
        })
        .then(() => {
            return createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId1)
                })
        })
})
var sipcallsXX = (() => {
    return createRoom({
        name: 'citest',
        options: {
            sip: {
                "sipServer": config.sipServer,
                "username": config.username2,
                "password": config.password
            },
            views: [{
                name: 'common',
                video: {
                }
            }]
        }
    })
        .then((room) => {
            roomId2 = room._id;
            return createToken(room._id, 'testuser2', 'presenter')
        })
        .then((token) => {
            return join(token)
        })
        .then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId2)
                })
        })
})
module.exports = {
    expect,
    ConferenceClient,
    MediaStreamFactory,
    Resolution,
    StreamSourceInfo,
    LocalStream,
    RemoteStream,
    config,
    listRooms,
    getRoom,
    createRoom,
    deleteRoom,
    updateRoom,
    listParticipants,
    getParticipant,
    forbidSub,
    forbidPub,
    dropParticipant,
    listStreams,
    getStream,
    mixStream,
    unmixStream,
    setRegion,
    updateLayout,
    pauseStream,
    playStream,
    dropStream,
    startStreamingIn,
    stopStreamingIn,
    listRecordings,
    startRecording,
    stopRecording,
    updateRecording,
    listStreamingOuts,
    startStreamingOut,
    stopStreamingOut,
    updateStreamingOut,
    listAnalytics,
    startAnalytics,
    stopAnalytics,
    listSipcalls,
    startSipcalls,
    updateSipcalls,
    stopSipcalls,
    createToken,
    createRoomX,
    joinX,
    sipcallsX,
    sipcallsXX,
}

