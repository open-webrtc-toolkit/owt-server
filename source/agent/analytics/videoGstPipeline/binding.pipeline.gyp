{
  'targets': [{
    'target_name': 'videoAnalyzer-pipeline',
    'sources': [
      './addon.cc',
      './VideoGstAnalyzerWrap.cc',
      './VideoGstAnalyzer.cpp',
      '../../addons/common/NodeEventRegistry.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/RawTransport.cpp',
      '../../../core/common/IOService.cpp',
      './GstInternalOut.cpp',
      './GstInternalIn.cpp',
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '/usr/include/gstreamer-1.0',
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/owt_base',
      '$(CORE_HOME)/addons/common',
      '$(DEFAULT_DEPENDENCY_PATH)/include',
      '$(CUSTOM_INCLUDE_PATH)'
    ],
    'libraries': [
      '-lboost_system',
      '-lboost_thread',
      '-lgstreamer-1.0',
      '-lgobject-2.0',
      '-lglib-2.0',
      '-lgstapp-1.0',
      '-lgthread-2.0',
      '-llog4cxx',
      '-L$(DEFAULT_DEPENDENCY_PATH)/lib',
      # '-lusrsctp'
    ],
    # 'INET', 'INET6' flags must be added for usrsctp lib, otherwise the arguments of receive callback would shift
    #'cflags_cc': ['-DINET', '-DINET6', '-DBUILD_FOR_RTSPSOURCE'],
    'cflags_cc': ['-DINET', '-DINET6', '-DBUILD_FOR_GST_ANALYTICS', '<!@(pkg-config --cflags glib-2.0)'],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O$(OPTIMIZATION_LEVEL)', '-g', '-std=c++11'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  }
  ]
}
