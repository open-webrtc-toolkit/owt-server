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
//createLocal
function createLocal() {
    return new Promise(function (resolve, reject) {
        var audioConstraints = {
            source: 'mic',
            deviceId: undefined,
            volume: undefined,
            sampleRate: undefined,
            channelCount: undefined,
        };
        var videoConstraints = {
            resolution: { width: 640, height: 480 },
            frameRate: undefined,
            deviceId: undefined,
            source: 'camera'
        };
        MediaStreamFactory.createMediaStream({ audio: audioConstraints, video: videoConstraints })
            .then((mediaStream) => {
                console.log('create media stream success: ', mediaStream);
                return new LocalStream(mediaStream, new StreamSourceInfo('mic', 'camera'), { testAttributes: 'test' })
            }, (err) => {
                console.log('create mediaStream failed: ', err);
            })
            .then((localStream) => {
                console.log('create local stream success: ', localStream);
                resolve(localStream);
            }, (err) => {
                console.log('create local stream failed: ', err);
                reject(err);
            })
    })
}
var client;
//join
function join(token) {
    client = new ConferenceClient();
    return client.join(token)

}
//pub
function publish(LocalStream) {

    let audio = [{
        codec: { name: "pcmu" },
    }];
    let video = [{
        'codec': { name: "vp8" },
    }];
    let options = {
        audio: audio,
        video: video,
    };
    return client.publish(LocalStream, options)
}
//pubs
function publishCase() {
    return new Promise((resolve, reject) => {
        createLocal()
            .then((LocalStream) => {
                let audio = [{
                    codec: { name: "pcmu" },
                }];
                let video = [{
                    'codec': { name: "vp8" },
                }];
                let options = {
                    audio: audio,
                    video: video,
                };
                client.publish(LocalStream, options)
                    .then((resp) => {
                        resolve(resp);
                    })
                    .catch((err) => {
                        reject(err);
                    })

            })
    })

}
//sub
function subscribe() {
    return new Promise((resolve, reject) => {
        client = client;
        createLocal()
            .then((localStream) => {

                localStream = localStream;
                return publish(localStream);
            })
            .then((publication) => {
                client.addEventListener("streamadded",
                    eve => {

                        let remoteStream = eve.stream;
                        let stream = remoteStream;
                        let audio = {
                            codecs: [{ name: "opus", channelCount: 2, clockRate: 48000 }]
                        };
                        let video = {
                            codecs: [{ name: 'vp8' }],
                            resolution: undefined,
                            frameRate: undefined,
                            bitrateMultiplier: undefined,
                            keyFrameInterval: undefined,
                        }
                        let options = {
                            audio: audio,
                            video: video
                        }
                        client.subscribe(stream, options)
                            .then((resp) => {
                                resolve(resp);
                            })
                            .catch((err) => {
                                reject(err);
                            })

                    })
            })
    })



}
module.exports = {
    createLocal,
    join,
    publish,
    publishCase,
    subscribe,
}
