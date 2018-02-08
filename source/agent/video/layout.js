'use strict';
require = require('module')._load('./AgentLoader');

/**
 * NodeJs LayoutProcessor
 *
 * @typedef LayoutSolution
 * @type {object}
 * @property {number} input - input ID.
 * @property {Region} region - the region.
 * Native layer takes this object to update layout.
 */

// Logger
const log = require('./logger').logger.getLogger('LayoutProcessor');
const util = require('util');
const EventEmitter = require('events').EventEmitter;


/**
 * @typedef Rational
 * @type {object}
 * @property {number} numerator
 * @property {number} denominator
 */
/**
 * @typedef Rectangle
 * @type {object}
 * @property {Rational} left
 * @property {Rational} top
 * @property {Rational} height
 * @property {Rational} width
 */

/**
 * @constructor Region
 * @param {object} options
 * @property {string} options.id
 * @property {string} options.shape 'rectangle'
 * @property {object} options.area
 */
function Region(options = {}) {
    this.id = options.id;
    if (options.area && options.shape) {
        this.shape = options.shape;
        this.area = options.area;
    }
}

/**
 * @function validate
 * @return {boolean}
 */
Region.prototype.validate = function() {
    return true;
};

/**
 * @constructor LayoutProcessor
 * @extends EventEmitter
 * @param {Region[][]} layouts: the array of layouts templates
 */
function LayoutProcessor(layouts) {
    EventEmitter.call(this);
    this.templates = {};
    this.maxCover = 0;
    this.inputList = [];
    this.currentRegions = [];

    var self = this;
    var numOfRegions;
    var validRegions;
    var region;

    // Parse templates
    if (Array.isArray(layouts)) {
        layouts.forEach(function(layout) {
            numOfRegions = layout.region.length;
            validRegions = [];
            layout.region.forEach(function(regObj) {
                region = new Region(regObj);
                if (region.validate()) {
                    validRegions.push(region);
                }
            });

            self.templates[numOfRegions] = validRegions;
            if (self.maxCover < numOfRegions) self.maxCover = numOfRegions;
        });
    } else {
        this.emit('error', new Error('Invalid layout'));
    }
}
util.inherits(LayoutProcessor, EventEmitter);

/**
 * @function addInput
 * @param {number} inputId: input ID
 * @param {boolean} front: whether add to front
 * @return {void}
 */
LayoutProcessor.prototype.addInput = function(inputId, front) {
    var oldPos = this.inputList.indexOf(inputId);
    if (oldPos >= 0) {
        // Already exists
        return;
    }

    if (front) {
        this.inputList.unshift(inputId);
    } else {
        this.inputList.push(inputId);
    }
    this.choseTemplate(this.inputList.length);
    this.updateInputPositions();
};

/**
 * @function removeInput
 * @param {number} inputId: inputID
 * @return {void}
 */
LayoutProcessor.prototype.removeInput = function(inputId) {
    var pos = this.inputList.indexOf(inputId);
    if (pos >= 0) {
        this.inputList.splice(pos, 1);
        this.choseTemplate(this.inputList.length);
        this.updateInputPositions();
    }
};

/**
 * @function specifyInputRegion
 * @param {number} inputId: inputID
 * @param {string} regionId: regionID
 * @return {boolean}
 */
LayoutProcessor.prototype.specifyInputRegion = function(inputId, regionId) {
    var inputPos;
    var regionPos;
    var tmpInput;
    inputPos = this.inputList.indexOf(inputId);
    for (regionPos = 0; regionPos < this.currentRegions.length; regionPos++) {
        if (this.currentRegions[regionPos].id === regionId) break;
    }

    if (inputPos === regionPos) {
        // Already match input - region
        return true;
    }

    if (inputPos >= 0 && regionPos < this.currentRegions.length
            && regionPos < this.inputList.length) {
        // Swap in inputList
        tmpInput = this.inputList[inputPos];
        this.inputList[inputPos] = this.inputList[regionPos];
        this.inputList[regionPos] = tmpInput;
        this.updateInputPositions();
        return true;
    }
    return false;
};

/**
 * @function setPrimary
 * @param {number} inputId: inputID
 */
LayoutProcessor.prototype.setPrimary = function(inputId) {
    var inputPos;
    var tmpInput;
    inputPos = this.inputList.indexOf(inputId);
    if (inputPos <= 0 || this.inputList.length <= 1) {
        // No input or at head
        return;
    }

    // Swap with first in inputList
    tmpInput = this.inputList[inputPos];
    this.inputList[inputPos] = this.inputList[0];
    this.inputList[0] = tmpInput;
    this.updateInputPositions();
};

/**
 * @function getRegion
 * @param {number} inputId: inputID
 * @return {Region}
 */
LayoutProcessor.prototype.getRegion = function(inputId) {
    var inputPos = this.inputList.indexOf(inputId);
    if (inputPos >= 0) {
        return this.currentRegions[inputPos];
    }
    return undefined;
};

/**
 * @function choseTemplate
 * @param {number} regionNum
 * @return {void}
 */
LayoutProcessor.prototype.choseTemplate = function(regionNum) {
    var cand;
    for (cand = regionNum; cand <= this.maxCover; cand++) {
        if (this.templates[cand]) {
            this.currentRegions = this.templates[cand];
            break;
        }
    }
};

/**
 * @function updateInputPositions
 * @param {void}
 * @return {void}
 */
LayoutProcessor.prototype.updateInputPositions = function() {
    var i;
    var layoutSolution = [];
    for (i = 0; i < this.currentRegions.length; i++) {
        if (typeof this.inputList[i] === 'number') {
            layoutSolution.push({
                input: this.inputList[i],
                region: this.currentRegions[i]
            });
        }
    }
    // Trigger "layoutChange" event
    this.emit('layoutChange', layoutSolution);
};

/**
 * @function getRegions
 * @param {void}
 * @return {Region[]}
 */
LayoutProcessor.prototype.getRegions = function() {
    return this.currentRegions;
};

exports.LayoutProcessor = LayoutProcessor;
