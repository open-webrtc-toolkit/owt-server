#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script is used to generate .gypi files and files in the config/platform
# directories needed to build libvpx.
# Every time libvpx source code is updated just run this script.
#
# For example:
# $ ./generate_gypi.sh
#
# And this will update all the .gypi and config files needed.
#

BASE_DIR=`pwd`
LIBVPX_SRC_DIR="source/libvpx"
LIBVPX_CONFIG_DIR="source/config"

# Convert a list of source files into gypi file.
# $1 - Input file.
# $2 - Output gypi file.
function convert_srcs_to_gypi {
  # Do the following here:
  # 1. Filter .c, .h, .s, .S and .asm files.
  # 2. Exclude certain files. If you want to add more files to exclude from
  #    the .gypi add a new -v option. I.e. grep -v '_offsets\.c' or grep -v
  #    'vp9_filter_sse4\.c'
  # 3. Replace .asm.s to .asm because gyp will do the conversion.
  local source_list=$(grep -E '(\.c|\.h|\.S|\.s|\.asm)$' $1)
  source_list=$(echo "$source_list" | grep -v '_offsets\.c')
  source_list=$(echo "$source_list" | grep -v 'vpx_config\.c')
  source_list=$(echo "$source_list" | grep -v 'denoising_sse2\.c')
  source_list=$(echo "$source_list" | grep -v 'vp9_filter_sse2\.c')
  source_list=$(echo "$source_list" | grep -v 'vp9_loopfilter_x86\.c')
  source_list=$(echo "$source_list" | grep -v 'vp9_sadmxn_x86\.c')
  source_list=$(echo "$source_list" | grep -v 'vp9_filter_sse4\.c')
  source_list=$(echo "$source_list" | sed s/\.asm\.s$/.asm/)

  # Build the gypi file.
  echo "# This file is generated. Do not edit." > $2
  echo "# Copyright (c) 2012 The Chromium Authors. All rights reserved." >> $2
  echo "# Use of this source code is governed by a BSD-style license that can be" >> $2
  echo "# found in the LICENSE file." >> $2
  echo "" >> $2
  echo "{" >> $2
  echo "  'sources': [" >> $2
  for f in $source_list
  do
    echo "    '<(libvpx_source)/$f'," >> $2
  done
  echo "  ]," >> $2
  echo "}" >> $2
}

# Clean files from previous make.
function make_clean {
  make clean > /dev/null
  rm -f libvpx_srcs.txt
}

# Lint a pair of vpx_config.h and vpx_config.asm to make sure they match.
# $1 - Header file directory.
function lint_config {
  $BASE_DIR/lint_config.sh \
    -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
    -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
}

# Print the configuration.
# $1 - Header file directory.
function print_config {
  $BASE_DIR/lint_config.sh -p \
    -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
    -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm
}

# Print the configuration from Header file.
# This function is an abridged version of print_config which does not use
# lint_config and it does not require existence of vpx_config.asm.
# $1 - Header file directory.
function print_config_basic {
  combined_config="$(cat $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
                   | grep -E ' +[01] *$')"
  combined_config="$(echo "$combined_config" | grep -v DO1STROUNDING)"
  combined_config="$(echo "$combined_config" | sed 's/[ \t]//g')"
  combined_config="$(echo "$combined_config" | sed 's/.*define//')"
  combined_config="$(echo "$combined_config" | sed 's/0$/=no/')"
  combined_config="$(echo "$combined_config" | sed 's/1$/=yes/')"
  echo "$combined_config" | sort | uniq
}

# Generate *_rtcd.h files.
# $1 - Header file directory.
# $2 - Architecture.
function gen_rtcd_header {
  echo "Generate $LIBVPX_CONFIG_DIR/$1/*_rtcd.h files."

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
  if [ "$2" = "mipsel" ]; then
    print_config_basic $1 > $BASE_DIR/$TEMP_DIR/libvpx.config
  else
    $BASE_DIR/lint_config.sh -p \
      -h $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.h \
      -a $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_config.asm \
      -o $BASE_DIR/$TEMP_DIR/libvpx.config
  fi

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.sh \
    --arch=$2 \
    --sym=vp8_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp8/common/rtcd_defs.sh \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp8_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.sh \
    --arch=$2 \
    --sym=vp9_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vp9/common/vp9_rtcd_defs.sh \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vp9_rtcd.h

  $BASE_DIR/$LIBVPX_SRC_DIR/build/make/rtcd.sh \
    --arch=$2 \
    --sym=vpx_scale_rtcd \
    --config=$BASE_DIR/$TEMP_DIR/libvpx.config \
    $BASE_DIR/$LIBVPX_SRC_DIR/vpx_scale/vpx_scale_rtcd.sh \
    > $BASE_DIR/$LIBVPX_CONFIG_DIR/$1/vpx_scale_rtcd.h

  rm -rf $BASE_DIR/$TEMP_DIR/libvpx.config
}

# Generate Config files. "--enable-external-build" must be set to skip
# detection of capabilities on specific targets.
# $1 - Header file directory.
# $2 - Config command line.
function gen_config_files {
  ./configure $2  > /dev/null

  # Generate vpx_config.asm. Do not create one for mips.
  if [[ "$1" != *mipsel ]]; then
    if [[ "$1" == *x64* ]] || [[ "$1" == *ia32* ]]; then
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h | awk '{print $2 " equ " $3}' > vpx_config.asm
    else
      egrep "#define [A-Z0-9_]+ [01]" vpx_config.h | awk '{print $2 " EQU " $3}' | perl $BASE_DIR/$LIBVPX_SRC_DIR/build/make/ads2gas.pl > vpx_config.asm
    fi
  fi

  cp vpx_config.* $BASE_DIR/$LIBVPX_CONFIG_DIR/$1
  make_clean
  rm -rf vpx_config.*
}

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

echo "Generate Config Files"
all_platforms="--enable-external-build --enable-postproc --disable-install-srcs --enable-multi-res-encoding --enable-temporal-denoising --disable-vp9-encoder --disable-unit-tests --disable-install-docs --disable-examples"
gen_config_files linux/ia32 "--target=x86-linux-gcc --disable-ccache --enable-pic --enable-realtime-only ${all_platforms}"
gen_config_files linux/x64 "--target=x86_64-linux-gcc --disable-ccache --enable-pic --enable-realtime-only ${all_platforms}"
gen_config_files linux/arm "--target=armv6-linux-gcc --enable-pic --enable-realtime-only --disable-install-bins --disable-install-libs ${all_platforms}"
gen_config_files linux/arm-neon "--target=armv7-linux-gcc --enable-pic --enable-realtime-only ${all_platforms}"
gen_config_files linux/mipsel "--target=mips32-linux-gcc --disable-fast-unaligned ${all_platforms}"
gen_config_files win/ia32 "--target=x86-win32-vs7 --enable-realtime-only ${all_platforms}"
gen_config_files win/x64 "--target=x86_64-win64-vs9 --enable-realtime-only ${all_platforms}"
gen_config_files mac/ia32 "--target=x86-darwin9-gcc --enable-pic --enable-realtime-only ${all_platforms}"
gen_config_files mac/x64 "--target=x86_64-darwin9-gcc --enable-pic --enable-realtime-only ${all_platforms}"

echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR

echo "Lint libvpx configuration."
lint_config linux/ia32
lint_config linux/x64
lint_config linux/arm
lint_config linux/arm-neon
lint_config win/ia32
lint_config win/x64
lint_config mac/ia32
lint_config mac/x64

echo "Create temporary directory."
TEMP_DIR="$LIBVPX_SRC_DIR.temp"
rm -rf $TEMP_DIR
cp -R $LIBVPX_SRC_DIR $TEMP_DIR
cd $TEMP_DIR

gen_rtcd_header linux/ia32 x86
gen_rtcd_header linux/x64 x86_64
gen_rtcd_header linux/arm armv6
gen_rtcd_header linux/arm-neon armv7
gen_rtcd_header linux/mipsel mipsel
gen_rtcd_header win/ia32 x86
gen_rtcd_header win/x64 x86_64
gen_rtcd_header mac/ia32 x86
gen_rtcd_header mac/x64 x86_64

echo "Prepare Makefile."
./configure --target=generic-gnu > /dev/null
make_clean

echo "Generate X86 source list."
config=$(print_config linux/ia32)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
convert_srcs_to_gypi libvpx_srcs.txt $BASE_DIR/libvpx_srcs_x86.gypi

# Copy vpx_version.h. The file should be the same for all platforms.
cp vpx_version.h $BASE_DIR/$LIBVPX_CONFIG_DIR

echo "Generate X86_64 source list."
config=$(print_config linux/x64)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
convert_srcs_to_gypi libvpx_srcs.txt $BASE_DIR/libvpx_srcs_x86_64.gypi

echo "Generate ARM source list."
config=$(print_config linux/arm)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
convert_srcs_to_gypi libvpx_srcs.txt $BASE_DIR/libvpx_srcs_arm.gypi

echo "Generate ARM NEON source list."
config=$(print_config linux/arm-neon)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
convert_srcs_to_gypi libvpx_srcs.txt $BASE_DIR/libvpx_srcs_arm_neon.gypi

echo "Generate MIPS source list."
config=$(print_config_basic linux/mipsel)
make_clean
make libvpx_srcs.txt target=libs $config > /dev/null
convert_srcs_to_gypi libvpx_srcs.txt $BASE_DIR/libvpx_srcs_mips.gypi

echo "Remove temporary directory."
cd $BASE_DIR
rm -rf $TEMP_DIR

# TODO(fgalligan): Is "--disable-fast-unaligned" needed on mipsel?
# TODO(fgalligan): Can we turn on "--enable-realtime-only" for mipsel?
