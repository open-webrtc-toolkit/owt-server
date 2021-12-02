{
  'targets': [{
    'target_name': 'quicCascading',
    'sources': [
      'addon.cc',
      'QuicTransportStream.cc',
      'QuicTransportSession.cc',
      'QuicTransportServer.cc',
      'QuicTransportClient.cc',
      'QuicFactory.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp',
      '../../../core/owt_base/MediaFrameMulticaster.cpp'
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '$(CORE_HOME)/common',
      '$(CORE_HOME)/owt_base',
      '$(CORE_HOME)/addons/common',
      '../../../../third_party/quic-lib/dist/include',
      '$(DEFAULT_DEPENDENCY_PATH)/include',
      '$(CUSTOM_INCLUDE_PATH)'
    ],
    'libraries': [
      '-ldl',
      '-llog4cxx',
      '-lboost_system',
      '-lboost_thread',
      '-L<(module_root_dir)/../../../../third_party/quic-lib/dist/lib',
      '-L$(DEFAULT_DEPENDENCY_PATH)/lib',
      '-lrawquic'
    ],
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
  }]
}
