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
