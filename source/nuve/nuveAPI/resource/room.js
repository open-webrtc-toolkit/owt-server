/* global exports, module */

function Room (spec) {
    'use strict';
    this.name = spec.name || '';
    this.mode = spec.mode || 0;
    this.publishLimit = spec.publishLimit || 0;
    this.userLimit = spec.userLimit || 0;
    this.mediaMixing = spec.mediaMixing || null;
    if (typeof this.mode === 'string') this.mode = parseInt(this.mode, 10);
    if (typeof this.publishLimit === 'string') this.publishLimit = parseInt(this.publishLimit, 10);
    if (typeof this.userLimit === 'string') this.userLimit = parseInt(this.userLimit, 10);
    if (typeof this.mediaMixing === 'string') {
      try {
        this.mediaMixing = JSON.parse(this.mediaMixing);
      } catch (e) {
        this.mediaMixing = null;
      }
    }
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
        mediaMixing: null
    });
};

Room.prototype.validate = function () {
    'use strict';
    return (typeof this.name === 'string') &&
        (typeof this.mode === 'number' && this.mode >= 0) &&
        (typeof this.publishLimit === 'number' && (!isNaN(this.publishLimit))) &&
        (typeof this.userLimit === 'number' && (!isNaN(this.userLimit))) &&
        (typeof this.mediaMixing === 'object');
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
  }
}
*/