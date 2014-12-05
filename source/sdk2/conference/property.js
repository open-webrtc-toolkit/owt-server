var Woogeen = (function() {
  'use strict';

  var Woogeen = {};

  Object.defineProperties(Woogeen, {
    version: {
      get: function() { return '<%= pkg.version %>'; }
    },
    name: {
      get: function() { return '<%= pkg.title %>'; }
    }
  });

  return Woogeen;
})();

var L = {};
var Erizo = {};
