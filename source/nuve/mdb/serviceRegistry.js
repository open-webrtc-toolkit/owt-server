/*global require, exports*/
'use strict';
var db = require('./dataBase').db;
var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('ServiceRegistry');

/*
 * Gets a list of the services in the data base.
 */
exports.getList = function (callback) {
    db.services.find({}).toArray(function (err, services) {
        if (err || !services) {
            log.info('Empty list');
        } else {
            callback(services);
        }
    });
};

var getService = exports.getService = function (id, callback) {
    var serviceId;
    try {
        serviceId = db.ObjectId(id);
    } catch (e) {
        log.error('Get service Error:', e);
        if (typeof callback === 'function') {
            callback(undefined);
        }
        return;
    }
    db.services.findOne({_id: serviceId}, function (err, service) {
        if (service === undefined || service === null) {
            log.info('Service not found');
        }
        if (typeof callback === 'function') {
            callback(service);
        }
    });
};

var hasService = exports.hasService = function (id, callback) {
    getService(id, function (service) {
        if (service === undefined) {
            callback(false);
        } else {
            callback(true);
        }
    });
};

/*
 * Adds a new service to the data base.
 */
exports.addService = function (service, callback) {
    service.rooms = [];
    db.services.save(service, function (error, saved) {
        if (error) log.info('MongoDB: Error adding service: ', error);
        callback(saved);
    });
};

/*
 * Updates a determined service in the data base.
 */
exports.updateService = function (service, on_ok, on_error) {
    db.services.save(service, function (error, saved) {
        if (error || !saved) {
            log.info('MongoDB: Error updating service: ', error);
            if (typeof on_error === 'function') on_error('Error updating service.');
        } else {
            if (typeof on_ok === 'function') on_ok();
        }
    });
};

/*
 * Removes a determined service from the data base.
 */
exports.removeService = function (id) {
    hasService(id, function (hasS) {
        if (hasS) {
            db.services.remove({_id: db.ObjectId(id)}, function (error, saved) {
                if (error) log.info('MongoDB: Error removing service: ', error);
            });
        }
    });
};

/*
 * Gets a determined room in a determined service. Returns undefined if room does not exists. 
 */
exports.getRoomForService = function (roomId, service, callback) {
    var room;
    for (room in service.rooms) {
        if (service.rooms.hasOwnProperty(room)) {
            if (String(service.rooms[room]._id) === String(roomId)) {
                callback(service.rooms[room]);
                return;
            }
        }
    }
    callback(undefined);
};


/*
 * Update a single room in service.
 * Our service collection also has room data same as room collection.
 * One update service request would update the room array inside service collection.
 * Then concurrent requests cause problems.
 * We may remove room array in service collection later?
 */
exports.updateRoomInService = function (serviceId, room, callback) {
    db.collection('services').update(
        { // Query
            '_id' : serviceId,
            'rooms._id' : room._id,
        },
        { // Update
            '$set' : {'rooms.$' : room }
        },
        // Callback
        function(err, result) {
            if (err) {
                log.error('Update Service Room Failed.');
                callback(err, null);
            } else {
                callback(err, result);
            }
        }
    );
};

// Add a single room in service
exports.addRoomInService = function (serviceId, room, callback) {
    db.collection('services').update(
        { // Query
            '_id' : serviceId,
        },
        { // Update
            '$push' : {'rooms' : room }
        },
        // Callback
        function(err, result) {
            if (err) {
                log.error('Add Service Room Failed.');
                callback(err, null);
            } else {
                callback(err, result);
            }
        }
    );
};

// Delete a single room in service
exports.deleteRoomInService = function (serviceId, room, callback) {
    db.collection('services').update(
        { // Query
            '_id' : serviceId,
        },
        { // Update
            '$pull' : {'rooms' : {'_id' : room._id } }
        },
        // Callback
        function(err, result) {
            if (err) {
                log.error('Delete Service Room Failed.');
                callback(err, null);
            } else {
                callback(err, result);
            }
        }
    );
};
