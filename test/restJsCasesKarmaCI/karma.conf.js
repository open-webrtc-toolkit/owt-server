module.exports = function (config) {
  config.set({
    basePath: '',
    frameworks: ['mocha-debug', 'mocha', 'chai', 'browserify'],
    files: [
  //   "https://cdnjs.cloudflare.com/ajax/libs/socket.io/2.2.0/socket.io.js",
   //   "https://webrtchacks.github.io/adapter/adapter-7.0.0.js",
   //   "https://code.jquery.com/jquery-3.2.1.min.js",
      'dependencies/adapter-7.0.0.js',
     'dependencies/jquery-3.2.1.min.js',
      'dependencies/socket.io.js',

      '../dist/extras/basic_example/public/scripts/owt.js',
      'test/restJsCase.js',
     // '../dist/extras/basic_example/public/scripts/adapter.js',
    // '../dist/extras/basic_example/public/scripts/jquery-3.2.1.min.js',
     // '../dist/extras/basic_example/public/scripts/socket.io.js',
    //  'test/restJscaseException.js',
    ],
    protocol: 'http:',
    hostname: 'localhost',
    browserNoActivityTimeout: 60000,
    exclude: [
    ],
    preprocessors: {
      'test/*.js': ['browserify']
    },
    reporters: ['progress', 'junit', 'mocha'],
    junitReporter: {
      outputDir: 'reports',
      outputFile: undefined,
      suite: '',
      useBrowserName: false,
      nameFormatter: undefined,
      classNameFormatter: undefined,
      properties: {}
    },
    port: 9876,
    colors: true,
    logLevel: config.LOG_DEBUG,
    browsers: ['./scripts/chrome-start.sh'],
    singleRun: true,
    concurrency: Infinity,
    plugins: [
      'karma-*'
    ]
  })
}
