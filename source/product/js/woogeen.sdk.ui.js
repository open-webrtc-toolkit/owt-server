/*
 * Intel WebRTC SDK version 2.0.0
 * Copyright (c) 2014 Intel <http://webrtc.intel.com>
 * Homepage: http://webrtc.intel.com
 */


(function(window) {


/**
 * Copyright 2013 Marc J. Schmidt. See the LICENSE file at the top-level
 * directory of this distribution and at
 * https://github.com/marcj/css-element-queries/blob/master/LICENSE.
 */
;
(function() {

    this.L = this.L || {};

    /**
     *
     * @type {Function}
     * @constructor
     */
    this.L.ElementQueries = function() {
        /**
         *
         * @param element
         * @returns {Number}
         */
        function getEmSize(element) {
            if (!element) {
                element = document.documentElement;
            }
            var fontSize = getComputedStyle(element, 'fontSize');
            return parseFloat(fontSize) || 16;
        }

        /**
         *
         * @copyright https://github.com/Mr0grog/element-query/blob/master/LICENSE
         *
         * @param element
         * @param value
         * @param units
         * @returns {*}
         */
        function convertToPx(element, value) {
            var units = value.replace(/[0-9]*/, '');
            value = parseFloat(value);
            switch (units) {
                case "px":
                    return value;
                case "em":
                    return value * getEmSize(element);
                case "rem":
                    return value * getEmSize();
                // Viewport units!
                // According to http://quirksmode.org/mobile/tableViewport.html
                // documentElement.clientWidth/Height gets us the most reliable info
                case "vw":
                    return value * document.documentElement.clientWidth / 100;
                case "vh":
                    return value * document.documentElement.clientHeight / 100;
                case "vmin":
                case "vmax":
                    var vw = document.documentElement.clientWidth / 100;
                    var vh = document.documentElement.clientHeight / 100;
                    var chooser = Math[units === "vmin" ? "min" : "max"];
                    return value * chooser(vw, vh);
                default:
                    return value;
                // for now, not supporting physical units (since they are just a set number of px)
                // or ex/ch (getting accurate measurements is hard)
            }
        }

        /**
         *
         * @param {HTMLElement} element
         * @constructor
         */
        function SetupInformation(element) {
            this.element = element;
            this.options = [];
            var i, j, option, width = 0, height = 0, value, actualValue, attrValues, attrValue, attrName;

            /**
             * @param option {mode: 'min|max', property: 'width|height', value: '123px'}
             */
            this.addOption = function(option) {
                this.options.push(option);
            }

            var attributes = ['min-width', 'min-height', 'max-width', 'max-height'];

            /**
             * Extracts the computed width/height and sets to min/max- attribute.
             */
            this.call = function() {
                // extract current dimensions
                width = this.element.offsetWidth;
                height = this.element.offsetHeight;

                attrValues = {};

                for (i = 0, j = this.options.length; i < j; i++) {
                    option = this.options[i];
                    value = convertToPx(this.element, option.value);

                    actualValue = option.property == 'width' ? width : height;
                    attrName = option.mode + '-' + option.property;
                    attrValue = '';

                    if (option.mode == 'min' && actualValue >= value) {
                        attrValue += option.value;
                    }

                    if (option.mode == 'max' && actualValue <= value) {
                        attrValue += option.value;
                    }

                    if (!attrValues[attrName]) attrValues[attrName] = '';
                    if (attrValue && -1 === (' '+attrValues[attrName]+' ').indexOf(' ' + attrValue + ' ')) {
                        attrValues[attrName] += ' ' + attrValue;
                    }
                }

                for (var k in attributes) {
                    if (attrValues[attributes[k]]) {
                        this.element.setAttribute(attributes[k], attrValues[attributes[k]].substr(1));
                    } else {
                        this.element.removeAttribute(attributes[k]);
                    }
                }
            };
        }

        /**
         * @param {HTMLElement} element
         * @param {Object}      options
         */
        function setupElement(element, options) {
            if (element.elementQueriesSetupInformation) {
                element.elementQueriesSetupInformation.addOption(options);
            } else {
                element.elementQueriesSetupInformation = new SetupInformation(element);
                element.elementQueriesSetupInformation.addOption(options);
                new ResizeSensor(element, function() {
                    element.elementQueriesSetupInformation.call();
                });
            }
            element.elementQueriesSetupInformation.call();
        }

        /**
         * @param {String} selector
         * @param {String} mode min|max
         * @param {String} property width|height
         * @param {String} value
         */
        function queueQuery(selector, mode, property, value) {
            var query;
            if (document.querySelectorAll) query = document.querySelectorAll.bind(document);
            if (!query && 'undefined' !== typeof $$) query = $$;
            if (!query && 'undefined' !== typeof jQuery) query = jQuery;

            if (!query) {
                throw 'No document.querySelectorAll, jQuery or Mootools\'s $$ found.';
            }

            var elements = query(selector);
            for (var i = 0, j = elements.length; i < j; i++) {
                setupElement(elements[i], {
                    mode: mode,
                    property: property,
                    value: value
                });
            }
        }

        var regex = /,?([^,\n]*)\[[\s\t]*(min|max)-(width|height)[\s\t]*[~$\^]?=[\s\t]*"([^"]*)"[\s\t]*]([^\n\s\{]*)/mgi;

        /**
         * @param {String} css
         */
        function extractQuery(css) {
            var match;
            css = css.replace(/'/g, '"');
            while (null !== (match = regex.exec(css))) {
                if (5 < match.length) {
                    queueQuery(match[1] || match[5], match[2], match[3], match[4]);
                }
            }
        }

        /**
         * @param {CssRule[]|String} rules
         */
        function readRules(rules) {
            var selector = '';
            if (!rules) {
                return;
            }
            if ('string' === typeof rules) {
                rules = rules.toLowerCase();
                if (-1 !== rules.indexOf('min-width') || -1 !== rules.indexOf('max-width')) {
                    extractQuery(rules);
                }
            } else {
                for (var i = 0, j = rules.length; i < j; i++) {
                    if (1 === rules[i].type) {
                        selector = rules[i].selectorText || rules[i].cssText;
                        if (-1 !== selector.indexOf('min-height') || -1 !== selector.indexOf('max-height')) {
                            extractQuery(selector);
                        }else if(-1 !== selector.indexOf('min-width') || -1 !== selector.indexOf('max-width')) {
                            extractQuery(selector);
                        }
                    } else if (4 === rules[i].type) {
                        readRules(rules[i].cssRules || rules[i].rules);
                    }
                }
            }
        }

        /**
         * Searches all css rules and setups the event listener to all elements with element query rules..
         */
        this.init = function() {
            for (var i = 0, j = document.styleSheets.length; i < j; i++) {
                readRules(document.styleSheets[i].cssText || document.styleSheets[i].cssRules || document.styleSheets[i].rules);
            }
        }
    }

    function init() {
        new L.ElementQueries().init();
    }

    if (window.addEventListener) {
        window.addEventListener('load', init, false);
    } else {
        window.attachEvent('onload', init);
    }

    /**
     * Class for dimension change detection.
     *
     * @param {Element|Element[]|Elements|jQuery} element
     * @param {Function} callback
     *
     * @constructor
     */
    this.L.ResizeSensor = function(element, callback) {
        /**
         * Adds a listener to the over/under-flow event.
         *
         * @param {HTMLElement} element
         * @param {Function}    callback
         */
        function addResizeListener(element, callback) {
            if (window.OverflowEvent) {
                //webkit
                element.addEventListener('overflowchanged', function(e) {
                    callback.call(this, e);
                });
            } else {
                element.addEventListener('overflow', function(e) {
                    callback.call(this, e);
                });
                element.addEventListener('underflow', function(e) {
                    callback.call(this, e);
                });
            }
        }

        /**
         *
         * @constructor
         */
        function EventQueue() {
            this.q = [];
            this.add = function(ev) {
                this.q.push(ev);
            };

            var i, j;
            this.call = function() {
                for (i = 0, j = this.q.length; i < j; i++) {
                    this.q[i].call();
                }
            };
        }

        /**
         * @param {HTMLElement} element
         * @param {String}      prop
         * @returns {String|Number}
         */
        function getComputedStyle(element, prop) {
            if (element.currentStyle) {
                return element.currentStyle[prop];
            } else if (window.getComputedStyle) {
                return window.getComputedStyle(element, null).getPropertyValue(prop);
            } else {
                return element.style[prop];
            }
        }

        /**
         *
         * @param {HTMLElement} element
         * @param {Function}    resized
         */
        function attachResizeEvent(element, resized) {
            if (!element.resizedAttached) {
                element.resizedAttached = new EventQueue();
                element.resizedAttached.add(resized);
            } else if (element.resizedAttached) {
                element.resizedAttached.add(resized);
                return;
            }

            /*if ('onresize' in element) {
                //internet explorer
                if (element.attachEvent) {
                    element.attachEvent('onresize', function() {
                        element.resizedAttached.call();
                    });
                } else if (element.addEventListener) {
                    element.addEventListener('resize', function(){
                        element.resizedAttached.call();
                    });
                }
            } else {*/
                var myResized = function() {
                    if (setupSensor()) {
                        element.resizedAttached.call();
                    }
                };
                element.resizeSensor = document.createElement('div');
                element.resizeSensor.className = 'resize-sensor';
                var style =
                    'position: absolute; left: 0; top: 0; right: 0; bottom: 0; overflow: hidden; z-index: -1;';
                element.resizeSensor.style.cssText = style;
                element.resizeSensor.innerHTML =
                    '<div class="resize-sensor-overflow" style="' + style + '">' +
                        '<div></div>' +
                        '</div>' +
                        '<div class="resize-sensor-underflow" style="' + style + '">' +
                        '<div></div>' +
                        '</div>';
                element.appendChild(element.resizeSensor);

                if ('absolute' !== getComputedStyle(element, 'position')) {
                    element.style.position = 'relative';
                }

                var x = -1,
                    y = -1,
                    firstStyle = element.resizeSensor.firstElementChild.firstChild.style,
                    lastStyle = element.resizeSensor.lastElementChild.firstChild.style;

                function setupSensor() {
                    var change = false,
                        width = element.resizeSensor.offsetWidth,
                        height = element.resizeSensor.offsetHeight;

                    if (x != width) {
                        firstStyle.width = (width - 1) + 'px';
                        lastStyle.width = (width + 1) + 'px';
                        change = true;
                        x = width;
                    }
                    if (y != height) {
                        firstStyle.height = (height - 1) + 'px';
                        lastStyle.height = (height + 1) + 'px';
                        change = true;
                        y = height;
                    }
                    return change;
                }

                setupSensor();
                addResizeListener(element.resizeSensor, myResized);
                addResizeListener(element.resizeSensor.firstElementChild, myResized);
                addResizeListener(element.resizeSensor.lastElementChild, myResized);
            /*}*/
        }

        if ('array' === typeof element
            || ('undefined' !== typeof jQuery && element instanceof jQuery) //jquery
            || ('undefined' !== typeof Elements && element instanceof Elements) //mootools
            ) {
            var i = 0, j = element.length;
            for (; i < j; i++) {
                attachResizeEvent(element[i], callback);
            }
        } else {
            attachResizeEvent(element, callback);
        }
    }

})();


/* global window, document, webkitURL, Erizo */
/*
 * AudioPlayer represents a Licode Audio component that shows either a local or a remote Audio.
 * Ex.: var player = AudioPlayer({id: id, stream: stream, elementID: elementID});
 * A AudioPlayer is also a View component.
 */
Erizo.AudioPlayer = function (spec) {
    'use strict';

    var that = Erizo.View({}),
        onmouseover,
        onmouseout;

    // Variables

    // AudioPlayer ID
    that.id = spec.id;

    // Stream that the AudioPlayer will play
    that.stream = spec.stream.mediaStream;

    // DOM element in which the AudioPlayer will be appended
    that.elementID = spec.elementID;

    that.stream_url = (window.URL || webkitURL).createObjectURL(that.stream);

    // Audio tag
    that.audio = document.createElement('audio');
    that.audio.setAttribute('id', 'stream' + that.id);
    that.audio.setAttribute('style', 'width: 100%; height: 100%; position: absolute');
    that.audio.setAttribute('autoplay', 'autoplay');

    if (spec.stream.local) {
        that.audio.volume = 0;
    }

    if (spec.stream.local) {
        that.audio.volume = 0;
    }


    if (that.elementID !== undefined) {

        // It will stop the AudioPlayer and remove it from the HTML
        that.destroy = function () {
            that.audio.pause();
            that.parentNode.removeChild(that.div);
        };

        onmouseover = function () {
            that.bar.display();
        };

        onmouseout = function () {
            that.bar.hide();
        };

        // Container
        that.div = document.createElement('div');
        that.div.setAttribute('id', 'player_' + that.id);
        that.div.setAttribute('style', 'width: 100%; height: 100%; position: relative; overflow: hidden;');

        document.getElementById(that.elementID).appendChild(that.div);
        that.container = document.getElementById(that.elementID);

        that.parentNode = that.div.parentNode;

        that.div.appendChild(that.audio);

        // Bottom Bar
        that.bar = new Erizo.Bar({elementID: 'player_' + that.id, id: that.id, stream: spec.stream, media: that.audio, options: spec.options});

        that.div.onmouseover = onmouseover;
        that.div.onmouseout = onmouseout;

    } else {
        // It will stop the AudioPlayer and remove it from the HTML
        that.destroy = function () {
            that.audio.pause();
            that.parentNode.removeChild(that.audio);
        };

        document.body.appendChild(that.audio);
        that.parentNode = document.body;
    }

    that.audio.src = that.stream_url;

    return that;
};



/* global document, clearTimeout, setTimeout, Erizo */

/*
 * Bar represents the bottom menu bar of every mediaPlayer.
 * It contains a Speaker and an icon.
 * Every Bar is a View.
 * Ex.: var bar = Bar({elementID: element, id: id});
 */
Erizo.Bar = function (spec) {
    'use strict';

    var that = Erizo.View({}),
        waiting,
        show;

    // Variables

    // DOM element in which the Bar will be appended
    that.elementID = spec.elementID;

    // Bar ID
    that.id = spec.id;

    // Container
    that.div = document.createElement('div');
    that.div.setAttribute('id', 'bar_' + that.id);

    // Bottom bar
    that.bar = document.createElement('div');
    that.bar.setAttribute('style', 'width: 100%; height: 15%; max-height: 30px; position: absolute; bottom: 0; right: 0; background-color: rgba(255,255,255,0.62)');
    that.bar.setAttribute('id', 'subbar_' + that.id);

    // Private functions
    show = function (displaying) {
        if (displaying !== 'block') {
            displaying = 'none';
        } else {
            clearTimeout(waiting);
        }
        that.div.setAttribute('style', 'width: 100%; height: 100%; position: relative; bottom: 0; right: 0; display:' + displaying);
    };

    // Public functions

    that.display = function () {
        show('block');
    };

    that.hide = function () {
        waiting = setTimeout(show, 1000);
    };

    document.getElementById(that.elementID).appendChild(that.div);
    that.div.appendChild(that.bar);

    // Speaker component
    if (!spec.stream.screen && (spec.options === undefined || spec.options.speaker === undefined || spec.options.speaker === true)) {
        that.speaker = new Erizo.Speaker({elementID: 'subbar_' + that.id, id: that.id, stream: spec.stream, media: spec.media});
    }

    that.display();
    that.hide();

    return that;
};



/* global document, Erizo*/
/*
 * Speaker represents the volume icon that will be shown in the mediaPlayer, for example.
 * It manages the volume level of the media tag given in the constructor.
 * Every Speaker is a View.
 * Ex.: var speaker = Speaker({elementID: element, media: mediaTag, id: id});
 */
Erizo.Speaker = function (spec) {
    'use strict';

    var that = Erizo.View({}),
        show,
        mute,
        unmute,
        lastVolume = 50;

    // Variables

    // DOM element in which the Speaker will be appended
    that.elementID = spec.elementID;

    // media tag
    that.media = spec.media || {};

    // Speaker id
    that.id = spec.id;

    // MediaStream
    that.stream = spec.stream;

    // Container
    that.div = document.createElement('div');
    that.div.setAttribute('style', 'width: 40%; height: 100%; max-width: 32px; position: absolute; right: 0;z-index:0;');

    // // Volume icon 
    // that.icon = document.createElement('img');
    // that.icon.setAttribute('id', 'volume_' + that.id);
    // that.icon.setAttribute('src', that.url + '/assets/sound48.png');
    // that.icon.setAttribute('style', 'width: 80%; height: 100%; position: absolute;');
    // that.div.appendChild(that.icon);


    if (that.stream instanceof Woogeen.RemoteStream) {

        // Volume bar
        that.picker = document.createElement('input');
        that.picker.setAttribute('id', 'picker_' + that.id);
        that.picker.type = 'range';
        that.picker.min = 0;
        that.picker.max = 100;
        that.picker.step = 10;
        that.picker.value = lastVolume;
        that.picker.orient = 'vertical'; //  FireFox supports range sliders as of version 23
        that.div.appendChild(that.picker);
        that.media.volume = that.picker.value / 100;
        that.media.muted = false;

        that.picker.oninput = function () {
            if (that.picker.value > 0) {
                that.media.muted = false;
                // that.icon.setAttribute('src', that.url + '/assets/sound48.png');
            } else {
                that.media.muted = true;
                // that.icon.setAttribute('src', that.url + '/assets/mute48.png');
            }
            that.media.volume = that.picker.value / 100;
        };

        // Private functions
        show = function (displaying) {
            that.picker.setAttribute('style', 'width: 32px; height: 100px; position: absolute; bottom: 90%; z-index: 1;' + that.div.offsetHeight + 'px; right: 0px; -webkit-appearance: slider-vertical; display: ' + displaying);
        };

        mute = function () {
            // that.icon.setAttribute('src', that.url + '/assets/mute48.png');
            lastVolume = that.picker.value;
            that.picker.value = 0;
            that.media.volume = 0;
            that.media.muted = true;
        };

        unmute = function () {
            // that.icon.setAttribute('src', that.url + '/assets/sound48.png');
            that.picker.value = lastVolume;
            that.media.volume = that.picker.value / 100;
            that.media.muted = false;
        };

        // that.icon.onclick = function () {
        //     if (that.media.muted) {
        //         unmute();
        //     } else {
        //         mute();
        //     }
        // };

        // Public functions
        that.div.onmouseover = function () {
            show('block');
        };

        that.div.onmouseout = function () {
            show('none');
        };

        show('none');

    } else if (that.stream instanceof Woogeen.LocalStream) {
        var muted = false;
        mute = function () {
            muted = true;
            // that.icon.setAttribute('src', that.url + '/assets/mute48.png');
            that.stream.stream.getAudioTracks()[0].enabled = false;
        };

        unmute = function () {
            muted = false;
            // that.icon.setAttribute('src', that.url + '/assets/sound48.png');
            that.stream.stream.getAudioTracks()[0].enabled = true;
        };

        // that.icon.onclick = function () {
        //     if (muted) {
        //         unmute();
        //     } else {
        //         mute();
        //     }
        // };
    }
  

    document.getElementById(that.elementID).appendChild(that.div);
    return that;
};



/* global window, document, webkitURL, Erizo, L */
/*
 * VideoPlayer represents a Licode video component that shows either a local or a remote video.
 * Ex.: var player = VideoPlayer({id: id, stream: stream, elementID: elementID});
 * A VideoPlayer is also a View component.
 */
Erizo.VideoPlayer1 = function (spec) {
    'use strict';

    var that = Erizo.View({}),
        onmouseover,
        onmouseout;

    // Variables

    // VideoPlayer ID
    that.id = spec.id;

    // Stream that the VideoPlayer will play
    that.stream = spec.stream.mediaStream;

    // DOM element in which the VideoPlayer will be appended
    that.elementID = spec.elementID;

    // Private functions
    onmouseover = function () {
        that.bar.display();
    };

    onmouseout = function () {
        that.bar.hide();
    };

    // Public functions

    // It will stop the VideoPlayer and remove it from the HTML
    that.destroy = function () {
        that.video.pause();
        delete that.resizer;
        that.parentNode.removeChild(that.div);
    };

    that.resize = function () {

        var width = that.container.offsetWidth,
            height = that.container.offsetHeight;

        if (spec.stream.screen || spec.options.crop === false) {

            if (width * (3 / 4) < height) {

                that.video.style.width = width + 'px';
                that.video.style.height = (3 / 4) * width + 'px';

                that.video.style.top = -((3 / 4) * width / 2 - height / 2) + 'px';
                that.video.style.left = '0px';

            } else {

                that.video.style.height = height + 'px';
                that.video.style.width = (4 / 3) * height + 'px';

                that.video.style.left = -((4 / 3) * height / 2 - width / 2) + 'px';
                that.video.style.top = '0px';

            }
        } else {
            if (width !== that.containerWidth || height !== that.containerHeight) {

                if (width * (3 / 4) > height) {

                    that.video.style.width = width + 'px';
                    that.video.style.height = (3 / 4) * width + 'px';

                    that.video.style.top = -((3 / 4) * width / 2 - height / 2) + 'px';
                    that.video.style.left = '0px';

                } else {

                    that.video.style.height = height + 'px';
                    that.video.style.width = (4 / 3) * height + 'px';

                    that.video.style.left = -((4 / 3) * height / 2 - width / 2) + 'px';
                    that.video.style.top = '0px';

                }
            }
        }

        that.containerWidth = width;
        that.containerHeight = height;

    };

    /*window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, key, value) {
        document.getElementById(key).value = unescape(value);
    });*/

    that.stream_url = (window.URL || webkitURL).createObjectURL(that.stream);

    // Container
    that.div = document.createElement('div');
    that.div.setAttribute('id', 'player_' + that.id);
    that.div.setAttribute('style', 'width: 100%; height: 100%; position: relative; background-color: black; overflow: hidden;');

    // Video tag
    that.video = document.createElement('video');
    that.video.setAttribute('id', 'stream' + that.id);
    that.video.setAttribute('style', 'width: 100%; height: 100%; position: absolute');
    that.video.setAttribute('autoplay', 'autoplay');

    if (spec.stream.local) {
        that.video.volume = 0;
    }

    if (that.elementID !== undefined) {
        document.getElementById(that.elementID).appendChild(that.div);
        that.container = document.getElementById(that.elementID);
    } else {
        document.body.appendChild(that.div);
        that.container = document.body;
    }

    that.parentNode = that.div.parentNode;

    that.div.appendChild(that.video);
    that.video.addEventListener('playing', function () {
        var display = function () {
            if (that.video.videoWidth * that.video.videoHeight > 4) { // why remote video size initially is 2*2 in chrome?
                L.Logger.info('video dimensions:', that.video.videoWidth, that.video.videoHeight);
                return;
            }
            setTimeout(display, 50);
        };
        display();
    });
    that.containerWidth = 0;
    that.containerHeight = 0;

    that.resizer = new L.ResizeSensor(that.container, that.resize);

    that.resize();

    // Bottom Bar
    that.bar = new Erizo.Bar({elementID: 'player_' + that.id, id: that.id, stream: spec.stream, media: that.video, options: spec.options});

    that.div.onmouseover = onmouseover;
    that.div.onmouseout = onmouseout;

    that.video.src = that.stream_url;

    return that;
};



/* global Erizo */

/*
 * View class represents a HTML component
 * Every view is an EventDispatcher.
 */
Erizo.View = function (spec) {
    'use strict';

    var that = Woogeen.EventDispatcher(spec);

    // Variables

    // URL where it will look for icons and assets
    that.url = spec.url || '.';
    return that;
};



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


window.Erizo = Erizo;
window.Woogeen = Woogeen;
window.L = L;
}(window));


