# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      ['target_arch=="arm" or target_arch=="armv7" or target_arch=="arm64"', {
        'use_opus_fixed_point%': 1,
      }, {
        'use_opus_fixed_point%': 0,
      }],
      ['target_arch=="arm" or target_arch=="armv7"', {
        'use_opus_arm_optimization%': 1,
      }, {
        'use_opus_arm_optimization%': 0,
      }],
      ['target_arch=="arm"', {
        'use_opus_rtcd%': 1,
      }, {
        'use_opus_rtcd%': 0,
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'opus',
      'type': 'static_library',
      'defines': [
        'OPUS_BUILD',
        'OPUS_EXPORT=',
      ],
      'include_dirs': [
        'src/celt',
        'src/include',
        'src/silk',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'src/include',
        ],
      },
      'includes': ['opus_srcs.gypi', ],
      'sources': ['<@(opus_common_sources)'],
      'conditions': [
        ['OS!="win"', {
          'defines': [
            'HAVE_LRINT',
            'HAVE_LRINTF',
            'VAR_ARRAYS',
          ],
        }, {
          'defines': [
            'USE_ALLOCA',
            'inline=__inline',
          ],
          'msvs_disabled_warnings': [
            4305,  # Disable truncation warning in celt/pitch.c .
            4334,  # Disable 32-bit shift warning in src/opus_encoder.c .
          ],
        }],
        ['os_posix==1', {
          'link_settings': {
            'libraries': [ '-lm' ],
          },
        }],
        ['os_posix==1 and OS!="android"', {
          # Suppress a warning given by opus_decoder.c that tells us
          # optimizations are turned off.
          'cflags': [
        #    '-Wno-#pragma-messages',
          ],
          'xcode_settings': {
            'WARNING_CFLAGS': [
              '-Wno-#pragma-messages',
            ],
          },
        }],
        ['os_posix==1 and (target_arch=="arm" or target_arch=="armv7" or target_arch=="arm64")', {
          'cflags!': ['-Os'],
          'cflags': ['-O3'],
        }],
        ['use_opus_fixed_point==0', {
          'include_dirs': [
            'src/silk/float',
          ],
          'sources': ['<@(opus_float_sources)'],
        }, {
          'defines': [
            'FIXED_POINT',
          ],
          'include_dirs': [
            'src/silk/fixed',
          ],
          'sources': ['<@(opus_fixed_sources)'],
          'conditions': [
            ['use_opus_arm_optimization==1', {
              'defines': [
                'OPUS_ARM_ASM',
                'OPUS_ARM_INLINE_ASM',
                'OPUS_ARM_INLINE_EDSP',
              ],
              'includes': [
                'opus_srcs_arm.gypi',
              ],
              'conditions': [
                ['use_opus_rtcd==1', {
                  'defines': [
                    'OPUS_ARM_MAY_HAVE_EDSP',
                    'OPUS_ARM_MAY_HAVE_MEDIA',
                    'OPUS_ARM_MAY_HAVE_NEON',
                    'OPUS_HAVE_RTCD',
                  ],
                  'includes': [
                    'opus_srcs_rtcd.gypi',
                  ],
                }],
              ],
            }],
          ],
        }],
      ],
    },  # target opus
    {
      'target_name': 'opus_demo',
      'type': 'executable',
      'dependencies': [
        'opus'
      ],
      'conditions': [
        ['OS == "win"', {
          'defines': [
            'inline=__inline',
          ],
        }],
        ['OS=="android"', {
          'link_settings': {
            'libraries': [
              '-llog',
            ],
          },
        }],
        ['clang==1', {
          'cflags': [ '-Wno-absolute-value' ],
        }]
      ],
      'sources': [
        'src/src/opus_demo.c',
      ],
      'include_dirs': [
        'src/celt',
        'src/silk',
      ],
    },  # target opus_demo
  ]
}
