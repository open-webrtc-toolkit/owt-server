# [PROPOSAL] Component Wide Configuration

Markus, Feb 2016.


        Disclaimer:
        This document is not finalized and the real implementation may slightly differ to work around with obstacles, keep pace with coworkers, etc.
        This design is mainly for WooGeen 3.1 release.
        Further effort is required for future developement after 3.1.


## Goals

- Each component has their own configuration file.
- Each component can be released/distributed alone.
    - Nuve
    - Portal
    - Agent
        - accessNode
        - audioNode
        - videoNode


## Directory Structure

```
.
├── contrib                             - configuration templates, patches, certifications, etc.
├── doc                                 - documentation for architecture, usage, design, etc.
├── scripts                             - helper scripts.
├── source
│   ├── common                          - common node modules.
│   ├── portal                          - Portal.
│   ├── agent                           - Agent.
│   │   ├── access                      - accessNode.
│   │   │   ├── rtspIn                  - woogeen-rtsp-in module.
│   │   │   ├── rtspOut                 - woogeen-rtsp-out module.
│   │   │   ├── mediaFile               - woogeen-mediafile-io module.
│   │   │   └── webrtc                  - woogeen-webrtc module.
│   │   ├── audio                       - audioNode.
│   │   │   └── audioMixer              - woogeen-audiomixer module.
│   │   ├── video                       - videoNode.
│   │   │   └── videoMixer              - woogeen-videomixer module.
│   │   └── addons                      - videoNode.
│   │       ├── internal                - woogeen-internal-io module.
│   │       └── mediaFrameMulticaster   - woogeen-mediaframemulticaster module.
│   ├── lib
│   │   ├── liberizo                    - liberizo.so.
│   │   ├── woogeen_base                - libwoogeen_base.so.
│   │   └── mcu                         - libmcu.so.
│   └── nuve
├── test
└── third_party             - only *source code*
    ├── mediaprocessor
    ├── srtp
    └── webrtc
```


### Phase out

- `source/client_sdk`: should be merged with primary JS SDK, and/or maintained in SDK repo.
- `source/extras`: should be maintained in SDK repo.
- `source/sdk2`: [as above]
- `source/product`: [as above]


## Addons in `source/erizo`

Current Dependencies:

```
  Node.js Layer         Node.js Layer               Node Addon Functionality


  accessNode     ->                     ->          Internal{In,Out}
                                                    MediaFile{In,Out}
                                                    RtspOut
                        rtspIn          ->          RtspIn
                        wrtcConnection  ->          WebRtcConnection
                                                    AudioFrameConstructor
                                                    VideoFrameConstructor
                                                    AudioFramePacketizer
                                                    VideoFramePacketizer


  audioNode      ->                                 Internal{In,Out}
                                                    MediaFrameMulticaster
                                                    AudioMixer

  videoNode      ->                                 Internal{In,Out}
                                                    MediaFrameMulticaster
                                                    VideoMixer
```

The addon would become these modules:

- `woogeen-rtsp-in`
    - `RtspIn`
- `woogeen-rtsp-out`
    - `RtspOut`
- `woogeen-mediafile-io`
    - `MediaFileIn`
    - `MediaFileOut`
- `woogeen-internal-io`
    - `InternalIn`
    - `InternalOut`
- `woogeen-mediaframemulticaster`
    - `MediaFrameMulticaster`
- `woogeen-audiomixer`
    - `AudioMixer`
- `woogeen-videomixer`
    - `VideoMixer`
- `woogeen-webrtc`
    - `WebRtcConnection`
    - `AudioFrameConstructor`
    - `VideoFrameConstructor`
    - `AudioFramePacketizer`
    - `VideoFramePacketizer`


The C++ Layer would remain untouched, or have minimal modifications.
