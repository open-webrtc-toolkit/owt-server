let {
    expect,
    ConferenceClient,
    MediaStreamFactory,
    Resolution,
    StreamSourceInfo,
    LocalStream,
    RemoteStream,
    config,
    add,
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
} = require('./restApi.js');
let {
    createLocal,
    join,
    publish,
    publishCase,
    subscribe,
} = require('./jssdk.js');
let {
    createRoomPars,
    updateRoomPars,
    forbidSubPars,
    forbidPubPars,
    startRecordingPars,
    updateRecordingPars,
    startrecordCreateroomOptions,
    updaterecordCreateroomOptions,
    jsonPost,
    sipcallsPostp,
    sipcallsPatchp,
} = require('./paramsRest.js');
const forEach = require('mocha-each');

describe('rest_api_case', function () {
    this.timeout(12000);

    //listRoomsException
    it('listRoomsEmpty', function (done) {
        listRooms()
            .then((resp) => {
                if (resp.length > 0) {
                    let ArrayDeleteRooms = [];
                    for (var i = 0; i <= resp.length - 1; i++) {
                        ArrayDeleteRooms.push(deleteRoom(resp[i]._id))
                    };
                    Promise.all(ArrayDeleteRooms)
                        .then((array) => {
                            console.log("delete all : ", array.length, array)
                            return listRooms();
                        })
                        .then((listRoomsEmpty) => {
                            console.log("lishRoomsEmpty", listRoomsEmpty)
                            expect(listRoomsEmpty.length).to.equal(0);
                            done();
                        })
                        .catch((err) => {
                            console.log("listRoomsEmpty catch : ", err);
                            done(err);
                        })
                }
                else {
                    listRooms()
                        .then((listRoomsEmpty) => {
                            console.log("lishRoomsEmpty", listRoomsEmpty)
                            expect(listRoomsEmpty.length).to.equal(0);
                            done();
                        })
                        .catch((err) => {
                            console.log("listRoomsEmpty catch : ", err);
                            done(err);
                        })
                }

            })
    })
    //getroomException
    it('getroomEmpty', function (done) {
        getRoom('5cb5ba3e99be6461b99c1a1')
            .then((err) => {
                done(err);
            })
            .catch((Expect) => {
                console.log("getroomEmpty catch : ", Expect)
                done();


            })
    })
    //createRoomException
    it('createRoomEmpty', function (done) {
        createRoom({})
            .then((err) => {
                done(err);
            })
            .catch((Expect) => {
                console.log("getroomEmpty catch : ", Expect)
                done();


            })
    })
    //deleteRoomException
    it('deleteRoomEmpty', function (done) {
        deleteRoom('5cb5bd574a733a61beb159f0')
            .then((err) => {
                done(err);
            })
            .catch((Expect) => {
                console.log("deleteRoomEmpty catch : ", Expect)
                done()
            })
    })

    //listParticipantsEmpty
    it('listParticipantsEmpty', function (done) {
        createRoomX()
            .then((room) => {
                return listParticipants(room._id);
            })
            .then((listParticipantsEmpty) => {
                console.log("listParticipantsEmpty", listParticipantsEmpty)
                expect(listParticipantsEmpty.length).to.equal(0);
                done();
            })
            .catch((err) => {
                console.log("listParticipantsEmpty catch : ", err)
                done(err);
            })


    })
    //getParticipantEmpty
    it('getParticipantEmpty', function (done) {
        createRoomX()
            .then((room) => {
                return getParticipant(room._id, "5VbiI4cRn0eTJnJnAAAB");

            })
            .then((err) => {
                document(err);
            })
            .catch((Expect) => {
                console.log("getParticipantEmpty catch : ", Expect)
                done();

            })

    })
    //dropParticipantEmpty
    it('dropParticipantEmpty', function (done) {
        createRoomX()
            .then((room) => {
                return dropParticipant(room._id, "5VbiI4cRn0eTJnJnAAAB");
            })
            .then((err) => {
                done(err);
            })
            .catch((Expect) => {
                console.log("dropParticipantEmpty Expect : ", Expect)
                done();
            })
    })
    //listStreamsEmpty
    it('listStreamsEmpty', function (done) {
        createRoomX()
            .then((room) => {
                return listStreams(room._id);
            })
            .then((listStreamsEmpty) => {
                console.log("listStreamsEmpty : ", listStreamsEmpty)
                expect(listStreamsEmpty.length).to.equal(0);
                done();

            })
            .catch(err => {
                console.log("listStreamsEmpty  catch : ", err)
                done(err);
            })
    })
    //listRooms
    it('listRooms', function (done) {
        createRoomX().then(() => {
            return listRooms();
        })
            .then((listRooms) => {
                console.log("Get listRoomS :", listRooms.length);
                if (listRooms.length >= 0) {
                    expect(listRooms[listRooms.length - 1]).to.have.property('_id');
                    deleteRoom(listRooms[listRooms.length - 1]._id)
                        .then(() => {
                            done();
                        })
                }
            })
            .catch((err) => {
                console.log("listRooms catch :", err);
                deleteRoom(listRooms[listRooms.length - 1]._id)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //getroom
    it('getRoom', function (done) {
        let roomId;
        createRoomX()
            .then((room) => {
                roomId = room._id;
                return createToken(roomId, 'testuser', 'presenter')
            })
            .then((token) => {
                return join(token)
            })
            .then(() => {
                return getRoom(roomId)
            })
            .then((getRoom) => {
                console.log("GetRoom", getRoom._id, roomId);
                expect(getRoom._id).to.equal(roomId);
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch(err => {
                console.log('Getroom catch: ', err);
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //createRoom
    forEach(createRoomPars)
        .it("createRoom", function (createRoomPars, done) {
            let roomId;
            createRoom(createRoomPars)
                .then((createRoom) => {
                    roomId = createRoom._id;
                    console.log("CreateRoom", createRoom._id)
                    expect(createRoom).to.have.property('_id');
                    expect(createRoom.name).to.equal(createRoomPars.name);
                    expect(createRoom.participantLimit).to.equal(createRoomPars.options.participantLimit);
                    expect(createRoom.inputLimit).to.equal(createRoomPars.options.inputLimit);
                    expect(createRoom.roles[0].role).to.equal(createRoomPars.options.roles[0].role);
                    expect(createRoom.roles[0].publish.video).to.equal(createRoomPars.options.roles[0].publish.video);
                    expect(createRoom.roles[0].publish.audio).to.equal(createRoomPars.options.roles[0].publish.audio);
                    expect(createRoom.roles[0].subscribe.video).to.equal(createRoomPars.options.roles[0].subscribe.video);
                    expect(createRoom.roles[0].subscribe.audio).to.equal(createRoomPars.options.roles[0].subscribe.audio);
                    expect(createRoom.views[0].label).to.equal(createRoomPars.options.views[0].label);
                    expect(createRoom.views[0].audio.format.codec).to.equal(createRoomPars.options.views[0].audio.format.codec);
                    expect(createRoom.views[0].audio.format.sampleRate).to.equal(createRoomPars.options.views[0].audio.format.sampleRate);
                    expect(createRoom.views[0].audio.format.channelNum).to.equal(createRoomPars.options.views[0].audio.format.channelNum);
                    expect(createRoom.views[0].audio.vad).to.equal(createRoomPars.options.views[0].audio.vad);
                    expect(createRoom.views[0].video.format.profile).to.equal(createRoomPars.options.views[0].video.format.profile);
                    expect(createRoom.views[0].video.format.codec).to.equal(createRoomPars.options.views[0].video.format.codec);
                    expect(createRoom.views[0].video.parameters.resolution.width).to.equal(createRoomPars.options.views[0].video.parameters.resolution.width);
                    expect(createRoom.views[0].video.parameters.resolution.height).to.equal(createRoomPars.options.views[0].video.parameters.resolution.height);
                    expect(createRoom.views[0].video.parameters.framerate).to.equal(createRoomPars.options.views[0].video.parameters.framerate);
                    expect(createRoom.views[0].video.parameters.bitrate).to.equal(createRoomPars.options.views[0].video.parameters.bitrate);
                    expect(createRoom.views[0].video.parameters.keyFrameInterval).to.equal(createRoomPars.options.views[0].video.parameters.keyFrameInterval);
                    expect(createRoom.views[0].video.maxInput).to.equal(createRoomPars.options.views[0].video.maxInput);
                    expect(createRoom.views[0].video.bgColor.r).to.equal(createRoomPars.options.views[0].video.bgColor.r);
                    expect(createRoom.views[0].video.bgColor.g).to.equal(createRoomPars.options.views[0].video.bgColor.g);
                    expect(createRoom.views[0].video.bgColor.b).to.equal(createRoomPars.options.views[0].video.bgColor.b);
                    expect(createRoom.views[0].video.motionFactor).to.equal(createRoomPars.options.views[0].video.motionFactor);
                    expect(createRoom.views[0].video.keepActiveInputPrimary).to.equal(createRoomPars.options.views[0].video.keepActiveInputPrimary);
                    expect(createRoom.views[0].video.layout.fitPolicy).to.equal(createRoomPars.options.views[0].video.layout.fitPolicy);
                    expect(createRoom.views[0].video.layout.templates.base).to.equal(createRoomPars.options.views[0].video.layout.templates.base);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].id).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].id);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].shape).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].shape);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].area.left);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].area.top);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].area.width);
                    expect(createRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(createRoomPars.options.views[0].video.layout.templates.custom[0].region[0].area.height);
                    expect(createRoom.mediaIn.audio.toString()).to.equal(createRoomPars.options.mediaIn.audio.toString());
                    expect(createRoom.mediaIn.video.toString()).to.equal(createRoomPars.options.mediaIn.video.toString());
                    expect(createRoom.mediaOut.audio.toString()).to.equal(createRoomPars.options.mediaOut.audio.toString());
                    expect(createRoom.mediaOut.video.format.toString()).to.equal(createRoomPars.options.mediaOut.video.format.toString());
                    expect(createRoom.mediaOut.video.parameters.resolution.toString()).to.equal(createRoomPars.options.mediaOut.video.parameters.resolution.toString());
                    expect(createRoom.mediaOut.video.parameters.framerate.toString()).to.equal(createRoomPars.options.mediaOut.video.parameters.framerate.toString());
                    expect(createRoom.mediaOut.video.parameters.bitrate.toString()).to.equal(createRoomPars.options.mediaOut.video.parameters.bitrate.toString());
                    expect(createRoom.mediaOut.video.parameters.keyFrameInterval.toString()).to.equal(createRoomPars.options.mediaOut.video.parameters.keyFrameInterval.toString());
                    expect(createRoom.transcoding.audio).to.equal(createRoomPars.options.transcoding.audio);
                    expect(createRoom.transcoding.video.parameters.resolution).to.equal(createRoomPars.options.transcoding.video.parameters.resolution);
                    expect(createRoom.transcoding.video.parameters.framerate).to.equal(createRoomPars.options.transcoding.video.parameters.framerate);
                    expect(createRoom.transcoding.video.parameters.bitrate).to.equal(createRoomPars.options.transcoding.video.parameters.bitrate);
                    expect(createRoom.transcoding.video.parameters.keyFrameInterval).to.equal(createRoomPars.options.transcoding.video.parameters.keyFrameInterval);
                    expect(createRoom.notifying.participantActivities).to.equal(createRoomPars.options.notifying.participantActivities);
                    expect(createRoom.sip.sipServer).to.equal(createRoomPars.options.sip.sipServer);
                    expect(createRoom.sip.username).to.equal(createRoomPars.options.sip.username);
                    expect(createRoom.sip.password).to.equal(createRoomPars.options.sip.password);
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("createRoom catch :", err);
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    //deleteRoom
    it("deleteRoom", function (done) {
        let roomId;
        createRoomX().then((resp) => {
            roomId = resp._id;
            return deleteRoom(roomId);
        })
            .then((resp) => {
                console.log("Deleteroom", resp)
                expect(resp).to.include('Room deleted');
                done();
            })
            .catch(err => {
                console.log('Delete roomid catch: ', err);
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //updateRoom
    forEach(updateRoomPars)
        .it("updateRoom", function (updateRoomPars, done) {
            let roomId;
            let config;
            createRoomX().then((resp) => {
                roomId = resp._id;

                return updateRoom(roomId, updateRoomPars);
            })
                .then((updateRoom) => {
                    console.log("PATCH updateRoom", updateRoom)
                    expect(updateRoom).to.have.property('_id');
                    expect(updateRoom.name).to.equal(updateRoomPars.name);
                    expect(updateRoom.participantLimit).to.equal(updateRoomPars.participantLimit);
                    expect(updateRoom.inputLimit).to.equal(updateRoomPars.inputLimit);
                    expect(updateRoom.roles[0].role).to.equal(updateRoomPars.roles[0].role);
                    expect(updateRoom.roles[0].publish.video).to.equal(updateRoomPars.roles[0].publish.video);
                    expect(updateRoom.roles[0].publish.audio).to.equal(updateRoomPars.roles[0].publish.audio);
                    expect(updateRoom.roles[0].subscribe.video).to.equal(updateRoomPars.roles[0].subscribe.video);
                    expect(updateRoom.roles[0].subscribe.audio).to.equal(updateRoomPars.roles[0].subscribe.audio);
                    expect(updateRoom.views[0].label).to.equal(updateRoomPars.views[0].label);
                    expect(updateRoom.views[0].audio.format.codec).to.equal(updateRoomPars.views[0].audio.format.codec);
                    expect(updateRoom.views[0].audio.format.sampleRate).to.equal(updateRoomPars.views[0].audio.format.sampleRate);
                    expect(updateRoom.views[0].audio.format.channelNum).to.equal(updateRoomPars.views[0].audio.format.channelNum);
                    expect(updateRoom.views[0].audio.vad).to.equal(updateRoomPars.views[0].audio.vad);
                    expect(updateRoom.views[0].video.format.profile).to.equal(updateRoomPars.views[0].video.format.profile);
                    expect(updateRoom.views[0].video.format.codec).to.equal(updateRoomPars.views[0].video.format.codec);
                    expect(updateRoom.views[0].video.parameters.resolution.width).to.equal(updateRoomPars.views[0].video.parameters.resolution.width);
                    expect(updateRoom.views[0].video.parameters.resolution.height).to.equal(updateRoomPars.views[0].video.parameters.resolution.height);
                    expect(updateRoom.views[0].video.parameters.framerate).to.equal(updateRoomPars.views[0].video.parameters.framerate);
                    expect(updateRoom.views[0].video.parameters.bitrate).to.equal(updateRoomPars.views[0].video.parameters.bitrate);
                    expect(updateRoom.views[0].video.parameters.keyFrameInterval).to.equal(updateRoomPars.views[0].video.parameters.keyFrameInterval);
                    expect(updateRoom.views[0].video.maxInput).to.equal(updateRoomPars.views[0].video.maxInput);
                    expect(updateRoom.views[0].video.bgColor.r).to.equal(updateRoomPars.views[0].video.bgColor.r);
                    expect(updateRoom.views[0].video.bgColor.g).to.equal(updateRoomPars.views[0].video.bgColor.g);
                    expect(updateRoom.views[0].video.bgColor.b).to.equal(updateRoomPars.views[0].video.bgColor.b);
                    expect(updateRoom.views[0].video.motionFactor).to.equal(updateRoomPars.views[0].video.motionFactor);
                    expect(updateRoom.views[0].video.keepActiveInputPrimary).to.equal(updateRoomPars.views[0].video.keepActiveInputPrimary);
                    expect(updateRoom.views[0].video.layout.fitPolicy).to.equal(updateRoomPars.views[0].video.layout.fitPolicy);
                    expect(updateRoom.views[0].video.layout.templates.base).to.equal(updateRoomPars.views[0].video.layout.templates.base);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].id).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].id);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].shape).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].shape);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].area.left);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].area.top);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].area.width);
                    expect(updateRoom.views[0].video.layout.templates.custom[0].region[0].area.left).to.equal(updateRoomPars.views[0].video.layout.templates.custom[0].region[0].area.height);
                    expect(updateRoom.mediaIn.audio.toString()).to.equal(updateRoomPars.mediaIn.audio.toString());
                    expect(updateRoom.mediaIn.video.toString()).to.equal(updateRoomPars.mediaIn.video.toString());
                    expect(updateRoom.mediaOut.audio.toString()).to.equal(updateRoomPars.mediaOut.audio.toString());
                    expect(updateRoom.mediaOut.video.format.toString()).to.equal(updateRoomPars.mediaOut.video.format.toString());
                    expect(updateRoom.mediaOut.video.parameters.resolution.toString()).to.equal(updateRoomPars.mediaOut.video.parameters.resolution.toString());
                    expect(updateRoom.mediaOut.video.parameters.framerate.toString()).to.equal(updateRoomPars.mediaOut.video.parameters.framerate.toString());
                    expect(updateRoom.mediaOut.video.parameters.bitrate.toString()).to.equal(updateRoomPars.mediaOut.video.parameters.bitrate.toString());
                    expect(updateRoom.mediaOut.video.parameters.keyFrameInterval.toString()).to.equal(updateRoomPars.mediaOut.video.parameters.keyFrameInterval.toString());
                    expect(updateRoom.transcoding.audio).to.equal(updateRoomPars.transcoding.audio);
                    expect(updateRoom.transcoding.video.parameters.resolution).to.equal(updateRoomPars.transcoding.video.parameters.resolution);
                    expect(updateRoom.transcoding.video.parameters.framerate).to.equal(updateRoomPars.transcoding.video.parameters.framerate);
                    expect(updateRoom.transcoding.video.parameters.bitrate).to.equal(updateRoomPars.transcoding.video.parameters.bitrate);
                    expect(updateRoom.transcoding.video.parameters.keyFrameInterval).to.equal(updateRoomPars.transcoding.video.parameters.keyFrameInterval);
                    expect(updateRoom.notifying.participantActivities).to.equal(updateRoomPars.notifying.participantActivities);
                    expect(updateRoom.sip.sipServer).to.equal(updateRoomPars.sip.sipServer);
                    expect(updateRoom.sip.username).to.equal(updateRoomPars.sip.username);
                    expect(updateRoom.sip.password).to.equal(updateRoomPars.sip.password);
                    roomId = updateRoom._id;
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch(err => {
                    console.log("updateRoom catch", err);
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    it('listParticipants', function (done) {
        joinX().then(() => {
            return listParticipants(roomId)
        })
            .then((listParticipants) => {
                console.log("Get  listParticipants", listParticipants, roomId);
                expect(listParticipants[0]).to.have.property('id');
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch(err => {
                console.log('listParticipants catch: ', err);
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    // listParti
    //getParticipant
    it('getParticipant', function (done) {
        joinX().then(() => {
            return listParticipants(roomId)
        })
            .then((listParticipants) => {
                return getParticipant(roomId, listParticipants[0].id);
            })
            .then((getParticipant) => {
                console.log("getParticipant", getParticipant)
                expect(getParticipant).to.have.property('id');
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch(err => {
                console.log('getparticipant catch: ', err);
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //forbidSub
    forEach(forbidSubPars)
        .it('forbidSub', function (forbidSubPars, done) {
            joinX().then((resp) => {
                console.log(`Join success`, resp);
                return listParticipants(roomId)
            })
                .then((resp) => {
                    let forbidSubParsArray = [];
                    forbidSubParsArray.push(forbidSubPars);
                    return forbidSub(roomId, resp[0].id, forbidSubParsArray);
                })
                .then((forbidSub) => {
                    console.log("forbidSub:", forbidSub)
                    expect(forbidSub).to.have.property('id');
                    if (forbidSubPars.path == "/permission/subscribe/audio" && forbidSubPars.value == false) {
                        expect(forbidSub.permission.subscribe.audio).to.equal(forbidSubPars.value);
                        console.log('start subscribe');
                        subscribe()
                            .then((resp) => {
                                expect(resp).to.equal('unauthorized');
                            }, err => {
                                expect(err).to.equal('unauthorized');
                                console.log(`caseAudioFalse success : ${err}`);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            })
                            .catch(function (err) {
                                console.log('caseAudioFalse catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                    else if (forbidSubPars.path == "/permission/subscribe/audio" && forbidSubPars.value == true) {
                        expect(forbidSub.permission.subscribe.audio).to.equal(forbidSubPars.value);
                        subscribe()
                            .then((resp) => {
                                expect(resp).to.have.property('id');
                                console.log("caseAudiotrue success : ", resp);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            }, err => {
                                expect(err).to.have.property('id');
                            })
                            .catch(function (err) {
                                console.log('caseAudiotrue catch : ', err)
                                deleteRoom(roomId)
                                    .then((err) => {
                                        done();
                                    })
                            })
                    }
                    else if (forbidSubPars.path == "/permission/subscribe/video" && forbidSubPars.value == false) {
                        expect(forbidSub.permission.subscribe.video).to.equal(forbidSubPars.value)
                        subscribe()
                            .then((resp) => {
                                expect(resp).to.equal('unauthorized');
                            }, err => {
                                expect(err).to.equal('unauthorized');
                                console.log(`caseVideoFalse success :  ${err}`);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            })
                            .catch(function (err) {
                                console.log('caseVideoFalse catch : ', err);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                    else if (forbidSubPars.path == "/permission/subscribe/video" && forbidSubPars.value == true) {
                        expect(forbidSub.permission.subscribe.video).to.equal(forbidSubPars.value)
                        subscribe()
                            .then((resp) => {
                                expect(resp).to.have.property('id');
                                console.log("caseVideoTrue success : ", resp);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            }, err => {
                                expect(err).to.have.property('id');
                            })
                            .catch(function (err) {
                                console.log('caseVideoTrue catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                })
                .catch(err => {
                    console.log('forbidSub catch: ', err);
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    //forbidPub
    forEach(forbidPubPars)
        .it('forbidPub', function (forbidPubPars, done) {
            joinX().then(() => {
                return listParticipants(roomId)
            })
                .then((resp) => {
                    let forbidPubParsArray = [];
                    forbidPubParsArray.push(forbidPubPars);
                    return forbidPub(roomId, resp[0].id, forbidPubParsArray);
                })
                .then((forbidPub) => {
                    console.log("forbidPub:", forbidPub)
                    expect(forbidPub).to.have.property('id');
                    if (forbidPubPars.path == "/permission/publish/audio" && forbidPubPars.value == false) {
                        expect(forbidPub.permission.publish.audio).to.equal(forbidPubPars.value);
                        console.log('start pubscribe');
                        publishCase()
                            .then((resp) => {
                                expect(resp).to.equal('unauthorized');
                            }, err => {
                                console.log("123", err)
                                expect(err.toString()).to.equal('Error: ICE connection failed or closed.');
                                console.log(`caseAudioFalse success : ${err}`);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            })
                            .catch(function (err) {
                                console.log('caseAudioFalse catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                    else if (forbidPubPars.path == "/permission/publish/audio" && forbidPubPars.value == true) {
                        expect(forbidPub.permission.publish.audio).to.equal(forbidPubPars.value);
                        publishCase()
                            .then((resp) => {
                                expect(resp).to.have.property('id');
                                console.log("caseAudioTrue success : ", resp)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            }, err => {
                                expect(err).to.have.property('id');
                            })
                            .catch(function (err) {
                                console.log('caseAudioTrue catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                    else if (forbidPubPars.path == "/permission/publish/video" && forbidPubPars.value == false) {
                        expect(forbidPub.permission.publish.video).to.equal(forbidPubPars.value)
                        publishCase()
                            .then((resp) => {
                                expect(resp).to.equal('unauthorized');
                            }, err => {
                                expect(err.toString()).to.equal('Error: ICE connection failed or closed.');
                                console.log(`caseVideofalse success :  ${err}`);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            })
                            .catch(function (err) {
                                console.log('caseVideofalse catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                    else if (forbidPubPars.path == "/permission/publish/video" && forbidPubPars.value == true) {
                        expect(forbidPub.permission.publish.video).to.equal(forbidPubPars.value)
                        publishCase()
                            .then((resp) => {
                                expect(resp).to.have.property('id');
                                console.log("caseVideotrue success : ", resp)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            }, err => {
                                expect(err).to.have.property('id');
                            })
                            .catch(function (err) {
                                console.log('caseVideotrue catch : ', err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    }
                })
                .catch(err => {
                    console.log('forbidPub err: ', err);
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    //dropParticipant
    it('dropParticipant', function (done) {
        joinX().then(() => {
            return listParticipants(roomId)
        })
            .then((resp) => {
                return dropParticipant(roomId, resp[0].id);
            })
            .then((dropParticipant) => {
                console.log("dropParticipant", dropParticipant)
                expect(dropParticipant).to.include('');
                listParticipants(roomId)
                    .then((listParticipants) => {
                        expect(listParticipants.length).to.equal(0);
                        deleteRoom(roomId)
                            .then(() => {
                                done();
                            })
                    })
            })
            .catch(err => {
                console.log('dropParticipant catch: ', err);
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //listStreams
    it('listStreams', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then(() => {
                    return listStreams(roomId)
                })
                .then((listStreams) => {
                    console.log("listStreams", listStreams)
                    expect(listStreams[0].id).to.equal(roomId + '-common');
                    expect(listStreams[1]).to.have.property('id');
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("listStreams catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //getStream
    it('getStream', function (done) {

        let streamId;
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    streamId = resp[1].id;
                    return getStream(roomId, resp[1].id)
                })
                .then((GetStream) => {
                    console.log('GetStream', GetStream)
                    expect(GetStream.id).to.equal(streamId);
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("getStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //unmixStream
    it('unmixStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    return unmixStream(roomId, resp[1].id, 'common')
                })
                .then((unmixStream) => {
                    console.log('UnmixStream', unmixStream)
                    expect(unmixStream).to.have.property('id');
                    listStreams(roomId)
                        .then((resp) => {
                            return setRegion(roomId, resp[0].id, 0, resp[1].id);
                        })
                        .then((resp) => {
                            console.log("resp:", resp)
                            expect(resp).to.equal('Internal Server Error');
                        }, (err) => {
                            expect(err).to.equal('Internal Server Error');
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                })
                .catch((err) => {
                    console.log("unmixStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //mixStream
    it('mixStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    unmixStream(roomId, resp[1].id, 'common')
                    return mixStream(roomId, resp[1].id, 'common')

                })
                .then((mixStream) => {
                    console.log('mixStream', mixStream)
                    expect(mixStream).to.have.property('id');
                    listStreams(roomId)
                        .then((resp) => {
                            return setRegion(roomId, resp[0].id, 0, resp[1].id);
                        })
                        .then((resp) => {
                            expect(resp).to.have.property('id');
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                })
                .catch((err) => {
                    console.log("mixStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //mixStream and unmixStream
    it('mixStream and unmixStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    return unmixStream(roomId, resp[1].id, 'common')
                })
                .then((unmixStream) => {
                    console.log('UnmixStream', unmixStream)
                    expect(unmixStream).to.have.property('id');
                    listStreams(roomId)
                        .then((resp) => {
                            return setRegion(roomId, resp[0].id, 0, resp[1].id);
                        })
                        .then((resp) => {
                            console.log("resp:", resp)
                        }, (err) => {
                            expect(err).to.equal('Internal Server Error');
                            listStreams(roomId)
                                .then((resp) => {
                                    return mixStream(roomId, resp[1].id, 'common')
                                })
                                .then((mixStream) => {
                                    console.log('mixStream', mixStream)
                                    expect(mixStream).to.have.property('id');
                                    listStreams(roomId)
                                        .then((resp) => {
                                            return setRegion(roomId, resp[0].id, 0, resp[1].id);
                                        })
                                        .then((resp) => {
                                            expect(resp).to.have.property('id');
                                            deleteRoom(roomId)
                                                .then(() => {
                                                    done();
                                                })
                                        })
                                })
                        })
                })
                .catch((err) => {
                    console.log("unmixStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //setRegion
    it('setRegion', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    mixStream(roomId, resp[1].id, 'common')
                        .then(() => {
                            return setRegion(roomId, resp[0].id, 0, resp[1].id);
                        })
                        .then((setRegion) => {
                            console.log('SetRegion', setRegion)
                            expect(setRegion).to.have.property('id');
                            expect(setRegion.info.layout[0].region.id).to.equal("1");
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                        .catch((err) => {
                            console.log("setRegion catch : ", err)
                            deleteRoom(roomId)
                                .then(() => {
                                    done(err);
                                })
                        })
                })
        })
    })
    //updateLayout
    it('updateLayout', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    publish(localStream)
                        .then(() => {
                            return publish(localStream);
                        })
                        .then((publication) => {
                            return listStreams(roomId)
                        })
                        .then((resp) => {
                            let Layout = [{
                                "stream": "forword-streams",
                                "region": {
                                    "id": "1",
                                    "shape": "rectangle",
                                    "area": {
                                        "left": "0",
                                        "top": "0",
                                        "width": "1/5",
                                        "height": "1/5"
                                    }
                                }
                            },
                            {
                                "stream": "forword-streams",
                                "region": {
                                    "id": "2",
                                    "shape": "rectangle",
                                    "area": {
                                        "left": "1/5",
                                        "top": "1/5",
                                        "width": "1/5",
                                        "height": "1/5"
                                    }
                                }
                            }]
                            Layout[0].stream = resp[1].id;
                            Layout[1].stream = resp[2].id;
                            return mixStream(roomId, resp[1].id, 'common')
                                .then(() => {
                                    return mixStream(roomId, resp[2].id, 'common')
                                })
                                .then(() => {
                                    return updateLayout(roomId, resp[0].id, Layout)
                                });
                        })
                        .then((updateLayout) => {
                            console.log('updateLayout', updateLayout)
                            expect(updateLayout).to.have.property('id');
                            expect(updateLayout.info.layout[0].region.id).to.equal("1");
                            expect(updateLayout.info.layout[0].region.area.left).to.equal("0/1");
                            expect(updateLayout.info.layout[0].region.area.top).to.equal("0/1");
                            expect(updateLayout.info.layout[0].region.area.width).to.equal("1/5");
                            expect(updateLayout.info.layout[0].region.area.height).to.equal("1/5");
                            expect(updateLayout.info.layout[1].region.area.left).to.equal("1/5");
                            expect(updateLayout.info.layout[1].region.area.top).to.equal("1/5");
                            expect(updateLayout.info.layout[1].region.area.width).to.equal("1/5");
                            expect(updateLayout.info.layout[1].region.area.height).to.equal("1/5");
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                        .catch((err) => {
                            console.log("updateLayout catch : ", err)
                            deleteRoom(roomId)
                                .then(() => {
                                    done(err);
                                })
                        })
                })

        })
    })
    //pauseStream
    it('pauseStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    return pauseStream(roomId, resp[1].id, 'av');
                })
                .then((pauseStream) => {
                    console.log('PauseStream', pauseStream)
                    expect(pauseStream).to.have.property('id');
                    expect(pauseStream.media.audio.status).to.equal("inactive");
                    expect(pauseStream.media.video.status).to.equal("inactive");
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("pauseStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //playStream
    it('playStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    return playStream(roomId, resp[1].id, 'av');
                })
                .then((playStream) => {
                    console.log('PlayStream', playStream)
                    expect(playStream).to.have.property('id');
                    expect(playStream.media.audio.status).to.equal("active");
                    expect(playStream.media.video.status).to.equal("active");
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("PlayStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //dropStream
    it('dropStream', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {

                    return dropStream(roomId, resp[1].id);
                })
                .then((dropStream) => {
                    console.log('DropStream', dropStream)
                    expect(dropStream).to.include('');
                    listStreams(roomId)
                        .then((listStreams) => {
                            expect(listStreams[1]).to.equal(undefined);
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                })
                .catch((err) => {
                    console.log("DropStream catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //startStreamingIn
    it('startStreamingInAV', function (done) {
        let roomId;
        createRoomX().then((room) => {
            roomId = room._id;
            return startStreamingIn(roomId)
        })
            .then((startStreamingIn) => {
                console.log('StartStreamingIn', startStreamingIn)
                expect(startStreamingIn.media.audio.status).to.equal('active');
                expect(startStreamingIn.media.video.status).to.equal('active');
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch((err) => {
                console.log("StartStreamingIn catch : ", err)
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })

    it('startStreamingInVideoonly', function (done) {
        let roomId;
        createRoomX().then((room) => {
            roomId = room._id;
            return startStreamingIn(roomId)
        })
            .then((startStreamingIn) => {
                console.log('StartStreamingIn', startStreamingIn)
                expect(startStreamingIn.media.video.status).to.equal('active');
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch((err) => {
                console.log("StartStreamingIn catch : ", err)
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })

    it('startStreamingInAudioonly', function (done) {
        let roomId;
        createRoomX().then((room) => {
            roomId = room._id;
            return startStreamingIn(roomId)
        })
            .then((startStreamingIn) => {
                console.log('StartStreamingIn', startStreamingIn)
                expect(startStreamingIn.media.audio.status).to.equal('active');
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch((err) => {
                console.log("StartStreamingIn catch : ", err)
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })

    //stopStreamingIn
    it('stopStreamingIn', function (done) {
        let roomId;

        createRoomX().then((room) => {
            roomId = room._id;
            return startStreamingIn(roomId)
        })
            .then((StartStreamingIn) => {
                console.log('StartStreamingIn', StartStreamingIn)
                return listStreams(roomId);
            })
            .then((listStreams) => {
                console.log('listStreams', listStreams)
                return stopStreamingIn(roomId, listStreams[1].id);
            })
            .then((stopStreamingIn) => {
                console.log('stopStreamingIn', stopStreamingIn)
                expect(stopStreamingIn).to.equal('')
                deleteRoom(roomId)
                    .then(() => {
                        done();
                    })
            })
            .catch((err) => {
                console.log("StopStreamingIn catch : ", err)
                deleteRoom(roomId)
                    .then(() => {
                        done(err);
                    })
            })
    })
    //startRecording
    forEach(startRecordingPars)
        .it('startRecording', function (startRecordingPars, done) {
            let roomId;
            createRoom(startrecordCreateroomOptions)
                .then((room) => {
                    roomId = room._id;
                    return createToken(room._id, 'testuser', 'presenter')
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
                        .then(() => {
                            return listStreams(roomId)
                        })
                        .then((resp) => {
                            startRecordingPars.media.audio.from = resp[0].id;
                            startRecordingPars.media.video.from = resp[0].id;
                            return startRecording(roomId, startRecordingPars);
                        })
                        .then((startRecording) => {
                            console.log('StartRecording', startRecording)
                            expect(startRecording).to.have.property('id');
                            expect(startRecording.media.audio.format.toString()).to.equal(startRecordingPars.media.audio.format.toString());
                            expect(startRecording.media.video.format.toString()).to.equal(startRecordingPars.media.video.format.toString());
                            expect(startRecording.media.video.parameters.resolution.toString()).to.equal(startRecordingPars.media.video.parameters.resolution.toString());
                            expect(startRecording.media.video.parameters.framerate).to.equal(startRecordingPars.media.video.parameters.framerate);
                            expect(startRecording.media.video.parameters.bitrate).to.equal(startRecordingPars.media.video.parameters.bitrate);
                            expect(startRecording.media.video.parameters.keyFrameInterval).to.equal(startRecordingPars.media.video.parameters.keyFrameInterval);
                            deleteRoom(roomId)
                                .then(() => {
                                    done();
                                })
                        })
                        .catch((err) => {
                            console.log("StartRecording catch : ", err)
                            deleteRoom(roomId)
                                .then(() => {
                                    done(err);
                                })
                        })
                })
        })
    //listRecordings
    it('listRecordings', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then((publication) => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    let options = {
                        media: {
                            audio: {
                                from: 'audioid'
                            },
                            video: {
                                from: 'videoid'
                            }
                        },
                        container: 'auto'
                    };
                    options.media.audio.from = resp[0].id;
                    options.media.video.from = resp[0].id;
                    return startRecording(roomId, options)
                })
                .then(() => {
                    return listRecordings(roomId);
                })
                .then((listRecordings) => {
                    console.log('listRecordings', listRecordings)
                    expect(listRecordings[0]).to.have.property('id');
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("ListRecordings catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done(err);
                        })
                })
        })
    })
    //stopRecording
    it('stopRecording', function (done) {
        joinX().then(() => {
            createLocal()
                .then((localStream) => {
                    localStream = localStream;
                    return publish(localStream);
                })
                .then(() => {
                    return listStreams(roomId)
                })
                .then((resp) => {
                    let options = {
                        media: {
                            audio: {
                                from: 'resp[1].id'
                            },
                            video: {
                                from: 'resp[1].id'
                            }
                        },
                        container: 'auto'
                    };
                    options.media.audio.from = resp[0].id;
                    options.media.video.from = resp[0].id;
                    return startRecording(roomId, options);
                })
                .then((startRecording) => {
                    return stopRecording(roomId, startRecording.id);
                })
                .then((stopRecording) => {
                    console.log('stopRecording', stopRecording)
                    expect(stopRecording).to.have.include('')
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
                .catch((err) => {
                    console.log("StopRecording catch : ", err)
                    deleteRoom(roomId)
                        .then(() => {
                            done();
                        })
                })
        })
    })
    //updateRecording
    forEach(updateRecordingPars)
        .it('updateRecording', function (updateRecordingPars, done) {
            let roomId;
            createRoom(updaterecordCreateroomOptions)
                .then((room) => {
                    roomId = room._id;
                    return createToken(room._id, 'testuser', 'presenter')
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
                            return listStreams(roomId)
                        })
                        .then((resp) => {
                            let id = resp[0].id;
                            let options = {
                                media: {
                                    audio: {
                                        from: 'resp[0].id'
                                    },
                                    video: {
                                        from: 'resp[0].id'
                                    }
                                },
                                container: 'auto'
                            };
                            options.media.audio.from = id;
                            options.media.video.from = id;
                            return startRecording(roomId, options)
                                .then((startRecording) => {
                                    updateRecordingPars.updateRecordPar[0].value = id;
                                    updateRecordingPars.updateRecordPar[1].value = id;
                                    return updateRecording(roomId, startRecording.id, updateRecordingPars.updateRecordPar);
                                })
                                .then((updateRecording) => {
                                    console.log('updateRecording', updateRecording)
                                    expect(updateRecording).to.have.property('id');
                                    expect(updateRecording.media.audio.from).to.equal(updateRecordingPars.updateRecordPar[0].value);
                                    expect(updateRecording.media.video.from).to.equal(updateRecordingPars.updateRecordPar[1].value);
                                    expect(updateRecording.media.video.parameters.bitrate).to.equal(updateRecordingPars.updateRecordPar[2].value);
                                    expect(updateRecording.media.video.parameters.resolution.toString()).to.equal(updateRecordingPars.updateRecordPar[3].value.toString());
                                    expect(updateRecording.media.video.parameters.framerate).to.equal(updateRecordingPars.updateRecordPar[4].value);
                                    expect(updateRecording.media.video.parameters.keyFrameInterval).to.equal(updateRecordingPars.updateRecordPar[5].value);
                                    deleteRoom(roomId)
                                        .then(() => {
                                            done();
                                        })
                                })
                                .catch((err) => {
                                    console.log("updateRecording catch : ", err)
                                    deleteRoom(roomId)
                                        .then(() => {
                                            done(err);
                                        })
                                })
                        })
                })
        })
    //startStreamingOut
    //stopStreamingOut
    //updateStreamingOut
    //listStreamingOuts
    //listAnalytics
    forEach(jsonPost)
        .it('listAnalytics', function (jsonPost, done) {
            joinX().then(() => {
                createLocal()
                    .then((localStream) => {
                        localStream = localStream;
                        return publish(localStream);
                    })
                    .then((publication) => {
                        return listStreams(roomId)
                    })
                    .then((resp) => {
                        jsonPost.media.audio.from = resp[0].id;
                        jsonPost.media.video.from = resp[0].id;
                        return startAnalytics(roomId, jsonPost)
                    })
                    .then(() => {
                        return listAnalytics(roomId);
                    })
                    .then((listAnalytics) => {
                        console.log('listAnalytics', listAnalytics)
                        expect(listAnalytics[0]).to.have.property('id');
                        deleteRoom(roomId)
                            .then(() => {
                                done();
                            })
                    })
                    .catch((err) => {
                        console.log("listAnalytics catch : ", err)
                        deleteRoom(roomId)
                            .then(() => {
                                done(err);
                            })
                    })
            })
        })
    //startAnalytics
    forEach(jsonPost)
        .it('startAnalytics', function (jsonPost, done) {
            joinX().then(() => {
                createLocal()
                    .then((localStream) => {
                        localStream = localStream;
                        return publish(localStream);
                    })
                    .then((publication) => {
                        return listStreams(roomId)
                    })
                    .then((resp) => {
                        jsonPost.media.audio.from = resp[0].id;
                        jsonPost.media.video.from = resp[0].id;
                        startAnalytics(roomId, jsonPost)
                            .then((startAnalytics) => {
                                console.log('startAnalytics', startAnalytics)
                                expect(startAnalytics).to.have.property('id');
                                expect(startAnalytics.media.video.from).to.equal(jsonPost.media.video.from);
                                expect(startAnalytics.media.video.format.codec).to.equal(jsonPost.media.video.format.codec);
                                expect(startAnalytics.media.video.format.profile).to.equal(jsonPost.media.video.format.profile);
                                expect(startAnalytics.media.video.parameters.resolution.width).to.equal(jsonPost.media.video.parameters.resolution.width);
                                expect(startAnalytics.media.video.parameters.resolution.height).to.equal(jsonPost.media.video.parameters.resolution.height);
                                expect(startAnalytics.media.video.parameters.framerate).to.equal(jsonPost.media.video.parameters.framerate);
                                expect(startAnalytics.media.video.parameters.bitrate).to.equal(jsonPost.media.video.parameters.bitrate);
                                expect(startAnalytics.media.video.parameters.keyFrameInterval).to.equal(jsonPost.media.video.parameters.keyFrameInterval);
                                deleteRoom(roomId)
                                    .then(() => {
                                        done();
                                    })
                            })
                            .catch((err) => {
                                console.log("startAnalytics catch : ", err)
                                deleteRoom(roomId)
                                    .then(() => {
                                        done(err);
                                    })
                            })
                    })

            })
        })
    //stopAnalytics
    forEach(jsonPost)
        .it('stopAnalytics', function (jsonPost, done) {
            joinX().then(() => {
                createLocal()
                    .then((localStream) => {
                        localStream = localStream;
                        return publish(localStream);
                    })
                    .then((publication) => {
                        return listStreams(roomId)
                    })
                    .then((resp) => {
                        jsonPost.media.audio.from = resp[0].id;
                        jsonPost.media.video.from = resp[0].id;
                        return startAnalytics(roomId, jsonPost)
                    })
                    .then(() => {
                        return listAnalytics(roomId);
                    })
                    .then((listAnalytics) => {
                        let analyticId = listAnalytics[0].id;
                        return stopAnalytics(roomId, analyticId)
                    })
                    .then((stopAnalytics) => {
                        console.log('stopAnalytics', stopAnalytics)
                        expect(stopAnalytics).to.equal('');
                        deleteRoom(roomId)
                            .then(() => {
                                done();
                            })
                    })
                    .catch((err) => {
                        console.log("stopAnalytics catch : ", err)
                        deleteRoom(roomId)
                            .then(() => {
                                done(err);
                            })
                    })
            })
        })

    //listSipcalls
    forEach(sipcallsPostp)
        .it('listSipcalls', function (sipcallsPostp, done) {
            sipcallsXX()
                .then(() => {
                    sipcallsX()
                        .then((resp) => {
                            sipcallsPostp.mediaOut.audio.from = resp[0].id;
                            sipcallsPostp.mediaOut.video.from = resp[0].id;
                            return startSipcalls(roomId1, sipcallsPostp)
                        })
                        .then(() => {
                            return listSipcalls(roomId1);
                        })
                        .then((listSipcalls) => {
                            console.log('listSipcalls', listSipcalls)
                            expect(listSipcalls[0]).to.have.property('id');
                            deleteRoom(roomId2)
                                .then(() => {
                                    deleteRoom(roomId1)
                                        .then(() => {
                                            done();
                                        })
                                })

                        })
                        .catch((err) => {
                            console.log("listSipcalls catch : ", err)
                            deleteRoom(roomId2)
                                .then(() => {
                                    deleteRoom(roomId1)
                                        .then(() => {
                                            done(err);
                                        })
                                })
                        })
                })

        })
    forEach(sipcallsPostp)
        .it('startSipcalls', function (sipcallsPostp, done) {
            sipcallsXX()
                .then(() => {
                    sipcallsX()
                        .then((resp) => {
                            sipcallsPostp.mediaOut.audio.from = resp[0].id;
                            sipcallsPostp.mediaOut.video.from = resp[0].id;
                            return startSipcalls(roomId1, sipcallsPostp)
                        })
                        .then((startSipcalls) => {
                            console.log('startSipcalls', startSipcalls)
                            expect(startSipcalls).to.have.property('id');
                            deleteRoom(roomId2)
                                .then(() => {
                                    deleteRoom(roomId1)
                                        .then(() => {
                                            done();
                                        })
                                })
                        })
                        .catch((err) => {
                            console.log("startSipcalls catch : ", err)
                            deleteRoom(roomId1)
                                .then(() => {
                                    deleteRoom(roomId2)
                                        .then(() => {
                                            done(err);
                                        })
                                })
                        })

                })

        })
    forEach(sipcallsPostp)
        .it('stopSipcalls', function (sipcallsPostp, done) {
            sipcallsXX()
                .then(() => {
                    sipcallsX()
                        .then((resp) => {
                            console.log('stopSipcalls', stopSipcalls)
                            sipcallsPostp.mediaOut.audio.from = resp[0].id;
                            sipcallsPostp.mediaOut.video.from = resp[0].id;
                            return startSipcalls(roomId1, sipcallsPostp)
                        })
                        .then(() => {
                            return listSipcalls(roomId1);
                        })
                        .then((resp) => {
                            let sipcallsId = resp[0].id;
                            return stopSipcalls(roomId1, sipcallsId)

                        })
                        .then((stopSipcalls) => {
                            console.log('stopSipcalls', stopSipcalls)
                            expect(stopSipcalls).to.equal('');
                            deleteRoom(roomId2)
                                .then(() => {
                                    deleteRoom(roomId1)
                                        .then(() => {
                                            done();
                                        })
                                })
                        })
                        .catch((err) => {
                            console.log("stopSipcalls catch : ", err)
                            deleteRoom(roomId1)
                                .then(() => {
                                    deleteRoom(roomId2)
                                        .then(() => {
                                            done(err);
                                        })
                                })
                        })

                })

        })

    forEach(sipcallsPatchp)
        .it('updateSipcalls', function (sipcallsPatchp, done) {
            sipcallsXX()
                .then(() => {
                    sipcallsX()
                        .then((resp) => {
                            sipcallsPostpp = {
                                "peerURI": "user@url",
                                "mediaIn": {
                                    "audio": true,
                                    "video": true
                                },
                                "mediaOut": {
                                    "audio": {
                                        "from": "5cac045611e93a0382ff83b4-common"
                                    },
                                    "video": {
                                        "from": "5cac045611e93a0382ff83b4-common",
                                    }
                                }
                            }
                            sipcallsPostpp.mediaOut.audio.from = resp[0].id;
                            sipcallsPostpp.mediaOut.video.from = resp[0].id;
                            sipcallsPatchp.patch[0].value = resp[0].id;
                            sipcallsPatchp.patch[1].value = resp[0].id;
                            return startSipcalls(roomId1, sipcallsPostpp)
                        })
                        .then((resp) => {
                            return listSipcalls(roomId1);
                        })
                        .then((listSipcalls) => {
                            sipcallsId = listSipcalls[0].id;
                            return updateSipcalls(roomId1, sipcallsId, sipcallsPatchp.patch);
                        })
                        .then((updateSipcalls) => {
                            console.log('updateSipcalls', updateSipcalls)
                            expect(updateSipcalls).to.have.property('id');
                            expect(updateSipcalls.output.media.audio.from).to.equal(sipcallsPatchp.patch[0].value);
                            expect(updateSipcalls.output.media.video.from).to.equal(sipcallsPatchp.patch[1].value);
                            expect(updateSipcalls.output.media.video.parameters.resolution.width).to.equal(sipcallsPatchp.patch[2].value.width);
                            expect(updateSipcalls.output.media.video.parameters.resolution.height).to.equal(sipcallsPatchp.patch[2].value.height);
                            expect(updateSipcalls.output.media.video.parameters.framerate).to.equal(sipcallsPatchp.patch[3].value);
                            expect(updateSipcalls.output.media.video.parameters.bitrate).to.equal(sipcallsPatchp.patch[4].value);
                            expect(updateSipcalls.output.media.video.parameters.keyFrameInterval).to.equal(sipcallsPatchp.patch[5].value);
                            deleteRoom(roomId1)
                                .then(() => {
                                    deleteRoom(roomId2)
                                        .then(() => {
                                            done();
                                        })
                                })
                        })
                        .catch((err) => {
                            console.log("updateSipcalls catch : ", err)
                            deleteRoom(roomId1)
                                .then(() => {
                                    deleteRoom(roomId2)
                                        .then(() => {
                                            done(err);
                                        })
                                })
                        })

                })

        })






})
