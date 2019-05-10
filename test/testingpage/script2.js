let publicationTmp;
let resolutionName2Value = {
    'cif': {
        width: 352,
        height: 288
    },
    'vga': {
        width: 640,
        height: 480
    },
    'svga': {
        width: 800,
        height: 600
    },
    'xga': {
        width: 1024,
        height: 768
    },
    'r640x360': {
        width: 640,
        height: 360
    },
    'hd720p': {
        width: 1280,
        height: 720
    },
    'sif': {
        width: 320,
        height: 240
    },
    'hvga': {
        width: 480,
        height: 320
    },
    'r480x360': {
        width: 480,
        height: 360
    },
    'qcif': {
        width: 176,
        height: 144
    },
    'r192x144': {
        width: 192,
        height: 144
    },
    'hd1080p': {
        width: 1920,
        height: 1080
    },
    'uhd_4k': {
        width: 3840,
        height: 2160
    },
    'r360x360': {
        width: 360,
        height: 360
    },
    'r480x480': {
        width: 480,
        height: 480
    },
    'r720x720': {
        width: 720,
        height: 720
    },
    'r1080x1920': {
        width: 1080,
        height: 1920
    },
    'r720x1280': {
        width: 720,
        height: 1280
    },
    'invaild': {
        width: 333,
        height: 222
    },
    'unbelievable': {
        width: 1111,
        height: 100,
    },
};
let defaultRoomId = '',
    role = '',
    isPublish,
    resolution = '',
    videoCodec = '',
    audioCodec = '',
    hasVideo,
    hasAudio,
    isJoin,
    isSignaling,
    mix;
defaultRoomId = getParameterByName("room") || defaultRoomId;
role = getParameterByName('role') || 'presenter';
isPublish = getParameterByName('publish') === 'false' ? false : true;
resolution = resolutionName2Value[getParameterByName('resolution') || 'vga'];
videoCodec = getParameterByName('videoCodec') || 'h264';
audioCodec = getParameterByName('audioCodec') || 'opus';
hasVideo = getParameterByName('hasVideo') === 'false' ? false : true;
hasAudio = getParameterByName('hasAudio') === 'false' ? false : true;
isJoin = getParameterByName('join') === 'false' ? false : true;
mix = getParameterByName('mix') === 'false' ? false : true;
isSignaling = getParameterByName('signaling') === 'true' ? true : false;
videoMaxBitrate = parseInt(getParameterByName('videoMaxBitrate')) || 800;
audioMaxBitrate = parseInt(getParameterByName('audioMaxBitrate')) || undefined;
hasForward = getParameterByName('forward') === 'false' ? false : true;
hasMixed = getParameterByName('mixed') === 'false' ? false : true;


class EventMap {
    constructor() {
        this._store = new Map();
        this._addListeners = [];
        this._removeListeners = [];
    }
    get(key) {
        return this._store.get(key);
    }

    set(key, value) {
        if (this._store.has(key)) {
            return
        }
        this._store.set(key, value);
        let len = this._addListeners.length
        if (len > 0) {
            for (let i = 0; i < len; i++) {
                this._addListeners[i](key, value);
            }
        }
    }

    delete(key) {
        this._store.delete(key);
        let len = this._removeListeners.length
        if (len > 0) {
            for (let i = 0; i < len; i++) {
                this._removeListeners[i](key);
            }
        }
    }
    forEach(cb) {
        this._store.forEach(cb);
    }
    getMap() {
        return this._store
    }
    clear() {
        this._store.clear();
    }
    onLengthAdd(listener) {
        this._addListeners.push(listener);
    }

    onLengthRemove(listener) {
        this._removeListeners.push(listener)
    }
}

let {
    LocalStream,
    MediaStreamFactory,
    StreamSourceInfo,
    StreamConstraints,
    MediaStreamDeviceConstraints,
    AudioTrackConstraints,
    VideoTrackConstraints,
    Resolution,
} = Owt.Base, {
    ConferenceClient,
    SioSignaling
} = Owt.Conference;

let ccOptions = {
    rtcConfiguration: {
        iceServers: [{
            urls: ['turn:47.100.173.136:443?transport=tcp'],
            credential: 'intel123',
            username: 'webrtc'
        }]
    }
};
let signaling = new SioSignaling({
    transports: ['websocket'],
});
let client = isSignaling ? new ConferenceClient(ccOptions, signaling) : new ConferenceClient(),
    localStreams = new EventMap(),
    remoteStreamMap = new EventMap(),
    subscriptions = new EventMap(),
    publications = new EventMap(),
    participants = new EventMap();
let allRooms = new EventMap();
let allSipcalls = new EventMap();


var send = function (method, entity, body, okCallback, errCallback) {
    var req = new XMLHttpRequest();
    req.onreadystatechange = function () {
        if (req.readyState === 4) {
            if (req.status === 200) {
                okCallback(req.responseText);
            } else {
                errCallback(req.responseText);
            }
        }
    };
    req.open(method, entity, true);
    req.setRequestHeader('Content-Type', 'application/json');
    if (body !== undefined) {
        req.send(JSON.stringify(body));
    } else {
        req.send();
    }
};

var onRespone = function (result) {
    if (result) {
        try {
            L.Logger.info('Result:', JSON.parse(result));
        } catch (e) {
            L.Logger.info('Result:', result);
        }
    } else {
        L.Logger.info('Null');
    }
};

var listRooms = function (ok_cb, err_cb) {
    send('GET', '/rooms/', undefined, ok_cb, err_cb);
};

var getRoom = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/', undefined, ok_cb, err_cb);
};

var createRoom = function (name = 'testNewRoom', options, ok_cb, err_cb) {
    //options.name = name;
    send('POST', '/rooms/', {
        name: name,
        options: options
    }, ok_cb, err_cb);
};

var deleteRoom = function (room, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/', undefined, ok_cb, err_cb);
};

var updateRoom = function (room, config, ok_cb, err_cb) {
    send('PUT', '/rooms/' + room + '/', config, ok_cb, err_cb);
};

var listSipcalls = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/sipcalls/', undefined, ok_cb, err_cb);
};

var createSipcalls = function (room, peerURI, mediaIn, mediaOut, ok_cb, err_cb) {
    send('post', '/rooms/' + room + '/sipcalls/', {
        peerURI: peerURI,
        mediaIn: mediaIn,
        mediaOut: mediaOut

    }, ok_cb, err_cb);
};

var deleteSipcalls = function (room, sipcalld, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/sipcalls/' + sipcalld + '/', undefined, ok_cb, err_cb);
};

var updateSipcalls = function (room, sipCalld, updateOptions, ok_cb, err_cb) {
    send('PATCH', '/rooms/' + room + '/sipcalls/' + sipCalld, updateOptions, ok_cb, err_cb);
};

var listParticipants = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/participants/', undefined, ok_cb, err_cb);
};

var getParticipant = function (room, participant, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/participants/' + participant + '/', undefined, ok_cb, err_cb);
};

function getForbidOptions() {
    let audio = $('#audioforbidoptions').val() === 'false' ? false : true;
    let video = $('#videoforbidoptions').val() === 'false' ? false : true;
    return { audio, video }
}
var forbidSub = function (room, participant, ok_cb, err_cb) {
    let { audio, video } = getForbidOptions();
    var jsonPatch = [{
        op: 'replace',
        path: '/permission/subscribe',
        value: {
            audio: audio,
            video: video
        }
    }];
    send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch, ok_cb, err_cb);
};

var forbidPub = function (room, participant, ok_cb, err_cb) {
    let { audio, video } = getForbidOptions();
    var jsonPatch = [{
        op: 'replace',
        path: '/permission/publish',
        value: {
            audio: audio,
            video: video
        }
    }];
    send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch, ok_cb, err_cb);
};

var dropParticipant = function (room, participant, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/participants/' + participant + '/', undefined, ok_cb, err_cb);
};

var listStreams = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streams/', undefined, ok_cb, err_cb, onerror);
};

var getStream = function (room, stream, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streams/' + stream, undefined, ok_cb, err_cb);
};

var mixStream = function (room, stream, view, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'add',
        path: '/info/inViews',
        value: view
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var unmixStream = function (room, stream, view, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'remove',
        path: '/info/inViews',
        value: view
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var setRegion = function (room, stream, region, subStream, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: `/info/layout/${region}/stream`,
        value: subStream
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var pauseStream = function (room, stream, track, ok_cb, err_cb) {
    var jsonPatch = [];
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
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var playStream = function (room, stream, track, ok_cb, err_cb) {
    var jsonPatch = [];
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
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var dropStream = function (room, stream, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streams/' + stream, undefined, ok_cb, err_cb);
};

var startStreamingIn = function (room, url, ok_cb, err_cb) {
    var options = {
        url: url,
        media: {
            audio: 'auto',
            video: true
        },
        transport: {
            protocol: 'tcp',
            bufferSize: 4096
        }
    };
    send('POST', '/rooms/' + room + '/streaming-ins', options, ok_cb, err_cb);
};

var stopStreamingIn = function (room, stream, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streaming-ins/' + stream, undefined, ok_cb, err_cb);
};

var listRecordings = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/recordings/', undefined, ok_cb, err_cb);
};

var startRecording = function (room, options, ok_cb, err_cb) {
    send('POST', '/rooms/' + room + '/recordings', options, ok_cb, err_cb);
};

var stopRecording = function (room, id, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/recordings/' + id, undefined, ok_cb, err_cb);
};

var updateRecording = function (room, id, updateOptions, ok_cb, err_cb) {
    send('PATCH', '/rooms/' + room + '/recordings/' + id, updateOptions, ok_cb, err_cb);
};

var startAnalyzing = function (room, options, ok_cb, err_cb) {
    send('POST', '/rooms/' + room + '/analytics', options, ok_cb, err_cb);
};

var stopAnalyzing = function (room, id, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/analytics/' + id, undefined, ok_cb, err_cb);
};

var listStreamingOuts = function (room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streaming-outs/', undefined, ok_cb, err_cb);
};

var startStreamingOut = function (room, url, mediaOptions, ok_cb, err_cb) {
    var options = {
        media: mediaOptions,
        url: url
    };
    send('POST', '/rooms/' + room + '/streaming-outs', options, ok_cb, err_cb);
};

var stopStreamingOut = function (room, id, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streaming-outs/' + id, undefined, ok_cb, err_cb);
};

var updateStreamingOut = function (room, id, updateOptions, ok_cb, err_cb) {
    send('PATCH', '/rooms/' + room + '/streaming-outs/' + id, updateOptions, ok_cb, err_cb);
};

var createToken = function (room, user, role, ok_cb, err_cb) {
    var body = {
        room: room,
        user: user,
        role: role
    };
    send('POST', '/tokens/', body, ok_cb, err_cb);
};

//restful action

function getRestfulParmas() {
    let roomId = $('#roomlist').val() || defaultRoomId,
        participantId = $('#participantlist').val(),
        forwardStreamId = $('#forwardstreamlist').val(),
        view = $('#viewlist').val(),
        trackKind = $('#resttrackkind').val(),
        mixedStreamId = $('#mixstreamlist').val(),
        audioFrom = $('#audiofromlist').val(),
        videoFrom = $('#videofromlist').val(),
        rtspUrl = $('#rtmpin').val() || $('#rtspurl').val(),
        rtmpUrl = $('#rtmpurl').val(),
        recorderId = $('#recorderid').val(),
        audioActive = $('#audioactive').val(),
        videoActive = $('#videoactive').val(),
        regionId = $('#regionid').val(),
        container = $('#container').val(),
        streamingOutId = $('#streamingoutid').val();
        algorithm = $('#algorithm').val(),
        analyticsId = $('#analyticsid').val(),
    sipcalld = $('#sipcallslist').val();
    peerURI = $('#peerURI').val();
    return {
        roomId,
        participantId,
        forwardStreamId,
        view,
        mixedStreamId,
        trackKind,
        audioFrom,
        videoFrom,
        rtspUrl,
        rtmpUrl,
        recorderId,
        regionId,
        streamingOutId,
        algorithm,
        analyticsId,
        audioActive,
        videoActive,
        container,
        sipcalld,
        peerURI,
    }
}
////room action
function restListRooms() {
    listRooms((rooms) => {
        rooms = JSON.parse(rooms);
        rooms.forEach((item) => {
            if (item.name === 'sampleRoom' && !defaultRoomId) defaultRoomId = item._id;
            allRooms.set(item._id, item);
        })
        console.log('list rooms sucess: ', rooms);
    }, (err) => {
        console.log('list rooms failed', err);
    })
}
restListRooms();

function restGetRoom() {
    let {
        roomId
    } = getRestfulParmas();
    getRoom(roomId, (room) => {
        room = JSON.parse(room);
        console.log('get room success: ', room);
    }, (err) => {
        console.log('get room failed: ', err);
    })
}

function restCreateRoom() {
    let options = {
        views: [{
            label: 'common1',
            audio: null,
            video: {
                parameters: {
                    resolution: {
                        width: 1280,
                        height: 720
                    }
                }
            }
        }]
    }
    createRoom('testCreateRoom', options, (room) => {
        room = JSON.parse(room);
        console.log('create room success: ', room);
    }, (err) => {
        console.log('create room failed: ', err);
    })
}

function restDeleteRoom() {
    let {
        roomId
    } = getRestfulParmas();
    deleteRoom(roomId, (resp) => {
        allRooms.delete(roomId);
        console.log(`delete room ${roomId} success: `, resp);
    }, err => {
        console.log('delete room failed: ', err);
    })
}

function restUpdateRoom() {
    let {
        roomId
    } = getRestfulParmas();
    let config = {
        sip: {
            sipServer: "10.239.44.36",
            username: "jp1",
            password: "123"
        },

    };

    updateRoom(roomId, config, (resp) => {
        console.log(`update room ${roomId} success: `, resp);
    }, err => {
        console.log('update room success: ', err);
    })
}

//sipcalls
function restListSipcalls() {
    let {
        roomId
    } = getRestfulParmas();
    listSipcalls(roomId, (sipcalls) => {
        sipcalls = JSON.parse(sipcalls);
        sipcalls.forEach((item) => {
            //  if (item.name === 'sampleRoom' && !defaultRoomId) defaultRoomId = item._id;
            allSipcalls.set(item.id, item);
        })
        console.log('list sipcalls sucess: ', sipcalls);
    }, (err) => {
        console.log('list sipcalls failed', err);
    })
}


function restCreateSipcalls() {
    let {
        roomId
    } = getRestfulParmas();

    let mediaIn = {
        audio: true,
        video: true
    }
    let mediaOut = {
        audio: {
            from: roomId + "-common"



        },
        video: {
            from: roomId + "-common",

            parameters: {
                resolution: {
                    width: 320,
                    height: 240
                },
                framerate: 6,
                bitrate: "x0.2",
                keyFrameInterval: 1
            }
        }
    }
    createSipcalls(roomId, peerURI, mediaIn, mediaOut, (sipcall) => {
        sipcall = JSON.parse(sipcall);
        console.log('create sipcall success: ', sipcall);
    }, (err) => {
        console.log('create sipcall failed: ', err);
    })
}

function restDropSipcalls() {
    let {
        roomId,
        sipcalld
    } = getRestfulParmas();
    deleteSipcalls(roomId, sipcalld, (resp) => {
        //allSipcalls.delete(sipcalld);
        console.log(`delete Sipcalls ${sipcalld} success: `, resp);
    }, err => {
        console.log('delete Sipcalls failed: ', err);
    })
}


function restUpdateSipcalls() {
    let {
        roomId,
        sipcalld,
        audioFrom,
        videoFrom,
    } = getRestfulParmas();
    let {
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();


    let updateOptions = [
        {
            "op": "replace",
            "path": "/output/media/audio/from",
            "value": roomId + "-common"

        },
        {
            "op": "replace",
            "path": "/output/media/video/from",
            "value": roomId + "-common"

        }
    ];

    if (bitrate) {
        let subscriptionControlInfo = {
            "op": "replace",
            "path": "/output/media/video/parameters/bitrate",


            "value": 'x' + bitrate

        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (resolution) {
        let subscriptionControlInfo = {
            "op": "replace",
            "path": "/output/media/video/parameters/resolution",
            "value": resolution

        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (frameRate) {
        let subscriptionControlInfo = {
            "op": "replace",
            "path": "/output/media/video/parameters/framerate",

            "value": frameRate

        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (kfi) {
        let subscriptionControlInfo = {
            "op": "replace",
            "path": "/output/media/video/parameters/keyFrameInterval",


            "value": kfi

        };
        updateOptions.push(subscriptionControlInfo);
    }
    updateSipcalls(roomId, sipcalld, updateOptions, (resp) => {
        console.log('update rtmp out success: ', resp);
    }, err => {
        console.log('update rtmp out failed: ', err);
    })
}
////participant action
function restListParticipants() {
    let {
        roomId
    } = getRestfulParmas();
    listParticipants(roomId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`get ${roomId} participants: `, resp);
        resp.forEach((item) => {
            participants.set(item.id, item);
        });
    }, err => {
        console.log(`get ${roomId} participants failed: `, err);
    })
}

function restGetParticipant() {
    let {
        roomId,
        participantId
    } = getRestfulParmas();
    getParticipant(roomId, participantId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`get participant ${participantId} success: `, resp);
    }, err => {
        console.log(`get participant ${participantId} failed`, err);
    })
}

function restForbidSub() {
    let {
        roomId,
        participantId
    } = getRestfulParmas();
    forbidSub(roomId, participantId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`forbid participant ${participantId} subscribe ability success: `, resp);
    }, err => {
        console.log(`forbid participant ${participantId} subscribe ability failed`, err);
    })
}

function restForbidPub() {
    let {
        roomId,
        participantId
    } = getRestfulParmas();
    forbidPub(roomId, participantId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`forbid participant ${participantId} publish ability success: `, resp);
    }, err => {
        console.log(`forbid participant ${participantId} publish ability failed`, err);
    })
}

function restDropParticipant() {
    let {
        roomId,
        participantId
    } = getRestfulParmas();
    dropParticipant(roomId, participantId, (resp) => {
        console.log(`drop participant ${participantId} success: `, resp);
    }, err => {
        console.log(`drop participant ${participantId} failed`, err);
    })
}
////stream actions
function restListStreams() {
    let {
        roomId
    } = getRestfulParmas();
    listStreams(roomId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`get streams success: `, resp);
        resp.forEach((item) => {
            remoteStreamMap.set(item.id, item);
        })
    }, err => {
        console.log('get streams failed: ', err);
    })
}

function restGetStream() {
    let {
        roomId,
        forwardStreamId
    } = getRestfulParmas();
    getStream(roomId, forwardStreamId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`get stream ${forwardStreamId} success: `, resp);
    }, err => {
        console.log(`get stream ${forwardStreamId} failed: `, err);
    })
}

function restDropStream() {
    let {
        roomId,
        forwardStreamId
    } = getRestfulParmas();
    dropStream(roomId, forwardStreamId, (resp) => {
        console.log(`drop stream ${forwardStreamId} success: `, resp);
    }, err => {
        console.log(`drop stream ${forwardStreamId} failed: `, err);
    })
}

function restSetRegion() {
    let {
        roomId,
        mixedStreamId,
        regionId,
        forwardStreamId
    } = getRestfulParmas();
    setRegion(roomId, mixedStreamId, regionId, forwardStreamId, (resp) => {
        console.log('set region success: ', resp);
    }, err => {
        console.log('set region failed: ', err);
    });

}

function restPauseStream() {
    let {
        roomId,
        forwardStreamId,
        trackKind
    } = getRestfulParmas();
    pauseStream(roomId, forwardStreamId, trackKind, (resp) => {
        resp = JSON.parse(resp);
        console.log(`pause stream ${forwardStreamId} success: `, resp);
    }, err => {
        console.log(`pause stream ${forwardStreamId} failed: `, err);
    })
}

function restPlayStream() {
    let {
        roomId,
        forwardStreamId,
        trackKind
    } = getRestfulParmas();
    playStream(roomId, forwardStreamId, trackKind, (resp) => {
        resp = JSON.parse(resp);
        console.log(`play stream ${forwardStreamId} success: `, resp);
    }, err => {
        console.log(`play stream ${forwardStreamId} failed: `, err);
    })
}

function restMixStream() {
    let {
        roomId,
        forwardStreamId,
        view
    } = getRestfulParmas();
    mixStream(roomId, forwardStreamId, view, (resp) => {
        resp = JSON.parse(resp);
        console.log(`mix stream ${forwardStreamId} to ${view} success: `, resp);
    }, err => {
        console.log(`mix stream ${forwardStreamId} to ${view} failed: `, err);
    })
}

function restUnmixStream() {
    let {
        roomId,
        forwardStreamId,
        view
    } = getRestfulParmas();
    unmixStream(roomId, forwardStreamId, view, (resp) => {
        resp = JSON.parse(resp);
        console.log(`mix stream ${forwardStreamId} to ${view} success: `, resp);
    }, err => {
        console.log(`mix stream ${forwardStreamId} to ${view} failed: `, err);
    })
}
////streaming in or out action
function restStartStreamingIn() {
    let {
        roomId,
        rtspUrl
    } = getRestfulParmas();
    startStreamingIn(roomId, rtspUrl, (resp) => {
        console.log('start rtsp in success: ', resp);
    }, err => {
        console.log('start rtsp in failed: ', err);
    })
}

function restStopStreamingIn() {
    let {
        roomId,
        forwardStreamId
    } = getRestfulParmas();
    stopStreamingIn(roomId, forwardStreamId, (resp) => {
        console.log('stop rtsp in success: ', resp);
    }, err => {
        console.log('stop rtsp in failed: ', err);
    })
}

function restStartStreamingOut() {
    let {
        roomId,
        rtmpUrl,
        audioFrom,
        videoFrom
    } = getRestfulParmas();
    let {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
        subscriptionId,
    } = getSubOptions();
    let mediaOptions = {
        audio: false,
        video: false,
    };
    if (audioFrom) {
        mediaOptions.audio = {
            from: audioFrom,
        }
        if (audioCodec) {
            mediaOptions.audio.format = {
                codec: audioCodec.name,
                channelNum: audioCodec.channelCount,
                sampleRate: audioCodec.clockRate,
            };
        }
    }
    if (videoFrom) {
        mediaOptions.video = {
            from: videoFrom,
        }
        if (videoCodec) {
            mediaOptions.video.format = {
                codec: videoCodec.name,
                profile: videoCodec.profile,
            }
        }
        if (bitrate || resolution || frameRate || kfi) {
            mediaOptions.video.parameters = {
                resolution,
                bitrate: bitrate ? 'x' + bitrate : undefined,
                framerate: frameRate,
                keyFrameInterval: kfi,
            }
        }
    }
    startStreamingOut(roomId, rtmpUrl, mediaOptions, (resp) => {
        console.log('start rtmp out success: ', resp);
        $('#streamingoutid').val(JSON.parse(resp).id);
    }, err => {
        console.log('start rtmp out failed: ', err);
    })
}

function restStopStreamingOut() {
    let {
        roomId,
        streamingOutId
    } = getRestfulParmas();
    stopStreamingOut(roomId, streamingOutId, (resp) => {
        console.log('stop rtmp out success: ', resp);
    }, err => {
        console.log('stop rtmp out failed: ', err);
    })
}

function restListStreamingOuts() {
    let {
        roomId
    } = getRestfulParmas();
    listStreamingOuts(roomId, (resp) => {
        console.log('list streaming out success: ', resp);
    }, err => {
        console.log('list streaming out failed: ', err);
    })
}

function restUpdateStreamingOut() {
    let {
        roomId,
        streamingOutId,
        audioFrom,
        videoFrom,
        audioActive,
        videoActive,
    } = getRestfulParmas();
    let {
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let updateOptions = [];
    if (audioActive) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/audio/status',
            value: audioActive,
        };
        updateOptions.push(subscriptionControlInfo);
    } else {
        if (audioFrom) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/audio/from',
                value: audioFrom,
            };
            updateOptions.push(subscriptionControlInfo);
        }
    }
    if (videoActive) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/status',
            value: videoActive,
        };
        updateOptions.push(subscriptionControlInfo);
    } else {
        if (videoFrom) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/video/from',
                value: videoFrom,
            };
            updateOptions.push(subscriptionControlInfo);
        }
        if (bitrate) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/video/parameters/bitrate',
                value: 'x' + bitrate,
            };
            updateOptions.push(subscriptionControlInfo);
        }
        if (resolution) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/video/parameters/resolution',
                value: resolution,
            };
            updateOptions.push(subscriptionControlInfo);
        }
        if (frameRate) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/video/parameters/framerate',
                value: frameRate,
            };
            updateOptions.push(subscriptionControlInfo);
        }
        if (kfi) {
            let subscriptionControlInfo = {
                op: 'replace',
                path: '/media/video/parameters/keyFrameInterval',
                value: kfi,
            };
            updateOptions.push(subscriptionControlInfo);
        }
    }

    updateStreamingOut(roomId, streamingOutId, updateOptions, (resp) => {
        console.log('update rtmp out success: ', resp);
    }, err => {
        console.log('update rtmp out failed: ', err);
    })
}
////recording
function restStartRecording() {
    let {
        roomId,
        audioFrom,
        videoFrom,
        container,
    } = getRestfulParmas();
    let {
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let options = {
        media: {
            audio: false,
            video: false,
        },
        container: container,
    }
    if (audioFrom) {
        options.media.audio = {
            from: audioFrom,
        }
        if (audioCodec) {
            options.media.audio.format = {
                codec: audioCodec.name,
                sampleRate: audioCodec.clockRate,
                channelNum: audioCodec.channelCount,
            }
        }
    }
    if (videoFrom) {
        options.media.video = {
            from: videoFrom,
        }
        if (videoCodec) {
            options.media.video.format = {
                codec: videoCodec.name,
                profile: videoCodec.profile,
            }
        }
        if (bitrate || resolution || frameRate || kfi) {
            options.media.video.parameters = {
                bitrate: bitrate ? 'x' + bitrate : undefined,
                resolution,
                framerate: frameRate,
                keyFrameInterval: kfi,
            }
        }
    }
    startRecording(roomId, options, (resp) => {
        resp = JSON.parse(resp);
        console.log(`start recording ${audioFrom}, ${videoFrom} success: `, resp);
        $('#recorderid').val(resp.id);
    }, err => {
        console.log('start recording failed: ', err);
    })
}

function restupdateRecording() {
    let {
        roomId,
        recorderId,
        audioFrom,
        videoFrom,
        audioActive,
        videoActive,
    } = getRestfulParmas();
    let {
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let updateOptions = [];

    if (audioFrom) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/audio/from',
            value: audioFrom,
        };
        updateOptions.push(subscriptionControlInfo);
    }

    if (videoFrom) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/from',
            value: videoFrom,
        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (bitrate) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/parameters/bitrate',
            value: 'x' + bitrate,
        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (resolution) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/parameters/resolution',
            value: resolution,
        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (frameRate) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/parameters/framerate',
            value: frameRate,
        };
        updateOptions.push(subscriptionControlInfo);
    }
    if (kfi) {
        let subscriptionControlInfo = {
            op: 'replace',
            path: '/media/video/parameters/keyFrameInterval',
            value: kfi,
        };
        updateOptions.push(subscriptionControlInfo);
    }

    updateRecording(roomId, recorderId, updateOptions, (resp) => {
        resp = JSON.parse(resp);
        console.log(`update recording ${recorderId} success: `, resp);
    }, err => {
        console.log('update recording failed: ', err);
    })
}

function restListRecordings() {
    let {
        roomId
    } = getRestfulParmas();
    listRecordings(roomId, (resp) => {
        resp = JSON.parse(resp);
        console.log(`list recording success: `, resp);
    }, err => {
        console.log('list recording failed: ', err);
    })
}

function restStopRecording() {
    let {
        roomId,
        recorderId
    } = getRestfulParmas();
    stopAnalyzing(roomId, recorderId, (resp) => {
        console.log(`stop recording ${recorderId} success: `, resp);
    }, err => {
        console.log('stop recording failed: ', err);
    })
}

function restStartAnalyzing() {
    let {
        roomId,
        audioFrom,
        videoFrom,
        algorithm,
    } = getRestfulParmas();
    let {
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let options = {
        media: {
            audio: false,
            video: false,
        },
        algorithm: algorithm,
    }
    if (audioFrom) {
        options.media.audio = {
            from: audioFrom,
        }
        if (audioCodec) {
            options.media.audio.format = {
                codec: audioCodec.name,
                sampleRate: audioCodec.clockRate,
                channelNum: audioCodec.channelCount,
            }
        }
    }
    if (videoFrom) {
        options.media.video = {
            from: videoFrom,
        }
        if (videoCodec) {
            options.media.video.format = {
                codec: videoCodec.name,
                profile: videoCodec.profile,
            }
        }
        if (bitrate || resolution || frameRate || kfi) {
            options.media.video.parameters = {
                bitrate: bitrate ? 'x' + bitrate : undefined,
                resolution,
                framerate: frameRate,
                keyFrameInterval: kfi,
            }
        }
    }
    startAnalyzing(roomId, options, (resp) => {
        resp = JSON.parse(resp);
        console.log(`start analyzing ${audioFrom}, ${videoFrom} success: `, resp);
        $('#analyticsid').val(resp.id);
    }, err => {
        console.log('start analytics failed: ', err);
    })
}

function restStopAnalyzing() {
    let {
        roomId,
        analyticsId
    } = getRestfulParmas();
    stopAnalyzing(roomId, analyticsId, (resp) => {
        console.log(`stop analytics ${analyticsId} success: `, resp);
    }, err => {
        console.log('stop analytics failed: ', err);
    })
}

function transCodec() {
    let {
        roomId,
        audioFrom,
        videoFrom,
        container,
    } = getRestfulParmas();

    let audioCodecs = [
        { name: "opus", channelCount: 2, clockRate: 48000 },
        { name: "pcma", channelCount: undefined, clockRate: undefined },
        { name: "pcmu", channelCount: undefined, clockRate: undefined },
        { "name": "aac", "channelCount": 2, "clockRate": 48000 },
    ];
    let videoCodecs = [
        { name: 'vp8' },
        { name: 'vp9' },
        { name: 'h265' },
        { name: 'h264', profile: 'H' },
        { name: 'h264', profile: 'CB' },
        { name: 'h264', profile: 'M' },
        { name: 'h264', profile: 'B' },
    ]
    audioCodecs.forEach(function (audioCodec) {
        videoCodecs.forEach(function (videoCodec) {
            let videoFormat = {
                codec: videoCodec.name,
                profile: videoCodec.profile
            }
            let options = {
                media: {
                    audio: {
                        from: audioFrom,
                        format: {
                            codec: audioCodec.name,
                            sampleRate: audioCodec.clockRate,
                            channelNum: audioCodec.channelCount,
                        }
                    },
                    video: {
                        from: videoFrom,
                        format: videoFormat
                    }
                }
            }
            startRecording(roomId, options, (resp) => {
                resp = JSON.parse(resp);
                console.log(`start recording ${audioFrom}, ${videoFrom} success: `, resp, 'recording options', options);
                setTimeout(function () {
                    let recorderId = resp.id;
                    stopRecording(roomId, recorderId, (resp) => {
                        console.log(`stop recording ${recorderId} success: `, resp);
                    }, err => {
                        console.log('stop recording failed: ', err, JSON.streamify(options));
                    })
                }, 10000)
            }, err => {
                console.log('start recording failed: ', err);
            })
        });
        /*
         let options = {
             media:{
                 audio:{
                     from: audioFrom,
                     format: {
                         codec: audioCodec.name,
                         sampleRate: audioCodec.clockRate,
                         channelNum: audioCodec.channelCount,
                     }
                 },
                 video:{
                     from: videoFrom
                 }
             }
          }
         startRecording(roomId, options, (resp) => {
             resp = JSON.parse(resp);
             console.log(`start recording ${audioFrom}, ${videoFrom} success: `, resp);
             setTimeout(function(){
                 let recorderId = resp.id;
                 stopRecording(roomId, recorderId, (resp) => {
                     console.log(`stop recording ${recorderId} success: `, resp);
                 }, err => {
                     console.log('stop recording failed: ', err);
                 })
             },10000)
         }, err => {
             console.log('start recording failed: ', err);
         })*/
    })

}



///////////////////////////
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

function displayStream(stream, category = 'forward') {
    let source = stream.source.audio || stream.source.video;

    function addVideoTag(style, stream) {
        let $div = $(`test${stream.id}`);
        if (!$div[0]) {
            let videoTag = `<div class=${style}><video playsinline autoplay controls ></video></div>`;
            $div = $(videoTag);
            $div.attr('id', `test${stream.id}`);
            $div.attr('title', `${stream.id}`);
            if (style === "smallVideo") {
                $('#container-video').append($div);
            } else {
                $('#mixVideo').append($div);
            }
        }
        $div.find('video').get(0).srcObject = stream.mediaStream;
    }
    if (category === 'forward') {
        if (source === 'mixed') {
            addVideoTag('normalVideo', stream);
        } else {
            addVideoTag('smallVideo', stream);
        }
    } else {
        if (source === "camera" || source === 'mic') {
            $('#localVideo video').attr('title', stream.id).get(0).srcObject = stream.mediaStream;
        } else {
            $('#screenVideo video').attr('title', stream.id).get(0).srcObject = stream.mediaStream;
        }
    }
}

function setWH(ele, percent, parentEleId) {
    var w = document.getElementById(parentEleId).offsetWidth * percent;
    ele.style.width = w + 'px';
    ele.style.height = 0.75 * w + 'px';
    setInterval(function () {
        if (ele && ele.parentNode && w != ele.parentNode.offsetWidth * percent) {
            w = ele.parentNode.offsetWidth * percent;
            ele.style.width = w + 'px';
            ele.style.height = 0.75 * w + 'px';
        }
    }, 100);
}

function subs(stream) {
    client.subscribe(stream, {
        audio: true,
        video: true
    })
        .then((subscription) => {
            subscription.originId = stream.id;
            subscriptions.set(subscription.id, subscription);
            displayStream(stream);
        }, (err) => {
            console.log(`subscribe ${stream.id} failed: ${err}`);
        })
}
//destroyStreamUi
function destroyStreamUi(stream) {
    $(`#test${stream.originId || stream.id}`).remove();
}

//event
function clientEvent(client) {
    let streamaddedListener = (eve) => {
        let remoteStream = eve.stream;
        let {
            roomId
        } = getRestfulParmas();
        console.log(`new stream added ${remoteStream.id}`);

        mix && mixStream(roomId, remoteStream.id, 'common', (resp) => {
            resp = JSON.parse(resp);
            console.log(`mix stream ${remoteStream.id} to common success: `, resp);
        }, err => {
            console.log(`mix stream ${remoteStream.id} to common failed: `, err);
        })

        remoteStreamMap.set(remoteStream.id, remoteStream);
        if (remoteStream.origin) {
            hasForward && subs(remoteStream);
        } else {
            hasMixed && subs(remoteStream);
        }
    },
        participantjoinedListener = (eve) => {
            let participant = eve.participant;
            participants.set(participant.id, participant);
            console.log('new participant joined:', participant);
        },
        messagereceivedListener = (eve) => {
            console.log('new message received: ', eve);
        }
    serverdisconnectedListener = () => {
        console.log('server disconnected');
        remoteStreamMap.clear();
        subscriptions.clear();
        publications.clear();
    };
    client.addEventListener("streamadded", streamaddedListener);
    client.addEventListener('participantjoined', participantjoinedListener);
    client.addEventListener('messagereceived', messagereceivedListener);
    client.addEventListener("serverdisconnected", serverdisconnectedListener);
}
client.clearEventListener('streamadded');
clientEvent(client);

function publicationEvent(publication) {
    let endedListener = () => {
        console.log(`publication ${publication.id} is ended`);
        publications.delete(publication.id);
    },
        audiomuteListener = () => {
            console.log(`publication ${publication.id} audio mute`);
        },
        audiounmuteListener = () => {
            console.log(`publication ${publication.id} audio unmute`);
        },
        videomuteListener = () => {
            console.log(`publication ${publication.id} video mute`);
        },
        videounmuteListener = () => {
            console.log(`publication ${publication.id} video unmute`);
        },
        muteListener = (event) => {
            console.log(`publication ${publication.id} muted ${event.kind}`);
        },
        unmuteListener = (event) => {
            console.log(`publication ${publication.id} unmuted ${event.kind}`);
        };
    publication.addEventListener('ended', endedListener);
    publication.addEventListener('audiomute', audiomuteListener);
    publication.addEventListener('audiounmute', audiounmuteListener);
    publication.addEventListener('videomute', videomuteListener);
    publication.addEventListener('videounmute', videounmuteListener);
    publication.addEventListener('mute', muteListener);
    publication.addEventListener('unmute', unmuteListener);
}

function subscriptionEvent(subscription) {
    let endedListener = () => {
        console.log(`subscription ${subscription.id} is ended`);
        subscriptions.delete(subscription.id);
    },
        audiomuteListener = () => {
            console.log(`subscription ${subscription.id} audio mute`);
        },
        audiounmuteListener = () => {
            console.log(`subscription ${subscription.id} audio unmute`);
        },
        videomuteListener = () => {
            console.log(`subscription ${subscription.id} video mute`);
        },
        videounmuteListener = () => {
            console.log(`subscription ${subscription.id} video unmute`);
        },
        muteListener = (event) => {
            console.log(`subscription ${subscription.id} muted ${event.kind}`);
        },
        unmuteListener = (event) => {
            console.log(`subscription ${subscription.id} unmuted ${event.kind}`);
        };
    subscription.addEventListener('ended', endedListener);
    subscription.addEventListener('audiomute', audiomuteListener);
    subscription.addEventListener('audiounmute', audiounmuteListener);
    subscription.addEventListener('videomute', videomuteListener);
    subscription.addEventListener('videounmute', videounmuteListener);
    subscription.addEventListener('mute', muteListener);
    subscription.addEventListener('unmute', unmuteListener);
}

function streamEvent(stream) {
    let enedListener = (event) => {
        if (stream instanceof LocalStream) {
            console.log(`local stream ${stream.id} is ended`);
            localStreams.delete(stream.source.video);
        } else {
            console.log(`remote stream ${stream.id} is ended`);
            remoteStreamMap.delete(stream.id);
        }
        destroyStreamUi(stream);
    };
    let activeaudioinputchangeListener = (event) => {
        console.log('activeaudioinputchange event triggered: ', event);
    };
    let layoutchangeListener = (event) => {
        console.log('layoutchange event triggered: ', event);
    };
    let update = (event) => {
        console.log('streamupdate event triggered: ', event, stream.id);
    }
    stream.addEventListener('ended', enedListener);
    stream.addEventListener('activeaudioinputchange', activeaudioinputchangeListener);
    stream.addEventListener('layoutchange', layoutchangeListener);
    stream.addEventListener('update', update);

}

function mediaStreamEvent(localStream) {
    localStream.mediaStream.getTracks().forEach(track => {
        track.onended = function () {
            console.log(`localStream ${localStream.id} ${track.kind} ended`);
            localStreams.delete(localStream.source.video);
        }
    })
}

function participantEvent(participant) {
    let left = () => {
        console.log(`participant ${participant.id} left`);
        participants.delete(participant.id);
    }
    participant.addEventListener('left', left);
}
//get info
publications.onLengthAdd((key, value) => {
    publicationEvent(publications.get(key));
    $('#availablepublication').append($(`<option value=${key}>${key}</option>`));
})
subscriptions.onLengthAdd((key, value) => {
    subscriptionEvent(subscriptions.get(key));
    $('#availablesubscription').append($(`<option value=${key}>${key}</option>`));
})
localStreams.onLengthAdd((key, value) => {
    mediaStreamEvent(localStreams.get(key));
    $('#localstreamid').append($(`<option value=${key}>${key}</option>`));
})
remoteStreamMap.onLengthAdd((key, value) => {
    streamEvent(remoteStreamMap.get(key));
    $('#availableremotestream').append($(`<option value=${key}>${key}</option>`));
    $('#forwardstreamlist').append($(`<option value=${key}>${key}</option>`));
    $('#audiofromlist').append($(`<option value=${key}>${key}</option>`));
    $('#videofromlist').append($(`<option value=${key}>${key}</option>`));
    let index = key.indexOf('-')
    if (index != -1) {
        let view = key.slice(index + 1);
        $('#viewlist').append($(`<option value=${view}>${view}</option>`));
        $('#mixstreamlist').append($(`<option value=${key}>${key}</option>`));
    }
})

participants.onLengthAdd((key, value) => {
    participantEvent(value);
    $('#participantsid').append($(`<option value=${key}>${key}</option>`));
    $('#participantlist').append($(`<option value=${key}>${key}</option>`));
})
allRooms.onLengthAdd((key, value) => {
    $('#roomlist').append($(`<option value=${key}>${key}</option>`));
})

allSipcalls.onLengthAdd((key, value) => {
    $('#sipcallslist').append($(`<option value=${key}>${key}</option>`));
})
//remove info
publications.onLengthRemove((key) => {
    $('#availablepublication').find(`[value=${key}]`).remove();
})
subscriptions.onLengthRemove((key) => {
    $('#availablesubscription').find(`[value=${key}]`).remove();
})
localStreams.onLengthRemove((key) => {
    $('#localstreamid').find(`[value=${key}]`).remove();
})
remoteStreamMap.onLengthRemove((key) => {
    $('#availableremotestream').find(`[value=${key}]`).remove();
    $('#forwardstreamlist').find(`[value=${key}]`).remove();
    $('#audiofromlist').find(`[value=${key}]`).remove();
    $('#videofromlist').find(`[value=${key}]`).remove();
})
participants.onLengthRemove((key) => {
    $('#participantsid').find('[value="' + key + '"]').remove();
    $('#participantlist').find('[value="' + key + '"]').remove();
})
allRooms.onLengthRemove((key) => {
    $('#roomlist').find(`[value=${key}]`).remove();
})

allSipcalls.onLengthRemove((key) => {
    $('#sipcallslist').find(`[value=${key}]`).remove();
})


//LocalStream action
function stopLocal() {
    let name = $('#localstreamid').val() || 'camera';
    let localStream = localStreams.get(name);
    if (localStream) {
        localStream.mediaStream.getTracks().forEach(track => {
            track.stop();
        })
        localStreams.delete(name);
    }
}
function createLocalStream() {
    let videoSource = $('input[name="videosource"]:checked').val() || false;
    let audioSource = $('input[name="audiosource"]:checked').val() || false;
    let extensionId = $('#extensionid').val() || undefined;
    let resolution = resolutionName2Value[$('#resolution').val()] || {
        width: 640,
        height: 480
    };
    let audioDeviceId = $('#audiodeviceid').val() || undefined;
    let videoDeviceId = $('#videodeviceid').val() || undefined;
    let audio, video;
    if (!audioSource) {
        audio = false;
        audioSource = undefined;
    } else {
        audio = {
            source: audioSource,
            deviceId: audioDeviceId

        }
    }
    if (!videoSource) {
        video = false;
        videoSource = undefined;
    } else {
        video = {
            source: videoSource,
            resolution: resolution,
            deviceId: videoDeviceId,
        }
    }
    let mediaStreamDeviceConstraints = {
        audio: audio,
        video: video,
        extensionId: extensionId
    };
    let streamSourceInfo = new StreamSourceInfo(audioSource, videoSource);
    MediaStreamFactory.createMediaStream(mediaStreamDeviceConstraints)
        .then((mediaStream) => {
            console.log('create media stream success: ', mediaStream);
            return new LocalStream(mediaStream, streamSourceInfo, { testAttributes: 'test' })
        }, (err) => {
            console.log('create mediaStream failed: ', err);
            return Promise.reject()
        })
        .then((localStream) => {
            displayStream(localStream, 'local');
            localStreams.set(localStream.source.video || localStream.source.audio, localStream);
            console.log('create local stream success: ', localStream);
        }, (err) => {
            console.log('create local stream failed: ', err);
        })

}

//client action
function leave() {
    client.leave()
        .then(() => {
            console.log('client leave success');
        }, (err) => {
            console.log('client leave failed: ', err);
        })
}
let newToken = '';

function createNewToken() {
    let role = $('#role').val();
    createToken(defaultRoomId, 'testuser', role, (token) => {
        newToken = token;
        console.log('create a new token success: ', token);
    }, (err) => {
        console.log('create a new token failed: ', err);
    })
}

function joinRoom() {
    client.join(newToken)
        .then((resp) => {
            console.log('join room with a new token success: ', resp);
            resp.remoteStreams.forEach(function (remoteStream) {
                remoteStreamMap.set(remoteStream.id, remoteStream);
            })
        }, (err) => {
            console.log('join room with a new token failed: ', err);
        })
}

function getAudioAndVideoCodec(audioCodecName, videoCodecName) {
    let audioCodec, videoCodec;
    switch (audioCodecName) {
        case 'g722-1':
            audioCodec = {
                name: 'g722',
                channelCount: 1,
            }
            break;
        case 'g722-2':
            audioCodec = {
                name: 'g722',
                channelCount: 2,
            }
            break;
        case 'isac16':
            audioCodec = {
                name: 'isac',
                clockRate: 16000,
            }
            break;
        case 'isac32':
            audioCodec = {
                name: 'isac',
                clockRate: 32000,
            }
            break;
        default:
            audioCodec = {
                name: audioCodecName,
            }
            break;
    }

    switch (videoCodecName) {
        case 'h264H':
            videoCodec = {
                name: 'h264',
                profile: 'high',
            }
            break;
        default:
            videoCodec = {
                name: videoCodecName,
            }
            break;
    }

    return {
        audioCodec,
        videoCodec,
    }
}
function publish() {
    let localStream = localStreams.get($('#localstreamid').val() || 'camera');
    let audioCodecName = $('#audiocodec').val();
    let videoCodecName = $('#videocodec').val();
    let videoMaxBitrate = $('#videomaxbitrate').val();
    let audioMaxBitrate = $('#audiomaxbitrate').val();
    let {
        audioCodec,
        videoCodec
    } = getAudioAndVideoCodec(audioCodecName, videoCodecName);

    let audio = false, video = false;
    if (audioCodec.name || audioMaxBitrate) {
        audio = [{
            codec: audioCodec,
            maxBitrate: parseInt(audioMaxBitrate),
        }]
    }
    if (videoCodec.name || videoMaxBitrate) {
        video = [{
            codec: videoCodec,
            maxBitrate: parseInt(videoMaxBitrate),
        }]
    }
    let options = {
        audio: audio,
        video: video,
    };
    let stream = localStreams.get(localStream.source.video || localStream.source.audio);
    client.publish(stream, options).then((publication) => {
        publications.set(publication.id, publication);
        console.log(`get publication ${publication.id}`);
        publicationTmp = publication;
    }, (err) => {
        console.log('publish local stream failed: ', err);
    })
}

function addOption(select, optionsArr) {
    select.children('option:not(:first)').remove();
    optionsArr.forEach((item) => {
        if (typeof item === 'object') {
            select.append($(`<option value=${JSON.stringify(item)}>${item.name ? item.name + (item.profile === undefined ? '' : '-' + item.profile) : item.width + 'X' + item.height}</option>`));
        } else {
            select.append($(`<option value=${item}>${item}</option>`));
        }
    })
}
$('#availableremotestream').change(function () {
    let remoteStream = remoteStreamMap.get($(this).val());
    let {
        audio,
        video
    } = remoteStream.capabilities
    if (audio) {
        let {
            'codecs': supportedAudioCodecs
        } = audio;
        supportedAudioCodecs && addOption($('#supportedaudiocodec'), supportedAudioCodecs);
    }
    if (video) {
        let {
            'codecs': supportedVideoCodecs,
            bitrateMultipliers,
            frameRates,
            keyFrameIntervals,
            resolutions,
        } = video;
        supportedVideoCodecs && addOption($('#supportedvideocodec'), supportedVideoCodecs);
        bitrateMultipliers && addOption($('#supportedbitrate'), bitrateMultipliers);
        resolutions && addOption($('#supportedresolution'), resolutions);
        frameRates && addOption($('#supportedframerate'), frameRates);
        keyFrameIntervals && addOption($('#supportedkfi'), keyFrameIntervals);
    }
})

function getSubOptions() {
    let remoteStreamId = $('#availableremotestream').val();
    console.log("remote stream id: ", remoteStreamId);
    if (!remoteStreamId) {
        return {};
    }
    let trackKind = $('#subscribetrackkind').val() || undefined;
    let videoCodec = $('#supportedvideocodec').val() ? JSON.parse($('#supportedvideocodec').val()) : undefined;
    let audioCodec = $('#supportedaudiocodec').val() ? JSON.parse($('#supportedaudiocodec').val()) : undefined;
    let bitrate = $('#supportedbitrate').val() ? $('#supportedbitrate').val() : undefined;
    let resolution = $('#supportedresolution').val() ? JSON.parse($('#supportedresolution').val()) : undefined;
    let frameRate = $('#supportedframerate').val() ? parseInt($('#supportedframerate').val()) : undefined;
    let kfi = $('#supportedkfi').val() ? parseInt($('#supportedkfi').val()) : undefined;
    let subscriptionId = $('#availablesubscription').val() || $('#availablesubscription option:eq(1)').val();
    return {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
        subscriptionId,
    }
}

function subscribe() {
    let {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let audio, video, subOptions;
    if (trackKind === 'audio') {
        video = false;
    } else {
        video = {
            codecs: [{ name: 'xxx' }, videoCodec],
            resolution: resolution,
            frameRate: frameRate,
            bitrateMultiplier: bitrate,
            keyFrameInterval: kfi,
        }
    }

    if (trackKind === 'video') {
        audio = false;
    } else {
        audio = {
            codecs: [audioCodec]
        }
    }
    subOptions = {
        audio: audio,
        video: video,
    }
    client.subscribe(remoteStreamMap.get(remoteStreamId), subOptions)
        .then((subscription) => {
            subscription.originId = remoteStreamId;
            subscriptions.set(subscription.id, subscription);
            displayStream(remoteStreamMap.get(remoteStreamId));
            console.log('subscribe success: ', subscription);
        }, err => {
            console.log('subscribe failed: ', err);
        })
}



function sendMessage() {
    let message = $('#sendcontent').val();
    let participantId = $('#participantsid').val();
    client.send(message, participantId)
        .then(() => {
            console.log(`send message to ${participantId} success`);
        }, (err) => {
            console.log(`send message to ${participantId} failed`, err);
        })
}
//publication action
function getPublicationParams() {
    let publicationArr = [];
    let id = $('#availablepublication').val();
    if (id === 'all') {
        publications.forEach((value) => {
            publicationArr.push(value);
        })
    } else {
        publicationArr.push(publications.get(id));
    }
    return [publicationArr, $('#publicationtrackkind').val()]
}

function publicationMute() {
    let [publicationArr, trackKind] = getPublicationParams();
    publicationArr.forEach((publication) => {
        publication.mute(trackKind)
            .then(() => {
                console.log(`${publication.id} mute ${trackKind} success`)
            }, err => {
                console.log(`${publication.id} mute failed: `, err);
            })
    })
}

function publicationUnmute() {
    let [publicationArr, trackKind] = getPublicationParams();
    publicationArr.forEach((publication) => {
        publication.unmute(trackKind)
            .then(() => {
                console.log(`${publication.id} unmute ${trackKind} success`)
            }, err => {
                console.log(`${publication.id} unmute failed: `, err);
            })
    })
}

function publicationGetStats() {
    let [publicationArr] = getPublicationParams();
    publicationArr.forEach((publication) => {
        publication.getStats()
            .then((stats) => {
                console.log(`${publication.id} getStats success`, stats);
            }, err => {
                console.log(`${publication.id} getStats failed: `, err);
            })
    })
}

function publicationStop() {
    let [publicationArr] = getPublicationParams();
    publicationArr.forEach((publication) => {
        publication.stop()
            .then((stats) => {
                console.log(`${publication.id} stop success`);
            }, err => {
                console.log(`${publication.id} stop failed: `, err);
            })
    })
}
//subscription action
function getSubcriptionParams() {
    let subscriptionArr = [];
    let id = $('#availablesubscription').val();
    if (id === 'all') {
        subscriptions.forEach((value) => {
            subscriptionArr.push(value);
        })
    } else {
        subscriptionArr.push(subscriptions.get(id));
    }
    return [subscriptionArr, $('#subscriptiontrackkind').val()]
}

function subscriptionMute() {
    let [subscriptionArr, trackKind] = getSubcriptionParams();
    subscriptionArr.forEach(subscription => {
        subscription.mute(trackKind)
            .then(() => {
                console.log(`${subscription.id} mute ${trackKind} success`)
            }, err => {
                console.log(`${subscription.id} mute failed: `, err);
            })
    })
}

function subscriptionUnmute() {
    let [subscriptionArr, trackKind] = getSubcriptionParams();
    subscriptionArr.forEach(subscription => {
        subscription.unmute(trackKind)
            .then(() => {
                console.log(`${subscription.id} unmute ${trackKind} success`)
            }, err => {
                console.log(`${subscription.id} unmute failed: `, err);
            })
    })
}

function subscriptionGetStats() {
    let [subscriptionArr] = getSubcriptionParams();
    subscriptionArr.forEach(subscription => {
        subscription.getStats()
            .then((stats) => {
                console.log(`${subscription.id} getStats success`, stats);
            }, err => {
                console.log(`${subscription.id} getStats failed: `, err);
            })
    })
}

function subscriptionStop() {
    let [subscriptionArr] = getSubcriptionParams();
    subscriptionArr.forEach(subscription => {
        subscription.stop()
        destroyStreamUi(subscription);
        console.log(`${subscription.id} stop success`);
    })
}

function subscriptionApplyOption() {
    let {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
        subscriptionId,
    } = getSubOptions();
    let options = {
        audio: {
            codecs: [audioCodec]
        },
        video: {
            codecs: [videoCodec],
            resolution: resolution,
            frameRate: frameRate,
            bitrateMultiplier: bitrate,
            keyFrameInterval: kfi,
        }
    }

    subscriptions.get(subscriptionId).applyOptions(options)
        .then(() => {
            console.log(`subscription ${subscriptionId} apply options success, options are: `, options);
        }, err => {
            console.log('subscription apply options failed: ', err);
        })
}
//multiple
Promise.prototype.delay = function (timeout = 3000, value) {
    return this.then(() => {
        return new Promise(function (resolve, reject) {
            setTimeout(resolve, value, timeout);
        });
    });
};
let promiseSub = Promise.resolve();

function subscribeTest(apply) {
    let remoteStreams = remoteStreamMap.getMap();
    remoteStreams.forEach((item, key) => {
        let { audio: audioCapabilities, video: videoCapabilities } = item.capabilities
        let audioCodecs = audioCapabilities.codecs;
        let { codecs: videoCodecs, bitrateMultipliers, frameRates, keyFrameIntervals, resolutions } = videoCapabilities;
        audioCodecs.forEach((audioCodec) => {
            videoCodecs.forEach((videoCodec => {
                frameRates.forEach((frameRate) => {
                    keyFrameIntervals.forEach((kfi) => {
                        resolutions.forEach((resolution) => {
                            bitrateMultipliers.forEach((bitrate) => {
                                let options = {
                                    audio: {
                                        codecs: [audioCodec],
                                    },
                                    video: {
                                        codecs: [videoCodec],
                                        resolution: resolution,
                                        frameRate: frameRate,
                                        bitrateMultiplier: bitrate,
                                        keyFrameInterval: kfi,
                                    }
                                };
                                let sp;
                                promiseSub = promiseSub.then(() => {
                                    return client.subscribe(item, options)
                                })
                                    .then(subscription => {
                                        displayStream(item);
                                        sp = subscription;
                                        return Promise.resolve(subscription);
                                    })
                                    .delay(10000)
                                    .then(() => {
                                        return sp.stop();
                                    })
                                    .then(() => {
                                        destroy(item);
                                    })
                            });
                        });
                    });
                });
            }));
        });
    });
};

function joinAllRooms() {
    let count = 0;
    listRooms((rooms) => {
        rooms = JSON.parse(rooms);
        rooms.forEach((item) => {
            count++;
            setTimeout(function () {
                createToken(item._id, 'testroom', 'presenter', (token) => {
                    let client = new ConferenceClient();
                    client.join(token)
                        .then((resp) => {
                            console.log('join room successsssss: ', resp, item._id);
                            client.leave()
                                .then(resp => {
                                    console.log('leave success, ', item._id);
                                }, err => {
                                    console.log('leave failed, ', item._id);
                                });
                        }, (err) => {
                            console.log('join room faileddddddd: ', err, item._id);
                        });
                }, (err) => {
                    console.log('create token failednnnnnn: ', err, item._id);
                })
            }, count * 000);
        })
    }, (err) => {
        console.log('list rooms failed', err);
    })
}
function listAllUsers() {
    listRooms((rooms) => {
        rooms = JSON.parse(rooms);
        rooms.forEach((item) => {
            let roomId = item._id;
            listParticipants(roomId, (resp) => {
                resp = JSON.parse(resp);
                console.log(`get ${roomId} participants: `, resp);
            }, err => {
                console.log(`get ${roomId} participants failed: `, err);
            })
        })
    }, (err) => {
        console.log('list rooms failed', err);
    })
}
function lotOfUserJoin(count = 100, leave) {
    for (let i = 0; i < count; i++) {
        let client = new ConferenceClient();
        setInterval(() => {
            createToken(defaultRoomId, 'lotof' + i, 'presenter', token => {
                client.join(token)
                    .then(resp => {
                        console.log('lotof join room success: ', resp, i);
                        leave && client.leave()
                            .then(resp => {
                                console.log('leave success, ', client.id);
                            }, err => {
                                console.log('leave failed, ', client.id);
                            });
                    }, err => {
                        console.log('lotof join foom failed: ', err, i);
                    });
            }, err => {
                console.log('lotof create token failed: ', i);
            });
        }, 200);
    }
}
//pub_unpub
function pub_unpub() {
    let count = parseInt($('#cycleCounts').val()) || 20;
    let intervals_tmp = parseInt($('#intervals').val()) || 200;
    let localStream = localStreams.get($('#localstreamid').val() || 'camera');
    let audioCodecName = $('#audiocodec').val();
    let videoCodecName = $('#videocodec').val();
    let videoMaxBitrate = $('#videomaxbitrate').val();
    let audioMaxBitrate = $('#audiomaxbitrate').val();
    let {
        audioCodec,
        videoCodec
    } = getAudioAndVideoCodec(audioCodecName, videoCodecName);

    let audio = false, video = false;
    if (audioCodec.name || audioMaxBitrate) {
        audio = [{
            codec: audioCodec,
            maxBitrate: parseInt(audioMaxBitrate),
        }]
    }
    if (videoCodec.name || videoMaxBitrate) {
        video = [{
            codec: videoCodec,
            maxBitrate: parseInt(videoMaxBitrate),
        }]
    }
    let options = {
        audio: audio,
        video: video,
    };
    let stream = localStreams.get(localStream.source.video || localStream.source.audio);
    var m = 0;
    let inter = setInterval(() => {
        client.publish(stream, options).then((publication) => {
            publications.set(publication.id, publication);
            console.log(`get publication ${publication.id}`);
            publicationTmp = publication;
            m++;
            console.log("Pub Success counts", m)
            publicationTmp.stop();
            console.log("Pub.stop Success counts", m)
        }, (err) => {
            console.log('publish local stream failed: ', err);
        })
        count--;
        if (count === 0) {
            clearInterval(inter);
        }
    }, intervals_tmp);
}
//sub_unsub
function sub_unsub() {
    let count = parseInt($('#cycleCounts').val()) || 20;
    let intervals_tmp = parseInt($('#intervals').val()) || 200;
    let {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();
    let audio, video, subOptions;
    if (trackKind === 'audio') {
        video = false;
    } else {
        video = {
            codecs: [{ name: 'xxx' }, videoCodec],
            resolution: resolution,
            frameRate: frameRate,
            bitrateMultiplier: bitrate,
            keyFrameInterval: kfi,
        }
    }

    if (trackKind === 'video') {
        audio = false;
    } else {
        audio = {
            codecs: [audioCodec]
        }
    }
    subOptions = {
        audio: audio,
        video: video,
    }
    var m = 0;
    let inter = setInterval(() => {
        client.subscribe(remoteStreamMap.get(remoteStreamId), subOptions)
            .then((subscription) => {
                subscription.originId = remoteStreamId;
                subscriptions.set(subscription.id, subscription);
                displayStream(remoteStreamMap.get(remoteStreamId));
                m++;
                console.log("Sub Success counts", m, subscription.id)
                subscription.stop();
                console.log("Sub.stop Success counts", m)
            }, err => {
                console.log('subscribe failed: ', err);
            })
        count--;
        if (count === 0) {
            clearInterval(inter);
        }
    }, intervals_tmp);
}
//mix_unmix
function mix_unmix() {
    let count = parseInt($('#cycleCounts').val()) || 20;
    let intervals_tmp = parseInt($('#intervals').val()) || 200;
    let {
        roomId,
        forwardStreamId,
        view
    } = getRestfulParmas();
    var m = 0;
    let inter = setInterval(() => {
        mixStream(roomId, forwardStreamId, view, (resp) => {
            m++;
            resp = JSON.parse(resp);
            console.log(`mix stream ${forwardStreamId} to ${view} success: `, resp, m);
            unmixStream(roomId, forwardStreamId, view, (resp) => {
                resp = JSON.parse(resp);
                console.log(`UNmix stream ${forwardStreamId} to ${view} success: `, resp, m);
            }, err => {
                console.log(`UNmix stream ${forwardStreamId} to ${view} failed: `, err);
            },
                err => {
                    console.log(`mix stream ${forwardStreamId} to ${view} failed: `, err);
                })
        })
        count--;
        if (count === 0) {
            clearInterval(inter);
        }
    }, intervals_tmp);
}
//pubmute_unmute
function pubmute_unmute() {
    let count = parseInt($('#cycleCounts').val()) || 20;
    let intervals_tmp = parseInt($('#intervals').val()) || 200;
    var m = 0;
    let inter = setInterval(() => {
        let [publicationArr, trackKind] = getPublicationParams();
        publicationArr.forEach((publication) => {
            publication.mute(trackKind)
                .then(() => {
                    m++;
                    console.log("pubmute_mute", m)
                    console.log(`${publication.id} mute ${trackKind} success`)
                    publicationArr.forEach((publication) => {
                        publication.unmute(trackKind)
                            .then(() => {
                                console.log("unmute", m)
                                console.log(`${publication.id} unmute ${trackKind} success`)
                            }, err => {
                                console.log(`${publication.id} unmute failed: `, err);
                            })
                    })
                }, err => {
                    console.log(`${publication.id} mute failed: `, err);
                })
        });
        count--;
        if (count === 0) {
            clearInterval(inter);
        }
    }, intervals_tmp);
}
//submute_unmute
function submute_unmute() {
    let count = parseInt($('#cycleCounts').val()) || 20;
    let intervals_tmp = parseInt($('#intervals').val()) || 200;
    let [subscriptionArr, trackKind] = getSubcriptionParams();
    var m = 0;
    let inter = setInterval(() => {
        subscriptionArr.forEach(subscription => {
            subscription.mute(trackKind)
                .then(() => {
                    console.log(`${subscription.id} mute ${trackKind} success`)
                    m++;
                    console.log("submute_mute", m)
                    subscriptionArr.forEach(subscription => {
                        subscription.unmute(trackKind)
                            .then(() => {
                                console.log("unmute", m)
                                console.log(`${subscription.id} unmute ${trackKind} success`)
                            }, err => {
                                console.log(`${subscription.id} unmute failed: `, err);
                            })
                    })
                }, err => {
                    console.log(`${subscription.id} mute failed: `, err);
                })
        })
        count--;
        if (count === 0) {
            clearInterval(inter);
        }
    }, intervals_tmp);
}


window.onload = function () {
    navigator.mediaDevices.enumerateDevices()
        .then(devicesInfos => {
            devicesInfos.forEach((deviceInfo) => {
                if (deviceInfo.kind === 'audioinput') {
                    $('#audiodeviceid').append(`<option value=${deviceInfo.deviceId}>${deviceInfo.deviceId}</option>`);
                }
            });
        })
        .catch(err => {
            console.log('get device info failed:', err);
        })
    navigator.mediaDevices.enumerateDevices()
        .then(devicesInfos => {
            devicesInfos.forEach((deviceInfo) => {
                if (deviceInfo.kind === 'videoinput') {
                    $('#videodeviceid').append(`<option value=${deviceInfo.deviceId}>${deviceInfo.deviceId}</option>`);
                }
            });
        })
        .catch(err => {
            console.log('get device info failed:', err);
        })


    let publishOputions = {
        audio: [{
            codec: {
                name: audioCodec
            },
            maxBitrate: audioMaxBitrate,
        }],
        video: [{
            codec: {
                name: videoCodec
            },
            maxBitrate: videoMaxBitrate,
        }]
    }


    isJoin && createToken(defaultRoomId, 'testuser', role, (token) => {
        client.join(token)
            .then((resp) => {
                console.log('join success:', resp);
                let {
                    'participants': participantArr,
                    remoteStreams
                } = resp;
                //subscribe remote streams.
                remoteStreams.forEach((stream) => {
                    remoteStreamMap.set(stream.id, stream);
                    if (stream.origin) {
                        hasForward && subs(stream);
                    } else {
                        hasMixed && subs(stream);
                    }
                })
                participantArr.forEach((participant) => {
                    participants.set(participant.id, participant);
                })
                let mstdcForMic = new AudioTrackConstraints('mic');
                let mstdcForCamera = new VideoTrackConstraints('camera');
                mstdcForCamera.resolution = resolution;
                let streamConstraints = new StreamConstraints(mstdcForMic, mstdcForCamera);
                MediaStreamFactory.createMediaStream({
                    audio: {
                        source: 'mic',
                        deviceId: undefined,
                        volume: undefined,
                        sampleRate: undefined,
                        channelCount: undefined,
                    },
                    video: {
                        resolution: resolution,
                        frameRate: undefined,
                        deviceId: undefined,
                        source: 'camera',
                    }
                })
                    .then((mediaStream) => {
                        console.log('media stream is', mediaStream);
                        return new LocalStream(mediaStream, new StreamSourceInfo('mic', 'camera'), { attributes: 'test attributes' });
                    }, (err) => {
                        console.log('create media stream failed: ', err);
                        return Promise.reject();
                    })
                    .then((localStream) => {
                        localStreams.set("camera", localStream);
                        console.log('local stream is:', localStream);
                        displayStream(localStream, 'local');
                        isPublish && client.publish(localStream, publishOputions)
                            .then((publication) => {
                                publications.set(publication.id, publication);
                                console.log('publish success:', publication);
                                publicationTmp = publication;
                            }, err => {
                                console.log('publish failed', err);
                            })
                    }, err => {
                        console.log('create local stream failed:', err);
                    });
            }, (err) => {
                console.log('join failed', err);
            })
    }, (err) => {
        console.error(`create token failed: ${err}`)
    })



};
window.addEventListener('unload', () => {
    console.log('first beforeunload');
});
window.onbeforeunload = function (event) {
    event.preventDefault();
    console.log('on beforeunload triggered');
    subscriptions.forEach((value) => {
        value.stop();
        /*     .then(resp => {
                 console.log(`stop subscription success ${resp}`);
             }, err => {
                 console.log(`stop subscription failed ${err}`);
             })*/
    });
    publications.forEach((value) => {
        value.stop();
        /*  .then(resp => {
              console.log(`stop publication success ${resp}`);
          }, err => {
              console.log(`stop publicattion failed ${err}`);
          })*/
    });
    stopLocal();
    stopLocal('screen-cast');
    client && client.leave();
    /*conference.leave();
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
    }*/


};
