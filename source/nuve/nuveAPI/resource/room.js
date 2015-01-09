/* global exports, module */

function Room (spec) {
    'use strict';
    this.name = spec.name || '';
    this.mode = spec.mode || 0;
    this.publishLimit = spec.publishLimit || 0;
    this.userLimit = spec.userLimit || 0;
    this.mediaMixing = spec.mediaMixing || null;
    this.needRecording = spec.needRecording || false;
}

Room.create = function (spec) {
    'use strict';
    return new Room(spec);
};

Room.createDefault = function (name) {
    'use strict';
    return new Room({
        name: name,
        mode: 0,
        publishLimit: 16,
        userLimit: 100,
        mediaMixing: null,
        needRecording: false
    });
};

Room.prototype.validate = function () {
    'use strict';
    return (typeof this.name === 'string') &&
        (typeof this.mode === 'number') &&
        (typeof this.publishLimit === 'number') &&
        (typeof this.userLimit === 'number') &&
        (typeof this.mediaMixing === 'object') &&
        (typeof this.needRecording === 'boolean');
};

Room.prototype.toString = function () {
    'use strict';
    return JSON.stringify(this);
};

Room.prototype.format = function () {

};

exports = module.exports = Room;

/*
{
  "name": "roomName", // type string
  "mode": 0, // type number
  "publishLimit": 16, // type number
  "userLimit": 100, // type number
  "mediaMixing": { // type object
    "video": {
      "resolution": 0, // type number
      "bitrate": 750, // type numer
      "bkColor": 0, // type number
      "layout": { // type object
        "base": 0, // type number
        "custom": [ // type object::Array or null
          {
            "maxinput": 5,
            "region": []
          },
          {
            "maxinput": 10,
            "region": []
          }
        ]
      }
    },
    "audio": null // type object
  },
  "needRecording": true // type boolean
}
*/