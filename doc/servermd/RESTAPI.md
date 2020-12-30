Open WebRTC Toolkit(OWT) Server Management REST API Guide
----------------------

# 1 Introduction {#RESTAPIsection1}
Open WebRTC Toolkit solution provides a set of REST (Representational State Transfer) API for conference management. Management clients can be implemented by different programming languages through these APIs.
# 2 Definitions {#RESTAPIsection2}
Resource: the kind of data under manipulation<br>

Verb: HTTP verb. The mapping to operations is as following:

    - Create operations: POST
    - Read operations: GET
    - Update operations(entirely): PUT
    - Update operations(partially): PATCH
    - Delete operations: DELETE
URI: the Universal Resource Identifier, or name through which the resource can be manipulated.<br>

Request Body: the request data that user need supply to call the API. JSON format data is requried.<br>

Response Body: the response data is returned through HTTP response in JSON/text format.

# 3 Making a Call {#RESTAPIsection3}
With all the management API, perform the following steps to implement each call.<br>

1. Create a string for the http request that consist of the format [HTTP verb][url] + [headers] + [request body](optional)
2. Use your preferred language's network library (or http library) to send the request to REST server.
3. For authentication, REST API accept HTTP authentication on each request, see **section 4**(Authentication and Authorization) for details.
4. Read the response in your programming language, and parse the response JSON data.
5. Use the data in your application.

The following common HTTP status code may be returned after you make a call:

    - 200 - OK. Some calls return with a JSON format response while some calls return a plain text response indicates success.
    - 401 - Not authenticated or permitted.
    - 403 - Forbidden.
    - 404 - NOT found or no such resource.

Besides the common HTTP response code, there are also some internal status code for specific error conditions. Following is an example for response body used to represent  internal error in JSON format.

    {
        "error":{
          "code": 1001,
          "message": "Resource not found"
        }
    }

**Note**: "code": 1001 is a status code for internal error.And all the error following the same format.<br>

Here is a table describing the internal error code.

| code | description |
| :-------|:-------|
| 1001 | general not found |
| 1002 | service not found |
| 1003 | room not found |
| 1004 | stream not found |
| 1005 | participant not found |
| 1101 | authentication not found |
| 1102 | permission denied |
| 1201 | failed to call cluster |
| 1301 | request data error |
| 1401 | database error |
| 2001 | unexpected server error |

# 4 Authentication and Authorization {#RESTAPIsection4}
The management API can only be called after authorized. To get authorized, each request should be a HTTP request with "Authorization" filed in the header. So the server can determine whether it is a valid client. The value of "Authorization" is something like:

    MAuth realm=http://webrtc.intel.com,
    mauth_signature_method=HMAC_SHA256,
    mauth_username=test,
    mauth_role=role,
    mauth_serviceid=53c74879209ee7f96e5cbc9c,
    mauth_cnonce=87428,
    mauth_timestamp=1406079112038,
    mauth_signature=ZjA5NTJlMjE0ZTY4NzhmZWRjZDkxYjNmZjkyOTIxYzMyZjg3NDBjZA==

- Mauth realm=http://webrtc.intel.com, mauth_signature_method=HMAC_SHA256
- mauth_username and mauth_role are optional.
- mauth_serviceid is the ID of the service.
- mauth_cnonce is a random value between 0 and 99999.
- mauth_timestamp is the timestamp of the request.
- mauth_signature is a signature of this request. It uses HMAC_SHA256 to sign timestamp, cnonce, username(optional), role(optional) with service key. Then encrypted it with BASE64.

Python example of encryption algorithm (JavaScript version can be found in provided samples):

    from hashlib import HMAC_SHA256
    import hmac
    import binascii

    toSign = str(timestamp) + ',' + str(cnonce)
    if (username != '' and role != ''):
        toSign += ',' + username + ',' + role
    signed = self.calculateSignature(toSign, key)
    def calculateSignature(toSign, key):
        hash = hmac.new(key, toSign, HMAC_SHA256)
        signed = binascii.b2a_base64(hash.hexdigest())[:-1]
        return signed
**toSign** is the signature.<br>

**Note:**
If authentication failed, a response with code 401 would be returned.
# 5 Management Resource {#RESTAPIsection5}
The management API uses REST model to accessing different resources. The resources in conference management can be grouped into 7 kinds:

- Rooms
- Participants
- Streams
- Streaming-ins
- Streaming-outs
- Recordings
- Sipcalls
- Analytics
- Token

## 5.1 Rooms {#RESTAPIsection5_1}
Description:<br>
A room is the conference where users can communicate by real-time video and audio. Rooms are served by services. These resources can only be accessed after authenticated to their owner services. Rooms can have different settings for various scenarios. Making call on room resources provides users the ability to customize rooms for different needs.<br>

Data Model:<br>

    object(Room):
    {
      _id: string,
      name: string,                        // name of the room
      participantLimit: number,            // -1 means no limit
      inputLimit: number,                  // the limit for input stream
      roles: [ object(Role) ],             // role definition
      views: [ object(View) ],             // view definition, represents a mixed stream, empty list means no mixing
      mediaIn: object(MediaIn),            // the input media constraints allowed for processing
      mediaOut: object(MediaOut),          // the output media constraints
      transcoding: object(Transcoding),    // the transcoding control
      notifying: object(Notifying),        // notification control
      selectActiveAudio: boolean,          // select 3 most active audio streams for the room
      sip: object(Sip)                     // SIP configuration
    }

    object(Role): {
      role: string,        // name of the role
      publish: {
        video: boolean,    // whether the role can publish video
        audio: boolean     // whether the role can publish audio
      },
      subscribe: {
        video: boolean,    // whether the role can subscribe video
        audio: boolean     // whether the role can subscribe audio
      }
    }

    object(View): {
      label: string,
      audio: object(ViewAudio),           // audio setting for the view
      video: object(ViewVideo) | false    // video setting for the view
    }

    object(ViewAudio): {
      format: {                // object(AudioFormat)
        codec: string,         // "opus", "pcmu", "pcma", "aac", "ac3", "nellymoser"
        sampleRate: number,    // "opus/48000/2", "isac/16000/2", "isac/32000/2", "g722/16000/1"
        channelNum: number     // E.g "opus/48000/2", "opus" is codec, 48000 is sampleRate, 2 is channelNum
      },
      vad: boolean             // whether enable Voice Activity Detection for mixed audio
    }

    object(ViewVideo):{
      format: {                                   // object(VideoFormat)
        codec: string,                            // "h264", "vp8", "h265", "vp9"
        profile: string                           // For "h264" output only, CB", "B", "M", "H"
      },
      parameters: {
        resolution: object(Resolution),           // valid resolutions see [media constraints](#6-Media-Constraints)
        framerate: number,                        // valid values in [6, 12, 15, 24, 30, 48, 60]
        bitrate: number,                          // Kbps
        keyFrameInterval: number,                 // valid values in [100, 30, 5, 2, 1]
      },
      maxInput: number,                           // input limit for the view, positive integer
      bgColor: {
        r: 0 ~ 255,
        g: 0 ~ 255,
        b: 0 ~ 255
      },
      motionFactor: number,                       // float, affact the bitrate
      keepActiveInputPrimary: boolean,            // keep active audio's related video in primary region in the view
      layout: {
        fitPolicy: string,                        // "letterbox" or "crop".
        templates: {
          base: string,                           // template base, valid values ["fluid", "lecture", "void"].
          custom: [                               // user customized layout applied on base
            { region: [ object(Region) ] },       // a region list of length K represents a K-region-layout
            { region: [ object(Region) ] }        // detail of Region refer to the object(Region)
          ]
         }
      }
    }

    object(Resolution): {
      width: number,    // resolution width
      height: number    // resolution height
    }

    object(Region): {
      id: string,
      shape: string,      // "rectangle"
      area: {
        left: number,     // the left corner ratio of the region, [0, 1]
        top: number,      // the top corner ratio of the region, [0, 1]
        width: number,    // the width ratio of the region, [0, 1]
        height: number    // the height ratio of the region, [0, 1]
      }
    }

    object(MediaIn): {
      audio: [ object(AudioFormat) ]，    // Refers to the AudioFormat above.
      video: [ object(VideoFormat) ]      // Refers to the VideoFormat above.
    }

    object(MediaOut): {
      audio: [ object(AudioFormat) ],       // Refers to the AudioFormat above.
      video: {
        format: [ object(VideoFormat) ],    // Refers to the VideoFormat above.
        parameters: {
          resolution: [ string ],           // Array of resolution.E.g. ["x3/4", "x2/3", ... "cif"]
          framerate: [ number ],            // Array of framerate.E.g. [5, 15, 24, 30, 48, 60]
          bitrate: [ number ],              // Array of bitrate.E.g. [500, 1000, ... ]
          keyFrameInterval: [ number ]      // Array of keyFrameInterval.E.g. [100, 30, 5, 2, 1]
        }
      }
    }

    object(Transcoding): {
      audio: boolean,                  // if allow transcoding format(opus, pcmu, ...) for audio
      video: {
        parameters: {
          resolution: boolean,         // if allow transcoding resolution for video
          framerate: boolean,          // if allow transcoding framerate for video
          bitrate: boolean,            // if allow transcoding bitrate for video
          keyFrameInterval: boolean    // if allow transcoding KFI for video
        },
        format: boolean                // if allow transcoding format(vp8, h264, ...) for video
      }
    }

    object(Notifying): {
      participantActivities: boolean,    // whether enable notification for participantActivities
      streamChange: boolean              // whether enable notification for streamChange
    }

    object(Sip): {
      sipServer: string,    // host or IP address for the SIP server
      username: string,     // username of SIP account
      password: string      // password of SIP account
    }
Resources:<br>

- /v1/rooms/
- /v1/rooms/{roomId}

### Create Room {#RESTAPIsection5_1_1}
**POST ${host}/v1/rooms**

Description:<br>
This request can create a room.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     |  object(RoomConfig) |

    object(RoomConfig):          // Configuration used to create room
    {
        name:name,               // Name of the room, required
        options: object(Room)    // Subset of the object(Room) definition above, optional.
    }
response body:

| type | content |
|:-------------|:-------|
|      json     |  object(Room) |

### List Rooms {#RESTAPIsection5_1_2}
**GET ${host}/v1/rooms**

Description:<br>
List the rooms in your service.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     | [ object(Room) ] |

Pagination<br>

Requests that return multiple rooms will not be paginated by default. To avoid too much data of one call, you can set a custom page size with the ?per_page parameter. You can also specify further pages with the ?page parameter.<br>

GET "https://sample.api/rooms?page=2&per_page=10"<br>

Note that page numbering is 1-based and that only omitting the ?page parameter will return the first page.

### Get Room {#RESTAPIsection5_1_3}
**GET ${host}/v1/rooms/{roomId}**

Description:<br>
Get information on the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     | object(Room) |

### Delete Room {#RESTAPIsection5_1_4}
**DELETE ${host}/v1/rooms/{roomId}**

Description:<br>
Delete the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

### Update Room {#RESTAPIsection5_1_5}
**PUT ${host}/v1/rooms/{roomId}**

Description:<br>
Update a room's configuration entirely.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     |  object(Room) |

**Note**: *Room* is same as *object(Room)* in "Create Room".<br>

response body:

| type | content |
|:-------------|:-------|
|      json     |  object(Room) |

## 5.2 Participants {#RESTAPIsection5_2}
Description:

ParticipantDetail model:

    object(Permission)：
    {
        publish: {
            audio: true | false,
            video: true | false
        } | false,
        subscribe: {
            audio: true | false,
            video: true | false
        } | false
    }
    object(ParticipantDeatil):
    {
        id: string(ParticipantId),
        role: string(participantRole),
        user: string(userId),
        permission: object(Permission)
    }
Resources:

- /v1/rooms/{roomId}/participants
- /v1/rooms/{roomId}/participants/{participantId}

### List Participants {#RESTAPIsection5_2_1}
**GET ${host}/v1/rooms/{roomId}/participants**

Description:<br>
List participants currently in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     |  [ object(ParticipantDeatil) ] |

### Get Participant {#RESTAPIsection5_2_2}
**GET ${host}/v1/rooms/{roomId}/participants/{participantId}**

Description:<br>
Get a participant's information from the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     |  object(ParticipantDeatil) |

### Update Participant {#RESTAPIsection5_2_3}
**PATCH ${host}/v1/rooms/{roomId}/participants/{participantId}**

Description:<br>
Update the permission of a participant in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     |  [ object(PermissionUpdate) ] |

**Note**: This is the definition of *PermissionUpdate*.<br>

    object(PermissionUpdate):
    {
        op: "replace",

        // There are 2 kinds of path and value. Choose one of them.
        path: "/permission/[publish|subscribe]",
        value: { audio: boolean, video: boolean }
          OR
        path: "/permission/[publish|subscribe]/[video|audio]",
        value: boolean
    }

response body:

| type | content |
|:-------------|:-------|
|      json     |  object(ParticipantDeatil) |

### Drop Participant {#RESTAPIsection5_2_4}
**DELETE ${host}/v1/rooms/{roomId}/participants/{participantId}**

Description:<br>
Drop a participant from a room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.3 Streams {#RESTAPIsection5_3}
Description:

Streams model:

    object(StreamInfo):
    {
        id: string(StreamID),
        type: "forward" | "mixed",
        media: object(MediaInfo),
        info: object(MixedInfo) | Object(ForwardInfo)
    }
    object(MediaInfo):
    {
        audio: object(AudioInfo) | false,
        video: object(VideoInfo) | false
    }
    object(AudioInfo):
    {
        status: "active" | "inactive", // For forward stream
        source: "mic" | "screen-cast" | "raw-file" | "encoded-file" | "streaming", // For forward stream
        format: object(AudioFormat),
        optional: {
            format: [object(AudioFormat)]
        }
    }
    object(AudioFormat):
    {
        codec: "pcmu" | "pcma" | "opus" | "g722" | "iSAC" | "iLBC" | "aac" | "ac3" | "nellymoser",
        sampleRate: number,              // Optional
        channelNum: number               // Optional
    }
    object(VideoInfo):
    {
        status: "active" | "inactive", // For forward stream
        source: "camera" | screen-cast" | "raw-file" | "encoded-file" | "streaming", // For forward stream
        original: [{
          format: object(VideoFormat),
          parameters: {
            resolution: object(Resolution),     // Optional
            framerate: number(FramerateFPS),    // Optional
            bitrate: number(Kbps),              // Optional
            keyFrameInterval: number(Seconds),  // Optional
          },
          simulcastRid: string(SimulcastRid)    // Optional
        }],
        optional: {                             // Not available for simulcast streams
          format: [object(VideoFormat)],
          parameters: {
            resolution: [object(Resolution)],
            framerate: [number(FramerateFPS)],
            bitrate: [number(Kbps)] | [string("x" + Multiple)],
            keyFrameInterval: [number(Seconds)]
          }
        }
      }
    }
    object(VideoFormat):
    {
        codec: "h264" | "h265" | "vp8" | "vp9",
        profile: "CB" | "B" | "M" | "H"    // For "h264" codec only
    }
    object(VideoParameters):
    {
        resolution: object(Resolution),    // Refers to section 5.1, object(Resolution).
        framerate: number,
        bitrate: number,
        keyFrameInterval: number
    }
    object(ForwardInfo):
    {
        owner: string(ParticipantId),
        type: "webrtc" | "streaming" | "sip" | "analytics",
        inViews: [string(ViewLabel)],
        attributes: object(ExternalDefinedObj)
    }
    object(MixedInfo):
    {
      label: string(ViewLabel),
      layout: [{
        stream: string(StreamId) | undefined,
        region: object(Region)
      }]
    }
Resources:

- /v1.1/rooms/{roomId}/streams
- /v1.1/rooms/{roomId}/streams/{streamId}

### List Streams {#RESTAPIsection5_3_1}
**GET ${host}/v1.1/rooms/{roomId}/streams**

Description:<br>
List streams in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     | [ Object(StreamInfo) ] |

### Get Stream {#RESTAPIsection5_3_2}
**GET ${host}/v1.1/rooms/{roomId}/streams/{streamId}**

Description:<br>
Get a stream's information from the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|      json     | object(StreamInfo) |

### Update Stream {#RESTAPIsection5_3_3}
**PATCH ${host}/v1.1/rooms/{roomId}/streams/{streamId}**

Description:<br>
Update a stream's given attributes in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | [ object(StreamUpdate) ] |

**Note**: Here is a definition of *StreamUpdate*.<br>

    // There are 4 kinds of StreamUpdate,
    // object(MixUpdate), object(StatusUpdate), for forward stream,
    // object(RegionUpdate), object(LayoutUpdate), for mixed stream,
    // choose one of them.
    object(MixUpdate):
    {
        op: "add" | "remove",
        path: "/info/inViews",
        value: string(viewLabel)
    }
    object(StatusUpdate):
    {
        op: "replace",
        path: "/media/audio/status" | "/media/video/status",
        value: "active" | "inactive"
    }
    object(RegionUpdate):
        op: "replace",
        path: "/info/layout/${regionIndex}/stream",    // ${regionIndex} is an integer represents the index of the region
        value: string(streamID)                        // the new streamID for the region
    }
    object(LayoutUpdate):
    {
        op: 'replace',
        path: '/info/layout',
        value: [
            object(StreamRegion):
            {
                stream: string,
                region: object(Region)
            } |
            object(EmptyRegion):          // empty regions can ONLY appear at the very last consecutive positions.
            {
                region: object(Region)
            }
        ]
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(StreamInfo) |


### Drop Stream {#RESTAPIsection5_3_4}
**DELETE ${host}/v1.1/rooms/{roomId}/streams/{streamId}**

Description:<br>
Delete the specified stream from the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.4 Streaming-ins {#RESTAPIsection5_4}
Description:

Streaming-ins model is same as Stream model.

Resources:

- /v1.1/rooms/{roomId}/streaming-ins
- /v1.1/rooms/{roomId}/streaming-ins/{streamId}

### Start Streaming-in {#RESTAPIsection5_4_1}
**POST ${host}/v1.1/rooms/{roomId}/streaming-ins**

Description:<br>
Add an external RTSP/RTMP stream to the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(StreamingInRequest) |

**Note**: Here is a definition of *StreamingInRequest*.<br>

    Object(StreamingInRequest) {
        connection: {
          url: string(url),                    // string, e.g. "rtsp://...."
          transportProtocol: "udp" | "tcp",    // "tcp" by default.
          bufferSize: number(bytes)            // The buffer size in bytes in case "udp" is specified, 8192 by default.
        },
        media: {
          audio: "auto" | true | false,
          video: "auto" | true | false
        }
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(StreamInfo) |

### Stop Streaming-in {#RESTAPIsection5_4_2}
**DELETE ${host}/v1.1/rooms/{roomId}/streaming-ins/{streamId}**

Description:<br>
Stop the specified external streaming-in in the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.5 Streaming-outs {#RESTAPIsection5_5}
Description:

Streaming-outs model:

    object(StreamingOut):
    {
        id: string(id),
        media: object(OutMedia),
        protocol: "rtmp" | "rtsp" | "hls" | "dash",
        url: string(url),
        parameters: object(HlsParameters) | object(DashParameters) | undefined
    }
    object(OutMedia):
    {
        audio: object(StreamingOutAudio) | false,
        video: object(StreamingOutVideo) | false
    }
    object(StreamingOutAudio):
    {
        from: string(StreamId),
        format: object(AudioFormat) | undefined   // Refers to object(AudioFormat) in 5.3, streams data model.
    }
    object(StreamingOutVideo):
    {
        from: string(StreamId),
        format: object(VideoFormat) | undefined,  // Refers to object(VideoFormat) in 5.3, streams data model.
        parameters: object(VideoParametersSpecification)
    }
    object(VideoParametersSpecification):
    {
         resolution: object(Resolution),  // Refers to object(Resolution) in 5.3, streams data model.
         framerate: number(WantedFrameRateFPS),
         bitrate: number(WantedBitrateKbps) | string(WantedBitrateMultiple), // Must be in array of VideoInfo.optional.parameters.bitrate in 5.3, streams data model.
         keyFrameInterval: number(WantedKeyFrameIntervalSecond)
    }
    object(HlsParameters):
    {
        method: "PUT" | "POST",
        hlsTime: number(HlsTime) | undefined,
        hlsListSize: number(HlsListSize) | undefined
    }
    object(DashParameters):
    {
        method: "PUT" | "POST",
        dashSegDuration: number(DashSegDuration) | undefined,
        dashWindowSize: number(DashWindowSize) | undefined
    }
Resources:

- /v1/rooms/{roomId}/streaming-outs
- /v1/rooms/{roomId}/streaming-outs/{streamOutId}

### List Streaming-outs {#RESTAPIsection5_5_1}
**GET ${host}/v1/rooms/{roomId}/streaming-outs**

Description:<br>
Get all the ongoing streaming-outs in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | [ object(StreamingOut) ] |

### Start Streaming-outs {#RESTAPIsection5_5_2}
**POST ${host}/v1/rooms/{roomId}/streaming-outs**

Description:<br>
Start a streaming-out to the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(StreamingOutRequest) |

**Note**: Here is a definition of *StreamingOutRequest*.<br>

    object(StreamingOutRequest):
    {
      protocol: "rtmp" | "rtsp" | "hls" | "dash",
      url: string(url),
      parameters: object(HlsParameters) | object(DashParameters), // optional, depends on protocol
      media: object(MediaSubOptions)
    }

    object(MediaSubOptions):
    {
        audio: {
            from: string,          // target StreamID
            format: {              // not work for streaming-out, "aac" will be used
              codec: string,       // for recording, "opus", "pcmu" ... available codec in target stream
              sampleRate: number,  // optional, depends on codec
              channelNum: number   // optional, depends on codec
            }
        } || false,
        video: {
            from: string,          // target StreamID
            format: {              // not work for streaming-out, "h264" will be used
              codec: string,       // for recording, "vp8", "h264" ... available codec in target stream
              profile: string      // optional, depends on codec ("h264")
            },
            parameters: {          // following values should be in stream's default/optional list
              resolution: { width: number, height: number },       // optional
              framerate: number,                                   // optional
              bitrate: string,                                     // optional, e.g. "x0.2", "x0.4"
              keyFrameInterval: number                             // optional
            }
        } || false
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(StreamingOut) |

### Update Streaming-out {#RESTAPIsection5_5_3}
**PATCH ${host}/v1/rooms/{roomId}/streaming-outs/{streaming-outId}**

Description:<br>
Update a streaming-out's given attributes in the specified room.<br>

parameters:

| type | content |
|:-------------|:-------|
|  json | [ object(SubscriptionControlInfo) ] |

**Note**: Here is the definition of *SubscriptionControlInfo*.<br>

    object(SubscriptionControlInfo):
    {
        // There are 6 kinds of op, path and value. Choose one of them.

        op: "replace",
        path: "/media/audio/from" | "/media/video/from",
        value: string
         OR
        op: "replace",
        path: "/media/video/parameters/resolution",
        value: object(Resolution)        // Refers to object(Resolution) in 5.3 streams, streams model.
         OR
        op: "replace",
        path: "/media/video/parameters/framerate",
        value: 6 | 12 | 15 | 24 | 30 | 48 | 60
         OR
        op: "replace",
        path: "/media/video/parameters/bitrate",
        value: "x0.8" | "x0.6" | "x0.4" | "x0.2"
         OR
        op: "replace",
        path: "/media/video/parameters/keyFrameInterval",
        value: 1 | 2 | 5 | 30 | 100
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(StreamingOut) |

### Stop Streaming-out {#RESTAPIsection5_5_4}
**DELETE ${host}/v1/rooms/{roomId}/streaming-outs/{streaming-outId}**

Description:<br>
Stop the specified streaming-out in the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.6 Recordings {#RESTAPIsection5_6}
Description:

Recordings data model:

    object(Recordings):
    {
        id: string(id),
        media: object(OutMedia),       // Refers to object(OutMedia) in 5.5.
        storage: {
          host: string(host),
          file: string(filename)
        }
    }

**Note**:<br>
*object(OutMedia)* is same as streaming-outs data model in 5.5.<br>

Resources:

- /v1/rooms/{roomId}/recordings
- /v1/rooms/{roomId}/recordings/{recordingId}

### List Recordings {#RESTAPIsection5_6_1}
**GET ${host}/v1/rooms/{roomId}/recordings**

Description:<br>
Get all the ongoing recordings in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | [ object(Recordings) ] |

### Start Recordings {#RESTAPIsection5_6_2}
**POST ${host}/v1/rooms/{roomId}/recordings**

Description:<br>
Start a recording in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(RecordingRequest) |

**Note**: Here is the definition of *RecordingRequest*.<br>

    object(RecordingRequest):
    {
        container: string(ContainerType),
        media: object(MediaSubOptions)       // Refers to object(MediaSubOptions) in 5.5. And only "pcmu", "pcma", "opus" and "aac" audio codec are supported.
    }
    /*
     * 1, If "auto" is specified, then the actual container type will be automatically determined by following the below rules.
     *     1) if audio codec is "aac" (which requires "lib-fdk-aac") and video is disabled or video codec is either "h264" or "h265", then "mp4" container type will be applied.
     *     2) otherwise, 'mkv' will be applied.
     * 2, If "mp4" can be specified only if: 1) audio codec is "aac" or audio is disabled, and 2) video codec is either "h264" or "h265" or video is disabled.
     * 3, "mkv" can always be specified and for all supported audio and video codecs.
     */
    ContainerType={ "auto" | "mkv" | "mp4" }

response body:

| type | content |
|:-------------|:-------|
|  json | object(Recordings) |

### Update Recording {#RESTAPIsection5_6_3}
**PATCH ${host}/v1/rooms/{roomId}/recordings/{recordingId}**

Description:<br>
Update a recording's given attributes in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | [ object(SubscriptionControlInfo) ] |

**Note**: object(SubscriptionControlInfo) is the same object in 5.5<br>

response body:

| type | content |
|:-------------|:-------|
|  json | object(Recordings) |

### Stop Recording {#RESTAPIsection5_6_4}
**DELETE ${host}/v1/rooms/{roomId}/recordings/{recordingId}**

Description:<br>
Stop the specified recording in the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.7 Sipcalls {#RESTAPIsection5_7}
Description:

Sip call data model:

    object(SipCall):
    {
        id: string(id),
        type: "dial-in" | "dial-out"
        peer: string(peerURI),
        input: object(StreamInfo),
        output: {
          id: string(SubscriptionId),
          media: object(SipOutMedia)
        }
    }
    object(SipOutMedia):
    {
        audio: object(SipOutAudio),
        video: object(SipOutVideo) | false
    }
    object(SipOutAudio):
    {
        from: string(StreamId),
    }
    object(SipOutVideo):
    {
        from: string(StreamId),
        parameters: object(VideoParametersSpecification) // Refer to definition in 5.5
    }

Resources:

- /v1/rooms/{roomId}/sipcalls
- /v1/rooms/{roomId}/sipcalls/{sipCallId}

### Get SIP Call {#RESTAPIsection5_7_1}
**GET ${host}/v1/rooms/{roomId}/sipcalls**

Description:<br>
Get all the ongoing sip calls in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | object(SipCall) |

### New SIP Call {#RESTAPIsection5_7_2}
**POST ${host}/v1/rooms/{roomId}/sipcalls**

Description:<br>
Make a 'dial-out' typed sip call in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(SipCallRequest) |

**Note**:<br>

    object(SipCallRequest):
    {
        peerURI: string,
        mediaIn: {
            audio: true,           // Audio must be true for sip calls.
            video: boolean         // Must be consistent with mediaOut.video
        }
        mediaOut: object(SipOutMedia) 
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(SipCall) |

### Update SIP call {#RESTAPIsection5_7_3}
**PATCH ${host}/v1/rooms/{roomId}/sipcalls/{sipCallId}**

Description:<br>
Update a sip calls specified output attributes in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | [ object(MediaOutControlInfo) ] |

**Note**: Here is the definition of *MediaOutControlInfo*.<br>

    object(MediaOutControlInfo):
    {
        // There are 6 kinds of op, path and value. Choose one of them.
        // The valid values of ("resolution", "framerate", "bitrate", "keyFrameInterval")
        // are according to those in Room-MediaOut-video-parameters

        op: "replace",
        path: "/output/media/audio/from" | "/output/media/video/from",
        value: string
         OR
        op: "replace",
        path: "/output/media/video/parameters/resolution",
        value: object(Resolution)                     // Refers to object(Resolution) in 5.3 streams, streams model.
         OR
        op: "replace",
        path: "/output/media/video/parameters/framerate",
        value: 6 | 12 | 15 | 24 | 30 | 48 | 60
         OR
        op: "replace",
        path: "/output/media/video/parameters/bitrate",
        value: "x0.8" | "x0.6" | "x0.4" | "x0.2"
         OR
        op: "replace",
        path: "/output/media/video/parameters/keyFrameInterval",
        value: 1 | 2 | 5 | 30 | 100
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(SipCall) |

### Delete SIP Call {#RESTAPIsection5_7_4}
**DELETE ${host}/v1/rooms/{roomId}/sipcalls/{sipCallId}**

Description:<br>
End the specified sip call in the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.8 Analytics {#RESTAPIsection5_8}
Description:

Analytics data model:

    object(Analytics):
    {
        id: string(analytic-id),
        analytics: {
          algorithm: string(algorithmId)
        },
        media: {
          format: object(VideoFormat),
          from: string(sourceStreamId)
        }
    }

Resources:

- /v1/rooms/{roomId}/analytics
- /v1/rooms/{roomId}/analytics/{analyticId}

### List Analytics {#RESTAPIsection5_8_1}
**GET ${host}/v1/rooms/{roomId}/analytics**

Description:<br>
Get all the ongoing analytics in the specified room.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | [ object(Analytics) ] |

### Start Analytics {#RESTAPIsection5_8_2}
**POST ${host}/v1/rooms/{roomId}/analytics**

Description:<br>
Start an analytics for a stream in the specified room.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(AnalyticsRequest) |

**Note**:<br>

    object(AnalyticsRequest):
    {
        algorithm: string(algorithmId),
        media: object(MediaSubOptions)       // Refers to object(MediaSubOptions) in 5.5.
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(Analytics) |

### Stop Analytics {#RESTAPIsection5_8_3}
**DELETE ${host}/v1/rooms/{roomId}/analytics/{analyticId}**

Description:<br>
End the specified analytic in the specified room.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.9 Token {#RESTAPIsection5_9}
Description:<br>
A token is the ticket for joining the room. The token contains information through which clients can connect to server application. Note that the different rooms may have different network information, so the room must be specified for token resource. The same token cannot be reused if it has been used once. Re-generate token if clients need to connect at the second time.<br>

Data Model:<br>
Token data in JSON example:

    object(Tokens):
    {
        room: roomID,
        user: participant's user ID,
        role: participant's role,
        preference: object(Preference)
    }
    object(Preference):
    {
        isp: string,
        region: string
    }
Resources:

- /v1/rooms/{roomId}/tokens

### Create Token {#RESTAPIsection5_9_1}
**POST ${host}/v1/rooms/{roomId}/tokens**

Description:<br>
Create a new token when a new participant access a room needs to be added.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(TokenRequest) |

**Note**:

    object(TokenRequest):
    {
        preference: object(Preference),     // Preference of this token would be used to connect through, refers to Data Model.
        user: string,           // Participant's user defined ID
        role: string            // Participant's role
    }

request body:

| type | content |
|:-------------|:-------|
|  json | object(TokenRequest) |

response body:

| type | content |
|:-------------|:-------|
|  text | A token string |

## 5.10 Service {#RESTAPIsection5_10}
Description:<br>
The service represents the host of a set of rooms. Each service has its own identifier. The service ID and key are required to generate authentication header for HTTP requests. Note that there is one super service whose ID can be specified by toml file. The service resource can only be manipulated under super service authentication while other resouces require corresponding host service's authentication. (This API may change in later versions)

Data Model:<br>
Service data in JSON example:

    object(Service):
    {
        _id: string,                   // The ID of the service
        name: string,                  // The name of the service
        key: string,                   // The key of the service
        encrypted: boolean             // Encrypted or not
        rooms: [ string ]              // The list of room ID under this service
    }
Resources:

- /services
- /services/{serviceId}

### List Services {#RESTAPIsection5_10_1}
**GET ${host}/services**

Description:<br>
Get all the services.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | [ object(Service) ] |

### Create Service {#RESTAPIsection5_10_2}
**POST ${host}/services**

Description:<br>
This request can create a service.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(ServiceConfig) |

**Note**: Here is a definition of *ServiceConfig*.<br>

    object(ServiceConfig):
    {
        name: string,   // Name of the service to create
        key: string     // Key of the service to create
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(Service) |

### Get Service {#RESTAPIsection5_10_3}
**GET ${host}/services/{serviceId}**

Description:<br>
Get information on the specified service.<br>

request body:

  **Empty**

response body:

| type | content |
|:-------------|:-------|
|  json | object(Service) |

### Delete Service
**DELETE ${host}/services/{serviceId}**

Description:<br>
Delete the specified service.<br>

request body:

  **Empty**

response body:

  **Empty**

# 6 Media Constraints {#RESTAPIsection6}
This section lists the media constraints of formats and parameters which can be passed to management API.

## 6.1 Audio {#RESTAPIsection6_1}
Audio formats(codec/sampleRate/channelNum):
- opus/48000/2
- isac/16000
- isac/32000
- g722/16000/1
- pcma
- pcmu
- ac3
- nellymoser
- ilbc
- aac(input)
- aac/48000/2(output)

## 6.2 Video {#RESTAPIsection6_2}
Video formats(codec/profile):
- h264(input)
- h264/CB(output)
- h264/B(output)
- h264/M(output)
- h264/H(output)
- vp8
- vp9
- h265

Video framerate: [6, 12, 15, 24, 30, 48, 60]
