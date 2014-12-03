/* global window, document, webkitURL, Erizo, L */
/*
 * VideoPlayer represents a Licode video component that shows either a local or a remote video.
 * Ex.: var player = VideoPlayer({id: id, stream: stream, elementID: elementID});
 * A VideoPlayer is also a View component.
 */
Erizo.VideoPlayer = function (spec) {
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
    that.video.addEventListener('playing', function display () {
        if (that.video.videoWidth * that.video.videoHeight > 4) { // why remote video size initially is 2*2 in chrome?
            L.Logger.debug('video dimensions:', that.video.videoWidth, that.video.videoHeight);
            return;
        }
        setTimeout(display, 50);
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
