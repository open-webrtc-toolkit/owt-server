/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_ENCODE_TEST_DRIVER_H_
#define TEST_ENCODE_TEST_DRIVER_H_
#include <string>
#include <vector>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"

namespace libvpx_test {

class VideoSource;

enum TestMode {
  kRealTime,
  kOnePassGood,
  kOnePassBest,
  kTwoPassGood,
  kTwoPassBest
};
#define ALL_TEST_MODES ::testing::Values(::libvpx_test::kRealTime, \
                                         ::libvpx_test::kOnePassGood, \
                                         ::libvpx_test::kOnePassBest, \
                                         ::libvpx_test::kTwoPassGood, \
                                         ::libvpx_test::kTwoPassBest)

#define ONE_PASS_TEST_MODES ::testing::Values(::libvpx_test::kRealTime, \
                                              ::libvpx_test::kOnePassGood, \
                                              ::libvpx_test::kOnePassBest)


// Provides an object to handle the libvpx get_cx_data() iteration pattern
class CxDataIterator {
 public:
  explicit CxDataIterator(vpx_codec_ctx_t *encoder)
    : encoder_(encoder), iter_(NULL) {}

  const vpx_codec_cx_pkt_t *Next() {
    return vpx_codec_get_cx_data(encoder_, &iter_);
  }

 private:
  vpx_codec_ctx_t  *encoder_;
  vpx_codec_iter_t  iter_;
};

// Implements an in-memory store for libvpx twopass statistics
class TwopassStatsStore {
 public:
  void Append(const vpx_codec_cx_pkt_t &pkt) {
    buffer_.append(reinterpret_cast<char *>(pkt.data.twopass_stats.buf),
                   pkt.data.twopass_stats.sz);
  }

  vpx_fixed_buf_t buf() {
    const vpx_fixed_buf_t buf = { &buffer_[0], buffer_.size() };
    return buf;
  }

  void Reset() {
    buffer_.clear();
  }

 protected:
  std::string  buffer_;
};


// Provides a simplified interface to manage one video encoding pass, given
// a configuration and video source.
//
// TODO(jkoleszar): The exact services it provides and the appropriate
// level of abstraction will be fleshed out as more tests are written.
class Encoder {
 public:
  Encoder(vpx_codec_enc_cfg_t cfg, unsigned long deadline,
          const unsigned long init_flags, TwopassStatsStore *stats)
    : cfg_(cfg), deadline_(deadline), init_flags_(init_flags), stats_(stats) {
    memset(&encoder_, 0, sizeof(encoder_));
  }

  ~Encoder() {
    vpx_codec_destroy(&encoder_);
  }

  CxDataIterator GetCxData() {
    return CxDataIterator(&encoder_);
  }

  const vpx_image_t *GetPreviewFrame() {
    return vpx_codec_get_preview_frame(&encoder_);
  }
  // This is a thin wrapper around vpx_codec_encode(), so refer to
  // vpx_encoder.h for its semantics.
  void EncodeFrame(VideoSource *video, const unsigned long frame_flags);

  // Convenience wrapper for EncodeFrame()
  void EncodeFrame(VideoSource *video) {
    EncodeFrame(video, 0);
  }

  void Control(int ctrl_id, int arg) {
    const vpx_codec_err_t res = vpx_codec_control_(&encoder_, ctrl_id, arg);
    ASSERT_EQ(VPX_CODEC_OK, res) << EncoderError();
  }

  void set_deadline(unsigned long deadline) {
    deadline_ = deadline;
  }

 protected:
  const char *EncoderError() {
    const char *detail = vpx_codec_error_detail(&encoder_);
    return detail ? detail : vpx_codec_error(&encoder_);
  }

  // Encode an image
  void EncodeFrameInternal(const VideoSource &video,
                           const unsigned long frame_flags);

  // Flush the encoder on EOS
  void Flush();

  vpx_codec_ctx_t      encoder_;
  vpx_codec_enc_cfg_t  cfg_;
  unsigned long        deadline_;
  unsigned long        init_flags_;
  TwopassStatsStore   *stats_;
};

// Common test functionality for all Encoder tests.
//
// This class is a mixin which provides the main loop common to all
// encoder tests. It provides hooks which can be overridden by subclasses
// to implement each test's specific behavior, while centralizing the bulk
// of the boilerplate. Note that it doesn't inherit the gtest testing
// classes directly, so that tests can be parameterized differently.
class EncoderTest {
 protected:
  EncoderTest() : abort_(false), init_flags_(0), frame_flags_(0),
                  last_pts_(0) {}

  virtual ~EncoderTest() {}

  // Initialize the cfg_ member with the default configuration.
  void InitializeConfig() {
    const vpx_codec_err_t res = vpx_codec_enc_config_default(
                                    &vpx_codec_vp8_cx_algo, &cfg_, 0);
    ASSERT_EQ(VPX_CODEC_OK, res);
  }

  // Map the TestMode enum to the deadline_ and passes_ variables.
  void SetMode(TestMode mode);

  // Main loop.
  virtual void RunLoop(VideoSource *video);

  // Hook to be called at the beginning of a pass.
  virtual void BeginPassHook(unsigned int pass) {}

  // Hook to be called at the end of a pass.
  virtual void EndPassHook() {}

  // Hook to be called before encoding a frame.
  virtual void PreEncodeFrameHook(VideoSource *video) {}
  virtual void PreEncodeFrameHook(VideoSource *video, Encoder *encoder) {}

  // Hook to be called on every compressed data packet.
  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {}

  // Hook to be called on every PSNR packet.
  virtual void PSNRPktHook(const vpx_codec_cx_pkt_t *pkt) {}

  // Hook to determine whether the encode loop should continue.
  virtual bool Continue() const { return !abort_; }

  bool                 abort_;
  vpx_codec_enc_cfg_t  cfg_;
  unsigned int         passes_;
  unsigned long        deadline_;
  TwopassStatsStore    stats_;
  unsigned long        init_flags_;
  unsigned long        frame_flags_;
  vpx_codec_pts_t      last_pts_;
};

}  // namespace libvpx_test

#endif  // TEST_ENCODE_TEST_DRIVER_H_
