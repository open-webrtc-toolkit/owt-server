const fs = require('fs');
var name = ["intel1"];
var participantLimit = [-1];
var inputLimit = [-1];
var rolesrole = ["presenter"];
var rolespublishvideo = [true];
var rolespublishaudio = [true];
var rolessubscribevideo = [true];
var rolessubscribeaudio = [true];
var viewslabel = ["common"];
var viewsaudioformat = [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }];
var viewsaudiovad = [true];
var viewsvideoformat = [{ "codec": "h264", "profile": "CB" }, { "codec": "vp8" }, { "codec": "vp9" }];
var viewsvideoparametersresolution = [{ width: 640, height: 480 }, { width: 1024, height: 768 }, { width: 1280, height: 720 }, { width: 480, height: 320 }, { width: 1920, height: 1080 }];
var viewsvideoparametersframerate = [6, 6, 6, 12, 15, 24, 30, 48, 60];
var viewsvideoparametersbitrate = [500];
var viewsvideoparameterskeyframeinterval = [1, 2, 5, 30, 100];
var viewsvideomaxinput = [16];
var viewsvideobgcolor = [{ r: 250, g: 160, b: 0 }];
var viewsvideomotionfactor = [0.8];
var viewsvideokeepactiveinputprimary = [true];
var viewsvideolayoutfitpolicy = ["crop", "letterbox"];
var viewsvideolayouttemplatesbase = ["void", "fluid", "lecture"];
var viewsvideolayouttemplatescustomregionid = ["123"];
var viewsvideolayouttemplatescustomregionshape = ["rectangle"];
var viewsvideolayouttemplatescustomregionarea = [{ left: 1, top: 1, width: 1, height: 1 }];
var mediainaudio = [[{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }]];
var mediainvideo = [[{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }]];
var mediaoutaudio = [[{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }]];
var mediaoutvideoformat = [[{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }]];
var mediaoutvideoparametersresolution = [["x1/2", "x3/4", "x2/3"]];
var mediaoutvideoparametersframerate = [[6, 15, 24, 30, 48, 60]];
var mediaoutvideoparametersbitrate = [[500, 1000]];
var mediaoutvideoparameterskeyframeinterval = [[1, 2, 5, 30, 100]];
var transcodingaudio = [true];
var transcodingvideoformat = [true];
var transcodingvideoframerate = [true];
var transcodingvideobitrate = [true];
var transcodingvideokeyframeinterval = [true];
var notifyingparticipantActivities = [true];
var notifyingstreamChange = [true];
var sipsipserver = [""];
var sipusername = [""];
var sippassword = [""];
var roomarray = [];
/*
  name：Required : string
  participantLimit： -1 means no limit
  inputLimit：inputLimit：number
  rolesrole：role：string
  rolespublishvideo：video：boolean
  rolespublishaudio：audio ：boolean
  rolessubscribevideo ：video：boolean
  rolessubscribeaudio ：audio：boolean
  viewslabel：views label： string
  viewsaudioformat：views audio codec
  viewsaudiovad ：views audio vad
  viewsvideoformat ：views video codec
  viewsvideoparametersresolution：views resolution
  viewsvideoparametersframerate：views framerate
  viewsvideoparametersbitrate：views   bitrate
  viewsvideoparameterskeyframeinterval：views  keyframeinterval
  viewsvideomaxinput ： maxinput
  viewsvideobgcolor：bgcolor
  viewsvideomotionfactor：motionfactor
  viewsvideokeepactiveinputprimary：keepactiveinputprimary
  viewsvideolayoutfitpolicy：fitpolicy："letterbox" or "crop"
  viewsvideolayouttemplatesbase：base：valid values ["fluid", "lecture", "void"].
  viewsvideolayouttemplatescustomregionid：region id：string
  viewsvideolayouttemplatescustomregionshape：region shape：string
  viewsvideolayouttemplatescustomregionarea：region area ：{left: number , top: number，width: number , height: number }
  mediainaudio：mediain audio codec：array[{
    codec: "pcmu" | "pcma" | "opus" | "g722" | "iSAC" | "iLBC" | "aac" | "ac3" | "nellymoser",
    sampleRate: number,              // Optional
    channelNum: number               // Optional
}]
  mediainvideo：mediain video codec：array[{
    codec: "h264" | "h265" | "vp8" | "vp9",
    profile: "CB" | "B" | "M" | "H"    // For "h264" codec only
}]
  mediaoutaudio：mediaout audio codec：array[{
    codec: "pcmu" | "pcma" | "opus" | "g722" | "iSAC" | "iLBC" | "aac" | "ac3" | "nellymoser",
    sampleRate: number,              // Optional
    channelNum: number               // Optional
}]
  mediaoutvideoformat：mediaout video codec：array[{
    codec: "h264" | "h265" | "vp8" | "vp9",
    profile: "CB" | "B" | "M" | "H"    // For "h264" codec only
}]
  mediaoutvideoparametersresolution：resolution
  mediaoutvideoparametersframerate：framerate
  mediaoutvideoparametersbitrate：bitrate
  mediaoutvideoparameterskeyframeinterval：keyframeinterval
  transcodingaudio： boolean
  transcodingvideoformat： boolean
  transcodingvideoframerate： boolean
  transcodingvideobitrate： boolean
  transcodingvideokeyframeinterval： boolean
  notifyingparticipantActivities：boolean
  notifyingstreamChange：boolean
 */
 name,
  participantLimit,
  inputLimit,
  rolesrole,
  rolespublishvideo,
  rolespublishaudio,
  rolessubscribevideo,
  rolessubscribeaudio,
  viewslabel,
  viewsaudioformat,
  viewsaudiovad,
  viewsvideoformat,
  viewsvideoparametersresolution,
  viewsvideoparametersframerate,
  viewsvideoparametersbitrate,
  viewsvideoparameterskeyframeinterval,
  viewsvideomaxinput,
  viewsvideobgcolor,
  viewsvideomotionfactor,
  viewsvideokeepactiveinputprimary,
  viewsvideolayoutfitpolicy,
  viewsvideolayouttemplatesbase,
  viewsvideolayouttemplatescustomregionid,
  viewsvideolayouttemplatescustomregionshape,
  viewsvideolayouttemplatescustomregionarea,
  mediainaudio,
  mediainvideo,
  mediaoutaudio,
  mediaoutvideoformat,
  mediaoutvideoparametersresolution,
  mediaoutvideoparametersframerate,
  mediaoutvideoparametersbitrate,
  mediaoutvideoparameterskeyframeinterval,
  transcodingaudio,
  transcodingvideoformat,
  transcodingvideoframerate,
  transcodingvideobitrate,
  transcodingvideokeyframeinterval,
  notifyingparticipantActivities,
  notifyingstreamChange,
  sipsipserver,
  sipusername,
  sippassword
) {
  var postmould = '{"name":"66","options":{"notifying":{"streamChange":true,"participantActivities":true},"transcoding":{"video":{"parameters":{"keyFrameInterval":true,"bitrate":true,"framerate":true,"resolution":true},"format":true},"audio":true},"mediaOut":{"video":{"parameters":{"keyFrameInterval":[1,30,5,2,1],"bitrate":["x0.8","x0.6","x0.4","x0.2"],"framerate":[6,12,15,24,30,48,60],"resolution":["x3/4","x2/3","x1/2","x1/3","x1/4","hd1080p","hd720p","svga","vga","cif"]},"format":[{"codec":"h264","profile":"CB"},{"codec":"vp8"},{"codec":"vp9"}]},"audio":[{"codec":"opus","sampleRate":48000,"channelNum":2},{"codec":"isac","sampleRate":16000},{"codec":"isac","sampleRate":32000},{"codec":"g722","sampleRate":16000,"channelNum":1},{"codec":"pcma"},{"codec":"pcmu"},{"codec":"aac","sampleRate":48000,"channelNum":2},{"codec":"ac3"},{"codec":"nellymoser"},{"codec":"ilbc"}]},"mediaIn":{"video":[{"codec":"h264","profile":"CB"},{"codec":"vp8"},{"codec":"vp9"}],"audio":[{"codec":"opus","sampleRate":48000,"channelNum":2},{"codec":"isac","sampleRate":16000},{"codec":"isac","sampleRate":32000},{"codec":"g722","sampleRate":16000,"channelNum":1},{"codec":"pcma"},{"codec":"pcmu"},{"codec":"aac"},{"codec":"ac3"},{"codec":"nellymoser"},{"codec":"ilbc"}]},"views":[{"video":{"layout":{"templates":{"custom":[{"region":[{"id":"rectang","shape":"rectangle","area":{"left":0.5,"top":0.5,"width":0.5,"height":0.5}}]}],"base":"fluid"},"fitPolicy":"letterbox"},"keepActiveInputPrimary":false,"bgColor":{"b":0,"g":0,"r":0},"motionFactor":0.8,"maxInput":16,"parameters":{"keyFrameInterval":100,"framerate":24,"resolution":{"height":480,"width":640},"bitrate":500},"format":{"codec":"vp8"}},"audio":{"vad":true,"format":{"channelNum":2,"sampleRate":48000,"codec":"opus"}},"label":"common"}],"roles":[{"subscribe":{"audio":true,"video":true},"publish":{"audio":true,"video":true},"role":"presenter"}],"participantLimit":-1,"inputLimit":-1,"sip":{"sipServer":"10.239.44.17","username":"user","password":"123"}}}';
  var postmouldjs = JSON.parse(postmould);
  postmouldjs.name = name;
  postmouldjs.options.participantLimit = participantLimit;
  postmouldjs.options.inputLimit = inputLimit;
  postmouldjs.options.roles[0].role = rolesrole;
  postmouldjs.options.roles[0].publish.video = rolespublishvideo;
  postmouldjs.options.roles[0].publish.audio = rolespublishaudio;
  postmouldjs.options.roles[0].subscribe.video = rolessubscribevideo;
  postmouldjs.options.roles[0].subscribe.audio = rolessubscribeaudio;
  postmouldjs.options.views[0].label = viewslabel;
  postmouldjs.options.views[0].audio.format = viewsaudioformat;
  postmouldjs.options.views[0].audio.vad = viewsaudiovad;
  postmouldjs.options.views[0].video.format = viewsvideoformat;
  postmouldjs.options.views[0].video.parameters.resolution = viewsvideoparametersresolution;
  postmouldjs.options.views[0].video.parameters.framerate = viewsvideoparametersframerate;
  postmouldjs.options.views[0].video.parameters.bitrate = viewsvideoparametersbitrate;
  postmouldjs.options.views[0].video.parameters.keyFrameInterval = viewsvideoparameterskeyframeinterval;
  postmouldjs.options.views[0].video.maxInput = viewsvideomaxinput;
  postmouldjs.options.views[0].video.bgColor = viewsvideobgcolor;
  postmouldjs.options.views[0].video.motionFactor = viewsvideomotionfactor;
  postmouldjs.options.views[0].video.keepActiveInputPrimary = viewsvideokeepactiveinputprimary;
  postmouldjs.options.views[0].video.layout.fitPolicy = viewsvideolayoutfitpolicy;
  postmouldjs.options.views[0].video.layout.templates.base = viewsvideolayouttemplatesbase;
  postmouldjs.options.views[0].video.layout.templates.custom[0].region[0].id = viewsvideolayouttemplatescustomregionid;
  postmouldjs.options.views[0].video.layout.templates.custom[0].region[0].shape = viewsvideolayouttemplatescustomregionshape;
  postmouldjs.options.views[0].video.layout.templates.custom[0].region[0].area = viewsvideolayouttemplatescustomregionarea;
  postmouldjs.options.mediaIn.audio = mediainaudio;
  postmouldjs.options.mediaIn.video = mediainvideo;
  postmouldjs.options.mediaOut.audio = mediaoutaudio;
  postmouldjs.options.mediaOut.video.format = mediaoutvideoformat;
  postmouldjs.options.mediaOut.video.parameters.resolution = mediaoutvideoparametersresolution;
  postmouldjs.options.mediaOut.video.parameters.framerate = mediaoutvideoparametersframerate;
  postmouldjs.options.mediaOut.video.parameters.bitrate = mediaoutvideoparametersbitrate;
  postmouldjs.options.mediaOut.video.parameters.keyFrameInterval = mediaoutvideoparameterskeyframeinterval;
  postmouldjs.options.transcoding.audio = transcodingaudio;
  postmouldjs.options.transcoding.video.parameters.resolution = transcodingvideoformat;
  postmouldjs.options.transcoding.video.parameters.framerate = transcodingvideoframerate;
  postmouldjs.options.transcoding.video.parameters.bitrate = transcodingvideobitrate;
  postmouldjs.options.transcoding.video.parameters.keyFrameInterval = transcodingvideokeyframeinterval;
  postmouldjs.options.notifying.participantActivities = notifyingparticipantActivities;
  postmouldjs.options.notifying.streamChange = notifyingstreamChange;
  postmouldjs.options.sip.sipServer = sipsipserver;
  postmouldjs.options.sip.username = sipusername;
  postmouldjs.options.sip.password = sippassword;
  roomarray.push(postmouldjs);
};

for (var i = 0; i < viewsvideoparametersframerate.length; i++) {

  appendJson(roomarray,
    name[i % name.length],
    participantLimit[i % participantLimit.length],
    inputLimit[i % inputLimit.length],
    rolesrole[i % rolesrole.length],
    rolespublishvideo[i % rolespublishvideo.length],
    rolespublishaudio[i % rolespublishaudio.length],
    rolessubscribevideo[i % rolessubscribevideo.length],
    rolessubscribeaudio[i % rolessubscribeaudio.length],
    viewslabel[i % viewslabel.length],
    viewsaudioformat[i % viewsaudioformat.length],
    viewsaudiovad[i % viewsaudiovad.length],
    viewsvideoformat[i % viewsvideoformat.length],
    viewsvideoparametersresolution[i % viewsvideoparametersresolution.length],
    viewsvideoparametersframerate[i],
    viewsvideoparametersbitrate[i % viewsvideoparametersbitrate.length],
    viewsvideoparameterskeyframeinterval[i % viewsvideoparameterskeyframeinterval.length],
    viewsvideomaxinput[i % viewsvideomaxinput.length],

    viewsvideobgcolor[i % viewsvideobgcolor.length],
    viewsvideomotionfactor[i % viewsvideomotionfactor.length],
    viewsvideokeepactiveinputprimary[i % viewsvideokeepactiveinputprimary.length],
    viewsvideolayoutfitpolicy[i % viewsvideolayoutfitpolicy.length],
    viewsvideolayouttemplatesbase[i % viewsvideolayouttemplatesbase.length],
    viewsvideolayouttemplatescustomregionid[i % viewsvideolayouttemplatescustomregionid.length],
    viewsvideolayouttemplatescustomregionshape[i % viewsvideolayouttemplatescustomregionshape.length],
    viewsvideolayouttemplatescustomregionarea[i % viewsvideolayouttemplatescustomregionarea.length],
    mediainaudio[i % mediainaudio.length],
    mediainvideo[i % mediainvideo.length],
    mediaoutaudio[i % mediaoutaudio.length],
    mediaoutvideoformat[i % mediaoutvideoformat.length],
    mediaoutvideoparametersresolution[i % mediaoutvideoparametersresolution.length],
    mediaoutvideoparametersframerate[i % mediaoutvideoparametersframerate.length],
    mediaoutvideoparametersbitrate[i % mediaoutvideoparametersbitrate.length],
    mediaoutvideoparameterskeyframeinterval[i % mediaoutvideoparameterskeyframeinterval.length],
    transcodingaudio[i % transcodingaudio.length],
    transcodingvideoformat[i % transcodingvideoformat.length],
    transcodingvideoframerate[i % transcodingvideoframerate.length],
    transcodingvideobitrate[i % transcodingvideobitrate.length],
    transcodingvideokeyframeinterval[i % transcodingvideokeyframeinterval.length],
    notifyingparticipantActivities[i % notifyingparticipantActivities.length],
    notifyingstreamChange[i % notifyingstreamChange.length],
    sipsipserver[i % sipsipserver.length],
    sipusername[i % sipusername.length],
    sippassword[i % sippassword.length]
  );



}


roomarrayjson = JSON.stringify(roomarray);
var filename = "./casesRoomTrue.json";
fs.writeFile(filename, roomarrayjson, (err) => { if (err) throw err; console.log('case ok'); });


