/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/decode_test_driver.h"
#include "test/ivf_video_source.h"

#if CONFIG_DECRYPT

namespace {

const uint8_t decrypt_key[32] = {
  255, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

}  // namespace

namespace libvpx_test {

TEST(TestDecrypt, NullKey) {
  vpx_codec_dec_cfg_t cfg = {0};
  vpx_codec_ctx_t decoder = {0};
  vpx_codec_err_t res = vpx_codec_dec_init(&decoder, &vpx_codec_vp8_dx_algo,
                                           &cfg, 0);
  ASSERT_EQ(VPX_CODEC_OK, res);

  res = vpx_codec_control(&decoder, VP8_SET_DECRYPT_KEY, NULL);
  ASSERT_EQ(VPX_CODEC_INVALID_PARAM, res);
}

TEST(TestDecrypt, DecryptWorks) {
  libvpx_test::IVFVideoSource video("vp80-00-comprehensive-001.ivf");
  video.Init();

  vpx_codec_dec_cfg_t dec_cfg = {0};
  Decoder decoder(dec_cfg, 0);

  // Zero decrypt key (by default)
  video.Begin();
  vpx_codec_err_t res = decoder.DecodeFrame(video.cxdata(), video.frame_size());
  ASSERT_EQ(VPX_CODEC_OK, res) << decoder.DecodeError();

  // Non-zero decrypt key
  video.Next();
  decoder.Control(VP8_SET_DECRYPT_KEY, decrypt_key);
  res = decoder.DecodeFrame(video.cxdata(), video.frame_size());
  ASSERT_NE(VPX_CODEC_OK, res) << decoder.DecodeError();
}

}  // namespace libvpx_test

#endif  // CONFIG_DECRYPT
