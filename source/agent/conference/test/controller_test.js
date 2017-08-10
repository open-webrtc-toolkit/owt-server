var assert = require("assert");
var mockery = require("mockery");
var expect = require('chai').use(require('chai-as-promised')).expect;
var sinon = require('sinon');

describe("MultipleViewController", () => {
    let Controller;
    let controller, commonViewController;
    let rpcMock = {
        makeRPC: sinon.stub()
    };
    let clusterName = "woogeen-cluster";
    let rpcClient = "fakeRpcClient";
    let roomName = "TestRoom";
    let roomRpcId = "fakeRpcId";
    let mixedStreams;
    let videoNodes = [];
    let audioNodes = [];

    function registerRpc(args, ret, error) {
        let validArgs = [
            sinon.match.any,
            sinon.match.any,
            sinon.match.any,
            sinon.match.any,
            sinon.match.func,
            sinon.match.func
        ];
        for (let i = args.length; i < validArgs.length; i++)
            args.push(validArgs[i]);
        if (ret)
            rpcMock.makeRPC.withArgs(...args).callsArgWith(4, ret);
        else if (error)
            rpcMock.makeRPC.withArgs(...args).callsArgWith(5, error);
    }

    before(() => {
        mockery.enable();

        // Fake makeRPC require
        mockery.registerMock("./makeRPC", rpcMock);

        // Fake logger require
        let print = () => {};
        let loggerMock = {
            logger: {
                getLogger: () => {
                    return {
                        debug: print,
                        info: print,
                        warn: print,
                        error: print
                    }
                }
            }
        };
        mockery.registerMock("./logger", loggerMock);

        // Fake rpc request
        rpcReq = {};
        let agentCount = 0;
        rpcReq.getMediaNode = (cm, purpose, whom) => {
            agentCount++;
            let mediaNode = {
                agent: purpose + agentCount,
                node: Math.random().toString().substr(2)
            };

            if (purpose === "video") {
                videoNodes.push(mediaNode);
            } else {
                audioNodes.push(mediaNode);
            }

            // Register rpc to media node
            let result = (purpose === "video")? {
                codecs: ['fake_codec1', 'fake_codec2'],
                resolutions: ['fake_resolution1', 'fake_resolution2']
            } : {
                codecs: ["fake_audio_codec1", "fake_audio_codec2"]
            };
            registerRpc([rpcClient, mediaNode.node, "init"], result);
            registerRpc([rpcClient, mediaNode.node, "generate"], purpose + "-stream-" + agentCount);
            registerRpc([rpcClient, mediaNode.node], "ok");

            return Promise.resolve(mediaNode);
        };
        rpcReq.recycleMediaNode = () => Promise.resolve('ok');

        mockery.registerAllowable("assert");
        mockery.registerAllowable("../controller");
        Controller = require("../controller");
    });

    after(() => {
        mockery.disable();
    });

    describe("Intialize/Destroy", () => {
        // We'll use the 2-view controller in the later tests.
        let configLabels = [
            "2-view-room",
            "old-style-room",
            "no-mixing-room"
        ];
        let roomConfigs = [
            { // 2-view
                publishLimit: -1,
                userLimit: -1,
                enableMixing: true,
                views: {
                    "first-view": {
                        mediaMixing: {
                            video: {
                                avCoordinated: 0,
                                maxInput: 16,
                                resolution: 'vga',
                                multistreaming: 0,
                                quality_level: 'standard',
                                bkColor: 'black',
                                crop: 0,
                                layout: {
                                    base: 'fluid',
                                    custom: []
                                }
                            },
                            audio: null
                        }
                    },
                    "second-view": {
                        mediaMixing: {
                            video: {
                                avCoordinated: 0,
                                maxInput: 16,
                                resolution: 'vga',
                                multistreaming: 1,
                                quality_level: 'standard',
                                bkColor: 'black',
                                crop: 0,
                                layout: {
                                    base: 'lecture',
                                    custom: []
                                }
                            },
                            audio: null
                        }
                    }
                }
            },
            { // old-style
                mode: 'hybrid',
                publishLimit: -1,
                userLimit: -1,
                enableMixing: true,
                views: {
                    "common": {
                        mediaMixing: {
                            video: {
                                avCoordinated: 0,
                                maxInput: 16,
                                resolution: 'vga',
                                multistreaming: 0,
                                quality_level: 'standard',
                                bkColor: 'black',
                                crop: 0,
                                layout: {
                                    base: 'fluid',
                                    custom: []
                                }
                            },
                            audio: null
                        }
                    }
                }
            },
            {
                // no-mixing
                publishLimit: -1,
                userLimit: -1,
                enableMixing: false,
            }
        ];
        let toDestroys = [];
        let okCallbacks = [
            (ctlResult) => { // 2-view
                controller = ctlResult;
                expect(ctlResult.getMixedStreams().length).to.equal(2);
                mixedStreams = ctlResult.getMixedStreams();
            },
            (ctlResult) => { // old-style
                commonViewController = ctlResult;
                expect(ctlResult.getMixedStreams().length).to.equal(1);
            },
            (ctlResult) => { // no-mixing
                expect(ctlResult.getMixedStreams().length).to.equal(0);
                toDestroys.push(ctlResult);
            },
        ];

        for (let i = 0; i < roomConfigs.length; i++) {
            it("Initialize-With-Config:" + configLabels[i], (done) => {
                let roomConfig = roomConfigs[i];
                roomConfig.internalConnProtocol = "sctp";
                roomConfig.enableAudioTranscoding = true;
                roomConfig.enableVideoTranscoding = true;

                Controller.create({
                    cluster: clusterName,
                    rpcReq: rpcReq,
                    rpcClient: rpcClient,
                    room: roomName,
                    config: roomConfig,
                    selfRpcId: roomRpcId
                }, function (ctlResult, mixStreams) {
                    okCallbacks[i](ctlResult, mixStreams);
                    done();
                }, function (reason) {
                    throw new Error(reason);
                    done();
                });
            });
        }

        it("Destroy-Controllers", () => {
            toDestroys.forEach(function(ctl) {
                ctl.destroy();
            });
        });
    });

    describe("Publish/Unpublish-Streams", () => {
        let publishDesc = ["PublishNotMix", "PublishWithMix"];
        let user = ["Alice", "Bob"];
        let stream = ["stream1", "stream2"];
        let streamInfo = {
            type: "webrtc",
            audio: {
                codec: "fake_audio_codec1"
            },
            video: {
                codec: "fake_codec2",
                resolution: "fake_resolution2"
            }
        };
        let mixNum = [0, 1];

        for (let i = 0; i < user.length; i++) {
            it(publishDesc[i], (done) => {
                let accessNode = {agent:"fakeAgent" + i, node: "fakeAccessNode" + i};
                registerRpc([rpcClient, accessNode.node], "ok");

                let mixStreamIds = mixedStreams.map((mstream) => mstream.streamId);
                let mixList = mixStreamIds.slice(0, mixNum[i]);
                controller.publish(user[i], stream[i], accessNode, streamInfo, mixList,
                    function onOk() {
                        done();
                    },
                    function onError(reason) {
                        throw new Error(reason);
                    }
                );
            });
        }

        it("UnpublishStreams", () => {
            for (let i = 0; i < user.length; i++) {
                controller.unpublish(user[i], stream[i]);
            }
        });
    });

    function userPublish() {
        let pubuser = ["Tom", "Jerry"];
        let stream = ["ustream1", "ustream2"];
        let streamInfo = [
            { // Video & Audio Stream
                type: "webrtc",
                audio: {
                    codec: "fake_audio_codec1"
                },
                video: {
                    codec: "fake_codec2",
                    resolution: "fake_resolution2"
                }
            },
            { // Audio-Only Stream
                type: "webrtc",
                audio: {
                    codec: "fake_audio_codec2"
                }
            }
        ];
        let accessNode = [
            {agent:"fakeTomAgent", node: "fakeTomNode"},
            {agent:"fakeJerryAgent", node: "fakeJerryNode"}
        ];

        return {
            before: (ctl, done) => {
                pubpromise = [];
                for (let i = 0; i < pubuser.length; i++) {
                    registerRpc([rpcClient, accessNode[i].node], "ok");
                    pubpromise.push(new Promise((resolve, reject) => {
                        ctl.publish(pubuser[i], stream[i], accessNode[i], streamInfo[i], false, resolve, reject);
                    }));
                }
                Promise.all(pubpromise).then((ret) => {
                    done();
                }).catch((err) => {
                    console.log("pub err", err);
                    throw new Error(reason);
                });
            },
            after: (ctl) => {
                for (let i = 0; i < pubuser.length; i++) {
                    ctl.unpublish(pubuser[i], stream[i]);
                }
            }
        };
    }

    let published = userPublish();
    describe("Subscribe/Unsubscribe-Streams", () => {
        before((done) => {
            published.before(controller, done);
        });
        after(() => {
            published.after(controller);
        });

        let testDesc = [
            "Subscribe-Audio-Only",
            "Subscribe-Video-Only",
            "Subscribe-Audio&Video",
            "Subscribe-Needs-Transcode",
            "Subscribe-Without-Specified-Codec&Resolution",
            "Subscribe-Non-Exist-Stream-Should-Fail",
            "Subscribe-Codec-Mismatch-Should-Fail",
            "Subscribe-Resolution-Mismatch-Should-Fail"
        ];
        let subInfo = [
            { // Audio only
                audio: {
                    codecs: ["fake_audio_codec1"],
                    fromStream: "TestRoom-first-view"
                }
            },
            { // Video only
                video: {
                    codecs: ["fake_codec1"],
                    resolution: "fake_resolution1",
                    fromStream: "TestRoom-second-view"
                }
            },
            { // Audio & Video
                audio: {
                    codecs: ["fake_audio_codec2"],
                    fromStream: "ustream2"
                },
                video: {
                    codecs: ["fake_codec2"],
                    resolution: "fake_resolution2",
                    fromStream: "TestRoom-first-view"
                }
            },
            { // Forward needs transcode
                audio: {
                    codecs: ["fake_audio_codec2"],
                    fromStream: "ustream1"
                }
            },
            { // No resolution and codec
                video: {fromStream: "ustream1"}
            },
            { // Stream not exist
                video: {fromStream: "ustream2"}
            },
            { // Codec mismatch
                audio: {
                    codecs: ["other_codec"],
                    fromStream: "TestRoom-first-view"
                }
            },
            { // Resolution mismatch
                video: {
                    resolution: "other_resolution",
                    fromStream: "TestRoom-second-view"
                }
            }
        ];
        let subExpect = [
            true,
            true,
            true,
            true,
            true,
            false,
            false,
            false
        ];
        let subscribed = [];

        for (let i = 0; i < testDesc.length; i++) {
            it(testDesc[i], (done) => {
                let ptcpt = "subuser" + i;
                let subId = ptcpt + "sub";
                let accessNode = {agent:"fakeSub" + i, node: "fakeSubNode" + i};
                registerRpc([rpcClient, accessNode.node], "ok");

                // Subscribe will be called in sequence here.
                controller.subscribe(ptcpt, subId, accessNode, subInfo[i],
                    function subOk() {
                        subscribed.push({p: ptcpt, s: subId});
                        if (subExpect[i]) {
                            done();
                        } else {
                            throw new Error("Subscribe should not succeed.");
                        }
                    },
                    function subError(reason) {
                        if (subExpect[i]) {
                            throw new Error(reason);
                        } else {
                            done();
                        }
                    }
                );
            });
        }

        it("Unsubscribe-Streams", () => {
            for (let i = 0; i < subscribed.length; i++) {
                controller.unsubscribe(subscribed[i].p, subscribed[i].s);
            }
        });
    });


    describe("Mix/Unmix/GetRegion/SetRegion/UpdateStream/SetMute", () => {
        before((done) => {
            published.before(controller, () => {
                published.before(commonViewController, done);
            });
        });
        after(() => {
            published.after(controller);
            published.after(commonViewController);
        });

        it("Mix", (done) => {
            // Mix on the second mix stream
            controller.mix("ustream1", ["TestRoom-second-view"],
                function onOk() {
                    done();
                },
                function onError(reason) {
                    throw new Error(reason);
                }
            );
        });

        it("Mix-Without-MixStreamID-On-CommonView", (done) => {
            // Mix on the second mix stream
            commonViewController.mix("ustream1", undefined,
                function onOk() {
                    commonViewController.mix("ustream2", undefined,
                        () => { done();},
                        (err) => { throw new Error(err); }
                    );
                },
                function onError(reason) {
                    throw new Error(reason);
                }
            );
        });

        it("Mix-Should-Fail-If-No-CommonView&MixStreamID", (done) => {
            // Mix on the second mix stream
            controller.mix("ustream1", undefined,
                function onOk() {
                    throw new Error("Mix not fail");
                },
                function onError(reason) {
                    done();
                }
            );
        });

        it("Unmix", (done) => {
            // Unmix on the first mix stream
            controller.unmix("ustream2", ["TestRoom-first-view"],
                function onOk() {
                    done();
                },
                function onError(reason) {
                    throw new Error(reason);
                }
            );
        });

        it("Unmix-Without-MixStreamID-On-CommonView", (done) => {
            // Unmix on the first mix stream
            commonViewController.unmix("ustream1", undefined,
                function onOk() {
                    done();
                },
                function onError(reason) {
                    throw new Error(reason);
                }
            );
        });

        it("Unmix-Should-Fail-If-No-CommonView&MixStreamID", (done) => {
            // Mix on the second mix stream
            controller.unmix("ustream1", undefined,
                function onOk() {
                    throw new Error("Unmix not fail");
                },
                function onError(reason) {
                    done();
                }
            );
        });

        it("GetRegion", (done) => {
            controller.getRegion("ustream1", "TestRoom-first-view",
                function onOk() {
                    done();
                },
                function onError(err) {
                    throw new Error(err);
                }
            );
        });

        it("GetRegion-Without-MixStreamID-On-CommonView", (done) => {
            commonViewController.getRegion("ustream2", undefined,
                function onOk() {
                    done();
                },
                function onError(err) {
                    throw new Error(err);
                }
            );
        });

        it("GetRegion-Without-MixStreamID&CommonView", (done) => {
            controller.getRegion("ustream1", null,
                function onOk() {
                    throw new Error("Get region not fail");
                },
                function onError(reason) {
                    done();
                }
            );
        });

        it("SetRegion", (done) => {
            controller.setRegion("ustream1", "fakeRegion", "TestRoom-first-view",
                function ok() {
                    done();
                },
                function onError(err) {
                    throw new Error(err);
                }
            );
        });

        it("SetRegion-Without-MixStreamID-On-CommonView", (done) => {
            commonViewController.setRegion("ustream2", "fakeRegion", undefined,
                function onOk() {
                    done();
                },
                function onError(err) {
                    throw new Error(err);
                }
            );
        });

        it("SetRegion-Without-MixStreamID&CommonView", (done) => {
            controller.setRegion("ustream1", "fakeRegion",  null,
                function onOk() {
                    throw new Error("Get region not fail");
                },
                function onError(reason) {
                    done();
                }
            );
        });

        it("UpdateStream", () => {
            controller.updateStream("ustream1", "video", "active");
        });

        it("setMute", () => {
            controller.setMute("ustream1", "video", true,
                function onOk() {
                    done();
                },
                function onError(err) {
                    throw new Error(err);
                }
            );
        });
    });
});