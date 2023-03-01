Open WebRTC Toolkit(OWT) Server Stream REST API Guide
----------------------

# 1 Introduction {#StreamAPIsection1}
Open WebRTC Toolkit solution provides a set of REST (Representational State Transfer) API for stream level management similar as [Conference Management API](RESTAPI.md).
Unlike conference management, this set of API manipulates low level streams in OWT server directly and customers can build their own control logic based on these APIs.

The OWT stream management follows pulish-subscribe model. There are several main concepts in stream management:
1. Pubilcation: A client's stream can be published to OWT server, then it becomes a publication. In other words, publiations represents streams available on OWT server which are streaming from clients continously.
2. Subscription: A published stream on OWT server can be subscribed by clients, then a subscription is created. A subscription represents an ongoing subscribe operation performed by a client. The streaming direction is from OWT server to clients, which is opposite to publication.
3. Processor: Stream on OWT server can be processed by processors. For example, video mixer and video transcoder are processors. A processor usually can take multiple published stream as input and produce new streams as output.
4. Participant: A participant represents a client which can perform publish and subscribe operations on OWT.
5. WorkerNode: A worker node is actual instance running streams on OWT. A worker node can receive streams from clients, send streams to clients or process streams.
6. Domain: Domain is the scope of above objects, objects from different domains can not interact with each other. When a domain is created with a name, a participant with same name is also created in that domain for user actions.

Because it's REST, management clients can be implemented by different programming languages through these APIs just by sending HTTP requests.

# 2 Enable Stream API {#StreamAPIsection2}
To enable stream API, add experimental targets `stream-service` and `customized-agent` during packing. `stream-service` is the module that provide stream related API. `customized-agent` is the module that provide server side customization for stream related API.

After packing, the stream API configuration is in stream_service/service.toml.
Edit portal/portal.toml, set `stream_engine_name` to the same value of stream API configuration(`service.name` or `scheduler.name` in stream_service/service.toml).
Edit management_api/management_api.toml, set `stream_engine` and `control_agent` to the same values of stream API configuration.

Start OWT service with updated configuration, stream API should be enabled.

# 3 Definitions {#StreamAPIsection3}
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

# 4 Making a Call {#StreamAPIsection4}
The process are same as conference management API, perform the following steps to implement each call.<br>

1. Create a string for the http request that consist of the format [HTTP verb][url] + [headers] + [request body](optional)
2. Use your preferred language's network library (or http library) to send the request to REST server.
3. For authentication, REST API accept HTTP authentication on each request, see **section 4**(Authentication and Authorization) for details.
4. Read the response in your programming language, and parse the response JSON data.
5. Use the data in your application.

Example of using curl:

    curl -X GET -H "Autthorization: ${YOUR_REQ_AUTH}" https://my-stream-api/streams"

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

# 4 Authentication and Authorization {#StreamAPIsection4}
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

# 5 API Resource {#StreamAPIsection5}
The stream API uses REST model to accessing different resources. Following resources are available:

- Tokens
- Publications
- Subscriptions
- Processors
- Nodes

## 5.1 Tokens {#StreamAPIsection5_1}
Description:<br>
Tokens are used for OWT WebRTC client to connect OWT server, see OWT client SDK join API for details.<br>

Data Model:<br>

Data Model:<br>
Token data in JSON example:

    object(Tokens):
    {
        room: roomID, // Same as domain name
        user: participant's user ID,
        role: participant's role,
    }

Resources:<br>

- /v1.1/stream-engine/tokens

### Create Token {#StreamAPIsection5_1_1}
**POST ${host}/v1.1/stream-engine/tokens**

Description:<br>
This request can create a token used for OWT client.<br>

request body:

    object(TokenRequest):
    {
        user: string,  // Participant's user name
        role: string,  // Participant's role
        preference: {
            isp: string,
            region: string
        },
        domain: string // Participant's domain
    }

request body:

| type | content |
|:-------------|:-------|
|  json | object(TokenRequest) |

response body:

| type | content |
|:-------------|:-------|
| text | A token string |


## 5.2 Publications {#StreamAPIsection5_2}
Description:<br>
Publiation represents stream published by users on OWT server.<br>

Data model:

    object(Publication):
    {
        id: string(StreamID),
        type: string(StreamType), // "webrtc", "streaming", ......
        source: {
            audio: [ object(SourceTrack) ],
            video: [ object(SourceTrack) ],
            data: [ object(SourceTrack) ],
        },
        info: object(StreamInfo),
        locality: { agent: string(AgentName), node: string(NodeName) },
        participant: string(ownerParticipantId),
        domain: string(domainName)
    }
    object(SourceTrack):
    {
        format: object(VideoFormat) | object(AudioFormat),
        parameters: object(VideoParameters) | undefined
    }

For *object(AudioFormat)*, *object(VideoFormat)*, *object(VideoParameters)*, refers to [REST API](RESTAPI.md#53-streams-StreamAPIsection53).

Resources:

- /v1.1/stream-engine/publications
- /v1.1/stream-engine/publications/{streamId}

### List Publications {#StreamAPIsection5_2_1}
**GET ${host}/v1.1/stream-engine/publications**
**GET ${host}/v1.1/stream-engine/publications/{streamId}**

Description:<br>
List publications in stream engine.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | object(ListQuery) |

**Note**: Definition of *ListQuery*.<br>

    object(ListQuery):
    {
        query: {
            KEY: VALUE // Specified key-value pair for query result, such as `{name: "default"}`
        }
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(ListResult) |

**Note**: Definition of *ListResult*.<br>

    object(ListResult):
    {
        total: number(ListSize),
        start: number(offsetInList),
        data: [ object(Publication) ]
    }

### Create publication {#StreamAPIsection5_2_2}
**POST ${host}/v1.1/stream-engine/publications**

Description:<br>
Create a publication with specified configuration.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(PublishRequest) |

**Note**: Definition of *PublishRequest*.<br>

    object(PublishRequest) {
        id: string(publicationId) | undefined, // will be generated if it's undefined.
        type: string(publishType), // E.g, "streaming", "video", ...
        participant: string(participantId), // Or use domain name as participant ID.
        media: object(MediaTrack) | object(MediaInfo),
        info: object(TypeSpecificInfo),
        connection: object(ConnectionInfo) | undefined, // For "streaming"
        processor: string(processorId) | undefined, // For "audio", "video"
    }
    object(MediaTrack) { // For WebRTC publications
        tracks: [ object(TrackInfo) ],
    }
    object(MediaInfo) { // For other publications
        audio: {
            format: object(AudioFormat),
        },
        video: {
            format: object(VideoFormat),
            parameters: object(VideoParameters)
        }
    }
    object(ConnectionInfo) { // For streaming publications
      url: string(streamingUrl),
      transportProtocol: "tcp" | "udp",
      bufferSize: number(bufferSize),
    },

For *object(TrackInfo)*, refers to [tracks in MediaOptions](../Client-Portal%20Protocol.md#331-participant-joins-a-room).
For *format* and *parameters*, refers to [REST API](RESTAPI.md#53-streams-StreamAPIsection53).

response body:

| type | content |
|:-------------|:-------|
|  json | object(IdObject) |

**Note**: Definition of *IdObject*.<br>

    Object(IdObject) {
        id: string(ID)
    }

### Delete publication {#StreamAPIsection5_2_3}
**DELETE ${host}/v1.1/stream-engine/publications/{streamId}**

Description:<br>
Delete the specified publication.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.3 Subscriptions {#StreamAPIsection5_3}
Description:<br>
Subscription represents subscribing to publications.<br>

Resources:

- /v1.1/stream-engine/subscriptions
- /v1.1/stream-engine/subscriptions/{subscribeId}

Data Model:

    object(Subscription):
    {
        id: string(subscribeId),
        type: string(subscribeType), // "webrtc", "streaming", ......
        sink: {
            audio: [ object(SinkTrack) ],
            video: [ object(SinkTrack) ],
            data: [ object(SinkTrack) ],
        },
        info: object(StreamInfo),
        locality: { agent: string(AgentName), node: string(NodeName) },
        participant: string(ownerParticipantId),
        domain: string(domainName)
    }
    object(SinkTrack):
    {
        from: string(SourceTrackId),
        format: object(VideoFormat) | object(AudioFormat),
        parameters: object(VideoParameters) | undefined
        // See format and parameters in [REST API](RESTAPI.md#53-streams-StreamAPIsection53)
    }

### List Subscriptions {#StreamAPIsection5_3_1}
**GET ${host}/v1.1/stream-engine/subscriptions**
**GET ${host}/v1.1/stream-engine/subscriptions/{subscribeId}**

Description:<br>
List subscriptions in stream engine.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | object(ListQuery) |

**Note**: Definition of *ListQuery*.<br>

    object(ListQuery):
    {
        query: {
            KEY: VALUE // Specified key-value pair for query result, such as `{name: "default"}`
        }
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(ListResult) |

**Note**: Definition of *ListResult*.<br>

    object(ListResult):
    {
        total: number(ListSize),
        start: number(offsetInList),
        data: [ object(Subscription) ]
    }

### Create Subscription {#StreamAPIsection5_3_2}
**POST ${host}/v1.1/stream-engine/subscriptions**

Description:<br>
Create a subscription with configuration.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(SubscribeRequest) |

**Note**: Definition of *SubscribeRequest*.<br>

    Object(SubscribeRequest) {
        id: string(subscriptionId) | undefined, // will be generated if it's undefined.
        type: string(subscribeType), // E.g, "streaming", "video", ...
        participant: string(participantId), // Or use domain name as participant ID.
        media: object(MediaTrack) | object(MediaInfo),
        info: object(TypeSpecificInfo),
        connection: object(ConnectionInfo) | undefined, // For "streaming", "recording"
        processor: string(processorId) | undefined, // For "audio", "video", "analytics"
    }
    object(MediaTrack) { // For WebRTC subscriptions
        tracks: [ object(TrackInfo) ],
    }
    object(MediaInfo) { // For other subscriptions
        audio: {
            from: string(sourceAudioId), // Could be publication ID or source track ID
            format: object(AudioFormat),
        } | boolean(enable),
        video: {
            from: string(sourceVideoId), // Could be publication ID or source track ID
            format: object(VideoFormat),
            parameters: object(VideoParameters)
        } | boolean(enable)
    }
    object(ConnectionInfo) {
        container: "mkv" | "mp4" | undefined, // For "recording"
        url: string(url) | undefined, // For "streaming"
        algorithm: string(algorithmName) | undefined, // For "analytics"
        video: { // For "analytics"
            format: object(VideoFormat), // Analytics output video format
            parameters: object(VideoParameters), // Analytics output video parameters
        } | undefined,
    }

For *object(TrackInfo)*, refers to [tracks in MediaOptions](../Client-Portal%20Protocol.md#331-participant-joins-a-room).
For *format* and *parameters*, refers to [REST API](RESTAPI.md#53-streams-StreamAPIsection53).

response body:

| type | content |
|:-------------|:-------|
|  json | object(IdObject) |

**Note**: Definition of *IdObject*.<br>

    Object(IdObject) {
        id: string(createdSubscriptionId)
    }

### Delete Subscription {#StreamAPIsection5_3_3}
**DELETE ${host}/v1.1/stream-engine/subscriptions/{subscribeId}**

Description:<br>
Stop the specified subscription.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.4 Processors {#StreamAPIsection5_4}
Description:<br>
Processor represents media processing nodes in OWT server, such as video mixer/transcoder.<br>

Resources:

- /v1.1/stream-engine/processors
- /v1.1/stream-engine/processors/{processorId}

Data Model:

    Object(Processor) {
        id: string(ProcessorID),
        type: string(ProcessorType), // "video", "audio", ......
        inputs: {
            audio: [ object(SinkTrack) ],
            video: [ object(SinkTrack) ],
            data: [ object(SinkTrack) ],
        },
        outputs: {
            audio: [ object(SourceTrack) ],
            video: [ object(SourceTrack) ],
            data: [ object(SourceTrack) ],
        },
        info: object(ProcessorInfo),
        locality: { agent: string(AgentName), node: string(NodeName) },
        participant: string(ownerParticipantId),
        domain: string(domainName)
    }

### List Processors {#StreamAPIsection5_4_1}
**GET ${host}/v1.1/stream-engine/processors**
**GET ${host}/v1.1/stream-engine/processors/{processorId}**

Description:<br>
List processors in stream engine.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | object(ListQuery) |

**Note**: Definition of *ListQuery*.<br>

    object(ListQuery):
    {
        query: {
            KEY: VALUE // Specified key-value pair for query result, such as `{name: "default"}`
        }
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(ListResult) |

**Note**: Definition of *ListResult*.<br>

    object(ListResult):
    {
        total: number(ListSize),
        start: number(offsetInList),
        data: [ object(Processor) ]
    }

### Create Processor {#StreamAPIsection5_4_2}
**POST ${host}/v1.1/stream-engine/processors**

Description:<br>
Create a processor with configuration.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(ProcessorRequest) |

**Note**: Definition of *ProcessorRequest*.<br>

    Object(ProcessorRequest) {
        id: string(processorId) | undefined, // will be generated if it's undefined.
        type: string(processorType), // "audio", "video", "analytics"
        participant: string(participantId), // Or use domain name as participant ID.
        mediaPreference: {
            audio: {encode:[string(codecName)], decode:[string(codecName)]},
            video: {encode:[string(codecName)], decode:[string(codecName)]}
        },
        ${TYPE_ATTRIBUTES}: ${ATTRIBUTES}, // Type specific attributes
    }

    List of ${TYPE_ATTRIBUTES}:
    // For "video" type processor
    mixing: { // Video mixing
        id: string(mixingId),
        view: string(viewName),
        maxInput: number(maxInputNumber),
        bgColor: {r, g, b}, // r, g, b are numbers from 0 ~ 255.
        parameters: object(VideoParameters),
        layout: {
            fitPolicy: string(policy), // "letterbox" or "crop"
            templates: [ { region: [ object(Region) ] } ],
        }
    },
    transcoding: { // Video transcoding
        id: string(transcoderId)
    }

    // For "audio" type processor
    mixing: { // Audio mixing
        id: string(mixingId),
        format: object(AudioFormat),
        vad: bool(enableVAD),
        view: string(viewName)
    },
    transcoding: { // Audio transcoding
        id: string(transcoderId)
    },
    selecting: { // Audio selecting
        id: string(selectId),
        activeStreamIds: [ string(UserDefinedId) ]
    }

    // For "analytics" type processor
    analytics: {
        id: string(analyticsId)
    }

    // For "sip" type processor
    sip: {
        server: string(serverHost),
        user: string(sipUser),
        password: string(sipPasswd)
    },
    stream: string(outgoingSipStream)

For *object(Region)*, refers to [REST API](RESTAPI.md#51-rooms-StreamAPIsection51).

response body:

| type | content |
|:-------------|:-------|
|  json | object(IdObject) |

**Note**: Definition of *IdObject*.<br>

    Object(IdObject) {
        id: string(createdProcessorId)
    }

### Delete Processor {#StreamAPIsection5_4_3}
**DELETE ${host}/v1.1/stream-engine/processors/{processorId}**

Description:<br>
Stop the specified processor, all related inputs and outputs will be stopped as well.<br>

request body:

  **Empty**

response body:

  **Empty**

## 5.5 Participants {#StreamAPIsection5_5}
Description:<br>
Participants represents owner of publications and subscriptions in OWT server.<br>

Resources:

- /v1.1/stream-engine/participants
- /v1.1/stream-engine/participants/{participantId}

Data Model:

    Object(Participant) {
        id: string(ParticipantID),
        domain: string(domainName), // For example, room ID
        portal: string(portalId),
        notifying: boolean(notifyOthers), // Notify other participants about join/leave.
    }

### List Participants {#StreamAPIsection5_5_1}
**GET ${host}/v1.1/stream-engine/participants**
**GET ${host}/v1.1/stream-engine/participants/{participantId}**

Description:<br>
List participants in stream engine.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | object(ListQuery) |

**Note**: Definition of *ListQuery*.<br>

    object(ListQuery):
    {
        query: {
            KEY: VALUE // Specified key-value pair for query result, such as `{name: "default"}`
        }
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(ListResult) |

**Note**: Definition of *ListResult*.<br>

    object(ListResult):
    {
        total: number(ListSize),
        start: number(offsetInList),
        data: [ object(Participant) ]
    }

### Create Participant {#StreamAPIsection5_5_2}
**POST ${host}/v1.1/stream-engine/participants**

Description:<br>
Create a participant with configuration.<br>

request body:

| type | content |
|:-------------|:-------|
|  json | object(ParticipantRequest) |

**Note**: Definition of *ParticipantRequest*.<br>

    Object(ParticipantRequest) {
        id: string(ParticipantID),
        domain: string(domainName),
        notifying: boolean(notifyOthers), // Notify other participants about join/leave.
    }

response body:

| type | content |
|:-------------|:-------|
|  json | object(IdObject) |

**Note**: Definition of *IdObject*.<br>

    Object(IdObject) {
        id: string(createdParticipantId)
    }

### Delete Participant {#StreamAPIsection5_5_3}
**DELETE ${host}/v1.1/stream-engine/participants/{participantId}**

Description:<br>
Drop the specified participant, all related publications and subscriptions will be stopped as well.<br>

request body:

  **Empty**

response body:

  **Empty**


## 5.6 Nodes {#StreamAPIsection5_6}
Description:<br>
Node represents working process in stream engine.<br>

Resources:

- /v1.1/stream-engine/nodes

Data Model:

    Object(WorkerNode) {
        id: string(ProcessorID),
        type: string(ProcessorType), // "video", "audio", ......
        agent: string(AgentLocalityName),
        pubs: [ string(PublicationId) ],
        subs: [ string(SubscriptionId) ],
        streamAddr: {ip: string(host), port: number(port)}
    }

### List Nodes {#StreamAPIsection5_6_1}
**GET ${host}/v1.1/stream-engine/nodes**

Description:<br>
List nodes in stream engine.<br>

request body:

| type | content |
|:-------------|:-------|
|      json     | object(ListQuery) |

**Note**: Definition of *ListQuery*.<br>

    object(ListQuery):
    {
        query: {
            KEY: VALUE // Specified key-value pair for query result, such as `{name: "default"}`
        }
    }

response body:

| type | content |
|:-------------|:-------|
|      json     | Object(ListResult) |

**Note**: Definition of *ListResult*.<br>

    object(ListResult):
    {
        total: number(ListSize),
        start: number(offsetInList),
        data: [ object(WorkerNode) ]
    }

# 6 Examples {#StreamAPIsection3}
This chapter introduces a simple sample which takes 1 WebRTC stream and 1 RTSP stream and generate a mixed stream for them.

## 6.1 Preparation {#StreamAPIsection6_1}
1. Follow the steps in [Chapter 2](#2-enable-stream-api-streamapisection2) and start OWT service.
2. Get an OWT service ID and key, which will be print after running `init-all.sh` or you can find one through database.
3. Create header for requests to stream API, get `AUTH` through `test/genAuth.js SERVICE_ID SERVICE_KEY`.
4. In this example, we use curl to send requests to stream API, make sure `curl` is installed.

## 6.2 Create a mixed stream {#StreamAPIsection6_2}
1. Use an OWT client to join service(both conference token and streamAPI token are OK), get client ID `CLIENT_0`, and publish a WebRTC stream through this client, get stream ID `RTC_0`.
2. Use stream API to publish an RTSP stream, get stream ID `RTSP_0` through response.

curl -k --request POST 'https://$HOSTNAME:3000/v1.1/stream-engine/publications' \
    --header 'Authorization: $AUTH' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "type": "streaming", 
        "connection": {
            "url": "RTSP_URL" // Can also use local file for test, /tmp/h264.mkv
        },
        "media": {
            "audio": false,
            "video": true
        },
        "participant": "$CLIENT_0"
    }'

3. Use stream API to create a video mixing processor, get processor ID `PROC_0` through response.

curl --location --request POST 'https://$HOSTNAME:3000/v1.1/stream-engine/processors' \
    --header 'Authorization: $AUTH' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "type": "video",
        "mixing": {
            "id": "test-mix",
            "maxInput": 16,
            "layout": {
                "fitPolicy": "letterbox",
                "templates":  [
                    {
                        "region": [
                        {
                            "id": "1",
                            "shape": "rectangle",
                            "area": {
                                "left": {"numerator" : 0, "denominator": 1 },
                                "top": {"numerator" : 0, "denominator": 1 },
                                "width": {"numerator" : 1, "denominator": 2 },
                                "height": {"numerator" : 1, "denominator": 2 }
                            }
                        },
                        {
                            "id": "2",
                            "shape": "rectangle",
                            "area": {
                                "left": {"numerator" : 1, "denominator": 2 },
                                "top": {"numerator" : 1, "denominator": 2 },
                                "width": {"numerator" : 1, "denominator": 2 },
                                "height": {"numerator" : 1, "denominator": 2 }
                            }
                        },
                        ]
                    }
                ]
            },
            "parameters": {
                "resolution": {"width": 640, "height": 480}
            },
            "bgColor": { "r": 155, "g": 155, "b": 155 }
        },
        "participant": "$CLIENT_0"
    }'

4. Subscribe 2 streams for video processor.

curl --location --request POST 'https://$HOSTNAME:3000/v1.1/stream-engine/subscriptions' \
    --header 'Authorization: $AUTH' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "type": "video",
        "media": {
            "video": {
                "from": "$RTC_0",
                "format": {"codec": "h264"}
            }
        },
        "info": {
            "owner": "test"
        },
        "processor": "$PROC_0",
        "participant": "$CLIENT_0"
    }'

curl --location --request POST 'https://$HOSTNAME:3000/v1.1/stream-engine/subscriptions' \
    --header 'Authorization: $AUTH' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "type": "video",
        "media": {
            "video": {
                "from": "$RTSP_0",
                "format": {"codec": "h264"}
            }
        },
        "info": {
            "owner": "test"
        },
        "processor": "$PROC_0",
        "participant": "$CLIENT_0"
    }'

5. Publish a mixed stream for video processor, get mixed ID `MIX_0`.

curl --location --request POST 'https://$HOSTNAME:3000/v1.1/stream-engine/publications' \
    --header 'Authorization: $AUTH' \
    --header 'Content-Type: application/json' \
    --data-raw '{
        "type": "video",
        "media": {
            "video": {
                "format": "vp8",
                "resolution": "unspecified",
                "framerate": "unspecified",
                "bitrate": "unspecified",
                "keyFrameInterval": "unspecified"
            }
        },
        "processor": "$PROC_0",
        "participant": "$CLIENT_0"
        "info": {
            "owner": "test"
        }
    }'

6. Client should be able to subscribe `MIX_0`.