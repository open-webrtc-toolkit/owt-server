MCU Management REST API
----------------------

# 1 Introduction {#RESTAPIsection1}
Intel WebRTC solution provides a set of REST (Representational State Transfer) API for conference management. Manager clients can be implemented by different programming languages through these APIs.
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
      name: string,
      participantLimit: number,                    // -1 means no limit
      inputLimit: number,
      roles: [ object(Role) ],
      views: [ object(View) ],
      mediaIn: object(MediaIn),
      mediaOut: object(MediaOut),
      transcoding: object(Transcoding),
      notifying: object(Notifying),
      sip: object(Sip)
    }
    object(Role): {
      role: string,
      publish: {
        video: boolean,
        audio: boolean
      },
      subscribe: {
        video: boolean,
        audio: boolean
      }
    }
    object(View): {
      label: string,
      audio: object(ViewAudio),
      video: object(ViewVideo) | false
    }
    object(ViewAudio): {
      format: {                                   // object(AudioFormat)
        codec: string,                            // "opus", "pcmu", "pcma", "aac", "ac3", "nellymoser"
        sampleRate: number,                       // "opus/48000/2", "isac/16000/2", "isac/32000/2", "g722/16000/1"
        channelNum: number                        // E.g "opus/48000/2", "opus" is codec, 48000 is sampleRate, 2 is channelNum
      },
      vad: boolean
    }
    object(ViewVideo):{
      format: {                                   // object(VideoFormat)
        codec: string,                            // "h264", "vp8", "h265", "vp9"
        profile: string                           // For "h264" output only, CB", "B", "M", "H"
      },
      parameters: {
        resolution: object(Resolution),
        framerate: number,                        // valid values in [6, 12, 15, 24, 30, 48, 60]
        bitrate: number,                          // Kbps
        keyFrameInterval: number,                 // valid values in [100, 30, 5, 2, 1]
      },
      maxInput: number,                           // positive integer
      bgColor: {
        r: 0 ~ 255,
        g: 0 ~ 255,
        b: 0 ~ 255
      },
      motionFactor: number,                       // float
      keepActiveInputPrimary: boolean,
      layout: {
        fitPolicy: string,                        // "letterbox" or "crop".
        templates: {
          base: string,                           // valid values ["fluid", "lecture", "void"].
          custom: [
            { region: [ object(Region) ] },
            { region: [ object(Region) ] }
          ]
         }
      }
    }
    object(Resolution): {
      width: number,
      height: number
    }
    object(Region): {
      id: string,
      shape: string,
      area: {
        left: number,
        top: number,
        width: number,
        height: number
      }
    }
    object(MediaIn): {
      audio: [ object(AudioFormat) ]，              // Refers to the AudioFormat above.
      video: [ object(VideoFormat) ]                // Refers to the VideoFormat above.
    }
    object(MediaOut): {
      audio: [ object(AudioFormat) ],               // Refers to the AudioFormat above.
      video: {
        format: [ object(VideoFormat) ],            // Refers to the VideoFormat above.
        parameters: {
          resolution: [ string ],                   // Array of resolution.E.g. ["x3/4", "x2/3", ... "cif"]
          framerate: [ number ],                    // Array of framerate.E.g. [5, 15, 24, 30, 48, 60]
          bitrate: [ number ],                      // Array of bitrate.E.g. [500, 1000, ... ]
          keyFrameInterval: [ number ]              // Array of keyFrameInterval.E.g. [100, 30, 5, 2, 1]
        }
      }
    }
    object(Transcoding): {
      audio: boolean,
      video: {
        parameters: {
          resolution: boolean,
          framerate: boolean,
          bitrate: boolean,
          keyFrameInterval: boolean
        },
        format: boolean
      }
    }
    object(Notifying): {
      participantActivities: boolean,
      streamChange: boolean
    }
    object(Sip): {
      sipServer: string,
      username: string,
      password: string
    }
Resources:<br>

- /v1/rooms/
- /v1/rooms/{roomId}

### /v1/rooms
#### POST
Description:<br>
This function can create a room.<br>

parameters:

| parameters |   type   | annotation |
| :----------|:------|:------------------|
| RoomConfig | request body | Configuration used to create room |

This is for *RoomConfig*:

    object(RoomConfig):
    {
        name:name, // Required
        options: object(Room)  // Refers to room data model listed above. Optional
    }
response body:

| type | content |
|:-------------|:-------|
|      json     |  A room data model represents the room created |
#### GET
Description:<br>
List the rooms in your service.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    null    | null |    null    |

response body:

| type | content |
|:-------------|:-------|
|      json     | List of room data |

Here is an example of json data:

    [room0, room1, ...]
**Note**: Elements in list are of room data model.<br>

Pagination<br>

Requests that return multiple rooms will not be paginated by default. To avoid too much data of one call, you can set a custom page size with the ?per_page parameter.You can also specify further pages with the ?page parameter.<br>

GET "https://sample.api/rooms?page=2&per_page=10"<br>

Note that page numbering is 1-based and that omitting the ?page parameter will return the first page.

### /v1/rooms/{roomId}
#### GET
Description:<br>
Get information on the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|      json     | The required room data model |
#### DELETE
Description:<br>
Delete the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID to be deleted   |
| undefined | request body| Request body is null |

response body: response body is **empty**.
#### PUT
Description:<br>
Update a room's configuration entirely.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|  options or {}  | request body | Room configuration|

**Note**: *options* is same as *object(Options)* in 5.1, /v1/rooms/POST.<br>

response body:

| type | content |
|:-------------|:-------|
|      json     | The updated room data model |

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

### /v1/rooms/{roomId}/participants
#### GET
Description:<br>
List participants currently in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| undefined  | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|      json     | All participants in the specified room |
### /v1/rooms/{roomId}/participants/{participantId}
#### GET
Description:<br>
Get a participant's information from the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {participantId} | URL | Participant ID|
|  undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|      json     | The detail information of the specified participant |
#### PATCH
Description:<br>
Update the permission of a participant in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {participantId}| URL | Participant ID|
| items| request body | Permission item list to be updated|

**Note**: This is the definition of *items*.<br>

    items=[
        object(PermissionUpdate)
    ]
    object(PermissionUpdate):
    {
        op: "replace",

        // There are 2 kinds of path and value. Choose one of them.

        path: "/permission/[publish|subscribe]",
        value: object(Value)
          OR
        path: "/permission/[publish|subscribe]/[video|audio]",
        value: boolean
    }
    object(Value):
    {
        audio: boolean,
        video: boolean
    }
response body:

| type | content |
|:-------------|:-------|
|      json     | The updated permission information of the specified participant |
#### DELETE
Description:<br>
Drop a participant from a room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {participantId} | URL | Participant ID|
|  undefined | request body | Request body is null |

response body: response body is **empty**.<br>
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
        format: object(VideoFormat),
        parameters: {
            resolution: object(Resolution),     // Optional
            framerate: number(FramerateFPS),    // Optional
            bitrate: number(Kbps), // Optional
            keyFrameInterval: number(Seconds),  // Optional
        },
        optional: {
          format: [object(VideoFormat)]
          parameters: {
            resolution: [object(Resolution)],
            framerate: [number(FramerateFPS)],
            bitrate: [number(Kbps)] | [string("x" + Multiple)],
            keyFrameRate: [number(Seconds)]
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

- /v1/rooms/{roomId}/streams
- /v1/rooms/{roomId}/streams/{streamId}

### /v1/rooms/{roomId}/streams
#### GET
Description:<br>
List streams in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| undefined | request body | Request body is null |

response body:

| type | content |
|:-------------|:-------|
|      json     | All streams in the specified room |
### /v1/rooms/{roomId}/streams/{streamId}
#### GET
Description:<br>
Get a stream's information from the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|    {streamId}  | URL |   Stream ID   |
| undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|      json     | A stream's information in the specified room |
#### PATCH
Description:<br>
Update a stream's given attributes in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|    {streamId}  | URL |   Stream ID   |
|    items   | request body | An array with attributes to be updated|

**Note**: Here is a definition of *items*.<br>

    items=[
        object(StreamInfoUpdate)
    ]
    object(StreamInfoUpdate):
    {
        // There are 4 kinds of op, path, value. Choose one of them.

        op: "add" | "remove",
        path: "/info/inViews",
        value: string
         OR
        op: "replace",
        path: "/media/audio/status" | "/media/video/status",
        value: "active" | "inactive"
         OR
        op: "replace",
        path: "/info/layout/[0-9]+/stream",    // "/info/layout/[0-9]+/stream" is a pattern to match the needed path.
        value: string
         OR
        op: 'replace',                         // For mixed stream
        path: '/info/layout',
        value: [
            object(StreamRegion):
            {
                stream: string,
                region: object(Region)
            } ||
            object(EmptyRegion):               // Empty regions can ONLY appear at the very last consecutive positions.
            {
                region: object(Region)
            }
        ]
    }
response body:

| type | content |
|:-------------|:-------|
|      json     | A stream's updated information in the specified room |
#### DELETE
Description:<br>
Delete the specified stream from the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|    {streamId}  | URL |   Stream ID   |
| undefined | request body | Request body is null |

response body: response body is **empty**.<br>
## 5.4 Streaming-ins {#RESTAPIsection5_4}
Description:

Streaming-ins model is same as Stream model.

Resources:

- /v1/rooms/{roomId}/streaming-ins
- /v1/rooms/{roomId}/streaming-ins/{streamId}

### /v1/rooms/{roomId}/streaming-ins
#### POST
Description:<br>
Add an external RTSP/RTMP stream to the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| pub_req | request body | JSON format data with connection and media |

**Note**:

    Object(StreamingInRequest) {
        connection: {
          url: url,                            // string, e.g. "rtsp://...."
          transportProtocol: "udp" | "tcp",    // "tcp" by default.
          bufferSize: number                   // The buffer size in bytes in case "udp" is specified, 8192 by default.
        },
        media: {
          audio: "auto" | true | false,
          video: "auto" | true | false
        }
    }

response body:

| type | content |
|:-------------|:-------|
|  json | The detail information of the external RTSP/RTMP stream |
### /v1/rooms/{roomId}/streaming-ins/{streamId}
#### DELETE
Description:<br>
Stop the specified external streaming-in in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|    {streamId}  | URL |   Stream ID   |
|   undefined | request body | Request body is null |

response body: response body is **empty**.<br>
## 5.5 Streaming-outs {#RESTAPIsection5_5}
Description:

Streaming-outs model:

    object(StreamingOut):
    {
        id: string(id),
        media: object(OutMedia),
        url: string(url)
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
Resources:

- /v1/rooms/{roomId}/streaming-outs
- /v1/rooms/{roomId}/streaming-outs/{streamOutId}

### /v1/rooms/{roomId}/streaming-outs
#### GET
Description:<br>
Get all the ongoing streaming-outs in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|  undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|  json | All the ongoing streaming-outs in the specified room |
#### POST
Description:<br>
Start a streaming-out to the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| options | request body | JSON format with url and media |

**Note**:

    options={
      url: url,
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
|  json | The started streaming-out |
### /v1/rooms/{roomId}/streaming-outs/{streaming-outId}
#### PATCH
Description:<br>
Update a streaming-out's given attributes in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {streaming-outId} | URL | Streaming-out ID|
| items | request body | An array with attributes to be updated|

**Note**: Here is the definition of *items*.<br>

    items=[
        object(SubscriptionControlInfo)
    ]
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
|  json | A updated streaming-out's attributes |
#### DELETE
Description:<br>
Stop the specified streaming-out in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {streaming-outId} | URL | Streaming-out ID|
| undefined | request body | Request body is null |

response body: response body is **empty**.<br>
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

### /v1/rooms/{roomId}/recordings
#### GET
Description:<br>
Get all the ongoing recordings in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|  undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|  json | All the ongoing recordings in the specified room |
#### POST
Description:<br>
Start a recording in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| options | request body | JSON format with container and media |
**Note**:<br>

    options={
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
|  json | A recording in the specified room |
### /v1/rooms/{roomId}/recordings/{recordingId}
#### PATCH
Description:<br>
Update a recording's given attributes in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|   {recordingId} | URL | Recording ID|
| items | request body  | An array with attributes to be updated|

**Note**:<br>

    items=[
        object(SubscriptionControlInfo)       // Refers to object(SubscriptionControlInfo) in 5.5.
    ]

response body:

| type | content |
|:-------------|:-------|
|  json | A updated recording's attributes in the specified room |
#### DELETE
Description:<br>
Stop the specified recording in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {recordingId} | URL | Recording ID|
|  undefined | request body | Request body is null |

response body: response body is **empty**.<br>
## 5.7 Sipcalls {#RESTAPIsection5_7}
Description:

Sip call data model:

    object(SipCall):
    {
        id: string(id),
        type: string(type),
        peer: string(peerURI),
        input: object(StreamInfo),
        output: {
          id: string(SubscriptionId),
          media: object(SipOutMedia)
        }
    }
    type={"dial-in" | "dial-out"}
    object(SipOutMedia):
    {
        audio: object(SipOutAudio) | false,
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

### /v1/rooms/{roomId}/sipcalls
#### GET
Description:<br>
Get all the ongoing sip calls in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|  undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|  json | All the ongoing sip calls in the specified room |
#### POST
Description:<br>
Make a 'dial-out' typed sip call in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| options | request body | JSON format with peer URI and input/output media requirement|
**Note**:<br>

    options={
        peerURI: string,
        mediaIn: {
            audio: boolean(),      // Must be consistent with mediaOut.audio
            video: boolean()       // Must be consistent with mediaOut.video
        }
        mediaOut: object(SipOutMedia)
    }

response body:

| type | content |
|:-------------|:-------|
|  json | A SipCall record in the specified room |
### /v1/rooms/{roomId}/sipcalls/{sipCallId}
#### PATCH
Description:<br>
Update a sip calls specified output attributes in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {sipCallId} | URL | Sip Call ID|
| items | request body | An array with attributes to be updated|

**Note**: Here is the definition of *items*.<br>

    items=[
        object(MediaOutControlInfo)
    ]
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
|  json | A updated SipCall data |
#### DELETE
Description:<br>
End the specified sip call in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {sipCallId} | URL | Sip Call ID|
|  undefined | request body | Request body is null |

## 5.8 Analytics {#RESTAPIsection5_7}
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

### /v1/rooms/{roomId}/analytics
#### GET
Description:<br>
Get all the ongoing analytics in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
|  undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|  json | All the ongoing analytics in the specified room |
#### POST
Description:<br>
Start an analytic for a stream in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| options | request body | JSON format with peer URI and input/output media requirement|
**Note**:<br>

    options={
        algorithm: string(algorithmId),
        media: object(MediaSubOptions)       // Refers to object(MediaSubOptions) in 5.5.
    }

response body:

| type | content |
|:-------------|:-------|
|  json | An analytics object in the specified room |

### /v1/rooms/{roomId}/analytics/{analyticId}
#### DELETE
Description:<br>
End the specified analytic in the specified room.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {analyticId} | URL | Analytic ID|
|  undefined | request body | Request body is null |

response body: response body is **empty**.<br>

## 5.9 Token {#RESTAPIsection5_8}
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

### /v1/rooms/{roomId}/tokens
#### POST
Description:<br>
Create a new token when a new participant access a room needs to be added.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {roomId}    | URL |    Room ID    |
| {preference_body} | request body | JSON format with preference, user and role |
**Note**:

    preference_body={
        preference: object(Preference),     // Preference of this token would be used to connect through, refers to Data Model.
        user: string,           // Participant's user ID
        role: string            // Participant's role
    }
response body:

| type | content |
|:-------------|:-------|
|  json | A token created for a new participant |

## 5.9 Service {#RESTAPIsection5_9}
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

### /services
#### GET
Description:<br>
Get all the services.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    null    | null |    null    |

response body:

| type | content |
|:-------------|:-------|
|      json     | List of service data |

Here is an example of json data:

    [service0, service1, ...]
**Note**: Elements in list are of service data model.<br>

#### POST
Description:<br>
This function can create a service.<br>

parameters:

| parameters |   type   | annotation |
| :----------|:------|:------------------|
| ServiceConfig | request body | Configuration used to create service |

This is for *ServiceConfig*:

    object(ServiceConfig):
    {
        name: string,   // Name of the service to create
        key: string     // Key of the service to create
    }
response body:

| type | content |
|:-------------|:-------|
|      json     |  A service data model represents the service created |

### /services/{serviceId}
#### GET
Description:<br>
Get information on the specified service.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {serviceId}    | URL |    Room ID    |
| undefined | request body | Request body is null |
response body:

| type | content |
|:-------------|:-------|
|      json     | The required service data model |

#### DELETE
Description:<br>
Delete the specified service.<br>

parameters:

| parameters | type | annotation |
|:----------|:----|:----------|
|    {serviceId}    | URL |    Room ID to be deleted   |
| undefined | request body| Request body is null |

response body: response body is **empty**.
