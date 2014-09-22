var Erizo = (function() {
  "use strict";

  var Erizo = {};

  Object.defineProperties(Erizo, {
    version: {
      get: function(){ return '<%= pkg.version %>'; }
    },
    name: {
      get: function(){ return '<%= pkg.title %>'; }
    }
  });

  return Erizo;
}());

var L = {};

