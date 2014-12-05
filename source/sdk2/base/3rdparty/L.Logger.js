/*global console*/

/*
 * API to write logs based on traditional logging mechanisms: debug, trace, info, warning, error
 */
L.Logger = (function () {
  "use strict";
  var DEBUG = 0, TRACE = 1, INFO = 2, WARNING = 3, ERROR = 4, NONE = 5, logLevel = DEBUG, setLogLevel, log, debug, trace, info, warning, error;

  // It sets the new log level. We can set it to NONE if we do not want to print logs
  setLogLevel = function (level) {
    if (level > NONE) {
      level = NONE;
    } else if (level < DEBUG) {
      level = DEBUG;
    }
    logLevel = level;
  };

  // Generic function to print logs for a given level: [DEBUG, TRACE, INFO, WARNING, ERROR]
  log = function () {
    var level = arguments[0];
    var args = arguments;
    if (level < logLevel) {
      return;
    }
    switch (level) {
    case DEBUG:
      args[0] = 'DEBUG:';
      break;
    case TRACE:
      args[0] = 'TRACE:';
      break;
    case INFO:
      args[0] = 'INFO:';
      break;
    case WARNING:
      args[0] = 'WARNING:';
      break;
    case ERROR:
      args[0] = 'ERROR:';
      break;
    default:
      return;
    }
    console.log.apply(console, args);
  };

  // It prints debug logs
  debug = function () {
    var args = [DEBUG];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints trace logs
  trace = function () {
    var args = [TRACE];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints info logs
  info = function () {
    var args = [INFO];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints warning logs
  warning = function () {
    var args = [WARNING];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  // It prints error logs
  error = function () {
    var args = [ERROR];
    for (var i = 0; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
    log.apply(this, args);
  };

  return {
    DEBUG: DEBUG,
    TRACE: TRACE,
    INFO: INFO,
    WARNING: WARNING,
    ERROR: ERROR,
    NONE: NONE,
    setLogLevel: setLogLevel,
    log: log,
    debug: debug,
    trace: trace,
    info: info,
    warning: warning,
    error: error
  };
}());