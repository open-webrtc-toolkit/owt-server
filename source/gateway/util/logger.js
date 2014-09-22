var winston = require('winston');

var logger = function() {
  var rt = new winston.Logger({
      transports: [
        new(winston.transports.File)({
          filename: 'woogeen-gateway.log',
          dirname: '../logs/',
          handleExceptions: true,
          timestamp: true,
          colorize: false,
          json: false
        })
      ],
      exitOnError: false
    });
    return rt;
}();

logger.info("Initialized logger");

exports.logger = logger;