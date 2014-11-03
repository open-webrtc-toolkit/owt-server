/* global Erizo */

Woogeen.Stream.prototype.show = function(elementId, options) {
  'use strict';
  if (!(this.mediaStream) || this.showing === true) return (this.showing === true);
  if (typeof elementId !== 'string') elementId = 'stream_'+this.id();
  options = options || {};
  this.elementId = elementId;
  if (this.hasVideo()) {
    this.player = new Erizo.VideoPlayer({id: this.id(), stream: this, elementID: elementId, options: options});
    this.showing = true;
  } else if (this.hasAudio()) {
    this.player = new Erizo.AudioPlayer({id: this.id(), stream: this, elementID: elementId, options: options});
    this.showing = true;
  }
  return (this.showing === true);
};

Woogeen.Stream.prototype.hide = function() {
  'use strict';
  if (this.showing === true) {
    if (this.player && typeof this.player.destroy === 'function') {
      this.player.destroy();
      this.showing = false;
    }
  }
};