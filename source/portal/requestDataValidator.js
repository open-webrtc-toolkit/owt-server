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

// PublcationRequest
const PublicationRequest = {
  anyOf: [
    { // Webrtc Publication
      type: 'object',
      properties: {
        'media': { $ref: '#/definitions/WebRTCMediaOptions' },
        'attributes': { type: 'object' }
      },
      additionalProperties: false,
      required: ['media']
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
              additionalProperties: false,
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
                'source': { enum: ["camera", "screen-cast", "raw-file", "encoded-file"] },
                'parameters': {
                  type: 'object',
                  properties: {
                    'resolution': { $ref: '/Resolution' },
                    'framerate': { type: 'number' }
                  }
                }
              },
              additionalProperties: false,
              required: ['source']
            },
            { 'const': false }
          ]
        }
      },
      additionalProperties: false,
      required: ['audio', 'video']
    }
  }
};

// StreamControlInfo
const StreamControlInfo = {
  type: 'object',
  anyOf: [
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['mix', 'unmix', 'get-region'] },
        'data': { type: 'string' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['set-region'] },
        'data': { $ref: '#/definitions/RegionSetting' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string', require: true },
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      },
      additionalProperties: false
    },
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'RegionSetting': {
      type: 'object',
      properties: {
        'view': { type: 'string' },
        'region': { type: 'string',  minLength: 1}
      },
      additionalProperties: false,
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
        'media': { $ref: '#/definitions/MediaSubOptions' },
      },
      additionalProperties: false,
      required: ['media']
    }
  ],

  definitions: {
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
        'parameters': { $ref: '#/definitions/VideoParametersSpecification' },
        'simulcastRid': { type: 'string' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'AudioFormat': {
      type: 'object',
      properties: {
        'codec': { enum: ['pcmu', 'pcma', 'opus', 'g722', 'iSAC', 'iLBC', 'aac', 'ac3', 'nellymoser'] },
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
        'profile': { enum: ['CB', 'B', 'M', 'E', 'H'] }
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

// SubscriptionControlInfo
const SubscriptionControlInfo = {
  type: 'object',
  anyOf: [
    {
      properties: {
        'id': { type: 'string' },
        'operation': { enum: ['update'] },
        'data': { $ref: '#/definitions/SubscriptionUpdate' }
      },
      additionalProperties: false
    },
    {
      properties: {
        'id': { type: 'string' },
        'operation': { enum: ['pause', 'play'] },
        'data': { enum: ['audio', 'video', 'av'] }
      },
      additionalProperties: false
    }
  ],
  required: ['id', 'operation', 'data'],

  definitions: {
    'SubscriptionUpdate': {
      type: 'object',
      properties: {
        'audio': { $ref: '#/definitions/AudioUpdate' },
        'video': { $ref: '#/definitions/VideoUpdate' }
      },
      additionalProperties: false
    },

    'AudioUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' }
      },
      additionalProperties: false,
      required: ['from']
    },

    'VideoUpdate': {
      type: 'object',
      properties: {
        'from': { type: 'string' },
        'parameters': { $ref: '#/definitions/VideoUpdateSpecification' }
      },
      additionalProperties: false
    },

    'VideoUpdateSpecification': {
      type: 'object',
      properties: {
        resolution: { $ref: '/Resolution' },
        framerate: { type: 'number' },
        bitrate: { type: ['number', 'string'] },
        keyFrameInterval: { type: 'number' }
      },
      additionalProperties: false
    }
  }
};

var validators = {
  'publication-request': generateValidator(PublicationRequest),
  'stream-control-info': generateValidator(StreamControlInfo),
  'subscription-request': generateValidator(SubscriptionRequest),
  'subscription-control-info': generateValidator(SubscriptionControlInfo)
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
