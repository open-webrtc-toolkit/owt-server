# Client-Portal Communication Protocol (version 1.2)
<!--
|date | contributor | revision |
| :-------: | :------------: | :------------: |
|12-06-2017 |Xiande|draft|
|28-06-2017 |Xiande|1.0 reviewed|
|08-04-2018 |Tianfang|1.0 final|
|08-09-2019 |ChenLi|1.1 reviewed|
-->
# 1. Overview
This documentation covers all signaling messages between Client and MCU component through Socket.io connections.
# 2. Terminology
Here is a table describing the detailed name and definition.

| Name | Definition |
|:----|:-----------|
| Portal | The MCU component listening at the Socket.io server port, accepting signaling connections initiated by Clients, receive/send signaling messages from/to Clients. |
| Client | The program running on end-user’s device which interacts with MCU Server by messages defined in this documentation.|
| Connection | A transport channel between client and server. Different kinds of connections may have different transport protocols. |
| Signaling Connection | The signaling channel between client and Portal established by Client to send, receive and fetch signaling messages. |
| WebRTC Connection | A WebRTC transport channel between client and WebRTC agent. It may carry audio and video data. Detailed information about WebRTC could be found [here](https://webrtc.org/). |
| QUIC Connection | A QUIC transport channel between client and QUIC agent. Detailed information about QUIC could be found [here](https://quicwg.org/). |
| Room | The logical entity of a conference in which all conference members (participants) exchange their video/audio streams, and where mixed streams are generated. Every room must be assigned with a unique identification **(RoomID)** in the MCU scope. |
| User | The service account defined in the third-party integration application. The user ID and user role must be specified when asking for a token. |
| Participant | The logical entity of a user when participating in a conference with a valid token. Every participant must be assigned with a unique identification **(ParticipantID)** in the room scope, and must be assigned a set of operation permissions according to its role. |
| Token | The credential when a particular participant requests to join a particular room. It must contain a unique identification, the hostname of the target portal which can be used to connect, and the checksum to verify its validity. |
| Stream | The object represents an audio and/or video media source which can be consumed (say, be subscribed, recorded, pushed to a live stream server) within a room. A stream can be either a participant published one which is called a **Forward Stream**, or a room generated one which is called a **Mixed Stream**. A stream object must be assigned with a unique identification **(StreamID)** in the room scope, Participants can select a particular stream to subscribe according to StreamID. A stream must also contain the audio information of the codec, sample rate, channel number, and video information about codec for Forward Stream or about codec, resolution, frame rate, key frame interval, quality level for Mixed Stream. Participants can determine whether the stream fulfill their expectation based on such information. |
| Subscription | The activity a participant consuming a stream, such as receiving a stream via a WebRTC PeerConnection, recording a stream to a MCU storage, pushing a stream to a live stream server. A unique identification **(SubscriptionID)** in the room scope must be assigned to the subscription once its request is accepted by MCU. Participants will use the identification to update/cancel/stop a specific subscription. |
| Session | The entity in which a real-time audio/video communication takes place. Typically, participants establish WebRTC sessions between a WebRTC client and MCU (accurately, the webrtc-agent) to publish or subscribe streams. Since a stream ID will be assigned when publishing a stream into a room and a subscription ID will be assigned when subscribing a stream (or a pair of stream if audio and video come from different source), the stream ID and subscription ID in these two cases are re-used to identify the corresponding session, and MCU must guarantee that the stream IDs and subscription IDs will not conflict within a room. |
# 3. Socket.io Signaling
## 3.1 Format
### 3.1.1 Client -> Portal
Given that Client has connected Portal successfully and the socket object is ready to send/receive data to/from Portal, Client must send all signaling message to Portal by calling the client socket object’s ***emit*** method in the following format (javascript code for example, same for rest of this documentation):

  clientSocket.emit(
                RequestName,
                RequestData,
                function(ResponseStatus, ResponseData) {
                    //Handle the status and response here.
                }
               );

**Note**:

- **RequestName** must be with type of string, and with value defined in this specification;
- **RequestData** must be with type of javascript object (or equivalent in other programming language), and with format defined in this specification. And will be absent if no request data is required;
- **ResponseStatus** must be with value either “ok” or “error”;
- **ResponseData** must be with type of javascript object or undefined if **ResponseStatus** equals to “ok”, and with type of **ErrorDescription** object defined as following if **ResponseStatus** equals to “error”.

    object(ErrorDescription)::
    {
      code: number(ErrorCode),
      description: string(Reason)
    }
### 3.1.2 Portal -> Client
Given that Portal has accepted Clients connecting and the socket object is ready to send/receive data to/from Client, Portal must send all signaling messages to Client by calling the server socket object’s ***emit*** method in the following format:

  serverSocket.emit(
                NotificationName,
                NotificationData
               );
**Note**:

- **NotificationName** must be with type of string, and with value defined in this specification;
- **NotificationData** must be with type of object, and with format defined in this specification, and will be absent if no notification data is present.

## 3.2 Signaling Connection Maintenance
### 3.2.1 Client Connects
Portal should be able to listen at either a secure or an insecure socket.io server port to accept Clients’ connecting requests.  If the secure socket.io server is enabled, the SSL certificate and private key store path must be correctly specified by configuration item portal.keystorePath in portal.toml.<br>

Socket.io server needs pop a 'connection' event indicating a new client’s successfully connected.
### 3.2.2 Client Keeps Alive
Socket.io server has its own underlying connection keep-alive mechanism, so no application level heart-beat is needed. But Client needs to refresh the reconnection ticket periodically before the current one expires if it wants to reconnect to Portal in case the network break or switch happens.

**Note**:<br>
　　The refreshing request must keep to the following format:

  RequestName: "refreshReconnectionTicket"
  RequestData: absent
  ResponseData: A refreshed base64-encoded ReconnectionTicket object if ResponseStatus is "ok".
### 3.2.3 Client Disconnects
The connected socket.io object at server side will be notified with a ‘disconnect’ event. The waiting for reconnecting timer **(Timer100)** will be started right after receiving the ‘disconnect’ event if the following conditions are fulfilled:

  - The connection is initiated from a mobile client;
  - No participant leaving signaling has been received.
**Note**:<br>
If Client has not reconnected successfully before **Timer100** expires, Portal will not wait for and accept the Client’s reconnection any longer. Otherwise, if Client reconnects successfully before **Timer100** expires, Portal will resume all the signaling activities with Client and kill **Timer100**.
### 3.2.4 Client Reconnects
This a format for client reconnects.

**RequestName**: "relogin"<br>

**RequestData**: The ReconnectionTicket object defined in 3.3.1 section.<br>

**ResponseData**: A refreshed base64-encoded ReconnectionTicket object if ResponseStatus is "ok".
## 3.3 Conferencing
### 3.3.1 Participant Joins a Room
**RequestName**: “login”<br>

**RequestData**: The LoginInfo object with following definition:

  object(LoginInfo)::
    {
     token: string(Base64EncodedToken),
     userAgent: object(ClientInfo),
     reconnection: object(ReconnectionOptions)/*If reconnection is required*/
                   | false/*If reconnection is not required*/
                   | undefined/*For compatibility (with 3.4 clients) purpose, will be considered as false or {keepTime: -1}*/,
     protocol: string(ProtocolVersion)/*e.g.  “1.1”*/
               | undefined/*For compatibility (with 3.4 clients) purpose, will be considered as "legacy"*/
    }

    object(ClientInfo)::
      {
       sdk: {
         type: string(SDKName),
         version: string(SDKVersion)
        },
       runtime: {
         name: string(RuntimeName),
         version: string(RuntimeVersion)
        },
       os: {
         name: string(OSName),
         version: string(OSVersion)
        }
      }

    object(ReconnectionOptions)::
      {
       keepTime: number(Seconds)/*-1: Use server side configured 'reconnection_timeout' value; Others: specified keepTime in seconds*/
      }
**ResponseData**: The LoginResult object with following definition if **ResponseStatus** is "ok":

    object(LoginResult)::
      {
       id: string(ParticipantId),
       user: string (UserId),
       role: string(ParticipantRole),
       permission: object(Permission),
       room: object(RoomInfo),
       reconnectionTicket: undefined/*when reconnection is not promised*/
                      | string(Base64Encoded(object(ReconnectionTicket)))/*when reconnection is promised*/
      }

      object(Permission)::
        {
         publish: {
             audio: true | false,
             video: true | false,
             data:  true | false
         },
         subscribe: {
             audio: true | false,
             video: true | false,
             data:  true | false
         }
        }

      object(RoomInfo)::
        {
         id: string(RoomId),
         views: [string(ViewLabel)],
         streams: [object(StreamInfo)],
         participants: [object(ParticipantInfo)]
        }

        object(StreamInfo)::
          {
           id: string(StreamID),
           type: "forward" | "augmented" |"mixed",
           media: object(MediaInfo) | null,
           data: true | false,
           info: object(PublicationInfo)/*If type equals "forward"*/
                | object(AugmentInfo)/*If type equals "augmented"*/
                | object(ViewInfo)/*If type equals "mixed"*/
          }

          object(MediaInfo)::
            {
             audio: object(AudioInfo) | undefined,
             video: object(VideoInfo) | undefined
            }

            object(AudioInfo)::
              {
               status: "active" | "inactive" | undefined,
               source: "mic" | "screen-cast" | "raw-file" | "encoded-file" | undefined,
               format: object(AudioFormat),
               optional:
                {
                 format: [object(AudioFormat)] | undefined
                }
                | undefined
              }

              object(AudioFormat)::
                {
                 codec: "pcmu" | "pcma" | "opus" | "g722" | "iSAC" | "iLBC" | "aac" | "ac3" | "nellymoser",
                 sampleRate: number(SampleRate) | undefined,
                 channelNum: number(ChannelNumber) | undefined
                }

            object(VideoInfo)::
              {
               status: "active" | "inactive" | undefined,
               source: "camera" | "screen-cast" | "raw-file" | "encoded-file" | undefined,
               original: [{
                 format: object(VideoFormat),
                 parameters: object(VideoParameters) | undefined,
                 simulcastRid: string(SimulcastRid) | undefined
               }],
               optional:
                 {
                  format: [object(VideoFormat)] | undefined,
                  parameters:
                    {
                     resolution: [object(Resolution)] | undefined,
                     framerate: [number(FramerateFPS)] | undefined,
                     bitrate: [number(BitrateKbps)] | [string(BitrateMultiple)] | undefined,
                     keyFrameInterval: [number(KeyFrameIntervalSecond)] | undefined
                    }
                    | undefined
                 }
                 | undefined
              }

              object(VideoFormat)::
                {
                 codec: "h264" | "h265" | "vp8" | "vp9",
                 profile: "B" | "CB" | "M" | | "E" | "H" //If codec equals "h264".
                       | undefined //If codec does NOT equal "h264"
                }

              object(VideoParameters)::
                {
                 resolution: object(Resolution) | undefined,
                 framerate: number(FramerateFPS) | undefined,
                 bitrate: number(BitrateKbps) | undefined,
                 keyFrameInterval: number(KeyFrameIntervalSecond) | undefined
                }

                object(Resolution)::
                  {
                   width: number(WidthPX),
                   height: number(HeightPX)
                  }

          object(PublicationInfo):
            {
             owner: string(ParticipantId),
             type: "webrtc" | "streaming" | "sip" | "quic",
             inViews: [String(ViewLabel)],
             attributes: object(ClientDefinedAttributes)
            }

          object(AugmentInfo):
          {
            origin: String(OriginStreamId),
            description: String(EffectDescription)
          }

          object(ViewInfo):
            {
             label: string(ViewLable),
             activeInput: string(StreamId) | "unknown",
             layout: [object(MixedVideoLayout)]
            }

            object(MixedVideoLayout)::
              {
               stream: string(StreamId) | undefined/*If this region is empty*/,
               region: object(Region)
              }

              object(Region)::
                {
                 id: string(RegionId),
                 shape: "rectangle" | "circle",
                 area: object(Rectangle)/*If shape equals "rectangle"*/
                       | object(Circle)/*If shape equals "circle"*/
                }

                object(Rectangle)::
                  {
                   left: string(LeftRatio),
                   top: string(TopRatio),
                   width: string(WidthRatio),
                   height: string(HeightRatio)
                  }

                object(Circle)::
                  {
                   centerW: string(CenterWRatio),
                   centerH: string(CenterHRatio),
                   radius: string(RadiusWRatio)
                  }

        object(ParticipantInfo)::
          {
           id: string(ParticilantId),
           role: string(ParticipantRole),
           user: string (UserId)
          }

      object(ReconnectionTicket)::
        {
         ParticipantId: string(Participantid),
         ticketId: string(RandomNumber),
         notBefore: number(ValidTimeBegin),
         notAfter: number(ValidTimeEnd),
         signature: string(Signature)
        }
### 3.3.2 Participant Leaves a Room
**RequestName**: “logout”<br>

**RequestData**: absent<br>

**ResponseData**: undefined if **ResponseStatus** is “ok”.
### 3.3.3 Participant Is Dropped by MCU
**NotificationName**: “drop”<br>

**NotificationData**: absent<br>
### 3.3.4 Participant Is Notified on Other Participant’s Action
**NotificationName**: “participant”<br>

**NotificationData**: The ParticipantAction object with following definition:

  object(ParticipantAction)::
    {
     action: "join" | "leave",
     data: object(ParticipantInfo)/*If action equals “join”*/
          | string(ParticipantId)/*If status equals "leave"*/
    }

### 3.3.5 Participant Sends a Text Message
**RequestName**: “text”<br>

**RequestData**: The TextSendMessage object with following definition:

  object(TextSendMessage)::
    {
     to: string(ParticipantId) | "all",
     message: string(Message) /*The message length must not greater than 2048*/
    }
**ResponseData**: undefined if **ResponseStatus** is “ok”.
### 3.3.6 Participant Receives Text Message from another Participant
**NotificationName**: “text”<br>

**NotificationData**: The TextReceiveMessage object with following definition:

  object(TextReceiveMessage)::
    {
     from: string(ParticipantId),
     to: 'me' | 'all',
     message: string(Message)
    }

### 3.3.7 Participant Starts Publishing a Stream to Room
**RequestName**: “publish”<br>

**RequestData**: The PublicationRequest object with following definition:

```
  object(PublicationRequest)::
    {
     media: object(WebRTCMediaOptions) | null,
     data: true | false,
     transport: object(TransportOptions),
     attributes: object(ClientDefinedAttributes) | null
    }
```

A publication can send either media or data, but a QUIC *transport* channel can support multiple stream for both media and data. Setting `media:null` and `data:false` is meaningless, so it should be rejected by server. Protocol itself doesn't forbit to create WebRTC connection for data. However, SCTP data channel is not implemented at server side, so currently `data:true` is only support by QUIC transport channels. 

```
  object(WebRTCMediaOptions)::
    {
      audio: {
            source: "mic" | "screen-cast" | "raw-file" | "encoded-file"
            }
            | false,
      video: {
            source: "camera"| "screen-cast"  | "raw-file" | "encoded-file",
            parameters:
              {
                resolution: object(Resolution),
                framerate: number(FramerateFPS)
              }
            }
            | false
    }
```

**ResponseData**: The PublicationResult object with following definition if **ResponseStatus** is “ok”:

  object(PublicationResult)::
    {
      transportId: string(transportId),  // Can be reused in the following publication or subscription.
     publicationId: string(SessionId) //will be used as the stream id when it gets ready.
    }
### 3.3.8 Participant Stops Publishing a Stream to Room
**RequestName**: “unpublish”<br>

**RequestData**: The UnpublicationRequest object with following definition:

  object(UnpublicationRequest)::
    {
     id: string(SessionId) //same as the stream id.
    }
**ResponseData**: undefined if **ResponseStatus** is “ok”.
### 3.3.9 Participant Is Notified on Streams Update in Room
**NotificationName**: “stream”<br>

**NotificationData**: The StreamUpdateMessage object with following definition.

  object(StreamUpdateMessage)::
    {
     id: string(StreamId),
     status: "add" | "update" | "remove",
     data: object(StreamInfo)/*If  status equals "add"*/
          | object(StreamUpdate)/*If status equals "update"*/
          | undefined /*If status equals "remove"*/
    }

    object(StreamUpdate)::
      {
       field: "audio.status" | "video.status"  | "video.layout" | "activeInput" | ".",
       value: "active" | "inactive" | [object(MixedVideoLayout)] | string(inputStreamId) | object(StreamInfo)
      }
### 3.3.10 Participant Controls a Stream
**RequestName**: “stream-control”<br>

**RequestData**: The StreamControlInfo object with following definition:

  object(StreamControlInfo)::
  {
      id: string(StreamId),    // Must refer to a forward stream
      operation: "pause" | "play",
      data: ("audio" | "video" | "av")  /*If operation equals "pause" or "play"*/
  }
**ResponseData**: **ResponseStatus** is “ok”.
### 3.3.11 Participant Subscribes a Stream to WebRTC Endpoint
**RequestName**: “subscribe”<br>

**RequestData**: The SubscriptionRequest object with following definition:

  object(SubscriptionRequest)::
    {
     media: object(MediaSubOptions),
     transport: object(TransportOptions),
    }

    object(MediaSubOptions)::
      {
       audio: object(AudioSubOptions) | false,
       video: object(VideoSubOptions) | false
      }

      object(AudioSubOptions)::
        {
         from: string(StreamId)
        }

      object(VideoSubOptions)::
        {
         from: string(StreamId),
         parameters: object(VideoParametersSpecification)/*If specific video parameters are wanted*/
                     | undefined/*If default video parameters are wanted*/,
         simulcastRid: string(rid) /* if simulcastRid is used, parameters will be ignored */
        }

        object(VideoParametersSpecification)::
          {
           resolution: object(Resolution) | undefined,
           framerate: number(WantedFrameRateFPS) | undefined,
           bitrate: number(WantedBitrateKbps) | string(WantedBitrateMultiple) | undefined,
           keyFrameInterval: number(WantedKeyFrameIntervalSecond) | undefined
          }
**ResponseData**: The SubscriptionResult object with following definition if **ResponseStatus** is “ok”:

  object(SubscriptionResult)::
    {
      transportId: string(transportId),  // Can be reused in the following publication or subscription.
     subscriptionId: string(SubscriptionId)
    }
### 3.3.12 Participant Stops a Self-Initiated Subscription
**RequestName**: “unsubscribe”<br>

**RequestData**: The UnsubscriptionRequest object with following definition:

  object(UnsubscriptionRequest)::
    {
     id: string(SubscriptionId)
    }
**ResponseData**: undefined if **ResponseStatus** is “ok”.
### 3.3.13 Participant Controls a Self-Initiated Subscription
**RequestName**: “subscription-control”<br>

**RequestData**: The SubscriptionControlInfo object with following definition:

  object(SubscriptionControlInfo)::
    {
     id: string(SubscriptionId),
     operation: "update" | "pause" | "play" | "querybwe",
     data: object(SubscriptionUpdate)/*If operation equals "update"*/
          | ("audio" | "video" | "av")/*If operation equals "pause" or "play"*/
          | undefined /*If operation equals "querybwe"*/
    }

    object(SubscriptionUpdate)::
      {
       audio: object(AudioUpdate) | undefined,
       video: object(VideoUpdate) | undefined
      }

      object(AudioUpdate)::
        {
         from: string(StreamId)
        }

      object(VideoUpdate)::
        {
         from: string(StreamId) | undefined,
         parameters: object(VideoParametersSpecification)/*If any video parameters are wanted to be re-specified*/
                     | undefined/*If the video parameters of the ongoing subscription are wanted to be kept*/
        }
**ResponseData**: undefined or object(BWEResult) for "querybwe" if **ResponseStatus** is “ok”.
  object(BWEResult)::
    {
      estimatedBitrate: number(bps) /*Send side estimated bitrate if enabled*/
    }
### 3.3.14 Participant Sends Session Signaling
**RequestName**: “soac”<br>

**RequestData**: The SOACMessage object with following definition:

  object(SOACMessage)::
    {
     id: string(transportId), /* Transport ID returned in publishing or subscribing*/
     signaling: object(OfferAnswer) | object(CandidateMessage) | object(RemovedCandidatesMessage)
    }

    object(OfferAnswer)::
      {
       type: "offer" | "answer",
       sdp: string(SDP) | null,  // WebRTC connection
      }

    object(CandidateMessage)::
      {
       type: "candidate",
       candidate: object(Candidate)
      }

    object(RemovedCandidatesMessage)::
      {
       type: "removed-candidates",
       candidates: [ (object(Candidate) ]
      }

    object(Candidate)::
      {
       sdpMid: string(mid),                 // optional in RemovedCandidatesMessage
       sdpMLineIndex: number(mLineIndex),   // optional in RemovedCandidatesMessage
       candidate: string(candidateSdp)
      }

    object(TransportOptions)::
      {
        type: "webrtc" | "quic",
        id: string(transportId) | null,  // null will result to create a new transport channel. Always be null if transport type is webrtc because webrtc agent doesn't support multiple transceivers on a single PeerConnection at this time.
      }

**ResponseData**: undefined if **ResponseStatus** is “ok”.
### 3.3.15 Participant Receives Session Progress
**NotificationName**: “progress”<br>

**NotificationData**: The SessionProgress or TransportProgress object with following definition.

  object(TransportProgress)::
    {
      id: string(transportId),
      status: "soac" | "ready" | "error",
      data: object(OfferAnswer) | object(CandidateMessage)  /*If status equals “soac”*/
          | (undefined/*If status equals “ready” and session is NOT for recording*/
          | string(Reason)/*If status equals “error”*/
    }

  object(SessionProgress)::
    {
     id: string(SessionId), /* StreamId returned in publishing or SubscriptionId returned in subscribing*/
     status: "ready" | "error"
    }

# 4. Examples

This section provides a few examples of signaling messages for specific purposes.

## 4.1 Forward data through data channel over QUIC

An endpoint is joined the meeting, and it wants to publish a data stream over a QUIC transport channel.

Step 1: Client sends a "publish" request.

```
{
  media: null,
  data: true,
  transport: {type: 'quic'}
}
```

Step 2: Receive a response from server.

```
{
  transportId: undefined,
  publicationId: '91e247051d494cddbd4ad461445637c4'
}
```

Step 3: Create a new WebTransport or get an existing WebTransport, then create a new BidirectionalStream or SendStream. Write data to stream. The URL of WebTransport should be included in token. WebTransport is shared by all media streams, data streams and signaling which belong to the same client.