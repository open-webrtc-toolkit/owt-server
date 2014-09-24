/* Copyright (c) 2011 The WebM project authors. All Rights Reserved. */
/*  */
/* Use of this source code is governed by a BSD-style license */
/* that can be found in the LICENSE file in the root of the source */
/* tree. An additional intellectual property rights grant can be found */
/* in the file PATENTS.  All contributing project authors may */
/* be found in the AUTHORS file in the root of the source tree. */
static const char* const cfg = "--target=mips32-linux-gcc --disable-fast-unaligned --enable-external-build --enable-postproc --disable-install-srcs --enable-multi-res-encoding --enable-temporal-denoising --disable-vp9-encoder --disable-unit-tests --disable-install-docs --disable-examples";
const char *vpx_codec_build_config(void) {return cfg;}
