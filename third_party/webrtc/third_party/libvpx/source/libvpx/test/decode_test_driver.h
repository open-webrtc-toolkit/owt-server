/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_DECODE_TEST_DRIVER_H_
#define TEST_DECODE_TEST_DRIVER_H_
#include <cstring>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "vpx_config.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"

namespace libvpx_test {

class CompressedVideoSource;

// Provides an object to handle decoding output
class DxDataIterator {
 public:
  explicit DxDataIterator(vpx_codec_ctx_t *decoder)
      : decoder_(decoder), iter_(NULL) {}

  const vpx_image_t *Next() {
    return vpx_codec_get_frame(decoder_, &iter_);
  }

 private:
  vpx_codec_ctx_t  *decoder_;
  vpx_codec_iter_t  iter_;
};

// Provides a simplified interface to manage one video decoding.
//
// TODO: similar to Encoder class, the exact services should be
// added as more tests are added.
class Decoder {
 public:
  Decoder(vpx_codec_dec_cfg_t cfg, unsigned long deadline)
      : cfg_(cfg), deadline_(deadline) {
    memset(&decoder_, 0, sizeof(decoder_));
  }

  ~Decoder() {
    vpx_codec_destroy(&decoder_);
  }

  void DecodeFrame(const uint8_t *cxdata, int size);

  DxDataIterator GetDxData() {
    return DxDataIterator(&decoder_);
  }

  void set_deadline(unsigned long deadline) {
    deadline_ = deadline;
  }

  void Control(int ctrl_id, int arg) {
    const vpx_codec_err_t res = vpx_codec_control_(&decoder_, ctrl_id, arg);
    ASSERT_EQ(VPX_CODEC_OK, res) << DecodeError();
  }

 protected:
  const char *DecodeError() {
    const char *detail = vpx_codec_error_detail(&decoder_);
    return detail ? detail : vpx_codec_error(&decoder_);
  }

  vpx_codec_ctx_t     decoder_;
  vpx_codec_dec_cfg_t cfg_;
  unsigned int        deadline_;
};

// Common test functionality for all Decoder tests.
class DecoderTest {
 public:
  // Main loop.
  virtual void RunLoop(CompressedVideoSource *video);

  // Hook to be called on every decompressed frame.
  virtual void DecompressedFrameHook(const vpx_image_t& img,
                                     const unsigned int frame_number) {}

 protected:
  DecoderTest() {}

  virtual ~DecoderTest() {}
};

}  // namespace libvpx_test

#endif  // TEST_DECODE_TEST_DRIVER_H_
