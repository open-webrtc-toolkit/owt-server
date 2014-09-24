#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to copy a file from several possible locations.
#
# Arguments:
#
# -d - Destination.
# -s - Source file path.
#

while getopts "d:s:" flag; do
  if [ "$flag" = "d" ]; then
    dest=$OPTARG
  elif [ "$flag" = "s" ]; then
    srcs="$OPTARG $srcs"
  fi
done

for f in $srcs; do
  if [ -a $f ]; then
    src=$f
    break
  fi
done

if [ -z "$src" ]; then
  echo "Unable to locate file."
  false
  exit
fi

cp "$src" "$dest"
