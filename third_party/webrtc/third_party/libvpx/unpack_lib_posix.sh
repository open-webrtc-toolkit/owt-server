#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to unpack a .a file into object files.
#
# Arguments:
#
# d - Output directory.
# a - List of possible locations of the archive.
# f - List of files to extract.
#

while getopts "d:a:f:" flag
do
  if [ "$flag" = "d" ]; then
    out_dir=$OPTARG
  elif [ "$flag" = "a" ]; then
    lib_files="$OPTARG $lib_files"
  elif [ "$flag" = "f" ]; then
    obj_files="$OPTARG $obj_files"
  fi
done

for f in $lib_files; do
  if [ -a $f ]; then
    lib_file=$f
    break
  fi
done

if [ -z "$lib_file" ]; then
  echo "Failed to locate a static library."
  false
  exit
fi

# Find the appropriate ar to use.
ar="ar"
if [ -n "$AR_target" ]; then
  ar=$AR_target
elif [ -n "$AR" ]; then
  ar=$AR
fi

obj_list="$($ar t $lib_file | grep '\.o$')"

function extract_object {
  for f in $obj_list; do
    filename="${f##*/}"

    if [ -z "$(echo $filename | grep $1)" ]; then
      continue
    fi

    # Only echo this if debugging.
    # echo "Extract $filename from archive to $out_dir/$1."
    $ar p $lib_file $filename > $out_dir/$1
    break
  done
}

for f in $obj_files; do
  extract_object $f
done
