# Copyright 2011 The LibYuv Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS. All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'variables': {
     'use_system_libjpeg%': 0,
  },
  'targets': [
    {
      'target_name': 'libyuv',
      'type': 'static_library',
      # 'type': 'shared_library',
      'conditions': [
         ['use_system_libjpeg==0', {
          'dependencies': [
             '<(DEPTH)/third_party/libjpeg_turbo/libjpeg.gyp:libjpeg',
          ],
        }, {
          'link_settings': {
            'libraries': [
              '-ljpeg',
            ],
          },
        }],
      ],
      'defines': [
        'HAVE_JPEG',
        # Enable the following 3 macros to turn off assembly for specified CPU.
        # 'LIBYUV_DISABLE_X86',
        # 'LIBYUV_DISABLE_NEON',
        # 'LIBYUV_DISABLE_MIPS',
        # Enable the following macro to build libyuv as a shared library (dll).
        # 'LIBYUV_USING_SHARED_LIBRARY',
      ],
      'include_dirs': [
        'include',
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
          '.',
        ],
      },
      'sources': [
        # includes.
        'include/libyuv.h',
        'include/libyuv/basic_types.h',
        'include/libyuv/compare.h',
        'include/libyuv/convert.h',
        'include/libyuv/convert_argb.h',
        'include/libyuv/convert_from.h',
        'include/libyuv/convert_from_argb.h',
        'include/libyuv/cpu_id.h',
        'include/libyuv/format_conversion.h',
        'include/libyuv/mjpeg_decoder.h',
        'include/libyuv/planar_functions.h',
        'include/libyuv/rotate.h',
        'include/libyuv/rotate_argb.h',
        'include/libyuv/row.h',
        'include/libyuv/scale.h',
        'include/libyuv/scale_argb.h',
        'include/libyuv/version.h',
        'include/libyuv/video_common.h',

        # sources.
        'source/compare.cc',
        'source/compare_common.cc',
        'source/compare_neon.cc',
        'source/compare_posix.cc',
        'source/compare_win.cc',
        'source/convert.cc',
        'source/convert_argb.cc',
        'source/convert_from.cc',
        'source/convert_from_argb.cc',
        'source/cpu_id.cc',
        'source/format_conversion.cc',
        'source/mjpeg_decoder.cc',
        'source/planar_functions.cc',
        'source/rotate.cc',
        'source/rotate_argb.cc',
        'source/rotate_mips.cc',
        'source/rotate_neon.cc',
        'source/row_any.cc',
        'source/row_common.cc',
        'source/row_mips.cc',
        'source/row_neon.cc',
        'source/row_posix.cc',
        'source/row_win.cc',
        'source/scale.cc',
        'source/scale_argb.cc',
        'source/scale_argb_neon.cc',
        'source/scale_mips.cc',
        'source/scale_neon.cc',
        'source/video_common.cc',
      ],
    },
  ], # targets.
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
