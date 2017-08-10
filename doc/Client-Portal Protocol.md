# <div align = "center">Client-Portal Protocol Specification</div>

**<div align = "center">Version: 1.0 draft</div>**

**<div align = "center">June 2017</div>**
____

|date | contributor | revision |
| :-------: | :------------: | :------------: |
|12-06-2017 |Xiande|draft|
|28-06-2017 |Xiande|1.0.0 reviewed|

## 1. Overview
This documentation covers all signaling messages between Client and Portal component, including signaling messages transported through Socket.io connections and RESTful interfaces. All the signaling interaction at the Client side should be encapsulated in the Client SDK, and Client SDK should provide an API suite for client side application integration.
## 2. Definitions
**Portal:** The MCU component listening at the HTTP server or Socket.io server port, accepting Signaling connections initiated by Clients, receive/send signaling messages from/to Clients.<br>
**Client:** The program running on end-user’s device being built on Client SDK which interacts with MCU Server by messages defined in this documentation. Client can be browsers including Chrome, Firefox and Edge, Android, iOS and Windows Apps.<br>
**Signaling Connection:** The signaling channel between Client and Portal established by Client to send, receive and fetch signaling messages.<br>
**Room:** The logical entity of a conference in which all conference members (participants) exchange their video/audio streams, and mixed streams are generated. Every room must be assigned with a unique identification **(RoomID)** in the MCU scope.<br>
**User:** The service account defined in the third-party integration application. The user ID and user role must be specified when asking for a token.<br>
**Participant:** The logical entity of a user when participating in a conference with a valid token. Every participant must be assigned with a unique identification **(ParticipantID)** in the room scope, and must be assigned a set of operation permissions according to its role.<br>
**Token:** The only credential when a particular participant request to join a particular room. It must contain a unique identification, the hostname of the target portal it can be used to connect, and the checksum to verify its validity.<br>
**Stream:** The object represents an audio and/or video media source which can be consumed (say, be subscribed, recorded, pushed to a live stream server) within a room. A stream can be either a participant published one which is called a **Forward Stream**, or a room generated one which is called a **Mixed Stream**. A stream object must be assigned with a unique identification **(StreamID)** in the room scope, Participants can select a particular stream to subscribe according to StreamID. A stream must also contain the audio information of the codec/sample rate/channel number, and video information of codec (for Forward Stream) and codec/resolution/frame rate/key frame interval/quality level (for Mixed Stream). Participants can determine whether the stream fulfill their expectation based on such information.<br>
**Subscription:** The activity a participant consuming a stream, such as subscribing a stream via a WebRTC PeerConnection, recording a stream to a MCU storage, pushing a stream to a live stream server. A unique identification **(SubscriptionID)** in the room scope must be assigned to the subscription once its request is accepted by MCU. Participants will use the identification to update/cancel/stop a specific subscription.<br>
**Session:** The entity in which a real-time audio/video communication takes place. Typically, participants establish WebRTC sessions between a WebRTC client and MCU (accurately, the webrtc-agent) to publish or subscribe streams. Since a stream ID will be assigned when publishing a stream into a room and a subscription ID will be assigned when subscribing a stream (or a pair of stream if audio and video come from different source), the stream ID and subscription ID in these two cases are re-used to identify the corresponding session, and MCU must guarantee that the stream IDs and subscription IDs will not conflict within a room.<br>
## 3. Socket.io Signaling
### 3.1 Format
#### 3.1.1 Client -> Portal
Given that Client has connected Portal successfully and the socket object is ready to send/receive data to/from Portal, Client must send all signaling message to Portal by calling the client socket object’s ***emit*** method in the following format (javascript code for example, same for rest of this documentation):
```
clientSocket.emit("signaling",
              RequestName,
              RequestData,
              function(ResponseStatus, ResponseData) {
                  //Handle the status and response here.
              }
             );
```
Where
1) **RequestName** must be with type of string, and with value defined in this specification;
2) **RequestData** must be with type of javascript object(or equivalent in other programming language), and with format defined in this specification;
3) **ResponseStatus** must be with value either “ok” or “error”;
4) **ResponseData** must be with type of javascript object or undefined if **ResponseStatus** equals to “ok”, and with type of ErrorDescription object defined as following if **ResponseStatus** equals to “error”.
```
object(ErrorDesciption)::
  {
    code: number(ErrorCode),
    description: string(Reason)
  }
```
#### 3.1.2 Portal -> Client
Given that Portal has accepted Clients connecting and the socket object is ready to send/receive data to/from Client, Portal must send all signaling messages to Client by calling the server socket object’s ***emit*** method in the following format:
```
serverSocket.emit("signaling",
              NotificationName,
              NotificationData
             );
```
Where
1) **NotificationName** must be with type of string, and with value defined in this specification;
2) **NotificationData** must be with type of object, and with format defined in this specification.
### 3.2 Connection Maintenance
#### 3.2.1 Client Connects
Portal should be able to listen at both a secure and an insecure socket.io server port to accept Clients’ connecting requests.  If the secure socket.io server is enabled, the SSL certificate and private key store path must be correctly specified by configuration item portal.keystorePath in portal.toml.
Socket.io server will pop a ‘connection’ event indicating a new client’s successfully connecting.
#### 3.2.2 Client Keeps Alive
Socket.io server has its own underlying connection keep-alive mechanism, so no application level heart-beat is needed. But Client needs to refresh the reconnection ticket periodically before the current one expires if it wants to reconnect to Portal in case the network break or switch happens. The refreshing request must keep to the following format:
**RequestName**:  “refresh”
**RequestData**: The RefreshTicketInfo object with following definition:
```
object(RefreshTicketInfo)::
  {
   id: string(ParticipantId) /*The participant id returned at successful login, see 3.3.1*/
  }
```
**ResponseData**: A refreshed ReconnectionTicket object if **ResponseStatus** is “ok”.
#### 3.2.3 Client Disconnects
The connected socket.io object at server side will be notified with a ‘disconnect’ event. The waiting for reconnecting timer **(Timer100)** will be started at receiving the ‘disconnect’ event if the following conditions are fulfilled:
1) The connection is initiated from a mobile client;
2) No participant leaving signaling has been received.
    If Client has not reconnected successfully before **Timer100** expires, Portal will not wait for and accept the Client’s reconnecting any longer. Otherwise, if Client reconnects successfully before **Timer100** expires, Portal will resume all the signaling activities with Client and kill **Timer100**.
#### 3.2.4 Client Reconnects
**RequestName**:  “relogin”
**RequestData**: The ReconnectionInfo object with following definition:
```
object(ReconnectionInfo)::
  {
   id: string(ParticipantId), /*The participant id returned at successful login, see 3.3.1*/
   ticket: object(ReconnectionTicket) /*The reconnectionTicket returned at successful login, see 3.3.1*/
  }
```
**ResponseData**: A refreshed ReconnectionTicket object if **ResponseStatus** is “ok”.
### 3.3 Conferencing
#### 3.3.1 Participant Joins a Room
**RequestName**:  “login”
**RequestData**: The LoginInfo object with following definition:
```
object(LoginInfo)::
  {
   token: string(Base64EncodedToken),
   userAgent: object(ClientInfo),
   protocol: string(ProtocolVersion) //e.g.  “1.0.0”
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
```
**ResponseData**: The LoginResult object with following definition if **ResponseStatus** is “ok”:
```
object(LoginResult)::
  {
   id: string(ParticipantId),
   user: string (UserId),
   role: string(ParticipantRole),
   permission: object(Permission),
   room: object(RoomInfo),
   reconnectionTicket: undefined/*when reconnection is not promised*/
                  | object(ReconnectionTicket)/*when reconnection is promised*/
  }

  object(Permission)::
    {
     publish: {
       type: ["webrtc" | "streaming"],
       media: {
         audio: true | false,
         video: true | false
       }
     } | false,
     subscribe: {
       type: ["webrtc" | "recording" | "streaming"],
       media: {
         audio: true | false,
         video: true | false
       }
     } | false,
     text: "to-all" | "to-peer" | false,
     manage: true | false
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
       type: "forward" | "mixed",
       media: object(MediaInfo),
       info: object(PublicationInfo)/*If type equals "forward"*/
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
           format: object(VideoFormat),
           parameters: object(VideoParameters) | undefined,
           optional:
             {
              format: [object(VideoFormat)] | undefined,
              parameters:
                {
                 resolution:[object(Resolution)] | undefined,
                 framerate: [number(FramerateFPS)] | undefined,
                 bitrate: [number(BitrateKbps)] | [string(BitrateMultiple)] |undefined,
                 keyFrameInterval: [number(KeyFrameIntervalSecond)] | undefined
                }
                | undefined
             }
             | undefined
          }

          object(VideoFormat)::
            {
             codec: "h264" | "h265" | "vp8" | "vp9",
             profile: "baseline" | "constrained-baseline" | "main" | "high" //If codec equals "h264".
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
         type: "webrtc" | "streaming" | "sip",
         attributes: object(ClientDefinedAttributes)
        }

      object(ViewInfo):
        {
         label: string(ViewLable),
         layout: [object(SubStream2RegionMapping)]
        }

        object(SubStream2RegionMapping)::
          {
           stream: string(StreamId),
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
               left: number(LeftRatio),
               top: number(TopRatio),
               width: number(WidthRatio),
               height: number(HeightRatio)
              }

            object(Circle)::
              {
               centerW: number(CenterWRatio),
               centerH: number(CenterHRatio),
               radius: number(RadiusWRatio)
              }

    object(ParticipantInfo)::
      {
       id: string(ParticilantId),
       role: string(ParticipantRole),
       user: string (UserId)
      }

  object(ReconnectionTicket)::
    {
     credential: string(Credential),
     notBefore: number(ValidTimeBegin),
     notAfter: number(ValidTimeEnd)
    }
```
#### 3.3.2 Participant Leaves a Room
**RequestName**:  “logout”
**RequestData**: The LogoutInfo object with following definition:
```
object(LogoutInfo)::
  {
   id: string(ParticipantId)
  }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.3 Participant Is Notified on Other Participant’s Action
**NotificationName**: “participant”
**NotificationData**: The ParticipantAction object with following definition:
```
object(ParticipantAction)::
  {
   action: "join" | "leave",
   data: object(ParticipantInfo)/*If action equals “join”*/
        | string(ParticipantId)/*If status equals "leave"*/
  }
```
#### 3.3.4 Participant Sends a Text Message
**RequestName**:  “text”
**RequestData**: The TextSendMessage object with following definition:
```
object(TextSendMessage)::
  {
   to: string(ParticipantId) | "all",
   message: string(Message) /*The message length must not greater than 2048*/
  }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.5 Participant Receives Text Message from another Participant
**NotificationName**: “text”
**NotificationData**: The TextReceiveMessage object with following definition:
```
object(TextReceiveMessage)::
  {
   from: string(ParticipantId),
   message: string(Message)
  }
```
#### 3.3.6 Participant Starts Publishing a Stream to Room
**RequestName**:  “publish”
**RequestData**: The PublicationInfo object with following definition:
```
object(PublicationInfo)::
  {
   type: "webrtc" | "streaming",
   connection: undefined/*If type equals "webrtc"*/
             | object(StreamingInConnectionOptions)/*If type equals "streaming"*/,
   media: object(WebRTCMediaOptions)/*If type equals "webrtc"*/
         | object(StreamingInMediaOptions)/*If type equals "streaming"*/,
   attributes: object(ClientDefinedAttributes)
  }

  object(WebRTCMediaOptions)::
    {
     audio: {
            source: "mic" | "screen-cast" | "raw-file" | "encoded-file"
           }
           | false,
     video: {
            source: "camera"| "screen-cast"  | "raw-file" | "encoded-file"
           }
           | false
    }

  object(StreamingInConnectionOptions):://Should categorize protocols.
    {
     url: string(URLOfSourceStreaming),
     transportProtocol: "tcp" | "udp", //optional, default: "tcp"
     bufferSize: number(BufferSizeBytes)      //optional, default: 8192 bytes
    }

  object(StreamingInMediaOptions)::
    {
     audio: "auto" | true | false,
     video: "auto" | true | false,
    }
```
**ResponseData**: The PublicationResult object with following definition if **ResponseStatus** is “ok”:
```
object(PublicationResult)::
  {
   id: string(SessionId) //will be used as the stream id when it gets ready.
  }
```
#### 3.3.7 Participant Stops Publishing a Stream to Room
**RequestName**:  “unpublish”
**RequestData**: The UnpublicationInfo object with following definition:
```
object(UnpublicationInfo)::
  {
   id: string(SessionId) //same as the stream id.
  }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.8 Participant Is Notified on Streams Update in Room
**NotificationName**: “stream”
**NotificationData**: The StreamUpdateMessage object with following definition.
```
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
     field: "audio.status" | "video.status"  | "video.layout",
     value: "active" | "inactive" | [object(SubStream2RegionMapping)]
    }
```
#### 3.3.9 Participant Controls a Stream
**RequestName**:  “stream-control”
**RequestData**: The StreamControlInfo object with following definition:
```
object(StreamControlInfo)::
  {
   id: string(StreamId),     //Must refer to a forward stream.
   operation: "mix" | "unmix" | "set-region" | "get-region" | "pause" | "play",
   data: string(ViewLabel)/*If operation equals "mix", "unmix" or "get-region*/
        | object(RegionSetting)/*If operation equals "set-region"*/
        | ("audio" | "video" | "av")/*If operation equals "pause" or "play"*/
  }

  object(RegionSetting)::
    {
     view: string(ViewLabel),
     region: string(RegionId)
    }
```
**ResponseData**: undefined in case operation is not “get-region”, and object RegionInfo defined below in case operation is “get-region” if **ResponseStatus** is “ok”.
```
object(RegionInfo)::
  {
   region: string(RegionId)
  }
```
#### 3.3.10 Participant Starts a Subscription
**RequestName**:  “subscribe”
**RequestData**: The SubscriptionInfo object with following definition:
```
object(SubscriptionInfo)::
  {
   type: "webrtc" | "streaming" | "recording",
   connection: undefined/*If type equals "webrtc"*/
             | object(StreamingOutConnectionOptions)/*If type equals "streaming"*/
             | object(RecordingStorageOptions)/*If type equals "recording"*/,
   media: object(MediaSubOptions)
  }

  object(StreamingOutConnectionOptions)::
    {
     url: string(TargetStreamingURL)
    }

  object(RecordingStorageOptions)::
    {
     container: "mkv" | "mp4" | "ts"/*Must be compatible with media.audo.codec and media.video.codec*/ //optional, default: determined by media.video.codec.
    }

  object(MediaSubOptions)::
    {
     audio: object(AudioSubOptions) | false,
     video: object(VideoSubOptions) | false
    }

    object(AudioSubOptions)::
      {
       from: string(StreamId),
       spec: object(AudioSubSpecification)/*If type does NOT equal "webrtc", and audio transcoding is needed or non-default mixed audio is wanted*/
            | undefined/*If type equals "webrtc", or original forward audio or default mixed audio is wanted*/
      }

      object(AudioSubSpecification)::
        {
         codec: string(WantedAudioCodec),
         sampleRate: number(WantedSampleRate) | undefined,
         channelNum: number(WantedChannelNum) | undefined
        }

    object(VideoSubOptions)::
      {
       from: string(StreamId),
       spec: object(VideoSubSpecification)/*If video transcoding is needed or non-default mixed video is wanted*/
            | undefined/*If original forward video or default mixed video is wanted*/
      }

      object(VideoSubSpecification)::
        {
         codec: string(WantedVideoCodec),//Will be ignored if type equals "webrtc"
         profile: string(WantedVideoProfile) /*Will be ignored if type equals "webrtc" or codec does NOT equal "h264"*/ | undefined,
                        resolution: object(Resolution) | undefined,
         framerate: number(WantedFrameRateFPS) | undefined,
         bitrate: number(WantedBitrateKbps) | string(WantedBitrateMultiple) | undefined,
         keyFrameInterval: number(WantedKeyFrameIntervalSecond) | undefined
        }
```
**ResponseData**: The SubscriptionResult object with following definition if **ResponseStatus** is “ok”:
```
object(SubscriptionResult)::
  {
   id: string(SubscriptionId)
  }
```
#### 3.3.11 Participant Stops a Self-Initiated Subscription
**RequestName**:  “unsubscribe”
**RequestData**: The UnsubscriptionInfo object with following definition:
```
object(UnsubscriptionInfo)::
  {
   id: string(SubscriptionId)
  }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.12 Participant Controls a Self-Initiated Subscription
**RequestName**:  “subscription-control”
**RequestData**: The SubscriptionControlInfo object with following definition:
```
object(SubscriptionControlInfo)::
  {
   id: string(SubscriptionId),
   operation: "update" | "pause" | "play",
   data: object(SubscriptionUpdate)/*If operation equals "update"*/
        | ("audio" | "video" | "av")/*If operation equals "pause" or "play"*/
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
       spec: object(VideoUpdateSpecification)/*If video transcoding is ongoing or mixed stream is being subscribed*/
            | undefined/*If original forward video or default mixed video is wanted*/
      }

      object(VideoUpdateSpecification)::
        {
         resolution: object(Resolution) | undefined,
         framerate: number(WantedFrameRateFPS) | undefined,
         bitrate: number(WantedBitrateKbps) | string(WantedBitrateMultiple) | undefined,
         keyFrameInterval: number(WantedKeyFrameIntervalSecond) | undefined
        }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.13 Participant Sends Session Signaling
**RequestName**:  “soac”
**RequestData**: The SOACMessage object with following definition:
```
object(SOACMessage)::
  {
   id: string(SessionId), /* StreamId returned in publishing or SubscriptionId returned in subscribing*/
   signaling: object(OfferAnswer) | object(Candidate)
  }

  object(OfferAnser)::
    {
     type: "offer" | "answer",
     sdp: string(SDP)
    }

  object(Candidate)::
    {
     type: "candidate",
     candidate: string(Candidate)
    }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.
#### 3.3.14 Participant Receives Session Progress
**NotificationName**: “progress”
**NotificationData**: The SessionProgress object with following definition.
```
object(SessionProgress)::
  {
   id: string(SessionId), /* StreamId returned in publishing or SubscriptionId returned in subscribing*/
   status: "soac" | "ready" | "error",
   data: object(SOAPMessage)/*If status equals “soap”*/
        | (undefined/*If status equals “ready” and session is NOT for recording*/
           | object(RecorderInfo)/*If status equals “ready” and session is for recording*/ )
        | string(Reason)/*If status equals “error”*/
  }

  object(RecorderInfo)::
    {
     host: string(HostnameOrIPOfRecorder),
     file: string(FullPathNameOfRecordedFile)
    }
```
#### 3.3.15 Participant Sets Permission of another Participant
**RequestName**:  “set-permission”
**RequestData**: The SetPermission object with following definition:
```
object(SetPermission)::
  {
   id: string(ParticipantId),
   authorities: [object(Authority)]
  }

  object(Authority)::
    {
     operation: "publish" | "subscribe" | "text",
     field: "media.audio" | "media.video" | "type.add" | "type.remove",
     value: true | false | "to-peer" | "to-all" | string(Type)
    }
```
**ResponseData**: undefined if **ResponseStatus** is “ok”.

## 4. RESTful Signaling
### 4.1 Connection Maintenance
#### 4.1.1 Client Connects
No dedicate signaling message being sent from client is needed. The very first request from a new client must be a client joining request, which is considered as the connecting request as well.
Portal should be able to listen at both an HTTP and an HTTPS server port to receive RESTful request from clients. If the HTTPS server is enabled, the proper SSL certificate and private key store path must be correctly configured by portal.keystorePath item in portal.toml.
#### 4.1.2 Client Keeps Alive
The participant’s querying request (3.3) will be considered as the keep alive heart-beat from client.
If three querying request loss are consecutively detected by Portal, Portal will judge that the connection has lost.
#### 4.1.3 Client Disconnects
#### 4.1.4 Client Reconnects
### 4.2 Conferencing
#### 4.2.1 Participant Joins
#### 4.2.2 Participant leaves

