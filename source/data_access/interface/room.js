/* global module, require */
'use strict';
var meta = require('./meta.json');
var log = require('./../../logger').logger.getLogger('Room');

function defaultMediaMixing () {
    return {
        video: {
            avCoordinated: 0,
            maxInput: 16,
            resolution: 'vga',
            multistreaming: 0,
            quality_level: 'standard',
            bkColor: 'black',
            crop: 0,
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
    this.views = spec.views;
    this.enableMixing = spec.enableMixing;
    this.sipInfo = spec.sipInfo;
}

function validateView(view) {
    if (typeof view.mediaMixing === 'string') {
        try {
            view.mediaMixing = JSON.parse(view.mediaMixing);
        } catch (e) {
            return null;
        }
    }

    if (view.mediaMixing === undefined || view.mediaMixing === null) {
        view.mediaMixing = defaultMediaMixing();
        return view;
    }
    if (typeof view.mediaMixing !== 'object') {
        return null;
    }

    if (view.mediaMixing.video === undefined || view.mediaMixing.video === null) {
        view.mediaMixing.video = defaultMediaMixing().video;
    } else if (typeof view.mediaMixing.video === 'object') {
        if (view.mediaMixing.video.avCoordinated === '1' ||
            view.mediaMixing.video.avCoordinated === 1 ||
            view.mediaMixing.video.avCoordinated === true) {
            view.mediaMixing.video.avCoordinated = 1;
        } else {
            view.mediaMixing.video.avCoordinated = 0;
        }

        if (view.mediaMixing.video.multistreaming === '1' ||
            view.mediaMixing.video.multistreaming === 1 ||
            view.mediaMixing.video.multistreaming === true) {
            view.mediaMixing.video.multistreaming = 1;
        } else {
            view.mediaMixing.video.multistreaming = 0;
        }

        if (view.mediaMixing.video.maxInput === undefined ||
            view.mediaMixing.video.maxInput === null) {
            view.mediaMixing.video.maxInput = 16;
        } else if (typeof view.mediaMixing.video.maxInput === 'string') {
            view.mediaMixing.video.maxInput = parseInt(view.mediaMixing.video.maxInput, 10);
            if (isNaN(view.mediaMixing.video.maxInput) ||
                view.mediaMixing.video.maxInput <= 0 ||
                view.mediaMixing.vidao.maxInput > 100/*FIXME: hard coded the up border of maxInputto 100*/) {
                return null;
            }
        } else if (typeof view.mediaMixing.video.maxInput !== 'number' ||
            view.mediaMixing.video.maxInput <= 0) {
            return null;
        }

        if (view.mediaMixing.video.resolution === undefined ||
            view.mediaMixing.video.resolution === null ||
            view.mediaMixing.video.resolution === '') {
            view.mediaMixing.video.resolution = 'vga';
        } else if (typeof view.mediaMixing.video.resolution !== 'string') {
            return null;
        } else {
            var resolution = view.mediaMixing.video.resolution.toLowerCase();
            if (meta.mediaMixing.video.resolution.indexOf(resolution) === -1) {
                return null;
            }
            view.mediaMixing.video.resolution = resolution;
        }

        if (typeof view.mediaMixing.video.quality_level !== 'string') {
            view.mediaMixing.video.quality_level = 'standard';
        } else {
            var quality_level = view.mediaMixing.video.quality_level.toLowerCase();
            if (meta.mediaMixing.video.quality_level.indexOf(quality_level) === -1) {
                view.mediaMixing.video.quality_level = 'standard';
            }
        }

        if (!view.mediaMixing.video.bkColor) {
            view.mediaMixing.video.bkColor = 'black';
        } else if (typeof view.mediaMixing.video.bkColor === 'object') {
            var rgbObj = view.mediaMixing.video.bkColor;

            if (!validateRGBValue(rgbObj.r) || !validateRGBValue(rgbObj.g) || !validateRGBValue(rgbObj.b)) {
                log.info('Invalid bkColor:', rgbObj);
                return null;
            }
        } else if (typeof view.mediaMixing.video.bkColor === 'string') {
            var bkColor = view.mediaMixing.video.bkColor.toLowerCase();
            if (meta.mediaMixing.video.bkColor.indexOf(bkColor) === -1) {
                log.info('Invalid string bkColor:', bkColor);
                return null;
            }
            view.mediaMixing.video.bkColor = bkColor;
        } else {
            log.info('Invalid bkColor:', view.mediaMixing.video.bkColor);
            return null;
        }

        if (view.mediaMixing.video.crop === '1' ||
            view.mediaMixing.video.crop === 1 ||
            view.mediaMixing.video.crop === true) {
            view.mediaMixing.video.crop = 1;
        } else {
            view.mediaMixing.video.crop = 0;
        }

        if (view.mediaMixing.video.layout === undefined ||
            view.mediaMixing.video.layout === null) {
            view.mediaMixing.video.layout = defaultMediaMixing().video.layout;
        } else if (typeof view.mediaMixing.video.layout === 'object') {
            if (view.mediaMixing.video.layout.base === undefined ||
                view.mediaMixing.video.layout.base === null ||
                view.mediaMixing.video.layout.base === '') {
                view.mediaMixing.video.layout.base = 'fluid';
            } else if (typeof view.mediaMixing.video.layout.base !== 'string') {
                return null;
            } else {
                var layoutBase = view.mediaMixing.video.layout.base.toLowerCase();
                if (meta.mediaMixing.video.layout.base.indexOf(layoutBase) === -1) {
                    return null;
                }
                view.mediaMixing.video.layout.base = layoutBase;
            }
            if (view.mediaMixing.video.layout.custom === undefined ||
                view.mediaMixing.video.layout.custom === null ||
                view.mediaMixing.video.layout.custom === '') {
                view.mediaMixing.video.layout.custom = [];
            } else if (typeof view.mediaMixing.video.layout.custom === 'string') {
                try {
                    view.mediaMixing.video.layout.custom = JSON.parse(view.mediaMixing.video.layout.custom);
                } catch (e) {
                    return null;
                }
            }
            if (!isTemplatesValid(view.mediaMixing.video.layout.custom)) {
                return null;
            }
            if (view.mediaMixing.video.layout.base === 'void' &&
                view.mediaMixing.video.layout.custom.length === 0) {
                return null;
            }
        } else {
            return null;
        }
    } else {
        return null;
    }

    return view;
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

    if (this.enableMixing === undefined || this.enableMixing === null) {
        this.enableMixing = 1;
    } else if (this.enableMixing === '1' || this.enableMixing === 1 || this.enableMixing === true) {
        this.enableMixing = 1;
    } else {
        this.enableMixing = 0;
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

    if (!this.views && !this.mediaMixing) {
        this.views = { "common": {} };
    }

    // Validate views
    if (typeof this.views === 'object') {
        for (var viewLabel in this.views) {
            if (validateView(this.views[viewLabel]) === null)
                return null;
        }
    } else {
        // Old style config
        if (validateView(this) === null)
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
        enableMixing: 1,
        views: { "common": { mediaMixing: defaultMediaMixing() } }
    });
};

Room.prototype.toString = function () {
    return JSON.stringify(this);
};

Room.genConfig = function (room) {

    var genMediaMixing = function(mediaMixing) {
        var layoutType = mediaMixing.video.layout.base;
        var maxInput = mediaMixing.video.maxInput || 16;
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

        var custom = mediaMixing.video.layout.custom;
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
            video: {
                avCoordinated: mediaMixing.video.avCoordinated === 1,
                multistreaming: mediaMixing.video.multistreaming === 1,
                maxInput: maxInput,
                quality_level: mediaMixing.video.quality_level || 'standard',
                resolution: mediaMixing.video.resolution || 'vga',
                bkColor: mediaMixing.video.bkColor || 'black',
                crop: mediaMixing.video.crop === 1,
                layout: layoutTemplates
            },
            audio: null
        };
    };

    var config = {
        mode: room.mode,
        publishLimit: room.publishLimit,
        userLimit: room.userLimit,
        enableMixing: room.enableMixing === 1,
    };

    if (room.enableMixing && room.views) {
        config.views = {};
        for (var view in room.views) {
            config.views[view] = { mediaMixing: genMediaMixing(room.views[view].mediaMixing) };
        }
    } else if (room.enableMixing && room.mediaMixing) {
        // Old style configuration
        config.views = { "common": { mediaMixing: genMediaMixing(room.mediaMixing) } };
    }

    if (room.sipInfo) {
        config.sip = room.sipInfo;
    }

    return config;
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
  "enableMixing": 1, // type number: 0/1
  "mediaMixing": { // type object
    "video": {
      "resolution": "vga", // type string
      "quality_level": "standard", // type string, enum in ["bestquality", "betterquality", "standard", "betterspeed", "bestspeed"]
      "bkColor": {  // or just use "black" with type string
        "r": 255, // type number, 0 ~ 255
        "g": 255, // type number, 0 ~ 255
        "b": 255  // type number, 0 ~ 255
      },
      "maxInput": 16, // type number
      "avCoordinated": 0, // type number: 0/1
      "multistreaming": 0, // type number: 0/1
      "crop": 0, // type number: 0/1
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

function validateRGBValue(val) {
    if (typeof val === 'number') {
        // NaN is not less than, equal to, or greater than any other number.
        if (val >= 0 && val <= 255) {
            return true;
        }
    }

    return false;
}

function Rational(num, den) {
    this.numerator = num;
    this.denominator = den;
}

const ZERO = new Rational(0, 1);
const ONE = new Rational(1, 1);
const ONE_THIRD = new Rational(1, 3);
const TWO_THIRDs = new Rational(2, 3);

function Rectangle({id = '', left, top, relativesize} = {}) {
    this.id = id;
    this.shape = 'rectangle',
    this.area = {
        left: left || ZERO,
        top: top || ZERO,
        width: relativesize || ZERO,
        height: relativesize || ZERO
    };
}

function generateLectureTemplates (maxInput) {
    var result = [
        {region:[new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: ONE })]},
        {region:[
            new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: TWO_THIRDs}),
            new Rectangle({id: '2', left: TWO_THIRDs, top: ZERO, relativesize: ONE_THIRD}),
            new Rectangle({id: '3', left: TWO_THIRDs, top: ONE_THIRD, relativesize: ONE_THIRD}),
            new Rectangle({id: '4', left: TWO_THIRDs, top: TWO_THIRDs, relativesize: ONE_THIRD}),
            new Rectangle({id: '5', left: ONE_THIRD, top: TWO_THIRDs, relativesize: ONE_THIRD}),
            new Rectangle({id: '6', left: ZERO, top: TWO_THIRDs, relativesize: ONE_THIRD})
        ]}
    ];
    if (maxInput > 6) { // for maxInput: 8, 10, 12, 14
        var maxDiv = maxInput / 2;
        maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
        maxDiv = maxDiv > 7 ? 7 : maxDiv;

        for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
            var mainReginRelative = new Rational(divFactor - 1, divFactor);
            var minorRegionRelative = new Rational(1, divFactor);

            var regions = [new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative})];
            var id = 2;
            for (var y = 0; y < divFactor; y++) {
                regions.push(new Rectangle({
                    id: ''+(id++),
                    left: mainReginRelative,
                    top: new Rational(y, divFactor),
                    relativesize: minorRegionRelative,
                }));
            }

            for (var x = divFactor - 2; x >= 0; x--) {
                regions.push(new Rectangle({
                    id: ''+(id++),
                    left: new Rational(x, divFactor),
                    top: mainReginRelative,
                    relativesize: minorRegionRelative,
                }));
            }
            result.push({region: regions});
        }

        if (maxInput > 14) { // for maxInput: 17, 21, 25
            var maxDiv = (maxInput + 3) / 4;
            maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
            maxDiv = maxDiv > 7 ? 7 : maxDiv;

            for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                var mainReginRelative = new Rational(divFactor - 2, divFactor);
                var minorRegionRelative = new Rational(1, divFactor);

                var regions = [new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative})];
                var id = 2;
                for (var y = 0; y < divFactor - 1; y++) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: mainReginRelative,
                        top: new Rational(y, divFactor),
                        relativesize: minorRegionRelative,
                    }));
                }

                for (var x = divFactor - 3; x >= 0; x--) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: new Rational(x, divFactor),
                        top: mainReginRelative,
                        relativesize: minorRegionRelative,
                    }));
                }

                for (var y = 0; y < divFactor; y++) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: new Rational(divFactor - 1, divFactor),
                        top: new Rational(y, divFactor),
                        relativesize: minorRegionRelative
                    }));
                }

                for (var x = divFactor - 2; x >= 0; x--) {
                    regions.push(new Rectangle({
                        id: ''+(id++),
                        left: new Rational(x, divFactor),
                        top: new Rational(divFactor - 1, divFactor),
                        relativesize: minorRegionRelative
                    }));
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
        var relativeSize = new Rational(1, divFactor);
        var id = 1;
        for (var y = 0; y < divFactor; y++)
            for(var x = 0; x < divFactor; x++) {
                var region = new Rectangle({id: String(id++),
                              left: new Rational(x, divFactor),
                              top: new Rational(y, divFactor),
                              relativesize: relativeSize});
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
            if (!region[j].area || !region[j].shape) {
                //FIXME: to make old layout configuration work
                if (region[j].top && region[j].left && region[j].relativesize) {
                    region[j].area = {
                        left: region[j].left,
                        top: region[j].top,
                        width: region[j].relativesize,
                        height: region[j].relativesize
                    };
                    region[j].shape = 'rectangle';
                    delete region[j].top;
                    delete region[j].left;
                    delete region[j].relativesize;
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}