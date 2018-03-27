var audioNames = [
  'pcmu', 'opus_48000_2', 'pcma', 'ilbc', 'isac_16000', 'isac_32000', 'g722_16000_1', 'g722_16000_2', 'aac', 'aac_48000_2', 'ac3', 'nellymoser'
];

var audioFormat = {
  type: 'string',
  enum: audioNames
};

var videoNames = [
  'h264', 'h265', 'vp8', 'vp9'
];

var videoFormat = {
  type: 'string',
  enum: videoNames
};

var videoPara = {
  type: 'object',
  properties: {
    resolution: { type: 'array', items: { type: 'string' } },
    framerate: { type: 'array', items: { type: 'number', enum: [6, 12, 15, 24, 30, 48, 60] } },
    bitrate: { type: 'array', items: { type: 'string', enum: ['x0.8', 'x0.6', 'x0.4', 'x0.2'] } },
    keyFrameInterval: { type: 'array', items: { type: 'number', enum: [100, 30, 5, 2, 1] } }
  }
};

var videoParaV = {
  type: 'object',
  properties: {
    resolution: {
      type: 'object',
      properties: {
        width: { type: 'number' },
        height: { type: 'number' },
      }
    },
    framerate: { type: 'number', enum: [6, 12, 15, 24, 30, 48, 60] },
    bitrate: { type: 'number' },
    keyFrameInterval: { type: 'number', enum: [100, 30, 5, 2, 1] }
  }
};

var customItem = {
  type: 'object',
  properties: {
    region: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          id: { type: 'string' },
          shape: { type: 'string', enum: ['rectangle'], default: 'rectangle' },
          area: {
            type: 'object',
            properties: {
              left: { type: 'string' },
              top: { type: 'string' },
              width: { type: 'string' },
              height: { type: 'string' },
            }
          }
        }
      }
    }
  }
};

var viewItem = {
  type: 'object',
  properties: {
    label: { type: 'string', default: 'common' },
    audio: {
      type: 'object',
      properties: {
        format: audioFormat,
        vad: { type: 'boolean', default: true }
      }
    },
    video: {
      type: 'object',
      properties: {
        format: videoFormat,
        parameters: videoParaV,
        maxInput: { type: 'number', minimum: 1 },
        motionFactor: { type: 'number' },
        bgColor: {
          type: 'object',
          properties: {
            r: { type: 'number' },
            g: { type: 'number' },
            b: { type: 'number' }
          }
        },
        keepActiveInputPrimary: { type: 'boolean', default: false },
        layout: {
          type: 'object',
          properties: {
            fitPolicy: { type: 'string', enum: ['letterbox', 'crop' /*stretch*/], default: 'letterbox' },
            templates: {
              type: 'object',
              properties: {
                base: { type: 'string', enum: ['fluid', 'lecture', 'custom'], default: 'fluid' },
                custom: {
                  type: 'array',
                  items: customItem
                }
              }
            }
          }
        }
      }
    }
  },
  required: ['label']
};

var roleItem = {
  type: 'object',
  properties: {
    role: { type: 'string' },
    publish: {
      type: 'object',
      properties: {
        audio: { type: 'boolean' },
        video: { type: 'boolean' }
      }
    },
    subscribe: {
      type: 'object',
      properties: {
        audio: { type: 'boolean' },
        video: { type: 'boolean' }
      }
    }
  },
  required: ['role']
};

var RoomSchema = {
  type: 'object',
  properties: {
    name: { type: 'string' },
    inputLimit: { type: 'number', minimum: -1 },
    participantLimit: { type: 'number', minimum: -1 },
    roles: { type: 'array', items: roleItem },
    views: { type: 'array', items: viewItem },
    mediaIn: {
      type: 'object',
      properties: {
        audio: {
          type: 'array',
          items: audioFormat
        },
        video: {
          type: 'array',
          items: videoFormat
        }
      }
    },
    mediaOut: {
      type: 'object',
      properties: {
        audio: {
          type: 'array',
          items: audioFormat
        },
        video: {
          type: 'object',
          properties: {
            format: { type: 'array', items: videoFormat },
            parameters: videoPara
          }
        }
      }
    },
    transcoding: {
      type: 'object',
      properties: {
        audio: { type: 'boolean', default: true },
        video: {
          type: 'object',
          properties: {
            format: { type: 'boolean', default: true },
            parameters: {
              type: 'object',
              properties: {
                resolution: { type: 'boolean', default: true },
                framerate: { type: 'boolean', default: true },
                bitrate: { type: 'boolean', default: true },
                keyFrameInterval: { type: 'boolean', default: true }
              }
            }
          }
        }
      }
    },
    sip: {
      type: 'object',
      properties: {
        sipServer: { type: 'string' },
        username: { type: 'string' },
        password: { type: 'string' }
      }
    },
    notifying: {
      type: 'object',
      properties: {
        participantActivities: { type: 'boolean', default: true },
        streamChange: { type: 'boolean', default: true },
      }
    }
  },
  required: ['name']
};
