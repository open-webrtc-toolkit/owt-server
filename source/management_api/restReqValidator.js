// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 * JSON Validator for Request Data
 */

const Ajv = require('ajv');
// Using json schema validate with 'defaults' and 'removeAdditional' properties
const ajv = new Ajv({ useDefaults: true });

function generateValidator(schema) {
  var rawValidate = ajv.compile(schema);
  var validate = function (instance) {
    if (rawValidate(instance)) {
      return Promise.resolve(instance);
    } else {
      return Promise.reject(getErrorMessage(rawValidate.errors));
    }
  };

  return validate;
}

function getErrorMessage(errors) {
  var error = errors.shift();
  var message = '';
  var messages = [];
  while (error) {
    if (error.keyword === 'anyOf') {
      error = errors.shift();
      continue;
    }
    var message = 'request' + error.dataPath + ' ' + error.message;
    for (let key in error.params) {
      let param = error.params[key];
      if (param) {
        message += ' ' + JSON.stringify(param);
      }
    }

    messages.push(message);
    error = errors.shift();
  }

  return messages.join(',');
}

const Resolution = {
  id: '/Resolution',
  type: 'object',
  properties: {
    'width': { type: 'number' },
    'height': { type: 'number' }
  },
  additionalProperties: false,
  required: ['width', 'height']
};
ajv.addSchema(Resolution);

const ParticipantUpdate = {
  type: 'array',
  items: {$ref: '#/definitions/PermissionUpdate'},
  additionalProperties: false,

  definitions: {
    'PermissionUpdate': {
      type: 'object',
      properties: {
        'op': { enum: ['replace'] },
        'path': { enum: ['/permission/publish', '/permission/publish/audio', '/permission/publish/video', '/permission/subscribe', '/permission/subscribe/audio', '/permission/subscribe/video']},
        'value': {
          anyOf: [{
            type: 'boolean'
          }, {
            type: 'object',
            properties: {
              'audio': {type: 'boolean'},
              'video': {type: 'boolean'}
            },
            additionalProperties: false,
            required: ['audio', 'video']
          }]
        }
      },
      additionalProperties: false,
      required: ['op', 'path', 'value']
    }
  }
};

const StreamingInRequest = {
  type: 'object',
  properties: {
    'type': { 'const': 'streaming' },
    'connection': { $ref: '#/definitions/StreamingInConnectionOptions' },
    'media': { $ref: '#/definitions/StreamingInMediaOptions' },
    'attributes': { type: 'object' }
  },
  additionalProperties: false,
  required: ['type', 'connection', 'media'],

  definitions: {
    'StreamingInConnectionOptions': {
      type: 'object',
      properties: {
        url: { type: 'string' },
        transportProtocol: { enum: ['tcp', 'udp'], 'default': 'tcp' }, //optional, default: "tcp"
        bufferSize: { type: 'number', 'default': 8192 },     //optional, default: 8192 bytes
        origin: {
          isp: { type: 'string'},
          region: { type: 'string'}
        }

      },
      additionalProperties: false,
      required: ['url']
    },

    'StreamingInMediaOptions': {
      type: 'object',
      properties: {
        'audio': { enum: ['auto', true, false] },
        'video': { enum: ['auto', true, false] }
      },
      additionalProperties: false,
      required: ['audio', 'video']
    }
  }
};

const StreamUpdate = {
  type: 'array',
  items: {$ref: '#/definitions/StreamInfoUpdate'},
  additionalProperties: false,

  definitions: {
    'StreamInfoUpdate': {
      type: 'object',
      anyOf: [
      {
        properties: {
          'op': { enum: ['add', 'remove'] },
          'path': { 'const': '/info/inViews'},
          'value': { type: 'string'}
        },
        additionalProperties: false
      }, {
        properties: {
          'op': { 'const': 'replace'},
          'path': { enum: ['/media/audio/status', '/media/video/status']},
          'value': { enum: ['active', 'inactive']}
        },
        additionalProperties: false
      }, {
        properties: {
          'op': { 'const': 'replace'},
          'path': { 'const': '/info/layout'},
          'value': {
            type: 'array',
            items: {
              type: 'object',
              properties: {
                'stream': { type: 'string'},
                'region': {
                  type: 'object',
                  properties: {
                    'id': { type: 'string'},
                    'shape': { 'const': 'rectangle'},
                    'area': {
                      type: 'object',
                      properties: {
                        left: {$ref: '#/definitions/RationalNumberStr'},
                        top: {$ref: '#/definitions/RationalNumberStr'},
                        width: {$ref: '#/definitions/RationalNumberStr'},
                        height: {$ref: '#/definitions/RationalNumberStr'}
                      },
                      additionalProperties: false,
                      required: ['left', 'top', 'width', 'height']
                    }
                  },
                  additionalProperties: false,
                  required: ['id', 'shape', 'area']
                }
              },
              additionalProperties: false,
              required: ['region']
            }
          }
        },
        additionalProperties: false
      }, {
        properties: {
          'op': { 'const': 'replace'},
          'path': { 'pattern': '/info/layout/[0-9]+/stream'},
          'value': { type: 'string'}
        },
        additionalProperties: false
      }],
      required: ['op', 'path', 'value']
    },
    'RationalNumberStr': {
      type: 'string',
      value: {'pattern': '/^0|1|(?:[1-9][0-9]*\/[1-9][0-9]*)/'}
    }
  }
};

const ServerSideSubscriptionRequest = {
  anyOf: [
    {
      // Streaming Subscription
      type: 'object',
      properties: {
        'type': { 'const': 'streaming' },
        'connection': { $ref: '#/definitions/StreamingOutConnectionOptions' },
        'media': { anyOf: [{ $ref: '#/definitions/MediaSubOptions' },{'const': null}]},
      },
      additionalProperties: false,
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
      additionalProperties: false,
      required: ['type', 'connection', 'media']
    },
    {
      // Analytics Subscription
      type: 'object',
      properties: {
        'type': { 'const': 'analytics' },
        'connection': { $ref: '#/definitions/AnalyticsOptions' },
        'media': { $ref: '#/definitions/MediaSubOptions' },
      },
      additionalProperties: false,
      required: ['type', 'connection', 'media']
    }
  ],

  definitions: {
    'StreamingOutConnectionOptions': {
      type: 'object',
      properties: {
        'protocol': {enum: ['rtmp', 'rtsp', 'hls', 'dash']},
        'url': { type: 'string' },
        'parameters': {anyOf: [
          {
            type: 'object',
            properties: {
              'method': {enum: ['PUT', 'POST']},
              'hlsTime': {type: 'number'},
              'hlsListSize': {type: 'number'}
            },
            additionalProperties: false,
            required: ['method', 'hlsTime', 'hlsListSize']
          },{
            type: 'object',
            properties: {
              'method': {enum: ['PUT', 'POST']},
              'dashSegDuration': {type: 'number'},
              'dashWindowSize': {type: 'number'}
            },
            additionalProperties: false,
            required: ['method', 'dashSegDuration', 'dashWindowSize']
          }
        ]}
      },
      additionalProperties: false,
      required: ['protocol', 'url']
    },

    'RecordingStorageOptions': {
      type: 'object',
      properties: {
        'container': { enum: ['mp4', 'mkv', 'ts', 'auto'] }
      },
      additionalProperties: false
    },

    'AnalyticsOptions': {
      type: 'object',
      properties: {
        'algorithm': { type: 'string' }
      },
      additionalProperties: false
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
      additionalProperties: false,
      required: ['audio', 'video']
    },

    'AudioSubOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'format': { $ref: '#/definitions/AudioFormat' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'VideoSubOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'format': { $ref: '#/definitions/VideoFormat' },
        'parameters': { $ref: '#/definitions/VideoParametersSpecification' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'AudioFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['pcmu', 'pcma', 'opus', 'g722', 'isac', 'ilbc', 'aac', 'ac3', 'nellymoser'] },
        'sampleRate': { type: 'number' },
        'channelNum': { type: 'number' }
      },
      additionalProperties: false,
      required: ['codec']
    },

    'VideoFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['h264', 'h265', 'vp8', 'vp9'] },
        'profile': { enum: ['B', 'CB', 'M', 'H', 'E'] }
      },
      additionalProperties: false,
      required: ['codec']
    },

    'VideoParametersSpecification': {
      type: 'object',
      properties: {
        'resolution': { $ref: '/Resolution' },
        'framerate': { type: 'number' },
        'bitrate': { type: ['string', 'number'] },
        'keyFrameInterval': { type: 'number' }
      },
      additionalProperties: false
    }
  }
};

const SubscriptionControlInfo = {
  id: '/SubscriptionControlInfo',
  type: 'object',
  anyOf: [
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { enum: ['/media/audio/status', '/media/video/status'] },
        'value': { enum: ['active', 'inactive'] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { enum: ['/media/audio/from', '/media/video/from'] },
        'value': { type: 'string' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/media/video/parameters/resolution' },
        'value': { $ref: '/Resolution' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/media/video/parameters/framerate' },
        'value': { enum: [6, 12, 15, 24, 30, 48, 60] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/media/video/parameters/bitrate' },
        'value': { enum: ['x0.8', 'x0.6', 'x0.4', 'x0.2'] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/media/video/parameters/keyFrameInterval' },
        'value': { enum: [1, 2, 5, 30, 100] }
      },
      additionalProperties: false
    }
  ],
  required: ['op', 'path', 'value']
};
ajv.addSchema(SubscriptionControlInfo);

const SubscriptionUpdate = {
  type: 'array',
  items: {$ref: '/SubscriptionControlInfo'}
};

const SipCallAdd = {
  type: 'object',
  properties: {
    'peerURI': { type: 'string' },
    'mediaIn': {
      type: 'object',
      properties: {
        'audio': {'const': true},
        'video': {type: 'boolean'}
      },
      additionalProperties: false,
      required: ['audio', 'video']
    },
    'mediaOut': { $ref: '#/definitions/MediaOutOptions' },
  },
  additionalProperties: false,
  required: ['peerURI', 'mediaIn', 'mediaOut'],

  definitions: {
    'MediaOutOptions': {
      type: 'object',
      properties: {
        'audio': {
          $ref: '#/definitions/AudioOutOptions',
        },
        'video': {
          anyOf: [
            { $ref: '#/definitions/VideoOutOptions'},
            { 'const': false }
          ]
        }
      },
      additionalProperties: false,
      required: ['audio', 'video']
    },

    'AudioOutOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'VideoOutOptions': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'parameters': { $ref: '#/definitions/VideoParametersSpecification' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'VideoParametersSpecification': {
      type: 'object',
      properties: {
        'resolution': { $ref: '/Resolution' },
        'framerate': { type: 'number' },
        'bitrate': { type: ['string', 'number'] },
        'keyFrameInterval': { type: 'number' }
      },
      additionalProperties: false
    }
  }
};

const SipCallControlInfo = {
  id: '/SipCallControlInfo',
  type: 'object',
  anyOf: [
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { enum: ['/output/media/audio/status', '/output/media/video/status'] },
        'value': { enum: ['active', 'inactive'] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { enum: ['/output/media/audio/from', '/output/media/video/from'] },
        'value': { type: 'string' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/output/media/video/parameters/resolution' },
        'value': { $ref: '/Resolution' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/output/media/video/parameters/framerate' },
        'value': { enum: [6, 12, 15, 24, 30, 48, 60] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/output/media/video/parameters/bitrate' },
        'value': { enum: ['x0.8', 'x0.6', 'x0.4', 'x0.2'] }
      },
      additionalProperties: false
    },
    {
      properties: {
        'op': { 'const': 'replace' },
        'path': { 'const': '/output/media/video/parameters/keyFrameInterval' },
        'value': { enum: [1, 2, 5, 30, 100] }
      },
      additionalProperties: false
    }
  ],
  required: ['op', 'path', 'value']
};
ajv.addSchema(SipCallControlInfo);

const SipCallUpdate = {
  type: 'array',
  items: {$ref: '/SipCallControlInfo'}
};
var validators = {
  'participant-update': generateValidator(ParticipantUpdate),
  'streamingIn-req': generateValidator(StreamingInRequest),
  'stream-update': generateValidator(StreamUpdate),
  'serverSideSub-req': generateValidator(ServerSideSubscriptionRequest),
  'subscription-update': generateValidator(SubscriptionUpdate),
  'sipcall-add': generateValidator(SipCallAdd),
  'sipcall-update': generateValidator(SipCallUpdate)
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
