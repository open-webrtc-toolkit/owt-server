/*
 * Class EventDispatcher provides event handling to sub-classes.
 * It is inherited from Publisher, Room, etc.
 */
Erizo.EventDispatcher = function (spec) {
    "use strict";
    var that = {};
    // Private vars
    spec.dispatcher = {};
    spec.dispatcher.eventListeners = {};

    // Public functions

    // It adds an event listener attached to an event type.
    that.addEventListener = function (eventType, listener) {
        if (spec.dispatcher.eventListeners[eventType] === undefined) {
            spec.dispatcher.eventListeners[eventType] = [];
        }
        spec.dispatcher.eventListeners[eventType].push(listener);
    };

    that.on = that.addEventListener;

    // It removes an available event listener.
    that.removeEventListener = function (eventType, listener) {
        var index;
        index = spec.dispatcher.eventListeners[eventType].indexOf(listener);
        if (index !== -1) {
            spec.dispatcher.eventListeners[eventType].splice(index, 1);
        }
    };

    // It dispatch a new event to the event listeners, based on the type 
    // of event. All events are intended to be LicodeEvents.
    that.dispatchEvent = function (event) {
        var listener;
        L.Logger.debug("Event: " + event.type);
        for (listener in spec.dispatcher.eventListeners[event.type]) {
            if (spec.dispatcher.eventListeners[event.type].hasOwnProperty(listener)) {
                spec.dispatcher.eventListeners[event.type][listener](event);
            }
        }
    };

    return that;
};

// **** EVENTS ****

/*
 * Class LicodeEvent represents a generic Event in the library.
 * It handles the type of event, that is important when adding
 * event listeners to EventDispatchers and dispatching new events. 
 * A LicodeEvent can be initialized this way:
 * var event = LicodeEvent({type: "room-connected"});
 */
Erizo.LicodeEvent = function (spec) {
    "use strict";
    var that = {};

    // Event type. Examples are: 'room-connected', 'stream-added', etc.
    that.type = spec.type;

    return that;
};

/*
 * Class StreamEvent represents an event related to a stream. It is a LicodeEvent.
 * It is usually initialized this way:
 * var streamEvent = StreamEvent({type:"stream-added", stream:stream1});
 * Event types:
 * 'stream-added' - indicates that there is a new stream available in the room.
 * 'stream-removed' - shows that a previous available stream has been removed from the room.
 */
Erizo.StreamEvent = function (spec) {
    "use strict";
    var that = Erizo.LicodeEvent(spec);

    // The stream related to this event.
    that.stream = spec.stream;

    that.msg = spec.msg;

    return that;
};

/*
 * Class ClientEvent represents an event related to a client. It is a LicodeEvent.
 * It is usually initialized this way:
 * var clientEvent = ClientEvent({type:"client-left", user: user1, attr: attributes});
 * Event types:
 * 'client-connected' - points out that the user has been successfully connected.
 * 'client-disconnected' - shows that the user has been already disconnected.
 * 'client-joined' - indicates that there is a new client joined.
 * 'client-left' - indicates that a client has left.
 *
 * NOTE: 'client-connected' event shall always trigger 'client-joined' event,
 * while 'client-disconnected' event shall always trigger 'client-left' event;
 */
Erizo.ClientEvent = function (spec) {
  "use strict";
  var that = Erizo.LicodeEvent(spec);
  that.user = spec.user;
  that.attr = spec.attr;
  that.streams = spec.streams;
  return that;
};

/*
 * Class MediaEvent represents an event related to a getUserMedia(). It is a LicodeEvent.
 * It usually initializes as:
 * var mediaEvent = MediaEvent({})
 * Event types:
 * 'access-accepted' - indicates that the user has accepted to share his camera and microphone
 * 'warning' - details are included in the event message (msg)
 * 'error' - details are included in the event message (msg)
 */
Erizo.MediaEvent = function (spec) {
    "use strict";
    var that = Erizo.LicodeEvent(spec);
    that.msg = spec.msg;
    return that;
};
