/*
  Copyright (c) 2012 The WebM project authors. All Rights Reserved.

  Use of this source code is governed by a BSD-style license
  that can be found in the LICENSE file in the root of the source
  tree. An additional intellectual property rights grant can be found
  in the file PATENTS.  All contributing project authors may
  be found in the AUTHORS file in the root of the source tree.
*/

#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "test/util.h"

namespace {

const int kMaxErrorFrames = 8;
const int kMaxDroppableFrames = 8;

class ErrorResilienceTest : public ::libvpx_test::EncoderTest,
    public ::libvpx_test::CodecTestWithParam<libvpx_test::TestMode> {
 protected:
  ErrorResilienceTest() : EncoderTest(GET_PARAM(0)),
                          psnr_(0.0),
                          nframes_(0),
                          mismatch_psnr_(0.0),
                          mismatch_nframes_(0),
                          encoding_mode_(GET_PARAM(1)) {
    Reset();
  }

  virtual ~ErrorResilienceTest() {}

  void Reset() {
    error_nframes_ = 0;
    droppable_nframes_ = 0;
  }

  virtual void SetUp() {
    InitializeConfig();
    SetMode(encoding_mode_);
  }

  virtual void BeginPassHook(unsigned int /*pass*/) {
    psnr_ = 0.0;
    nframes_ = 0;
    mismatch_psnr_ = 0.0;
    mismatch_nframes_ = 0;
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  virtual void PSNRPktHook(const vpx_codec_cx_pkt_t *pkt) {
    psnr_ += pkt->data.psnr.psnr[0];
    nframes_++;
  }

  virtual void PreEncodeFrameHook(libvpx_test::VideoSource *video) {
    frame_flags_ &= ~(VP8_EFLAG_NO_UPD_LAST |
                      VP8_EFLAG_NO_UPD_GF |
                      VP8_EFLAG_NO_UPD_ARF);
    if (droppable_nframes_ > 0 &&
        (cfg_.g_pass == VPX_RC_LAST_PASS || cfg_.g_pass == VPX_RC_ONE_PASS)) {
      for (unsigned int i = 0; i < droppable_nframes_; ++i) {
        if (droppable_frames_[i] == nframes_) {
          std::cout << "             Encoding droppable frame: "
                    << droppable_frames_[i] << "\n";
          frame_flags_ |= (VP8_EFLAG_NO_UPD_LAST |
                           VP8_EFLAG_NO_UPD_GF |
                           VP8_EFLAG_NO_UPD_ARF);
          return;
        }
      }
    }
  }

  double GetAveragePsnr() const {
    if (nframes_)
      return psnr_ / nframes_;
    return 0.0;
  }

  double GetAverageMismatchPsnr() const {
    if (mismatch_nframes_)
      return mismatch_psnr_ / mismatch_nframes_;
    return 0.0;
  }

  virtual bool DoDecode() const {
    if (error_nframes_ > 0 &&
        (cfg_.g_pass == VPX_RC_LAST_PASS || cfg_.g_pass == VPX_RC_ONE_PASS)) {
      for (unsigned int i = 0; i < error_nframes_; ++i) {
        if (error_frames_[i] == nframes_ - 1) {
          std::cout << "             Skipping decoding frame: "
                    << error_frames_[i] << "\n";
          return 0;
        }
      }
    }
    return 1;
  }

  virtual void MismatchHook(const vpx_image_t *img1,
                            const vpx_image_t *img2) {
    double mismatch_psnr = compute_psnr(img1, img2);
    mismatch_psnr_ += mismatch_psnr;
    ++mismatch_nframes_;
    // std::cout << "Mismatch frame psnr: " << mismatch_psnr << "\n";
  }

  void SetErrorFrames(int num, unsigned int *list) {
    if (num > kMaxErrorFrames)
      num = kMaxErrorFrames;
    else if (num < 0)
      num = 0;
    error_nframes_ = num;
    for (unsigned int i = 0; i < error_nframes_; ++i)
      error_frames_[i] = list[i];
  }

  void SetDroppableFrames(int num, unsigned int *list) {
    if (num > kMaxDroppableFrames)
      num = kMaxDroppableFrames;
    else if (num < 0)
      num = 0;
    droppable_nframes_ = num;
    for (unsigned int i = 0; i < droppable_nframes_; ++i)
      droppable_frames_[i] = list[i];
  }

  unsigned int GetMismatchFrames() {
    return mismatch_nframes_;
  }

 private:
  double psnr_;
  unsigned int nframes_;
  unsigned int error_nframes_;
  unsigned int droppable_nframes_;
  double mismatch_psnr_;
  unsigned int mismatch_nframes_;
  unsigned int error_frames_[kMaxErrorFrames];
  unsigned int droppable_frames_[kMaxDroppableFrames];
  libvpx_test::TestMode encoding_mode_;
};

TEST_P(ErrorResilienceTest, OnVersusOff) {
  const vpx_rational timebase = { 33333333, 1000000000 };
  cfg_.g_timebase = timebase;
  cfg_.rc_target_bitrate = 2000;
  cfg_.g_lag_in_frames = 25;

  init_flags_ = VPX_CODEC_USE_PSNR;

  libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                     timebase.den, timebase.num, 0, 30);

  // Error resilient mode OFF.
  cfg_.g_error_resilient = 0;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  const double psnr_resilience_off = GetAveragePsnr();
  EXPECT_GT(psnr_resilience_off, 25.0);

  // Error resilient mode ON.
  cfg_.g_error_resilient = 1;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  const double psnr_resilience_on = GetAveragePsnr();
  EXPECT_GT(psnr_resilience_on, 25.0);

  // Test that turning on error resilient mode hurts by 10% at most.
  if (psnr_resilience_off > 0.0) {
    const double psnr_ratio = psnr_resilience_on / psnr_resilience_off;
    EXPECT_GE(psnr_ratio, 0.9);
    EXPECT_LE(psnr_ratio, 1.1);
  }
}

TEST_P(ErrorResilienceTest, DropFramesWithoutRecovery) {
  const vpx_rational timebase = { 33333333, 1000000000 };
  cfg_.g_timebase = timebase;
  cfg_.rc_target_bitrate = 500;

  init_flags_ = VPX_CODEC_USE_PSNR;

  libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                     timebase.den, timebase.num, 0, 30);

  // Error resilient mode ON.
  cfg_.g_error_resilient = 1;

  // Set an arbitrary set of error frames same as droppable frames
  unsigned int num_droppable_frames = 2;
  unsigned int droppable_frame_list[] = {5, 16};
  SetDroppableFrames(num_droppable_frames, droppable_frame_list);
  SetErrorFrames(num_droppable_frames, droppable_frame_list);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  // Test that no mismatches have been found
  std::cout << "             Mismatch frames: "
            << GetMismatchFrames() << "\n";
  EXPECT_EQ(GetMismatchFrames(), (unsigned int) 0);

  // reset previously set error/droppable frames
  Reset();

  // Now set an arbitrary set of error frames that are non-droppable
  unsigned int num_error_frames = 3;
  unsigned int error_frame_list[] = {3, 10, 20};
  SetErrorFrames(num_error_frames, error_frame_list);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  // Test that dropping an arbitrary set of inter frames does not hurt too much
  // Note the Average Mismatch PSNR is the average of the PSNR between
  // decoded frame and encoder's version of the same frame for all frames
  // with mismatch.
  const double psnr_resilience_mismatch = GetAverageMismatchPsnr();
  std::cout << "             Mismatch PSNR: "
            << psnr_resilience_mismatch << "\n";
  EXPECT_GT(psnr_resilience_mismatch, 20.0);
}

VP8_INSTANTIATE_TEST_CASE(ErrorResilienceTest, ONE_PASS_TEST_MODES);
VP9_INSTANTIATE_TEST_CASE(ErrorResilienceTest, ONE_PASS_TEST_MODES);

}  // namespace
