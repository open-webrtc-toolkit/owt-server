var audioFormat = {
  type: 'object',
  properties: {
    codec: {
      type: 'string',
      enum: [
        'pcmu', 'pcma', 'opus',
        'g722', 'isac', 'ilbc',
        'aac', 'ac3', 'nellymoser'
      ]
    },
    sampleRate: { type: 'number' },
    channelNum: { type: 'number' }
  },
  required: ['codec']
};

var videoFormat = {
  type: 'object',
  properties: {
    codec: { type: 'string', enum: ['h264', 'h265', 'vp8', 'vp9'] },
    profile: { type: 'string' }
  },
  required: ['codec']
};

var videoPara = {
  type: 'object',
  properties: {
    resolution: { type: 'array', items: { type: 'string' } },
    framerate: { type: 'array', items: { type: 'number' } },
    bitrate: { type: 'array', items: { type: 'string' } },
    keyFrameInterval: { type: 'array', items: { type: 'number' } }
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
    framerate: { type: 'number' },
    bitrate: { type: 'number' },
    keyFrameInterval: { type: 'number' }
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
        maxInput: { type: 'number' },
        motionFactor: { type: 'number' },
        bgColor: {
          type: 'object',
          properties: {
            r: { type: 'number' },
            g: { type: 'number' },
            b: { type: 'number' }
          }
        },
        keepActiveSpeakerInPrimary: { type: 'boolean', default: false },
        layout: {
          type: 'object',
          properties: {
            fitPolicy: { type: 'string', enum: ['letterbox', 'crop'], default: 'letterbox' },
            //setRegionEffect: { en}
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
      audio: { type: 'boolean', default: true },
      video: {
        format: { type: 'boolean', default: true },
        parameters: {
          resolution: { type: 'boolean', default: true },
          framerate: { type: 'boolean', default: true },
          bitrate: { type: 'boolean', default: true },
          keyFrameInterval: { type: 'boolean', default: true }
        }
      }
    },
    sip: {
      sipServer: { type: 'string' },
      username: { type: 'string' },
      password: { type: 'string' }
    },
    notifying: {
      participantActivities: { type: 'boolean', default: true },
      streamChange: { type: 'boolean', default: true },
    }
  },
  required: ['name']
};
