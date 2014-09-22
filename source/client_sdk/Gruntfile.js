/*global module:false*/

module.exports = function(grunt) {

  var srcFiles = [
    'src/Property.js',
    'src/utils/L.Base64.js',
    'src/utils/L.Logger.js',
    'src/Connection.js',
    'src/Events.js',
    'src/Client.js',
    'src/Stream.js',
    'src/webrtc-stacks/BowserStack.js',
    'src/webrtc-stacks/ChromeCanaryStack.js',
    'src/webrtc-stacks/ChromeStableStack.js',
    'src/webrtc-stacks/FcStack.js',
    'src/webrtc-stacks/FirefoxStack.js'
  ];

  var uiSrcFiles = [
    'src/views/AudioPlayer.js',
    'src/views/Bar.js',
    'src/views/Speaker.js',
    'src/views/VideoPlayer.js',
    'src/views/View.js'
  ];

  // Project configuration.
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    meta: {
      banner: '\
/*\n\
 * Intel WebRTC SDK version <%= pkg.version %>\n\
 * Copyright (c) <%= grunt.template.today("yyyy") %> Intel <http://webrtc.intel.com>\n\
 * Homepage: http://webrtc.intel.com\n\
 */\n\n\n',
      header: '(function(window) {\n\n\n',
      footer: '\
\n\n\nwindow.Erizo = Erizo;\n\
window.L = L;\n\
}(window));\n\n\n'
    },
    concat: {
      dist: {
        src: srcFiles,
        dest: 'dist/<%= pkg.name %>-<%= pkg.version %>.js',
        options: {
          banner: '<%= meta.banner %>' + '<%= meta.header %>',
          separator: '\n\n\n',
          footer: '<%= meta.footer %>',
          process: true
        },
        nonull: true
      },
      devel: {
        src: uiSrcFiles,
        dest: 'dist/<%= pkg.name %>.ui-<%= pkg.version %>.js',
        options: {
          banner: '<%= meta.banner %>' + '<%= meta.header %>',
          separator: '\n\n\n',
          footer: '<%= meta.footer %>',
          process: true
        },
        nonull: true
      },
      merge: {
        src: ['dist/<%= pkg.name %>-<%= pkg.version %>.js', 'src/utils/L.Resizer.js', 'lib/socket.io.js'],
        dest: 'dist/<%= pkg.name %>-<%= pkg.version %>.js',
        nonull: true
      }
    },
    jshint: {
      dist: 'dist/<%= pkg.name %>-<%= pkg.version %>.js',
      devel: 'dist/<%= pkg.name %>-ui.js',
      options: {
        browser: true,
        curly: true,
        eqeqeq: true,
        immed: true,
        latedef: true,
        newcap: false,
        noarg: true,
        sub: true,
        undef: true,
        boss: true,
        eqnull: true,
        onecase:true,
        unused:true,
        supernew: true,
        laxcomma: true
      },
      globals: {}
    },
    uglify: {
      dist: {
        files: {
          'dist/<%= pkg.name %>-<%= pkg.version %>.min.js': ['dist/<%= pkg.name %>-<%= pkg.version %>.js'],
          'dist/<%= pkg.name %>.ui-<%= pkg.version %>.min.js': ['dist/<%= pkg.name %>.ui-<%= pkg.version %>.js']
        }
      },
      options: {
        banner: '<%= meta.banner %>'
      }
    }
  });


  // Load Grunt plugins.
  grunt.loadNpmTasks('grunt-contrib-concat');
  grunt.loadNpmTasks('grunt-contrib-uglify');
  grunt.loadNpmTasks('grunt-contrib-jshint');


  grunt.registerTask('build', ['concat:dist', 'concat:devel', 'jshint:dist', 'concat:merge', 'uglify:dist']);
  // grunt.registerTask('devel', ['concat:devel', 'includereplace:devel', 'jshint:devel', 'concat:post_devel']);

  // Default task is an alias for 'build'.
  grunt.registerTask('default', ['build']);

};
