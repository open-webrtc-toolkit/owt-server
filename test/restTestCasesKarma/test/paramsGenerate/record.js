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
recordarray是一个数组，主要储存不同参数的request body的集合（recordjs）
container={"mp4" | "mkv" | "auto" | "ts"}  The container type of the recording file, "auto" by default.
mediaaudiofrom：target audio StreamID
mediaaudioformat：audio codec
mediavideofrom  ：target video StreamID
mediavideoformat ：video codec
mediavideoparametersresolution：resolution { width: number, height: number }
mediavideoparametersframerate  ：framerate：number
mediavideoparametersbitrate  ：bitrate：number
mediavideoparameterskeyFrameInterval ：keyFrameInterval ：number
recordjs是发送post http://localhost:3000/v1/rooms/{{roomId}}/recordings 请求时的request body
recordtest函数，主要作用是给recordjs中的每个参数赋值，并且把赋值后的recordjs对象添加到recordarray数组中
recordarray数组里的元素是赋值后的recordjs，每次赋值recordjs，数组末尾追加一个recordjs元素
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
fs.writeFile(filename, recordarrayjson, (err) => { if (err) throw err; console.log('case集合生成完毕'); });









