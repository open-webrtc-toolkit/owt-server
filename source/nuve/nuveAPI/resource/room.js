/* global module, require */
'use strict';
var meta = require('../public/meta.json');
var log = require('./../logger').logger.getLogger('Room');

function defaultMediaMixing () {
    return {
        video: {
            avCoordinated: 0,
            maxInput: 16,
            resolution: 'vga',
            bitrate: 0,
            bkColor: 'black',
            layout: {
                base: 'fluid',
                custom: []
            }
        },
        audio: null
    };
}

function Room (spec) {
    this.name = spec.name + ''; // ensure type string
    this.mode = spec.mode;
    this.publishLimit = spec.publishLimit;
    this.userLimit = spec.userLimit;
    this.mediaMixing = spec.mediaMixing;
    this.enableMixing = spec.enableMixing;
}

Room.prototype.validate = function() {
    if (this.mode === undefined || this.mode === null || this.mode === '') {
        this.mode = 'hybrid';
    } else if (typeof this.mode === 'string') {
        this.mode = this.mode.toLowerCase();
        if (meta.mode.indexOf(this.mode) === -1) {
            return null;
        }
    } else {
        return null;
    }

    if (this.publishLimit === undefined || this.publishLimit === null) {
        this.publishLimit = -1;
    } else if (typeof this.publishLimit === 'string') {
        this.publishLimit = parseInt(this.publishLimit, 10);
        if (isNaN(this.publishLimit) || this.publishLimit < -1) {
            return null;
        }
    } else if (typeof this.publishLimit !== 'number' || this.publishLimit < -1) {
        return null;
    }

    if (this.userLimit === undefined || this.userLimit === null) {
        this.userLimit = -1;
    } else if (typeof this.userLimit === 'string') {
        this.userLimit = parseInt(this.userLimit, 10);
        if (isNaN(this.userLimit) || this.userLimit < -1) {
            return null;
        }
    } else if (typeof this.userLimit !== 'number' || this.userLimit < -1) {
        return null;
    }

    if (typeof this.mediaMixing === 'string') {
        try {
            this.mediaMixing = JSON.parse(this.mediaMixing);
        } catch (e) {
            return null;
        }
    }

    if (this.mediaMixing === undefined || this.mediaMixing === null) {
        this.mediaMixing = defaultMediaMixing();
        return this;
    }
    if (typeof this.mediaMixing !== 'object') {
        return null;
    }

    if (this.mediaMixing.video === undefined || this.mediaMixing.video === null) {
        this.mediaMixing.video = defaultMediaMixing().video;
    } else if (typeof this.mediaMixing.video === 'object') {
        if (this.mediaMixing.video.avCoordinated === '1' ||
            this.mediaMixing.video.avCoordinated === 1 ||
            this.mediaMixing.video.avCoordinated === true) {
            this.mediaMixing.video.avCoordinated = 1;
        } else {
            this.mediaMixing.video.avCoordinated = 0;
        }

        if (this.mediaMixing.video.maxInput === undefined ||
            this.mediaMixing.video.maxInput === null) {
            this.mediaMixing.video.maxInput = 16;
        } else if (typeof this.mediaMixing.video.maxInput === 'string') {
            this.mediaMixing.video.maxInput = parseInt(this.mediaMixing.video.maxInput, 10);
            if (isNaN(this.mediaMixing.video.maxInput) ||
                this.mediaMixing.video.maxInput <= 0) {
                return null;
            }
        } else if (typeof this.mediaMixing.video.maxInput !== 'number' ||
            this.mediaMixing.video.maxInput <= 0) {
            return null;
        }

        if (this.mediaMixing.video.resolution === undefined ||
            this.mediaMixing.video.resolution === null ||
            this.mediaMixing.video.resolution === '') {
            this.mediaMixing.video.resolution = 'vga';
        } else if (typeof this.mediaMixing.video.resolution !== 'string') {
            return null;
        } else {
            var resolution = this.mediaMixing.video.resolution.toLowerCase();
            if (meta.mediaMixing.video.resolution.indexOf(resolution) === -1) {
                return null;
            }
            this.mediaMixing.video.resolution = resolution;
        }

        if (this.mediaMixing.video.bitrate === undefined ||
            this.mediaMixing.video.bitrate === null) {
            this.mediaMixing.video.bitrate = 0;
        } else if (typeof this.mediaMixing.video.bitrate === 'string') {
            this.mediaMixing.video.bitrate = parseInt(this.mediaMixing.video.bitrate, 10);
            if (isNaN(this.mediaMixing.video.bitrate) || this.mediaMixing.video.bitrate < 0) {
                return null;
            }
        } else if (typeof this.mediaMixing.video.bitrate !== 'number' ||
            this.mediaMixing.video.bitrate < 0) {
            return null;
        }

        if (this.mediaMixing.video.bkColor === undefined ||
            this.mediaMixing.video.bkColor === null ||
            this.mediaMixing.video.bkColor === '') {
            this.mediaMixing.video.bkColor = 'black';
        } else if (typeof this.mediaMixing.video.bkColor !== 'string') {
            return null;
        } else {
            var bkColor = this.mediaMixing.video.bkColor.toLowerCase();
            if (meta.mediaMixing.video.bkColor.indexOf(bkColor) === -1) {
                return null;
            }
            this.mediaMixing.video.bkColor = bkColor;
        }

        if (this.mediaMixing.video.layout === undefined ||
            this.mediaMixing.video.layout === null) {
            this.mediaMixing.video.layout = defaultMediaMixing().video.layout;
        } else if (typeof this.mediaMixing.video.layout === 'object') {
            if (this.mediaMixing.video.layout.base === undefined ||
                this.mediaMixing.video.layout.base === null ||
                this.mediaMixing.video.layout.base === '') {
                this.mediaMixing.video.layout.base = 'fluid';
            } else if (typeof this.mediaMixing.video.layout.base !== 'string') {
                return null;
            } else {
                var layoutBase = this.mediaMixing.video.layout.base.toLowerCase();
                if (meta.mediaMixing.video.layout.base.indexOf(layoutBase) === -1) {
                    return null;
                }
                this.mediaMixing.video.layout.base = layoutBase;
            }
            if (this.mediaMixing.video.layout.custom === undefined ||
                this.mediaMixing.video.layout.custom === null ||
                this.mediaMixing.video.layout.custom === '') {
                this.mediaMixing.video.layout.custom = [];
            } else if (typeof this.mediaMixing.video.layout.custom === 'string') {
                try {
                    this.mediaMixing.video.layout.custom = JSON.parse(this.mediaMixing.video.layout.custom);
                } catch (e) {
                    return null;
                }
            }
            if (!isTemplatesValid(this.mediaMixing.video.layout.custom)) {
                return null;
            }
            if (this.mediaMixing.video.layout.base === 'void' &&
                this.mediaMixing.video.layout.custom.length === 0) {
                return null;
            }
        } else {
            return null;
        }
    } else {
        return null;
    }

    return this;
};

Room.create = function (spec) {
    return (new Room(spec)).validate();
};

Room.createDefault = function (name) {
    return Room.create({
        name: name,
        mode: 'hybrid',
        publishLimit: -1,
        userLimit: -1,
        mediaMixing: defaultMediaMixing()
    });
};

Room.prototype.toString = function () {
    return JSON.stringify(this);
};

Room.genConfig = function (room) {
    var layoutType = room.mediaMixing.video.layout.base;
    var maxInput = room.mediaMixing.video.maxInput || 16;
    var layoutTemplates;

    if (layoutType === 'fluid') {
        layoutTemplates = generateFluidTemplates(maxInput);
    } else if (layoutType === 'lecture') {
        layoutTemplates = generateLectureTemplates(maxInput);
    } else if (layoutType === 'void') {
        layoutTemplates = [];
    } else {
        layoutTemplates = generateFluidTemplates(maxInput);
    }

    var custom = room.mediaMixing.video.layout.custom;
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
    }

    return {
        mode: room.mode,
        publishLimit: room.publishLimit,
        userLimit: room.userLimit,
        enableMixing: room.enableMixing,
        mediaMixing: {
            video: {
                avCoordinated: room.mediaMixing.video.avCoordinated === 1 ? true : false,
                maxInput: maxInput,
                bitrate: room.mediaMixing.video.bitrate || 0,
                resolution: room.mediaMixing.video.resolution || 'vga',
                bkColor: room.mediaMixing.video.bkColor || 'black',
                layout: layoutTemplates
            },
            audio: null
        }
    };
};

Room.prototype.genConfig = function() {
    return Room.genConfig(this);
};

module.exports = Room;

/*
{
  "name": "roomName", // type string
  "mode": "hybrid", // type string
  "publishLimit": 16, // type number
  "userLimit": 100, // type number
  "mediaMixing": { // type object
    "video": {
      "resolution": "vga", // type string
      "bitrate": 0, // type numer
      "bkColor": "black", // type string
      "maxInput": 16, // type number
      "avCoordinated": 0, // type number: 0/1
      "layout": { // type object
        "base": "fluid", // type string
        "custom": [ // type object::Array or null
        ]
      }
    },
    "audio": null // type object
  }
}
*/

function generateLectureTemplates (maxInput) {
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
