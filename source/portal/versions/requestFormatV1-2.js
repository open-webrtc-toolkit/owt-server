// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Request Data Format for version 1.0 - 1.1
 */
'use strict';

const {
  StreamControlInfo,
} = require('./requestFormatV1-0');

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

const TransportOptions = {
  type: 'object',
  properties: {
    'type': { enum: ["webrtc", "quic"] },
    'id': { type: 'string' }
  }
};

// PublcationRequest
const PublicationRequest = {
  anyOf: [
    { // Webrtc Publication
      type: 'object',
      properties: {
        'media': { $ref: '#/definitions/WebRTCMediaOptions' },
        'attributes': { type: 'object' },
        'transportId': { type: 'string' }
      },
      additionalProperties: false,
      required: ['media']
    }
  ],

  definitions: {
    'WebRTCMediaOptions': {
      type: 'object',
      properties: {
        'tracks': {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              'type': { enum: ["audio", "video"] },
              'mid': { type: 'string' },
              'source': { enum: ["mic", "camera", "screen-cast", "raw-file", "encoded-file"] },
            }
          }
        }
      },
      additionalProperties: false,
      required: ['tracks']
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
        'transportId': { type: 'string' }
      },
      additionalProperties: false,
      required: ['media']
    }
  ],

  definitions: {
    'MediaSubOptions': {
      type: 'object',
      properties: {
        'tracks': {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              'type': { enum: ["audio", "video"] },
              'mid': { type: 'string' },
              'from': { type: 'string' },
              'format': {
                anyOf: [
                  { $ref: '#/definitions/AudioFormat' },
                  { $ref: '#/definitions/VideoFormat' },
                ]
              },
              'parameters': { $ref: '#/definitions/VideoParametersSpecification' },
            }
          }
        }
      },
      additionalProperties: false,
      required: ['tracks']
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
        'resolution': Resolution,
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
        'tracks': {
          type: 'array',
          items: {
            type: 'object',
            properties: {
              'id': { type: 'string' },
              'from': { type: 'string' },
              'parameters': { $ref: '#/definitions/VideoUpdateSpecification' }
            },
            additionalProperties: false
          }
        }
      },
      additionalProperties: false
    },

    'VideoUpdateSpecification': {
      type: 'object',
      properties: {
        resolution: Resolution,
        framerate: { type: 'number' },
        bitrate: { type: ['number', 'string'] },
        keyFrameInterval: { type: 'number' }
      },
      additionalProperties: false
    }
  }
};

module.exports = {
  PublicationRequest,
  StreamControlInfo,
  SubscriptionRequest,
  SubscriptionControlInfo,
};
