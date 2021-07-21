const chromeFlags = [
  '--enable-experimental-web-platform-features',
  '--enable-quic',
  '--no-sandbox',
  '--reduce-security-for-testing',
  '--allow-insecure-localhost',
  '--headless',
];

process.on('infrastructure_error', (error) => {
  console.error('infrastructure_error', error);
});

module.exports = function(config) {
  config.set({
    basePath: '../../',
    frameworks: ['mocha', 'chai', 'browserify'],
    browsers: ['ChromeWithFlags'],
    customLaunchers: {
      ChromeWithFlags: {base: 'ChromeHeadless', flags: chromeFlags},
    },
    files: [
      './node_modules/socket.io/client-dist/socket.io.js',
      './e2e-test/js/deps/owt.js',
      './e2e-test/js/deps/rest-sample.js',
      './e2e-test/js/webtransport.js',
    ],
    client: {
      mocha: {
        reporter: 'html',
      },
      cert_fingerprint: config.cert_fingerprint
    },
    reporters: ['mocha'],
    singleRun: true,
    concurrency: 1,
    color: true,
    logLevel: config.LOG_INFO,
    plugins: [
      'karma-chrome-launcher',
      'karma-mocha',
      'karma-chai',
      'karma-mocha-reporter',
      'karma-browserify',
    ],
  });
};