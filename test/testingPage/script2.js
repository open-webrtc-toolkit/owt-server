let defaultRoomId = '',
        role = '',
        isPublish,
        resolution = '',
        videoCodec = '',
        audioCodec = '',
        hasVideo,
        hasAudio,
        isJoin;


class EventMap{
    constructor (){
        this._store=new Map();
        this._addListeners = [];
        this._removeListeners = [];
    }
    get(key){
        return this._store.get(key);
    }

    set(key, value){
        if(this._store.has(key)){
            return
        }
        this._store.set(key, value);
        let len = this._addListeners.length
        if(len > 0){
            for(let i=0;i<len;i++){
                this._addListeners[i](key, value);
            }
        }
    }

    delete(key){
        this._store.delete(key);
        let len = this._removeListeners.length
        if(len > 0){
            for(let i=0;i<len;i++){
                this._removeListeners[i](key);
            }
        }
    }
    forEach(cb){
        this._store.forEach(cb);
    }
    getMap(){
        return this._store
    }

    onLengthAdd(listener){
        this._addListeners.push(listener);
    }

    onLengthRemove(listener){
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
        Resolution
    } = Ics.Base,
    {ConferenceClient} = Ics.Conference;

let client = new ConferenceClient(),
    localStreams = new EventMap(),
    remoteStreamMap = new EventMap(),
    subscriptions = new EventMap(),
    publications = new EventMap(),
    participants = new EventMap();
let allRooms = new EventMap();


var resolutionName2Value = {
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
    }
};
var send = function(method, entity, body, okCallback, errCallback) {
    var req = new XMLHttpRequest();
    req.onreadystatechange = function() {
        if (req.readyState === 4) {
            if(req.status === 200){
                okCallback(req.responseText);
            }else{
                errCallback(req.responseTest);
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

var onRespone = function(result) {
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

var listRooms = function(ok_cb, err_cb) {
    send('GET', '/rooms/', undefined, ok_cb, err_cb);
};

var getRoom = function(room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/', undefined, ok_cb, err_cb);
};

var createRoom = function(name='testNewRoom', options, ok_cb, err_cb) {
    send('POST', '/rooms/', {
        name: name,
        options: options
    }, ok_cb, err_cb);
};

var deleteRoom = function(room, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/', undefined, ok_cb, err_cb);
};

var updateRoom = function(room, config, ok_cb, err_cb) {
    send('PUT', '/rooms/' + room + '/', config, ok_cb, err_cb);
};

var listParticipants = function(room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/participants/', undefined, ok_cb, err_cb);
};

var getParticipant = function(room, participant, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/participants/' + participant + '/', undefined, ok_cb, err_cb);
};

var forbidSub = function(room, participant, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: '/permission/subscribe',
        value: false
    }];
    send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch, ok_cb, err_cb);
};

var forbidPub = function(room, participant, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: '/permission/publish',
        value: false
    }];
    send('PATCH', '/rooms/' + room + '/participants/' + participant + '/', jsonPatch, ok_cb, err_cb);
};

var dropParticipant = function(room, participant, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/participants/' + participant + '/', undefined, ok_cb, err_cb);
};

var listStreams = function(room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streams/', undefined, ok_cb, err_cb, onerror);
};

var getStream = function(room, stream, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streams/' + stream, undefined, ok_cb, err_cb);
};

var mixStream = function(room, stream, view, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'add',
        path: '/info/inViews',
        value: view
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var unmixStream = function(room, stream, view, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'remove',
        path: '/info/inViews',
        value: view
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var setRegion = function(room, stream, region, subStream, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: `/info/layout/${region}/stream`,
        value: subStream
    }];
    send('PATCH', '/rooms/' + room + '/streams/' + stream, jsonPatch, ok_cb, err_cb);
};

var pauseStream = function(room, stream, track, ok_cb, err_cb) {
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

var playStream = function(room, stream, track, ok_cb, err_cb) {
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

var dropStream = function(room, stream, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streams/' + stream, undefined, ok_cb, err_cb);
};

var startStreamingIn = function(room, url, ok_cb, err_cb) {
    var options = {
        url: url,
        media: {
            audio: 'auto',
            video: true
        },
        transport: {
            protocol: 'udp',
            bufferSize: 2048
        }
    };
    send('POST', '/rooms/' + room + '/streaming-ins', options, ok_cb, err_cb);
};

var stopStreamingIn = function(room, stream, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streaming-ins/' + stream, undefined, ok_cb, err_cb);
};

var listRecordings = function(room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/recordings/', undefined, ok_cb, err_cb);
};

var startRecording = function(room, audioFrom, videoFrom, container, ok_cb, err_cb) {
    var options = {
        media: {
            audio: {
                from: audioFrom
            },
            video: {
                from: videoFrom
            }
        },
        container: (container ? container : 'auto')
    };
    send('POST', '/rooms/' + room + '/recordings', options, ok_cb, err_cb);
};

var stopRecording = function(room, id, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/recordings/' + id, undefined, ok_cb, err_cb);
};

var updateRecording = function(room, id, audioFrom, videoFrom, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: '/media/audio/from',
        value: audioFrom
    }, {
        op: 'replace',
        path: '/media/video/from',
        value: videoFrom
    }];
    send('PATCH', '/rooms/' + room + '/recordings/' + id, jsonPatch, ok_cb, err_cb);
};

var listStreamingOuts = function(room, ok_cb, err_cb) {
    send('GET', '/rooms/' + room + '/streaming-outs/', undefined, ok_cb, err_cb);
};

var startStreamingOut = function(room, url, audioFrom, videoFrom, ok_cb, err_cb) {
    var options = {
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
    send('POST', '/rooms/' + room + '/streaming-outs', options, ok_cb, err_cb);
};

var stopStreamingOut = function(room, id, ok_cb, err_cb) {
    send('DELETE', '/rooms/' + room + '/streaming-outs/' + id, undefined, ok_cb, err_cb);
};

var updateStreamingOut = function(room, id, audioFrom, videoFrom, ok_cb, err_cb) {
    var jsonPatch = [{
        op: 'replace',
        path: '/media/audio/from',
        value: audioFrom
    }, {
        op: 'replace',
        path: '/media/video/from',
        value: videoFrom
    }];
    send('PATCH', '/rooms/' + room + '/streaming-outs/' + id, jsonPatch, ok_cb, err_cb);
};

var createToken = function(room, user, role, ok_cb, err_cb) {
    var body = {
        room: room,
        user: user,
        role: role
    };
    send('POST', '/tokens/', body, ok_cb, err_cb);
};

//restful action

function getRestfulParmas () {
    let roomId = $('#roomlist').val() || defaultRoomId,
        participantId = $('#participantlist').val(),
        forwardStreamId = $('#forwardstreamlist').val(),
        view = $('#viewlist').val(),
        trackKind = $('#resttrackkind').val(),
        mixedStreamId = $('#mixstreamlist').val(),
        audioFrom = $('#audiofromlist').val(),
        videoFrom = $('#videofromlist').val(),
        rtspUrl = $('#rtspurl').val(),
        rtmpUrl = $('#rtmpurl').val(),
        recorderId = $('#recorderid').val();
        regionId = $('#regionid').val();
        streamingOutId = $('#streamingoutid').val();
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
    }
}
////room action
function restListRooms () {
    listRooms((rooms)=>{
        rooms = JSON.parse(rooms);
        rooms.forEach((item)=>{
            if (item.name === 'sampleRoom' && !defaultRoomId) defaultRoomId = item._id;
            allRooms.set(item._id, item);
        })
        console.log('list rooms sucess: ', rooms);
    },(err)=>{
        console.log('list rooms failed', err);
    })
}
restListRooms();

function restGetRoom () {
    let {roomId} = getRestfulParmas();
    getRoom(roomId, (room)=>{
        room = JSON.parse(room);
        console.log('get room success: ', room);
    },(err)=>{
        console.log('get room failed: ', err);
    })
}
function restCreateRoom () {
    let options = {
        name: 'testRoom',
        viewports: [{
            name: 'common1',
            mediaMixing: {
                audio: null,
                video: {
                    resolution: 'hd720p'
                }
            }
        }]
    }
    createRoom('testCreateRoom', options, (room)=>{
        room = JSON.parse(room);
        console.log('create room success: ', room);
    }, (err)=>{
        console.log('create room failed: ', err);
    })
}
function restDeleteRoom () {
    let {roomId} = getRestfulParmas();
    deleteRoom(roomId, (resp)=>{
        allRooms.delete(roomId);
        console.log(`delete room ${roomId} success: `, resp);
    }, err=>{
        console.log('delete room failed: ', err);
    })
}
function restUpdateRoom () {
    let {roomId} = getRestfulParmas();
    let config = {};
    updateRoom(roomId, config, (resp)=>{
        console.log(`update room ${roomId} success: `, resp);
    }, err=>{
        console.log('update room success: ', err);
    })
}
////participant action
function restListParticipants () {
    let {roomId} = getRestfulParmas();
    listParticipants(roomId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`get ${roomId} participants: `, resp);
        resp.forEach((item)=>{
            participants.set(item.id, item);
        });
    },err=>{
        console.log(`get ${roomId} participants failed: `, err);
    })
}
function restGetParticipant () {
    let {roomId, participantId} = getRestfulParmas();
    getParticipant(roomId, participantId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`get participant ${participantId} success: `, resp);
    }, err=>{
        console.log(`get participant ${participantId} failed`, err);
    })
}

function restForbidSub () {
    let {roomId, participantId} = getRestfulParmas();
    forbidSub(roomId, participantId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`forbid participant ${participantId} subscribe ability success: `, resp);
    }, err=>{
        console.log(`forbid participant ${participantId} subscribe ability failed`, err);
    })
}

function restForbidPub () {
    let {roomId, participantId} = getRestfulParmas();
    forbidPub(roomId, participantId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`forbid participant ${participantId} publish ability success: `, resp);
    }, err=>{
        console.log(`forbid participant ${participantId} publish ability failed`, err);
    })
}
function restDropParticipant () {
    let {roomId, participantId} = getRestfulParmas();
    dropParticipant(roomId, participantId, (resp)=>{
        console.log(`drop participant ${participantId} success: `, resp);
    }, err=>{
        console.log(`drop participant ${participantId} failed`, err);
    })
}
////stream actions
function restListStreams () {
    let {roomId} = getRestfulParmas();
    listStreams(roomId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`get streams success: `, resp);
        resp.forEach((item)=>{
            remoteStreamMap.set(item.id, item);
        })
    },err=>{
        console.log('get streams failed: ', err);
    })
}

function restGetStream () {
    let {roomId, forwardStreamId} = getRestfulParmas();
    getStream(roomId, forwardStreamId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`get stream ${forwardStreamId} success: `, resp);
    }, err=>{
        console.log(`get stream ${forwardStreamId} failed: `, err);
    })
}
function restDropStream () {
    let {roomId, forwardStreamId} = getRestfulParmas();
    dropStream(roomId, forwardStreamId, (resp)=>{
        console.log(`drop stream ${forwardStreamId} success: `, resp);
    }, err=>{
        console.log(`drop stream ${forwardStreamId} failed: `, err);
    })
}
function restSetRegion () {
    let {roomId, mixStreamId, regionId, forwardStreamId} = getRestfulParmas();
    setRegion(roomId, mixStreamId, regionId, forwardStreamId, (resp)=>{
        console.log('set region success: ', resp);
    }, err=>{
        console.log('set region failed: ', err);
    });

}

function restPauseStream () {
    let {roomId, forwardStreamId, trackKind} = getRestfulParmas();
    pauseStream(roomId, forwardStreamId, trackKind, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`pause stream ${forwardStreamId} success: `, resp);
    }, err=>{
        console.log(`pause stream ${forwardStreamId} failed: `, err);
    })
}
function restPlayStream () {
    let {roomId, forwardStreamId, trackKind} = getRestfulParmas();
    playStream(roomId, forwardStreamId, trackKind, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`play stream ${forwardStreamId} success: `, resp);
    }, err=>{
        console.log(`play stream ${forwardStreamId} failed: `, err);
    })
}
function restMixStream () {
    let {roomId, forwardStreamId, view} = getRestfulParmas();
    mixStream(roomId, forwardStreamId, view, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`mix stream ${forwardStreamId} to ${view} success: `, resp);
    }, err=>{
        console.log(`mix stream ${forwardStreamId} to ${view} failed: `, err);
    })
}
function restUnmixStream () {
    let {roomId, forwardStreamId, view} = getRestfulParmas();
    unmixStream(roomId, forwardStreamId, view, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`mix stream ${forwardStreamId} to ${view} success: `, resp);
    }, err=>{
        console.log(`mix stream ${forwardStreamId} to ${view} failed: `, err);
    })
}
////streaming in or out action
function restStartStreamingIn () {
    let {roomId, rtspUrl} = getRestfulParmas();
    startStreamingIn(roomId, rtspUrl, (resp)=>{
        console.log('start rtsp in success: ', resp);
    }, err=>{
        console.log('start rtsp in failed: ', err);
    })
}
function restStopStreamingIn () {
    let {roomId, forwardStreamId} = getRestfulParmas();
    stopStreamingIn(roomId, forwardStreamId, (resp)=>{
        console.log('stop rtsp in success: ', resp);
    }, err=>{
        console.log('stop rtsp in failed: ', err);
    })
}
function restStartStreamingOut () {
    let {roomId, rtmpUrl, audioFrom, videoFrom} = getRestfulParmas();
    startStreamingOut(roomId, rtmpUrl, audioFrom, videoFrom, (resp)=>{
        console.log('start rtmp out success: ', resp);
    }, err=>{
        console.log('start rtmp out failed: ', err);
    })
}
function restStopStreamingOut () {
    let {roomId, streamingOutId} = getRestfulParmas();
    stopStreamingOut(roomId, streamingOutId, (resp)=>{
        console.log('stop rtmp out success: ', resp);
    }, err=>{
        console.log('stop rtmp out failed: ', err);
    })
}
function restListStreamingOuts () {
    let {roomId} = getRestfulParmas();
    listStreamingOuts(roomId, (resp)=>{
        console.log('list streaming out success: ', resp);
    }, err=>{
        console.log('list streaming out failed: ', err);
    })
}
function restUpdateStreamingOut () {
    let {roomId, streamingOutId, audioFrom, videoFrom} = getRestfulParmas();
    updateStreamingOut(roomId, streamingOutId, audioFrom, videoFrom, (resp)=>{
        console.log('update rtmp out success: ', resp);
    }, err=>{
        console.log('update rtmp out failed: ', err);
    })
}
////recording
function restStartRecording () {
    let {roomId, audioFrom, videoFrom} = getRestfulParmas();
    startRecording(roomId, audioFrom, videoFrom, undefined, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`start recording ${audioFrom}, ${videoFrom} success: `, resp);
    }, err=>{
        console.log('start recording failed: ', err);
    })
}

function restupdateRecording () {
    let {roomId, recorderId, audioFrom, videoFrom} = getRestfulParmas();
    updateRecording(roomId, recorderId, audioFrom, videoFrom, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`update recording ${recorderId} success: `, resp);
    }, err=>{
        console.log('update recording failed: ', err);
    })
}

function restListRecordings () {
    let {roomId} = getRestfulParmas();
    listRecordings(roomId, (resp)=>{
        resp = JSON.parse(resp);
        console.log(`list recording success: `, resp);
    }, err=>{
        console.log('list recording failed: ', err);
    })
}
function restStopRecording () {
    let {roomId, recorderId} = getRestfulParmas();
    stopRecording(roomId, recorderId, (resp)=>{
        console.log(`stop recording ${recorderId} success: `, resp);
    }, err=>{
        console.log('stop recording failed: ', err);
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
function displayStream (stream , category='forward') {
    let source = stream.source.audio || stream.source.video;
    function addVideoTag (style, stream) {
        let $div = $(`test${stream.id}`);
        if(!$div[0]){
            let videoTag = `<div class=${style}><video playsinline muted autoplay controls ></video></div>`;
            $div = $(videoTag);
            $div.attr('id', `test${stream.id}`);
            $div.attr('title', `${stream.id}`);
            if(style === "smallVideo"){
                $('#container-video').append($div);
            }else{
                $('#mixVideo').append($div);
            }
        }
        $div.find('video').get(0).srcObject=stream.mediaStream;
    }
    if(category === 'forward'){
        if(source === 'mixed'){
            addVideoTag('normalVideo', stream);
        }else{
            addVideoTag('smallVideo', stream);
        }
    }else{
        if(source === "camera" || source === 'mic'){
            $('#localVideo video').attr('title', stream.id).get(0).srcObject = stream.mediaStream;
        }else{
            $('#screenVideo video').attr('title', stream.id).get(0).srcObject = stream.mediaStream;
        }
    }
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

function subs(stream) {
     client.subscribe(stream, {audio: true, video: true})
     .then((subscription)=>{
        subscription.originId = stream.id;
        subscriptions.set(subscription.id, subscription);
        displayStream(stream);
     }, (err)=>{
        console.log(`subscribe ${stream.id} failed: ${err}`);
     })
}
//destroyStreamUi
function destroyStreamUi (stream) {
    $(`#test${stream.originId || stream.id}`).remove();
}

//event
function clientEvent (client) {
    let streamaddedListener = (eve)=>{
            let remoteStream = eve.stream;
            remoteStreamMap.set(remoteStream.id, remoteStream);
            console.log(`new stream added ${remoteStream.id}`);
            subs(remoteStream);
        },
        participantjoinedListener = (eve)=>{
            let participant = eve.participant;
            participants.set(participant.id, participant);
            console.log('new participant joined:', participant);
        },
        messagereceivedListener = (eve)=>{
            console.log('new message received: ', eve);
        }
        serverdisconnectedListener = ()=>{
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

function publicationEvent (publication) {
    let endedListener = ()=>{
            console.log(`publication ${publication.id} is ended`);
            publications.delete(publication.id);
        },
        audiomuteListener = ()=>{
            console.log(`publication ${publication.id} audio mute`);
        },
        audiounmuteListener = ()=>{
            console.log(`publication ${publication.id} audio unmute`);
        },
        videomuteListener = ()=>{
            console.log(`publication ${publication.id} video mute`);
        },
        videounmuteListener = ()=>{
            console.log(`publication ${publication.id} video unmute`);
        },
        muteListener = (event)=>{
            console.log(`publication ${publication.id} muted ${event.kind}`);
        },
        unmuteListener = (event)=>{
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

function subscriptionEvent (subscription) {
    let endedListener = ()=>{
            console.log(`subscription ${subscription.id} is ended`);
            subscriptions.delete(subscription.id);
        },
        audiomuteListener = ()=>{
            console.log(`subscription ${subscription.id} audio mute`);
        },
        audiounmuteListener = ()=>{
            console.log(`subscription ${subscription.id} audio unmute`);
        },
        videomuteListener = ()=>{
            console.log(`subscription ${subscription.id} video mute`);
        },
        videounmuteListener = ()=>{
            console.log(`subscription ${subscription.id} video unmute`);
        },
        muteListener = (event)=>{
            console.log(`subscription ${subscription.id} muted ${event.kind}`);
        },
        unmuteListener = (event)=>{
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

function streamEvent (stream) {
    let enedListener = (event)=>{
        if(stream instanceof LocalStream){
            console.log(`local stream ${stream.id} is ended`);
            localStreams.delete(stream.id);
        }else{
            console.log(`remote stream ${stream.id} is ended`);
            remoteStreamMap.delete(stream.id);
        }
        destroyStreamUi(stream);
    };
    stream.addEventListener('ended', enedListener)
}
function participantEvent (participant) {
    let left = ()=>{
        console.log(`participant ${participant} left`);
        participants.delete(participant.id);
    }
}
//get info
publications.onLengthAdd((key, value)=>{
    publicationEvent(publications.get(key));
    $('#availablepublication').append($(`<option value=${key}>${key}</option>`));
})
subscriptions.onLengthAdd((key, value)=>{
    subscriptionEvent(subscriptions.get(key));
    $('#availablesubscription').append($(`<option value=${key}>${key}</option>`));
})
localStreams.onLengthAdd((key, value)=>{
    streamEvent(localStreams.get(key));
    $('#localstreamid').append($(`<option value=${key}>${key}</option>`));
})
remoteStreamMap.onLengthAdd((key, value)=>{
    streamEvent(remoteStreamMap.get(key));
    $('#availableremotestream').append($(`<option value=${key}>${key}</option>`));
    $('#forwardstreamlist').append($(`<option value=${key}>${key}</option>`));
    $('#audiofromlist').append($(`<option value=${key}>${key}</option>`));
    $('#videofromlist').append($(`<option value=${key}>${key}</option>`));
    let index = key.indexOf('-')
    if( index != -1){
        let view = key.slice(index+1);
        $('#viewlist').append($(`<option value=${view}>${view}</option>`));
        $('#mixstreamlist').append($(`<option value=${key}>${key}</option>`));
    }
})

participants.onLengthAdd((key, value)=>{
    participantEvent(value);
    $('#participantsid').append($(`<option value=${key}>${key}</option>`));
    $('#participantlist').append($(`<option value=${key}>${key}</option>`));
})
allRooms.onLengthAdd((key, value)=>{
    $('#roomlist').append($(`<option value=${key}>${key}</option>`));
})

//remove info
publications.onLengthRemove((key)=>{
    $('#availablepublication').find(`[value=${key}]`).remove();
})
subscriptions.onLengthRemove((key)=>{
    $('#availablesubscription').find(`[value=${key}]`).remove();
})
localStreams.onLengthRemove((key)=>{
    $('#localstreamid').find(`[value=${key}]`).remove();
})
remoteStreamMap.onLengthRemove((key)=>{
    $('#availableremotestream').find(`[value=${key}]`).remove();
    $('#forwardstreamlist').find(`[value=${key}]`).remove();
    $('#audiofromlist').find(`[value=${key}]`).remove();
    $('#videofromlist').find(`[value=${key}]`).remove();
})
participants.onLengthRemove((key)=>{
    $('#participantsid').find(`[value=${key}]`).remove();
    $('#participantlist').find(`[value=${key}]`).remove();
})
allRooms.onLengthRemove((key)=>{
    $('#roomlist').find(`[value=${key}]`).remove();
})


//LocalStream action
function stopLocal (name='camera') {
    let localStream = localStreams.get(name);
    if(localStream){
        localStream.mediaStream.getTracks().forEach(track=>{
            track.stop();
        })
        localStreams.delete(name);
    }
}
function createLocalStream () {
    stopLocal();
    let videoSource = $('input[name="videosource"]:checked').val();
    let audioSource = $('input[name="audiosource"]:checked').val();
    let extensionId = $('#extensionid').val();
    let resolution = resolutionName2Value[$('#resolution').val()] || {width: 640, height: 480};
    let mediaStreamDeviceConstraints = {
        audio:{
            source: audioSource
        },
        video:{
            source: videoSource,
            resolution: resolution
        }
    };
    let streamSourceInfo = new StreamSourceInfo(audioSource, videoSource);
    MediaStreamFactory.createMediaStream(mediaStreamDeviceConstraints)
    .then((mediaStream)=>{
        console.log('create media stream success: ', mediaStream);
        return new LocalStream(mediaStream, streamSourceInfo)
    },(err)=>{
        console.log('create mediaStream failed: ', err);
    })
    .then((localStream)=>{
        displayStream(localStream, 'local');
        localStreams.set(localStream.source.video, localStream);
        console.log('create local stream success: ', localStream);
    }, (err)=>{
        console.log('create local stream failed: ', localStream);
    })

}

//client action
function leave () {
    client.leave()
    .then(()=>{
        console.log('client leave success');
    },(err)=>{
        console.log('client leave failed: ', err);
    })
}
let newToken='';
function createNewToken () {
    let role = $('#role').val();
    createToken(defaultRoomId, 'testuser', role, (token)=>{
        newToken=token;
        console.log('create a new token success: ', token);
    },(err)=>{
        console.log('create a new token failed: ', err);
    })
}
function joinRoom() {
    client.join(newToken)
    .then((resp)=>{
        console.log('join room with a new token success: ', resp);
        resp.remoteStreams.forEach(function (remoteStream) {
            remoteStreamMap.set(remoteStream.id, remoteStream);
        })
    }, (err)=>{
        console.log('join room with a new token failed: ', err);
    })
}

function publish () {
    let localStream = localStreams.get($('#localstreamid').val());
    let audioCodec = $('#audiocodec').val();
    let videoCodec = $('#videocodec').val();
    let options = {
        audio:[{
            codec: audioCodec,
        }],
        video:[{
            codec: videoCodec,
        }]
    };
    client.publish(localStreams.get(localStream.source.video), options).then((publication)=>{
        publications.set(publication.id, publication);
        console.log(`get publication ${publication.id}`);
    },(err)=>{
        console.log('publish local stream failed: ', err);
    })
}

function addOption (select, optionsArr) {
    select.children('option:not(:first)').remove();
    optionsArr.forEach((item)=>{
        if(typeof item === 'object'){
            select.append($(`<option value=${JSON.stringify(item)}>${item.name || item.width + 'X' + item.height}</option>`));
        }else{
            select.append($(`<option value=${item}>${item}</option>`));
        }
    })
}
$('#availableremotestream').change(function () {
   let remoteStream = remoteStreamMap.get($(this).val());
   let supportedAudioCodecs = remoteStream.capabilities.audio.codecs;
   let {'codecs': supportedVideoCodecs, bitrateMultipliers, frameRates, keyFrameIntervals, resolutions} = remoteStream.capabilities.video;
   addOption($('#supportedaudiocodec'), supportedAudioCodecs);
   addOption($('#supportedvideocodec'), supportedVideoCodecs);
   addOption($('#supportedbitrate'), bitrateMultipliers);
   addOption($('#supportedresolution'), resolutions);
   addOption($('#supportedframerate'), frameRates);
   addOption($('#supportedkfi'), keyFrameIntervals);
})

function getSubOptions () {
    let remoteStreamId = $('#availableremotestream').val();
    if(!remoteStreamId) throw new Error('You must select one remote stream');
    let trackKind = $('#subscribetrackkind').val() || undefined;
    let videoCodec = $('#supportedvideocodec').val()?JSON.parse($('#supportedvideocodec').val()):undefined;
    let audioCodec = $('#supportedaudiocodec').val()?JSON.parse($('#supportedaudiocodec').val()):undefined;
    let bitrate = $('#supportedbitrate').val()?parseFloat($('#supportedbitrate').val()):undefined;
    let resolution = $('#supportedresolution').val()?JSON.parse($('#supportedresolution').val()):undefined;
    let frameRate =$('#supportedframerate').val()?parseInt($('#supportedframerate').val()):undefined;
    let kfi = $('#supportedkfi').val()?parseInt($('#supportedkfi').val()):undefined;
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

function subscribe () {
    let {
        remoteStreamId,
        trackKind,
        videoCodec,
        audioCodec,
        bitrate,
        resolution,
        frameRate,
        kfi,
    } = getSubOptions();;
    let subOptions = {
        audio:{
            codecs:[audioCodec]
        },
        video:{
            codecs:[videoCodec],
            resolution: resolution,
            frameRate: frameRate,
            bitrateMultiplier: bitrate,
            keyFrameInterval: kfi,
        }
    }
    client.subscribe(remoteStreamMap.get(remoteStreamId), subOptions)
    .then((subscription)=>{
        subscription.originId = remoteStreamId;
        subscriptions.set(subscription.id, subscription);
        displayStream(remoteStreamMap.get(remoteStreamId));
        console.log('subscribe success: ', subscription);
    }, err =>{
        console.log('subscribe failed: ', err);
    })
}
function sendMessage () {
    let message = $('#sendcontent').val();
    let participantId = $('#participantsid').val();
    client.send(message, participantId)
    .then(()=>{
        console.log(`send message to ${participantId} success`);
    }, (err)=>{
        console.log(`send message to ${participantId} failed`, err);
    })
}
//publication action
function getPublicationParams () {
    let publicationArr = [];
    let id = $('#availablepublication').val();
    if(id === 'all'){
        publications.forEach((value)=>{
            publicationArr.push(value);
        })
    }else{
        publicationArr.push(publications.get(id));
    }
    return [publicationArr, $('#publicationtrackkind').val()]
}
function publicationMute() {
    let [publicationArr, trackKind] = getPublicationParams();
    publicationArr.forEach((publication)=>{
        publication.mute(trackKind)
        .then(()=>{
            console.log(`${publication.id} mute ${trackKind} success`)
        }, err=>{
            console.log(`${publication.id} mute failed: `, err);
        })
    })
}
function publicationUnmute() {
    let [publicationArr, trackKind] = getPublicationParams();
    publicationArr.forEach((publication)=>{
        publication.unmute(trackKind)
        .then(()=>{
            console.log(`${publication.id} unmute ${trackKind} success`)
        }, err=>{
            console.log(`${publication.id} unmute failed: `, err);
        })
    })
}
function publicationGetStats() {
    let [publicationArr] = getPublicationParams();
    publicationArr.forEach((publication)=>{
        publication.getStats()
        .then((stats)=>{
            console.log(`${publication.id} getStats success`, stats);
        }, err=>{
            console.log(`${publication.id} getStats failed: `, err);
        })
    })
}
function publicationStop() {
    let [publicationArr] = getPublicationParams();
    publicationArr.forEach((publication)=>{
        publication.stop()
        .then((stats)=>{
            console.log(`${publication.id} stop success`);
        }, err=>{
            console.log(`${publication.id} stop failed: `, err);
        })
    })
}
//subscription action
function getSubcriptionParams () {
    let subscriptionArr = [];
    let id = $('#availablesubscription').val();
    if(id === 'all'){
        subscriptions.forEach((value)=>{
            subscriptionArr.push(value);
        })
    }else{
        subscriptionArr.push(subscriptions.get(id));
    }
    return [subscriptionArr, $('#subscriptiontrackkind').val()]
}
function subscriptionMute() {
    let [subscriptionArr, trackKind] = getSubcriptionParams();
    subscriptionArr.forEach(subscription=>{
        subscription.mute(trackKind)
        .then(()=>{
            console.log(`${subscription.id} mute ${trackKind} success`)
        }, err=>{
            console.log(`${subscription.id} mute failed: `, err);
        })
    })
}
function subscriptionUnmute() {
    let [subscriptionArr, trackKind] = getSubcriptionParams();
    subscriptionArr.forEach(subscription=>{
        subscription.unmute(trackKind)
        .then(()=>{
            console.log(`${subscription.id} unmute ${trackKind} success`)
        }, err=>{
            console.log(`${subscription.id} unmute failed: `, err);
        })
    })
}
function subscriptionGetStats() {
    let [subscriptionArr] = getSubcriptionParams();
    subscriptionArr.forEach(subscription=>{
        subscription.getStats()
        .then((stats)=>{
            console.log(`${subscription.id} getStats success`, stats);
        }, err=>{
            console.log(`${subscription.id} getStats failed: `, err);
        })
    })
}
function subscriptionStop() {
    let [subscriptionArr] = getSubcriptionParams();
    subscriptionArr.forEach(subscription=>{
        subscription.stop()
        .then((stats)=>{
            destroyStreamUi(subscription);
            console.log(`${subscription.id} stop success`);
        }, err=>{
            console.log(`${subscription.id} stop failed: `, err);
        })
    })
}

function subscriptionApplyOption () {
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
        audio:{
            codecs:[audioCodec]
        },
        video:{
            codecs:[videoCodec],
            resolution: resolution,
            frameRate: frameRate,
            bitrateMultiplier: bitrate,
            keyFrameInterval: kfi,
        }
    }

    subscriptions.get(subscriptionId).applyOptions(options)
    .then(()=>{
        console.log(`subscription ${subscriptionId} apply options success, options are: `, options);
    }, err=>{
        console.log('subscription apply options failed: ', err);
    })
}

window.onload = function() {
    defaultRoomId = getParameterByName("room");
    role = getParameterByName('role') || 'presenter';
    isPublish = getParameterByName('publish') === 'false'?false:true;
    resolution = resolutionName2Value[getParameterByName('resolution') || 'vga'];
    videoCodec = getParameterByName('videoCodec') || 'h264';
    audioCodec = getParameterByName('audioCodec') || 'opus';
    hasVideo = getParameterByName('hasVideo') === 'false'?false:true;
    hasAudio = getParameterByName('hasAudio') === 'false'?false:true;
    isJoin = getParameterByName('join') === 'false'?false:true;
    let publishOputions = {
        audio:[{
            codec: {name: audioCodec}
        }],
        video:[{
            codec: {name: videoCodec}
        }]
    }


    isJoin && createToken(defaultRoomId, 'testuser', role, (token)=>{
        client.join(token)
        .then((resp)=>{
            console.log('join success:', resp);
            let{'participants': participantArr, remoteStreams} = resp;
            //subscribe remote streams.
            remoteStreams.forEach((stream)=>{
                remoteStreamMap.set(stream.id, stream);
                subs(stream)
            })
            participantArr.forEach((participant)=>{
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
            .then((mediaStream)=>{
                console.log('media stream is', mediaStream);
                return new LocalStream(mediaStream, new StreamSourceInfo('mic', 'camera'));
            },(err)=>{
                console.log('create media stream failed: ', err);
            })
            .then((localStream)=>{
                localStreams.set("camera", localStream);
                console.log('local stream is:', localStream);
                displayStream(localStream, 'local');
                isPublish && client.publish(localStream, publishOputions)
                .then((publication)=>{
                    publications.set(publication.id, publication);
                    console.log('publish success:', publication);
                }, err=>{
                    console.log('publish failed', err);
                })
            }, err =>{
                console.log('create local stream failed:', err);
            });
        }, (err)=>{
            console.log('join failed', err);
        })
    }, (err)=>{
        console.error(`create token failed: ${err}`)
    })



};


window.onbeforeunload = function() {
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
