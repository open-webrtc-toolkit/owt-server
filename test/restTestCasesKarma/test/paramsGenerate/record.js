const fs = require("fs");
var container = ["mkv", "auto"];
var mediaaudiofrom = ["{{streamsinsid}}"];
var mediaaudioformat = [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }];
var mediavideofrom = ["{{streamsinsid}}"];
var mediavideoformat = [{ "codec": "h264", "profile": "CB" }, { "codec": "vp8" }, { "codec": "vp9" }];
var mediavideoparametersresolution = [{ width: 640, height: 480 }, { width: 480, height: 360 }, { width: 426, height: 320 }, { width: 352, height: 288 }, { width: 320, height: 240 }, { width: 212, height: 160 }, { width: 160, height: 120 }];
var mediavideoparametersframerate = [6, 6, 6, 6, 6, 6, 6, 12, 15, 24, 30, 48, 60];
var mediavideoparametersbitrate = ["x0.2", "x0.6", "x0.4", "x0.8"];
var mediavideoparameterskeyFrameInterval = [1, 2, 5, 30, 100];
var recordarray = [];
/*
container={"mp4" | "mkv" | "auto" | "ts"}  The container type of the recording file, "auto" by default.
mediaaudiofrom：target audio StreamID
mediaaudioformat：audio codec
mediavideofrom  ：target video StreamID
mediavideoformat ：video codec
mediavideoparametersresolution：resolution { width: number, height: number }
mediavideoparametersframerate  ：framerate：number
mediavideoparametersbitrate  ：bitrate：number
mediavideoparameterskeyFrameInterval ：keyFrameInterval ：number
*/
var recordtest = function (recordarray,
	container,
	mediaaudiofrom,
	mediaaudioformat,
	mediavideofrom,
	mediavideoformat,
	mediavideoparametersresolution,
	mediavideoparametersframerate,
	mediavideoparametersbitrate,
	mediavideoparameterskeyFrameInterval) {
	var recordjson = '{"container":"mp4","media":{"audio":{"from":"142020205294123520","format":{"codec":"opus","sampleRate":48000,"channelNum":2}},"video":{"from":"142020205294123520","format":{"codec":"h264","profile":"CB"},"parameters":{"resolution":{"width":640,"height":480},"framerate":15,"bitrate":"x0.2","keyFrameInterval":2}}}}';
	var recordjs = JSON.parse(recordjson);
	recordjs.container = container;
	recordjs.media.audio.from = mediaaudiofrom;
	recordjs.media.audio.format = mediaaudioformat;
	recordjs.media.video.from = mediavideofrom;
	recordjs.media.video.format = mediavideoformat;
	recordjs.media.video.parameters.resolution = mediavideoparametersresolution;
	recordjs.media.video.parameters.framerate = mediavideoparametersframerate;
	recordjs.media.video.parameters.bitrate = mediavideoparametersbitrate;
	recordjs.media.video.parameters.keyFrameInterval = mediavideoparameterskeyFrameInterval;
	recordarray.push(recordjs);
};

for (var i = 0; i < mediavideoparametersframerate.length; i++) {
	recordtest(recordarray,
		container[i % container.length],
		mediaaudiofrom[i % container.length],
		mediaaudioformat[i % container.length],
		mediavideofrom[i % container.length],
		mediavideoformat[i % container.length],
		mediavideoparametersresolution[i % container.length],
		mediavideoparametersframerate[i],
		mediavideoparametersbitrate[i % container.length],
		mediavideoparameterskeyFrameInterval[i % container.length]);
}

var recordarrayjson = JSON.stringify(recordarray);
var filename = "./casesPostRecord.json";
fs.writeFile(filename, recordarrayjson, (err) => { if (err) throw err; console.log('case ok'); });









