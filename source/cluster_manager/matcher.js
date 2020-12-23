// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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


var videoMatcher = function () {
    this.match = function (preference, workers, candidates) {
        if (!preference || !preference.video)
            return candidates;

        var formatContain = function (listA, listB) {
            var count = 0;
            listB.forEach((fmtB) => {
                if (listA.indexOf(fmtB) > -1)
                    count++;
            });
            return (count === listB.length);
        };

        var result = [],
	    found_sweet = false;
        var availableCandidates = candidates.filter(function(cid) {
            var capacity = workers[cid].info.capacity;
            var encodeOk = false;
            var decodeOk = false;
            if (capacity.video) {
                encodeOk = formatContain(capacity.video.encode, preference.video.encode);
                decodeOk = formatContain(capacity.video.decode, preference.video.decode);
            }
            if (!encodeOk) {
                log.warn('No available workers for encoding:', JSON.stringify(preference.video.encode));
            }
            if (!decodeOk) {
                log.warn('No available workers for decoding:', JSON.stringify(preference.video.decode));
            }
            return (encodeOk && decodeOk);
        });

        for (var i in availableCandidates) {
            var id = availableCandidates[i];
            var capacity = workers[id].info.capacity;
            if (is_isp_applicable(capacity.isps, preference.origin.isp)) {
                if (is_region_suited(capacity.regions, preference.origin.region)) {
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

var analyticsMatcher = function() {
    this.match = function (preference, workers, candidates) {
        if (!preference || !preference.algorithm)
            return candidates;

        var result = candidates.filter(function(cid) {
            var capacity = workers[cid].info.capacity;
            return capacity.algorithms.includes(preference.algorithm);
        });

        if (result.length === 0) {
            log.warn('No available workers for analytics:', preference.algorithm);
        }
        return result;
    }
}

var generalMatcher = function () {
    this.match = function (preference, workers, candidates) {
        return candidates;
    };
};

var audioMatcher = function () {
    this.match = function (preference, workers, candidates) {
        var result = [],
            found_sweet = false;
        for (var i in candidates) {
            var id = candidates[i];
            var capacity = workers[id].info.capacity;
            if (is_isp_applicable(capacity.isps, preference.origin.isp)) {
                if (is_region_suited(capacity.regions, preference.origin.region)) {
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

var streamingMatcher = webrtcMatcher;
var portalMatcher = webrtcMatcher;

exports.create = function (purpose) {
    switch (purpose) {
        case 'portal':
            return new portalMatcher();
        case 'conference':
            return new generalMatcher();
        case 'webrtc':
            return new webrtcMatcher();
        case 'recording':
            return new generalMatcher();
        case 'streaming':
            return new streamingMatcher();
        case 'sip':
            return new generalMatcher();
        case 'audio':
            return new audioMatcher();
        case 'video':
            return new videoMatcher();
        case 'analytics':
            return new analyticsMatcher();
        case 'quic':
            return new generalMatcher();
        default:
            log.warn('Invalid specified purpose:', purpose, ', apply general-matcher instead.');
            return new generalMatcher();
    }
};

