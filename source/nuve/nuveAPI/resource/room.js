/* global module */
var meta = require('../public/meta.json');
var log = require('./../logger').logger.getLogger("Room");

function defaultMediaMixing () {
    'use strict';
    return {
        video: {
            avCoordinated: 0,
            maxInput: 16,
            resolution: 0,
            bitrate: 0,
            bkColor: 0,
            layout: {
                base: 0,
                custom: null
            }
        },
        audio: null
    };
}

function Room (spec) {
    'use strict';
    this.name = spec.name + ''; // ensure type string
    this.mode = spec.mode;
    this.publishLimit = spec.publishLimit;
    this.userLimit = spec.userLimit;
    this.mediaMixing = spec.mediaMixing;

    if (typeof this.mode === 'string') {
        this.mode = parseInt(this.mode, 10);
        if (isNaN(this.mode)) this.mode = 0;
    } else if (typeof this.mode !== 'number') {
        this.mode = 0;
    }

    if (typeof this.publishLimit === 'string'){
        this.publishLimit = parseInt(this.publishLimit, 10);
        if (isNaN(this.publishLimit)) this.publishLimit = -1;
    } else if (typeof this.publishLimit !== 'number') {
        this.publishLimit = -1;
    }

    if (typeof this.userLimit === 'string'){
        this.userLimit = parseInt(this.userLimit, 10);
        if (isNaN(this.userLimit)) this.userLimit = -1;
    } else if (typeof this.userLimit !== 'number') {
        this.userLimit = -1;
    }

    if (typeof this.mediaMixing === 'string') {
        try {
            this.mediaMixing = JSON.parse(this.mediaMixing);
        } catch (e) {
            this.mediaMixing = null;
        }
    }

    if (typeof this.mediaMixing !== 'object') {
        this.mediaMixing = null;
    }

    if (typeof this.mediaMixing === 'object' && this.mediaMixing !== null) {
        if (this.mediaMixing.video) {
            if (this.mediaMixing.video.avCoordinated === '1' ||
                this.mediaMixing.video.avCoordinated === 1 ||
                this.mediaMixing.video.avCoordinated === true) {
                this.mediaMixing.video.avCoordinated = 1;
            } else {
                this.mediaMixing.video.avCoordinated = 0;
            }

            if (typeof this.mediaMixing.video.maxInput === 'string') {
                this.mediaMixing.video.maxInput = parseInt(this.mediaMixing.video.maxInput, 10);
            } else if (typeof this.mediaMixing.video.maxInput !== 'number') {
                this.mediaMixing.video.maxInput = 16;
            }

            if (typeof this.mediaMixing.video.resolution === 'string') {
                this.mediaMixing.video.resolution = parseInt(this.mediaMixing.video.resolution, 10);
            } else if (typeof this.mediaMixing.video.resolution !== 'number') {
                this.mediaMixing.video.resolution = 0;
            }

            if (typeof this.mediaMixing.video.bitrate === 'string') {
                this.mediaMixing.video.bitrate = parseInt(this.mediaMixing.video.bitrate, 10);
            } else if (typeof this.mediaMixing.video.bitrate !== 'number') {
                this.mediaMixing.video.bitrate = 0;
            }

            if (typeof this.mediaMixing.video.bkColor === 'string') {
                this.mediaMixing.video.bkColor = parseInt(this.mediaMixing.video.bkColor, 10);
            } else if (typeof this.mediaMixing.video.bkColor !== 'number') {
                this.mediaMixing.video.bkColor = 0;
            }

            if (typeof this.mediaMixing.video.layout === 'object' && this.mediaMixing.video.layout !== null) {
                if (typeof this.mediaMixing.video.layout.base === 'string') {
                    this.mediaMixing.video.layout.base = parseInt(this.mediaMixing.video.layout.base, 10);
                    if (isNaN(this.mediaMixing.video.layout.base)) {
                        this.mediaMixing.video.layout.base = 0;
                    }
                } else if (typeof this.mediaMixing.video.layout.base !== 'number') {
                    this.mediaMixing.video.layout.base = 0;
                }
            } else {
                this.mediaMixing.video.layout = defaultMediaMixing().video.layout;
            }
        } else {
            this.mediaMixing.video = defaultMediaMixing().video;
        }
    } else {
        this.mediaMixing = defaultMediaMixing();
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
        publishLimit: -1,
        userLimit: -1,
        mediaMixing: defaultMediaMixing()
    });
};

Room.prototype.toString = function () {
    'use strict';
    return JSON.stringify(this);
};

Room.genConfig = function (room) {
    'use strict';
    var layoutType = meta.mediaMixing.video.layout.base[room.mediaMixing.video.layout.base];
    var maxInput = room.mediaMixing.video.maxInput || 16;
    var layoutTemplates;

    if (layoutType === 'fluid') {
        layoutTemplates = generateFluidTemplates(maxInput);
    } else if (layoutType === 'lecture') {
        layoutTemplates = generateLectureTemplates(maxInput);
    } else {
        layoutTemplates = generateFluidTemplates(maxInput);
    }

    var custom = room.mediaMixing.video.layout.custom;
    if (typeof custom === 'string') {
        try {
            custom = JSON.parse(custom);
        } catch (e) {}
    }

    if (isTemplatesValid(custom)) {
        log.info('apply custom layout templates');
        custom.map(function (tpl) {
            var len = tpl.region.length;
            var pos;
            for (var j in layoutTemplates) {
                if (layoutTemplates.hasOwnProperty(j)) {
                    if (layoutTemplates[j].region.length >= len) {
                        pos = j;
                        break;
                    }
                }
            }
            if (pos === undefined) {
                layoutTemplates.push(tpl);
            } else if (layoutTemplates[pos].region.length === len) {
                layoutTemplates.splice(pos, 1, tpl);
            } else {
                layoutTemplates.splice(pos, 0, tpl);
            }
        });
    } else if (custom) {
        log.info('invalid custom layout templates');
    }

    return {
        mode: meta.mode[room.mode],
        publishLimit: room.publishLimit,
        userLimit: room.userLimit,
        mediaMixing: {
            video: {
                avCoordinated: room.mediaMixing.video.avCoordinated === 1 ? true : false,
                maxInput: maxInput,
                resolution: meta.mediaMixing.video.resolution[room.mediaMixing.video.resolution] || 'vga',
                bkColor: meta.mediaMixing.video.bkColor[room.mediaMixing.video.bkColor] || 'black',
                layout: layoutTemplates
            },
            audio: null
        }
    };
};

Room.prototype.genConfig = function() {
    'use strict';
    return Room.genConfig(this);
};

module.exports = Room;

/*
{
  "name": "roomName", // type string
  "mode": 0, // type number
  "publishLimit": 16, // type number
  "userLimit": 100, // type number
  "mediaMixing": { // type object
    "video": {
      "resolution": 0, // type number
      "bitrate": 0, // type numer
      "bkColor": 0, // type number
      "maxInput": 16, // type number
      "avCoordinated": 0, // type number: 0/1
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

function generateLectureTemplates (maxInput) {
    'use strict';
    var result = [ {region:[{id: '1', left: 0, top: 0, relativesize: 1.0, priority: 1.0}]},
                   {region:[{id: '1', left: 0, top: 0, relativesize: 0.667, priority: 1.0},
                              {id: '2', left: 0.667, top: 0, relativesize: 0.333, priority: 1.0},
                              {id: '3', left: 0.667, top: 0.333, relativesize: 0.333, priority: 1.0},
                              {id: '4', left: 0.667, top: 0.667, relativesize: 0.333, priority: 1.0},
                              {id: '5', left: 0.333, top: 0.667, relativesize: 0.333, priority: 1.0},
                              {id: '6', left: 0, top: 0.667, relativesize: 0.333, priority: 1.0}]}];
    if (maxInput > 6) { // for maxInput: 8, 10, 12, 14
        var maxDiv = maxInput / 2;
        maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
        maxDiv = maxDiv > 7 ? 7 : maxDiv;

        for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
            var mainReginRelative = ((divFactor - 1) * 1.0 / divFactor);
            var minorRegionRelative = 1.0 / divFactor;

            var regions = [{id: '1', left: 0, top: 0, relativesize: mainReginRelative, priority: 1.0}];
            var id = 2;
            for (var y = 0; y < divFactor; y++) {
                regions.push({id: ''+(id++),
                              left: mainReginRelative,
                              top: y*1.0 / divFactor,
                              relativesize: minorRegionRelative,
                              priority: 1.0});
            }

            for (var x = divFactor - 2; x >= 0; x--) {
                regions.push({id: ''+(id++),
                              left: x*1.0 / divFactor,
                              top: mainReginRelative,
                              relativesize: minorRegionRelative,
                              priority: 1.0});
            }
            result.push({region: regions});
        }

        if (maxInput > 14) { // for maxInput: 17, 21, 25
            var maxDiv = (maxInput + 3) / 4;
            maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
            maxDiv = maxDiv > 7 ? 7 : maxDiv;

            for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                var mainReginRelative = ((divFactor - 2) * 1.0 / divFactor);
                var minorRegionRelative = 1.0 / divFactor;

                var regions = [{id: '1', left: 0, top: 0, relativesize: mainReginRelative, priority: 1.0}];
                var id = 2;
                for (var y = 0; y < divFactor - 1; y++) {
                    regions.push({id: ''+(id++),
                                  left: mainReginRelative,
                                  top: y*1.0 / divFactor,
                                  relativesize: minorRegionRelative,
                                  priority: 1.0});
                }

                for (var x = divFactor - 3; x >= 0; x--) {
                    regions.push({id: ''+(id++),
                                  left: x*1.0 / divFactor,
                                  top: mainReginRelative,
                                  relativesize: minorRegionRelative,
                                  priority: 1.0});
                }

                for (var y = 0; y < divFactor; y++) {
                    regions.push({id: ''+(id++),
                                  left: mainReginRelative + minorRegionRelative,
                                  top: y*1.0 / divFactor,
                                  relativesize: minorRegionRelative,
                                  priority: 1.0});
                }

                for (var x = divFactor - 2; x >= 0; x--) {
                    regions.push({id: ''+(id++),
                                  left: x*1.0 / divFactor,
                                  top: mainReginRelative + minorRegionRelative,
                                  relativesize: minorRegionRelative,
                                  priority: 1.0});
                }
                result.push({region: regions});
            }
        }
    }
    return result;
}

function generateFluidTemplates (maxInput) {
    'use strict';

    var result = [];
    var maxDiv = Math.sqrt(maxInput);
    if (maxDiv > Math.floor(maxDiv))
        maxDiv = Math.floor(maxDiv) + 1;
    else
        maxDiv = Math.floor(maxDiv);

    for (var divFactor = 1; divFactor <= maxDiv; divFactor++) {
        var regions = [];
        var relativeSize = 1.0 / divFactor;
        var id = 1;
        for (var y = 0; y < divFactor; y++)
            for(var x = 0; x < divFactor; x++) {
                var region = {id: String(id++),
                              left: x*1.0 / divFactor,
                              top: y*1.0 / divFactor,
                              relativesize: relativeSize,
                              priority: 1.0};
                regions.push(region);
            }

        result.push({region: regions});
    }
    return result;
}

function isTemplatesValid (templates) {
    'use strict';
    if (!(templates instanceof Array)) {
        return false;
    }

    for (var i in templates) {
        var region = templates[i].region;
        if (!(region instanceof Array))
            return false;

        for (var j in region) {
            if (((typeof region[j].left) !== 'number') || region[j].left < 0.0 || region[j].left > 1.0 ||
              ((typeof region[j].top) !== 'number') || region[j].top < 0.0 || region[j].top > 1.0 ||
              ((typeof region[j].relativesize) !== 'number') || region[j].relativesize < 0.0 ||
              region[j].relativesize > 1.0)
                return false;
        }
    }
    return true;
}
