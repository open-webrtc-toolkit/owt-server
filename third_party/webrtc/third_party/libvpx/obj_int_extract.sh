#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to run obj_int_extract and output the result to a
# file.
#
# Arguments:
#
# -e - Executable of obj_int_extract.
# -f - ASM format.
# -b - Object binary file.
# -o - Output file.
#

while getopts "e:f:b:o:" flag; do
  if [ "$flag" = "e" ]; then
    bin_file=$OPTARG
  elif [ "$flag" = "f" ]; then
    asm_format=$OPTARG
  elif [ "$flag" = "b" ]; then
    obj_file=$OPTARG
  elif [ "$flag" = "o" ]; then
    out_file=$OPTARG
  fi
done

"$bin_file" "$asm_format" "$obj_file" > "$out_file"
