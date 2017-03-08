/*global require, exports*/
'use strict';
var logger = require('./logger').logger;

// Logger
var log = logger.getLogger('Matcher');

var is_isp_applicable = function (supported_isps, preferred_isp) {
    return supported_isps.indexOf(preferred_isp) !== -1 || supported_isps.length === 0;
};

var is_region_suited = function (supported_regions, preferred_region) {
    return supported_regions.indexOf(preferred_region) !== -1;
};

var portalMatcher = function () {
    this.match = function (preference, workers, candidates) {
        var result = [],
            found_sweet = false;
        for (var i in candidates) {
            var id = candidates[i];
            var capacity = workers[id].info.capacity;
            if (is_isp_applicable(capacity.isps, preference.isp)) {
                if (is_region_suited(capacity.regions, preference.region)) {
                    if (!found_sweet) {
                        found_sweet = true;
                        result = [id];
                    } else {
                        result.push(id);
                    }
                } else {
                    if (!found_sweet) {
                        result.push(id);
                    }
                }
            }
        }
        return result;
    };
};

var webrtcMatcher = function () {
    this.match = function (preference, workers, candidates) {
        var result = [],
            found_sweet = false;
        for (var i in candidates) {
            var id = candidates[i];
            var capacity = workers[id].info.capacity;
            if (is_isp_applicable(capacity.isps, preference.isp)) {
                if (is_region_suited(capacity.regions, preference.region)) {
                    if (!found_sweet) {
                        found_sweet = true;
                        result = [id];
                    } else {
                        result.push(id);
                    }
                } else {
                    if (!found_sweet) {
                        result.push(id);
                    }
                }
            }
        }
        return result;
    };
};

var generalMatcher = function () {
    this.match = function (preference, workers, candidates) {
        return candidates;
    };
};

var audioMatcher = generalMatcher;
var videoMatcher = generalMatcher;

exports.create = function (purpose) {
    switch (purpose) {
        case 'portal':
            return new portalMatcher();
        case 'session':
            return new generalMatcher();
        case 'webrtc':
            return new webrtcMatcher();
        case 'recording':
            return new generalMatcher();
        case 'avstream':
            return new generalMatcher();
        case 'sip':
            return new generalMatcher();
        case 'audio':
            return new audioMatcher();
        case 'video':
            return new videoMatcher();
        default:
            log.warn('Invalid specified purpose:', purpose, ', apply general-matcher instead.');
            return new generalMatcher();
    }
};
