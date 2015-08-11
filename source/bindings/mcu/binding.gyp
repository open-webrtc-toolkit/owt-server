{
  'targets': [
  {
    'target_name': 'addon',
      'sources': [ 'addon.cc', 'Mixer.cc', '../erizoAPI/WebRtcConnection.cc', '../woogeen_base/ExternalInput.cc', '../woogeen_base/Gateway.cc' ],
      'include_dirs' : ['$(CORE_HOME)/common', '$(CORE_HOME)/erizo/src/erizo', '$(CORE_HOME)/woogeen_base', '$(CORE_HOME)/mcu', '$(CORE_HOME)/../../build/libdeps/build/include', '$(CORE_HOME)/../../third_party/libuv/include'],
      'libraries': ['-L$(CORE_HOME)/build/mcu', '-lmcu', '-L$(CORE_HOME)/build/woogeen_base', '-lwoogeen_base', '-L$(CORE_HOME)/build/erizo/src/erizo', '-lerizo', '-L$(CORE_HOME)/../../third_party/libuv', '-luv'],
      'conditions': [
        [ 'OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
              'GCC_ENABLE_CPP_RTTI': 'YES',              # -fno-rtti
              'MACOSX_DEPLOYMENT_TARGET' : '10.7',      #from MAC OS 10.7
              'OTHER_CFLAGS': [
              '-g -O3 -stdlib=libc++',
            ]
          },
        }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags_cc' : ['-Wall', '-O3', '-g' , '-std=c++0x', '-frtti'],
          'cflags_cc!' : ['-fno-exceptions']
        }],
        ]
  }
  ]
}
