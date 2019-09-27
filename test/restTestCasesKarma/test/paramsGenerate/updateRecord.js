const fs = require("fs");
var op0 = ["replace"];
var path0 = ["/media/audio/from"];
var value0 = ["{{roomId}}-common"];
var op1 = ["replace"];
var path1 = ["/media/video/from"];
var value1 = ["{{roomId}}-common"];
var op2 = ["replace"];
var path2 = ["/media/video/parameters/bitrate"];
var value2 = ["x0.2", "x0.4", "x0.6", "x0.8"];
var op3 = ["replace"];
var path3 = ["/media/video/parameters/resolution"];
var value3 = [{ width: 640, height: 480 }, { width: 480, height: 360 }, { width: 426, height: 320 }, { width: 352, height: 288 }, { width: 320, height: 240 }, { width: 212, height: 160 }, { width: 160, height: 120 }];
var op4 = ["replace"];
var path4 = ["/media/video/parameters/framerate"];
var value4 = [6, 6, 6, 6, 6, 6, 6, 12, 15, 24, 30, 48, 60];
var op5 = ["replace"];
var path5 = ["/media/video/parameters/keyFrameInterval"];
var value5 = [1, 2, 5, 30, 100];
var recordarray = [];
/*
op0: "replace",
path0: "/media/audio/from"
value0: string
op1: "replace",
path1: "/media/video/from"
value1: string
op2: "replace",
path2: "/media/video/parameters/bitrate",
value2: "x0.8" | "x0.6" | "x0.4" | "x0.2"
op3: "replace",
path3: "/media/video/parameters/resolution",
value3: object(Resolution)
op4: "replace",
path4: "/media/video/parameters/framerate",
value4: "6" | "12" | "15" | "24" | "30" | "48" | "60"
op5: "replace",
path5: "/media/video/parameters/keyFrameInterval",
value5: "1" | "2" | "5" | "30" | "100"
*/
var recordtest = function (recordarray,
	op0,
	path0,
	value0,
	op1,
	path1,
	value1,
	op2,
	path2,
	value2,
	op3,
	path3,
	value3,
	op4,
	path4,
	value4,
	op5,
	path5,
	value5
) {
	var recordjs = [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.6" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 160, "height": 120 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 2 }];
	recordjs[0].op = op0;
	recordjs[0].path = path0;
	recordjs[0].value = value0;
	recordjs[1].op = op1;
	recordjs[1].path = path1;
	recordjs[1].value = value1;
	recordjs[2].op = op2;
	recordjs[2].path = path2;
	recordjs[2].value = value2;
	recordjs[3].op = op3;
	recordjs[3].path = path3;
	recordjs[3].value = value3;
	recordjs[4].op = op4;
	recordjs[4].path = path4;
	recordjs[4].value = value4;
	recordjs[5].op = op5;
	recordjs[5].path = path5;
	recordjs[5].value = value5;
	recordarray.push(recordjs);
};
for (var i = 0; i < value4.length; i++) {
	recordtest(recordarray,
		op0[i % op0.length],
		path0[i % op0.length],
		value0[i % op0.length],
		op1[i % op0.length],
		path1[i % op0.length],
		value1[i % op0.length],
		op2[i % op0.length],
		path2[i % op0.length],
		value2[i % op0.length],
		op3[i % op0.length],
		path3[i % op0.length],
		value3[i % op0.length],
		op4[i % op0.length],
		path4[i % op0.length],
		value4[i],
		op5[i % op0.length],
		path5[i % op0.length],
		value5[i % op0.length]
	);
}
var recordarrayjson = JSON.stringify(recordarray);
var filename = "./casesPatchRecord.json";
fs.writeFile(filename, recordarrayjson, (err) => { if (err) throw err; console.log('case ok'); });
