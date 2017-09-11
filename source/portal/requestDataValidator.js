/*
 * JSON Validator for Request Data
 */

const Ajv = require('ajv');
// Using json schema validate with 'defaults' and 'removeAdditional' properties
const ajv = new Ajv({ useDefaults: true, removeAdditional: true });

function generateValidator(schema) {
  var rawValidate = ajv.compile(schema);
  var validate = function (instance) {
    if (rawValidate(instance)) {
      return Promise.resolve(instance);
    } else {
      let errPaths = rawValidate.errors.map((err) => err.dataPath);
      let message = 'RequestData invalid ' + errPaths.join(',');
      return Promise.reject(message);
    }
  };

  return validate;
}

const Resolution = {
  id: '/Resolution',
  type: 'object',
  properties: {
    'width': { type: 'number' },
    'height': { type: 'number' }
  },
  required: ['width', 'height']
};
ajv.addSchema(Resolution);

// PublcationRequest
const PublicationRequest = {
  anyOf: [
    { // Webrtc Publication
      type: 'object',
      properties: {
        'type': { 'const': 'webrtc' },
        'media': { $ref: '#/definitions/WebRTCMediaOptions' },
        'attributes': { type: 'object' }
      },
      required: ['type', 'media']
    },
    {
      // Streaming Publication
      type: 'object',
      properties: {
        'type': { 'const': 'streaming' },
        'connection': { $ref: '#/definitions/StreamingInConnectionOptions' },
        'media': { $ref: '#/definitions/StreamingInMediaOptions' },
        'attributes': { type: 'object' }
      },
      required: ['type', 'connection', 'media']
    }
  ],

  definitions: {
    'WebRTCMediaOptions': {
      type: 'object',
      properties: {
        'audio': {
          anyOf: [
            {
              type: 'object',
              properties: {
                'source': { enum: ["mic", "screen-cast", "raw-file", "encoded-file"] }
              },
              required: ['source']
            },
            { 'const': false }
          ]
        },
        'video': {
          anyOf: [
            {
              type: 'object',
              properties: {
                'source': { enum: ["camera", "screen-cast", "raw-file", "encoded-file"] }
              },
              required: ['source']
            },
            { 'const': false }
          ]
        }
      },
      required: ['audio', 'video']
    },

    'StreamingInConnectionOptions': {
      type: 'object',
      properties: {
        url: { type: 'string' },
        transportProtocol: { enum: ['tcp', 'udp'], 'default': 'tcp' }, //optional, default: "tcp"
        bufferSize: { type: 'number', 'default': 8192 }     //optional, default: 8192 bytes
      },
      required: ['url']
    },

    'StreamingInMediaOptions': {
      type: 'object',
      properties: {
        'audio': { enum: ['auto', true, false] },
        'video': { enum: ['auto', true, false] }
      },
      required: ['audio', 'video']
    }
  }
};

// StreamControlInfo
const StreamControlInfo = {
  type: 'object',
  properties: {
    'id': { type: 'string', require: true }
  },
  anyOf: [
    {
      properties: {
        'operation': { enum: ['mix', 'unmix', 'get-region'] },
        'data': { type: 'string' }
      }
    },
    {
      properties: {
        'operation': { enum: ['set-region'] },
        'data': { $ref: '#/definitions/RegionSetting' }
      }
    },
    {
      properties: {
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      }
    },
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'RegionSetting': {
      type: 'object',
      properties: {
        'view': { type: 'string' },
        'region': { type: 'string' }
      },
      required: ['view', 'region']
    }
  }
};

// SubscriptionRequest
const SubscriptionRequest = {
  anyOf: [
    { // Webrtc Subscription
      type: 'object',
      properties: {
        'type': { 'const': 'webrtc' },
        'media': { $ref: '#/definitions/MediaSubOptions' },
      },
      required: ['type', 'media']
    },
    {
      // Streaming Subscription
      type: 'object',
      properties: {
        'type': { 'const': 'streaming' },
        'connection': { $ref: '#/definitions/StreamingOutConnectionOptions' },
        'media': { $ref: '#/definitions/MediaSubOptions' },
      },
      required: ['type', 'connection', 'media']
    },
    {
      // Recording Subscription
      type: 'object',
      properties: {
        'type': { 'const': 'recording' },
        'connection': { $ref: '#/definitions/RecordingStorageOptions' },
        'media': { $ref: '#/definitions/MediaSubOptions' },
      },
      required: ['type', 'connection', 'media']
    }
  ],

  definitions: {
    'StreamingOutConnectionOptions': {
      type: 'object',
      properties: {
        'url': { type: 'string' }
      },
      required: ['url']
    },

    'RecordingStorageOptions': {
      type: 'object',
      properties: {
        'container': { enum: ['mp4', 'mkv', 'ts', 'auto'] }
      }
    },

    'MediaSubOptions': {
      type: 'object',
      properties: {
        'audio': {
          anyOf: [
            { $ref: '#/definitions/AudioSubOptions'},
            { 'const': false }
          ]
        },
        'video': {
          anyOf: [
            { $ref: '#/definitions/VideoSubOptions'},
            { 'const': false }
          ]
        }
      },
      required: ['audio', 'video']
    },

    'AudioSubOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'format': { $ref: '#/definitions/AudioFormat' }
      },
      required: ['from']
    },

    'VideoSubOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'format:': { $ref: '#/definitions/VideoFormat' },
        'parameters': { $ref: '#/definitions/VideoParametersSpecification' }
      },
      required: ['from']
    },

    'AudioFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['pcmu', 'pcma', 'opus', 'g722', 'iSAC', 'iLBC', 'aac', 'ac3', 'nellymoser'] },
        'sampleRate': { type: 'number' },
        'channelNum': { type: 'number' }
      },
      required: ['codec']
    },

    'VideoFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['h264', 'h265', 'vp8', 'vp9'] },
        'profile': { enum: ['baseline', 'constrained-baseline', 'main', 'high'] }
      },
      required: ['codec']
    },

    'VideoParametersSpecification': {
      type: 'object',
      properties: {
        'resolution': { $ref: '/Resolution' },
        'framerate': { type: 'number' },
        'bitrate': { type: ['string', 'number'] },
        'keyFrameInterval': { type: 'number' }
      }
    }
  }
};

// SubscriptionControlInfo
const SubscriptionControlInfo = {
  type: 'object',
  properties: {
    'id': { type: 'string' },
  },
  anyOf: [
    {
      properties: {
        'operation': { enum: ['update'] },
        'data': { $ref: '#/definitions/SubscriptionUpdate' }
      }
    },
    {
      properties: {
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      }
    }
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'SubscriptionUpdate': {
      type: 'object',
      properties: {
        'audio': { $ref: '#/definitions/AudioUpdate' },
        'video': { $ref: '#/definitions/VideoUpdate' }
      }
    },

    'AudioUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' }
      },
      required: ['from']
    },

    'VideoUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'spec': { $ref: '#/definitions/VideoUpdateSpecification' }
      },
      required: ['from']
    },

    'VideoUpdateSpecification': {
      type: 'object',
      properties: {
        resolution: { $ref: '/Resolution' },
        framerate: { type: 'number' },
        bitrate: { type: ['number', 'string'] },
        keyFrameInterval: { type: 'number' }
      }
    }
  }
};

// SetPermission
const SetPermission = {
  type: 'object',
  properties: {
    'id': { type: 'string' },
    'authorities': {
      type: 'array',
      items: { $ref: '#/definitions/Authority' }
    }
  },
  required: ['id', 'authorities'],

  definitions: {
    'Authority': {
      type: 'object',
      properties: {
        'operation': { enum: ['publish', 'subscribe', 'text'] },
        'field': { enum: ['media.audio', 'media.video', 'type.add', 'type.remove'] },
        'value': { type: ['boolean', 'string'] }
      },
      required: ['operation', 'value']
    }
  }
};

var validators = {
  'publication-request': generateValidator(PublicationRequest),
  'stream-control-info': generateValidator(StreamControlInfo),
  'subscription-request': generateValidator(SubscriptionRequest),
  'subscription-control-info': generateValidator(SubscriptionControlInfo),
  'set-permission': generateValidator(SetPermission),
};

// Export JSON validator functions
module.exports = {
  validate: (spec, data) => {
    if (validators[spec]) {
      return validators[spec](data);
    } else {
      throw new Error(`No such validator: ${spec}`);
    }
  }
};
