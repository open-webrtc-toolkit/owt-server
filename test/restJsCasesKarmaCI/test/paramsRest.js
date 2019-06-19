let createRoomPars = [{ "name": "WEBRTC", "options": { "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 1, "framerate": 6, "resolution": { "width": 640, "height": 480 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": true, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 2, "framerate": 6, "resolution": { "width": 1024, "height": 768 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": false, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 5, "framerate": 6, "resolution": { "width": 1280, "height": 720 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": true, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 30, "framerate": 12, "resolution": { "width": 480, "height": 320 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": false, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 100, "framerate": 15, "resolution": { "width": 1920, "height": 1080 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": true, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 1, "framerate": 24, "resolution": { "width": 640, "height": 480 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": false, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 2, "framerate": 30, "resolution": { "width": 1024, "height": 768 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": true, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 5, "framerate": 48, "resolution": { "width": 1280, "height": 720 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": false, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }, { "name": "WEBRTC", "options": { "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 30, "framerate": 60, "resolution": { "width": 480, "height": 320 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": true, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } } }]
let updateRoomPars = [{ "name": "WEBRTC", "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 1, "framerate": 6, "resolution": { "width": 640, "height": 480 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": true, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 2, "framerate": 6, "resolution": { "width": 1024, "height": 768 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": false, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 5, "framerate": 6, "resolution": { "width": 1280, "height": 720 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": true, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 30, "framerate": 12, "resolution": { "width": 480, "height": 320 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": false, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 100, "framerate": 15, "resolution": { "width": 1920, "height": 1080 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": true, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 1, "framerate": 24, "resolution": { "width": 640, "height": 480 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": false, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "void" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 2, "framerate": 30, "resolution": { "width": 1024, "height": 768 }, "bitrate": 500 }, "format": { "codec": "h264", "profile": "CB" } }, "audio": { "vad": true, "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": false, "participantActivities": false }, "transcoding": { "video": { "parameters": { "keyFrameInterval": false, "bitrate": false, "framerate": false, "resolution": false }, "format": true }, "audio": false }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "fluid" }, "fitPolicy": "letterbox" }, "keepActiveInputPrimary": false, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 5, "framerate": 48, "resolution": { "width": 1280, "height": 720 }, "bitrate": 500 }, "format": { "codec": "vp8" } }, "audio": { "vad": false, "format": { "codec": "g722", "sampleRate": 16000, "channelNum": 1 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": false, "video": false }, "publish": { "audio": false, "video": false }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }, { "name": "WEBRTC", "notifying": { "streamChange": true, "participantActivities": true }, "transcoding": { "video": { "parameters": { "keyFrameInterval": true, "bitrate": true, "framerate": true, "resolution": true }, "format": true }, "audio": true }, "mediaOut": { "video": { "parameters": { "keyFrameInterval": [1, 2, 5, 30, 100], "bitrate": [500, 1000], "framerate": [5, 15, 24, 30, 48, 60], "resolution": ["x1/2", "x3/4", "x2/3"] }, "format": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }] }, "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "mediaIn": { "video": [{ "codec": "h264", "profile": "CB" }, { "codec": "h264", "profile": "B" }, { "codec": "h264", "profile": "M" }, { "codec": "h264", "profile": "H" }, { "codec": "h265" }, { "codec": "vp8" }, { "codec": "vp9" }], "audio": [{ "codec": "opus", "sampleRate": 48000, "channelNum": 2 }, { "codec": "pcmu" }, { "codec": "pcma" }, { "codec": "isac", "sampleRate": "16000" }, { "codec": "isac", "sampleRate": "32000" }, { "codec": "g722", "sampleRate": 16000, "channelNum": 1 }, { "codec": "acc", "sampleRate": 48000, "channelNum": 2 }, { "codec": "ac3" }, { "codec": "ilbc" }] }, "views": [{ "video": { "layout": { "templates": { "custom": [{ "region": [{ "id": "123", "shape": "rectangle", "area": { "left": '1/2', "top": '1/2', "width": '1/2', "height": '1/2' } }] }], "base": "lecture" }, "fitPolicy": "crop" }, "keepActiveInputPrimary": true, "bgColor": { "r": 250, "g": 160, "b": 0 }, "motionFactor": 0.8, "maxInput": 16, "parameters": { "keyFrameInterval": 30, "framerate": 60, "resolution": { "width": 480, "height": 320 }, "bitrate": 500 }, "format": { "codec": "vp9" } }, "audio": { "vad": true, "format": { "codec": "acc", "sampleRate": 48000, "channelNum": 2 } }, "label": "common" }], "roles": [{ "subscribe": { "audio": true, "video": true }, "publish": { "audio": true, "video": true }, "role": "presenter" }], "participantLimit": -1, "inputLimit": -1, "sip": { "sipServer": "", "username": "", "password": "" } }];
let forbidSubPars = [[{
    "op": "replace",
    "path": "/permission/subscribe/audio",
    "value": false
}], [{
    "op": "replace",
    "path": "/permission/subscribe/audio",
    "value": true
}], [{
    "op": "replace",
    "path": "/permission/subscribe/video",
    "value": false
}], [{
    "op": "replace",
    "path": "/permission/subscribe/video",
    "value": true
}]
];
let forbidPubPars = [[{
    "op": "replace",
    "path": "/permission/publish/audio",
    "value": false
}], [{
    "op": "replace",
    "path": "/permission/publish/audio",
    "value": true
}], [{
    "op": "replace",
    "path": "/permission/publish/video",
    "value": false
}], [{
    "op": "replace",
    "path": "/permission/publish/video",
    "value": true
}]
];
let startRecordingPars = [{ "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 6, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 6, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 6, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 6, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 6, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 6, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 6, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 12, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 15, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 24, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 30, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }, { "container": "auto", "media": { "audio": { "format": { "codec": "pcmu" } }, "video": { "format": { "codec": "vp8" }, "parameters": { "resolution": { "width": 480, "height": 360 }, "framerate": 48, "bitrate": "x0.6", "keyFrameInterval": 2 } } } }, { "container": "mkv", "media": { "audio": { "from": "{{streamsinsid}}", "format": { "codec": "opus", "sampleRate": 48000, "channelNum": 2 } }, "video": { "from": "{{streamsinsid}}", "format": { "codec": "h264", "profile": "CB" }, "parameters": { "resolution": { "width": 640, "height": 480 }, "framerate": 60, "bitrate": "x0.2", "keyFrameInterval": 1 } } } }]

let updateRecordingPars = [{ updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 6 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 12 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 15 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 24 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 30 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 48 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }, { updateRecordPar: [{ "op": "replace", "path": "/media/audio/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/from", "value": "{{roomId}}-common" }, { "op": "replace", "path": "/media/video/parameters/bitrate", "value": "x0.2" }, { "op": "replace", "path": "/media/video/parameters/resolution", "value": { "width": 640, "height": 480 } }, { "op": "replace", "path": "/media/video/parameters/framerate", "value": 60 }, { "op": "replace", "path": "/media/video/parameters/keyFrameInterval", "value": 1 }] }]
startrecordCreateroomOptions = {
    "name": "6",
    "options": {
        "notifying": {
            "streamChange": true,
            "participantActivities": true
        },
        "sip": {
            "sipServer": "10.239.44.36",
            "username": "jp63",
            "password": "123"
        },
        "transcoding": {
            "video": {
                "parameters": {
                    "keyFrameInterval": true,
                    "bitrate": true,
                    "framerate": true,
                    "resolution": true
                },
                "format": true
            },
            "audio": true
        },
        "mediaOut": {
            "video": {
                "parameters": {
                    "keyFrameInterval": [
                        100,
                        30,
                        5,
                        2,
                        1
                    ],
                    "bitrate": [
                        "x0.8",
                        "x0.6",
                        "x0.4",
                        "x0.2"
                    ],
                    "framerate": [
                        6,
                        12,
                        15,
                        24,
                        30,
                        48,
                        60

                    ],
                    "resolution": [
                        "x3/4",
                        "x2/3",
                        "x1/2",
                        "x1/3",
                        "x1/4",
                        "hd1080p",
                        "hd720p",
                        "svga",
                        "vga",
                        "cif"
                    ]
                },
                "format": [
                    {
                        "codec": "h264",
                        "profile": "CB"
                    },
                    {
                        "codec": "vp8"
                    },
                    {
                        "codec": "vp9"
                    }
                ]
            },
            "audio": [
                {
                    "codec": "g722",
                    "sampleRate": 16000,
                    "channelNum": 1
                },
                {
                    "codec": "isac",
                    "sampleRate": 16000
                },
                {
                    "codec": "isac",
                    "sampleRate": 32000
                },
                {
                    "codec": "pcma"
                },
                {
                    "codec": "pcmu"
                },
                {
                    "codec": "aac",
                    "sampleRate": 48000,
                    "channelNum": 2
                },
                {
                    "codec": "ac3"
                },
                {
                    "codec": "nellymoser"
                },
                {
                    "codec": "ilbc"
                },
                {
                    "codec": "opus",
                    "sampleRate": 48000,
                    "channelNum": 2
                }
            ]
        },
        "mediaIn": {
            "video": [
                {
                    "codec": "h264"
                },
                {
                    "codec": "vp8"
                },
                {
                    "codec": "vp9"
                }
            ],
            "audio": [
                {
                    "channelNum": 2,
                    "sampleRate": 48000,
                    "codec": "opus"
                },
                {
                    "sampleRate": 16000,
                    "codec": "isac"
                },
                {
                    "sampleRate": 32000,
                    "codec": "isac"
                },
                {
                    "channelNum": 1,
                    "sampleRate": 16000,
                    "codec": "g722"
                },
                {
                    "codec": "pcma"
                },
                {
                    "codec": "pcmu"
                },
                {
                    "codec": "aac"
                },
                {
                    "codec": "ac3"
                },
                {
                    "codec": "nellymoser"
                },
                {
                    "codec": "ilbc"
                }
            ]
        },
        "views": [
            {
                "video": {
                    "layout": {
                        "templates": {
                            "custom": [],
                            "base": "fluid"
                        },
                        "fitPolicy": "letterbox"
                    },
                    "keepActiveInputPrimary": false,
                    "bgColor": {
                        "b": 0,
                        "g": 0,
                        "r": 0
                    },
                    "motionFactor": 0.8,
                    "maxInput": 16,
                    "parameters": {
                        "keyFrameInterval": 100,
                        "framerate": 60,
                        "resolution": {
                            "height": 480,
                            "width": 640
                        }
                    },
                    "format": {
                        "codec": "vp8"
                    }
                },
                "audio": {
                    "vad": true,
                    "format": {
                        "channelNum": 2,
                        "sampleRate": 48000,
                        "codec": "opus"
                    }
                },
                "label": "common"
            }
        ],
        "roles": [
            {
                "subscribe": {
                    "video": true,
                    "audio": true
                },
                "publish": {
                    "video": true,
                    "audio": true
                },
                "role": "presenter"
            },
            {
                "subscribe": {
                    "video": true,
                    "audio": true
                },
                "publish": {
                    "video": false,
                    "audio": false
                },
                "role": "viewer"
            },
            {
                "subscribe": {
                    "video": false,
                    "audio": true
                },
                "publish": {
                    "video": false,
                    "audio": true
                },
                "role": "audio_only_presenter"
            },
            {
                "subscribe": {
                    "video": true,
                    "audio": false
                },
                "publish": {
                    "video": false,
                    "audio": false
                },
                "role": "video_only_viewer"
            },
            {
                "subscribe": {
                    "video": true,
                    "audio": true
                },
                "publish": {
                    "video": true,
                    "audio": true
                },
                "role": "sip"
            }
        ],
        "participantLimit": -1,
        "inputLimit": -1
    }
}
updaterecordCreateroomOptions = {
    name: 'citest',
    options: {
        "views": [
            {
                "video": {
                    "layout": {
                        "templates": {
                            "custom": [],
                            "base": "fluid"
                        },
                        "fitPolicy": "letterbox"
                    },
                    "keepActiveInputPrimary": false,
                    "bgColor": {
                        "b": 0,
                        "g": 0,
                        "r": 0
                    },
                    "motionFactor": 0.8,
                    "maxInput": 16,
                    "parameters": {
                        "keyFrameInterval": 100,
                        "framerate": 60,
                        "resolution": {
                            "height": 480,
                            "width": 640
                        }
                    },
                    "format": {
                        "codec": "vp8"
                    }
                },
                "audio": {
                    "vad": true,
                    "format": {
                        "channelNum": 2,
                        "sampleRate": 48000,
                        "codec": "opus"
                    }
                },
                "label": "common"
            }
        ],
    }
}
jsonPost = [{
    "algorithm": "dc51138a8284436f873418a21ba8cfa7",
    "media":
    {
        "audio": {
            "from": "77377660289427820",
            "format": {
                "codec": "opus",
                "sampleRate": 48000,
                "channelNum": 2
            }
        },
        "video": {
            "from": "77377660289427820",
            "format": {
                "codec": "h264",
                "profile": "CB"
            },
            "parameters": {
                "resolution": { "width": 640, "height": 480 },
                "framerate": 6,
                "bitrate": "x0.2",
                "keyFrameInterval": 1
            }
        }
    }
},
{
    "algorithm": "dc51138a8284436f873418a21ba8cfa7",
    "media":
    {
        "audio": {
            "from": "77377660289427820",
            "format": {
                "codec": "opus",
                "sampleRate": 48000,
                "channelNum": 2
            }
        },
        "video": {
            "from": "77377660289427820",
            "format": {
                "codec": "h264",
                "profile": "CB"
            },
            "parameters": {
                "resolution": { "width": 640, "height": 480 },
                "framerate": 6,
                "bitrate": "x0.6",
                "keyFrameInterval": 1
            }
        }
    }
}]
sipcallsPostp = [
    {
        "peerURI": "jp22@webrtctest18.sh.intel.com",
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

                "parameters": {
                    "resolution": {
                        "width": 640,
                        "height": 480
                    },
                    "framerate": 6,
                    "bitrate": "x0.2",
                    "keyFrameInterval": 1
                }
            }
        }
    }

]
sipcallsPatchp = [
    {patch:[
        {
            "op": "replace",
            "path": "/output/media/audio/from",
            "value": "719267872246669200"

        },
        {
            "op": "replace",
            "path": "/output/media/video/from",
            "value": "719267872246669200"

        },
        {
            "op": "replace",
            "path": "/output/media/video/parameters/resolution",
            "value": {
                "width": 640,
                "height": 480
            }

        },
        {
            "op": "replace",
            "path": "/output/media/video/parameters/framerate",

            "value": 6

        },
        {
            "op": "replace",
            "path": "/output/media/video/parameters/bitrate",


            "value": "x0.8"

        },

        {
            "op": "replace",
            "path": "/output/media/video/parameters/keyFrameInterval",
            "value": 1

        }
    ]}
]





module.exports = {
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
}