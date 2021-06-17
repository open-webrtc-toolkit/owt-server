| OWT Active Audio Stream Introduction |


# Overview

OWT conference provide functionality to select K (K = 3 by default) most active audio streams in the conference.

# How to enable
The active audio selecting feature can be enabled by setting `selectActiveAudio` to true in room configuration. When it's enabled, an audio node will be used for selecting audio streams for each room, and K audio-only streams will be produced as most active audio streams in the conference.

# Stream
```
stream.info = {
  type: 'selecting',
  owner: 'unknown' | string(ParticipantId),
  activeInput: 'unknown' | string(StreamId),
}
```

# Events
Stream notification for input change of active audio stream.
```
{
  id: activeAudioId,
  status: 'update',
  data: {field: 'activeInput', value: {id: originInput, volume: 'major' | 'minor'}}
}
```

