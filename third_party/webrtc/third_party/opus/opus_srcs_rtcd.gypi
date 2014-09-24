# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'sources': [
    'src/celt/arm/arm_celt_map.c',
    'src/celt/arm/armcpu.c',
    'src/celt/arm/armcpu.h',
    '<(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
  ],
  'actions': [
    {
      'action_name': 'convert_assembler',
      'inputs': [
        'src/celt/arm/arm2gnu.pl',
        'src/celt/arm/celt_pitch_xcorr_arm.s',
      ],
      'outputs': [
        '<(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
      ],
      'action': [
        'bash',
        '-c',
        'perl src/celt/arm/arm2gnu.pl src/celt/arm/celt_pitch_xcorr_arm.s | sed "s/OPUS_ARM_MAY_HAVE_[A-Z]*/1/g" | sed "/.include/d" > <(INTERMEDIATE_DIR)/celt_pitch_xcorr_arm_gnu.S',
      ],
      'message': 'Convert Opus assembler for ARM.'
    },
  ],
}
