/*global exports, require, console, Buffer, setTimeout, clearTimeout*/
var tokenRegistry = require('./../mdb/tokenRegistry');
var serviceRegistry = require('./../mdb/serviceRegistry');
var cloudHandler = require('./../cloudHandler');
var logger = require('../logger').logger;

// Logger
var log = logger.getLogger("RPCPublic");

/*
 * This function is used to consume a token. Removes it from the data base and returns to erizoController.
 * Also it removes old tokens.
 */
exports.deleteToken = function (id, callback) {
    "use strict";

    tokenRegistry.removeOldTokens();

    tokenRegistry.getToken(id, function (token) {

        if (token === undefined) {
            callback('callback', 'error');
        } else {

            if (token.use !== undefined) {
                //Is a test token
                var time = ((new Date()).getTime()) - token.creationDate.getTime(),
                    s;
                if (token.use > 490) { // || time*1000 > 3600*24*30) {
                    s = token.service;
                    serviceRegistry.getService(s, function (service) {
                        delete service.testToken;
                        serviceRegistry.updateService(service);
                        tokenRegistry.removeToken(id, function () {
                            log.info('TestToken expiration time. Deleting ', token._id, 'from room ', token.room, ' of service ', token.service);
                            callback('callback', 'error');
                        });

                    });
                } else {
                    token.use += 1;
                    tokenRegistry.updateToken(token);
                    log.info('Using (', token.use, ') testToken ', token._id, 'for testRoom ', token.room, ' of service ', token.service);
                    callback('callback', token);
                }

            } else {
                tokenRegistry.removeToken(id, function () {
                    log.info('Consumed token ', token._id, 'from room ', token.room, ' of service ', token.service);
                    callback('callback', token);
                });
            }
        }
    });
};

exports.getRoomConfig = function (id, callback) {
    var generateFluidTemplates = function(maxInput) {
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
                    var region = {"id": String(id++),
                                  "left": x*1.0 / divFactor,
                                  "top": y*1.0 / divFactor,
                                  "relativesize": relativeSize,
                                  "priority": 1.0};
                    regions.push(region);
                }

            result.push({"region": regions});
        }
        return result;
    };

    var generateLectureTemplates = function(maxInput) {
        var result = [ {"region":[{"id": "1", "left": 0, "top": 0, "relativesize": 1.0, "priority": 1.0}]},
                       {"region":[{"id": "1", "left": 0, "top": 0, "relativesize": 0.667, "priority": 1.0},
                                  {"id": "2", "left": 0.667, "top": 0, "relativesize": 0.333, "priority": 1.0},
                                  {"id": "3", "left": 0.667, "top": 0.333, "relativesize": 0.333, "priority": 1.0},
                                  {"id": "4", "left": 0.667, "top": 0.667, "relativesize": 0.333, "priority": 1.0},
                                  {"id": "5", "left": 0.333, "top": 0.667, "relativesize": 0.333, "priority": 1.0},
                                  {"id": "6", "left": 0, "top": 0.667, "relativesize": 0.333, "priority": 1.0}]}];
        if (maxInput > 6) { // for maxInput: 8, 10, 12, 14
            var maxDiv = maxInput / 2;
            maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
            maxDiv = maxDiv > 7 ? 7 : maxDiv;

            for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                var inputCount = divFactor * 2;
                var mainReginRelative = ((divFactor - 1) * 1.0 / divFactor);
                var minorRegionRelative = 1.0 / divFactor;

                var regions = [{"id": "1", "left": 0, "top": 0, "relativesize": mainReginRelative, "priority": 1.0}];
                var id = 2;
                for (var y = 0; y < divFactor; y++) {
                    var region = {"id": String(id++),
                                  "left": mainReginRelative,
                                  "top": y*1.0 / divFactor,
                                  "relativesize": minorRegionRelative,
                                  "priority": 1.0};
                    regions.push(region);
                }

                for (var x = divFactor - 2; x >= 0; x--) {
                    var region = {"id": String(id++),
                                  "left": x*1.0 / divFactor,
                                  "top": mainReginRelative,
                                  "relativesize": minorRegionRelative,
                                  "priority": 1.0};
                    regions.push(region);
                }
                result.push({"region": regions});
            }

            if (maxInput > 14) { // for maxInput: 17, 21, 25
                var maxDiv = (maxInput + 3) / 4;
                maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
                maxDiv = maxDiv > 7 ? 7 : maxDiv;

                for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
                    var inputCount = divFactor * 4 - 3;
                    var mainReginRelative = ((divFactor - 2) * 1.0 / divFactor);
                    var minorRegionRelative = 1.0 / divFactor;

                    var regions = [{"id": "1", "left": 0, "top": 0, "relativesize": mainReginRelative, "priority": 1.0}];
                    var id = 2;
                    for (var y = 0; y < divFactor - 1; y++) {
                        var region = {"id": String(id++),
                                      "left": mainReginRelative,
                                      "top": y*1.0 / divFactor,
                                      "relativesize": minorRegionRelative,
                                      "priority": 1.0};
                        regions.push(region);
                    }

                    for (var x = divFactor - 3; x >= 0; x--) {
                        var region = {"id": String(id++),
                                      "left": x*1.0 / divFactor,
                                      "top": mainReginRelative,
                                      "relativesize": minorRegionRelative,
                                      "priority": 1.0};
                        regions.push(region);
                    }

                    for (var y = 0; y < divFactor; y++) {
                        var region = {"id": String(id++),
                                      "left": mainReginRelative + minorRegionRelative,
                                      "top": y*1.0 / divFactor,
                                      "relativesize": minorRegionRelative,
                                      "priority": 1.0};
                        regions.push(region);
                    }

                    for (var x = divFactor - 2; x >= 0; x--) {
                        var region = {"id": String(id++),
                                      "left": x*1.0 / divFactor,
                                      "top": mainReginRelative + minorRegionRelative,
                                      "relativesize": minorRegionRelative,
                                      "priority": 1.0};
                        regions.push(region);
                    }
                    result.push({"region": regions});
                }
            }
        }
        return result;
    };

    var isTemplatesValid = function(templates) {
        if (templates === undefined)
            return false;

        for (var i in templates) {
            var region = templates[i].region;
            if (!(region instanceof Array))
                return false;

            for (var j in region) {
                if (((typeof region[j].left) !== "number") || region[j].left < 0.0 || region[j].left > 1.0
                    || ((typeof region[j].top) !== "number") || region[j].top < 0.0 || region[j].top > 1.0
                    || ((typeof region[j].relativesize) !== "number") || region[j].relativesize < 0.0 || region[j].relativesize > 1.0)
                    return false;
            }
        }
        return true;
    };

    var layoutType = "fluid";
    var maxInput = 16;
    var layoutTemplates = {};

    if (layoutType === "fluid") {
        layoutTemplates = generateFluidTemplates(maxInput);
    } else if (layoutType === "lecture") {
        layoutTemplates = generateLectureTemplates(maxInput);
    } else {
        layoutTemplates = generateFluidTemplates(maxInput);
    }

    var config = {video: {avCoordinated:false,
                          maxInput:maxInput,
                          resolution:"vga",
                          bkColor: "black",
                          layout: layoutTemplates
                         }
                  };

    callback('callback', config);
}

exports.addNewErizoController = function(msg, callback) {
    cloudHandler.addNewErizoController(msg, function (id) {
        callback('callback', id);   
    });
}

exports.keepAlive = function(id, callback) {
    cloudHandler.keepAlive(id, function(result) {
        callback('callback', result);
    });
}

exports.setInfo = function(params, callback) {
    cloudHandler.setInfo(params);
    callback('callback');
}

exports.killMe = function(ip, callback) {
    cloudHandler.killMe(ip);
    callback('callback');
}
