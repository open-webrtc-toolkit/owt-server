{
  'targets': [{
    'target_name': 'quicIO',
    'sources': [
      'addon.cc',
      'QuicTransport.cc',
      'InternalQuic.cc',
      '../../../core/owt_base/MediaFramePipeline.cpp'
    ],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '../../../core/owt_base',
      '../../../agent/addons/common',
      '../../../../third_party/quic-lib/dist/include'
    ],
    'libraries': [
      '-lboost_system',
      '-lboost_thread',
      '-L<(module_root_dir)/../../../../third_party/quic-lib/dist/lib',
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
