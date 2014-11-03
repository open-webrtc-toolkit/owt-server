/*
 * Class EventDispatcher provides event handling to sub-classes.
 * It is inherited from Publisher, Room, etc.
 */
Woogeen.EventDispatcher = function (spec) {
  'use strict';
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
    if (!spec.dispatcher.eventListeners[eventType]) {return;}
    var index = spec.dispatcher.eventListeners[eventType].indexOf(listener);
    if (index !== -1) {
      spec.dispatcher.eventListeners[eventType].splice(index, 1);
    }
  };

  // It removes all event listeners for one type.
  that.clearEventListener = function (eventType) {
    spec.dispatcher.eventListeners[eventType] = [];
  };

  // It dispatch a new event to the event listeners, based on the type
  // of event. All events are intended to be LicodeEvents.
  that.dispatchEvent = function (event) {
    if (!spec.dispatcher.eventListeners[event.type]) {return;}
    spec.dispatcher.eventListeners[event.type].map(function (listener) {
      listener(event);
    });
  };

  return that;
};

// **** EVENTS ****

function WoogeenEvent (spec) { // base event class
  'use strict';
  this.type = spec.type;
  this.attributes = spec.attributes;
  this.label = spec.label;
}

/*
 * Class StreamEvent represents an event related to a stream.
 * It is usually initialized this way:
 * var streamEvent = StreamEvent({type:'stream-added', stream:stream1});
 * Event types:
 * 'stream-added' - indicates that there is a new stream available in the room.
 * 'stream-removed' - shows that a previous available stream has been removed from the room.
 */
Woogeen.StreamEvent = function WoogeenStreamEvent (spec) {
  'use strict';
  WoogeenEvent.call(this, spec);
  this.stream = spec.stream;
  this.msg = spec.msg;
};

/*
 * Class ClientEvent represents an event related to a client.
 * It is usually initialized this way:
 * var clientEvent = ClientEvent({type:'peer-left', user: user1, attr: attributes});
 * Event types:
 * 'client-disconnected' - shows that the user has been already disconnected.
 * 'peer-joined' - indicates that there is a new peer joined.
 * 'peer-left' - indicates that a peer has left.
 */
Woogeen.ClientEvent = function WoogeenClientEvent (spec) {
  'use strict';
  WoogeenEvent.call(this, spec);
  this.user = spec.user;
};

// inheritance
Woogeen.StreamEvent.prototype = Object.create(WoogeenEvent.prototype);
Woogeen.StreamEvent.prototype.constructor = Woogeen.StreamEvent;
Woogeen.ClientEvent.prototype = Object.create(WoogeenEvent.prototype);
Woogeen.ClientEvent.prototype.constructor = Woogeen.ClientEvent;