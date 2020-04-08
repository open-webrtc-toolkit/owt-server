{
  'targets': [{
    'target_name': 'sipIn',
    'sources': [
      'addon.cc',
      'SipGateway.cc',
      'SipCallConnection.cpp',
      '../../addons/common/NodeEventRegistry.cc',
    ],
    'dependencies': ['sipLib'],
    'cflags_cc': ['-DWEBRTC_POSIX', '-DWEBRTC_LINUX'],
    'include_dirs': [
      "<!(node -e \"require('nan')\")",
      '../../../core/common',
      '../../../core/owt_base',
      '../../../../third_party/licode/erizo/src/erizo',
    ],
    'libraries': [
      '-llog4cxx',
      '-lboost_thread',
      '-lboost_system',
      # Add following arguments to help ldd find libraries during packing
      '-L$(CORE_HOME)/../../build/libdeps/build/lib',
      '-lre',
      '-Wl,-rpath,<!(pwd)/build/$(BUILDTYPE)',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g' , '-std=c++11'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ]
  },
  {
    'target_name': 'sipLib',
    'type': 'shared_library',
    'sources': [
        'sip_gateway/SipEP.cpp',
        'sip_gateway/SipGateway.cpp',
        'sip_gateway/SipCallConnection.cpp',
    ],
    'direct_dependent_settings': {
        'include_dirs': ['sip_gateway'],
    },
    'include_dirs': [
        'sip_gateway',
        'sip_gateway/sipua/include',
        '$(CORE_HOME)/common',
        '../../../../third_party/licode/erizo/src/erizo',
        '$(CORE_HOME)/../../build/libdeps/build/include',
    ],
    'libraries': [
        '-L<!(pwd)/sip_gateway/sipua', '-lsipua',
        '-L$(CORE_HOME)/../../build/libdeps/build/lib',
        '-lre',
        '-llog4cxx',
        '-lboost_thread',
        '-lboost_system',
    ],
    'conditions': [
      [ 'OS=="mac"', {
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
          'OTHER_CFLAGS': ['-g -O3 -stdlib=libc++']
        },
      }, { # OS!="mac"
        'cflags!':    ['-fno-exceptions'],
        'cflags_cc':  ['-Wall', '-O3', '-g', '-std=c++0x'],
        'cflags_cc!': ['-fno-exceptions']
      }],
    ],
    'actions': [
      {
        'action_name': 'sipua_build',
        'inputs': [
          '<!@(find <!(pwd)/sip_gateway/sipua -type f -name "*.h")',
          '<!@(find <!(pwd)/sip_gateway/sipua -type f -name "*.c")',
        ],
        'outputs': [
          '<!(pwd)/sip_gateway/sipua/libsipua.a'
        ],
        'action': ['eval', 'cd <!(pwd)/sip_gateway/sipua && make clean && make RE_HOME=$(CORE_HOME)/../../build/libdeps/build'],
      }
    ]
  },
  ]
}
