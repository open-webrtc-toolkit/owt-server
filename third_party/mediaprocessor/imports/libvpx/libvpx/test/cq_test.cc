/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <cmath>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "test/util.h"

namespace {

// CQ level range: [kCQLevelMin, kCQLevelMax).
const int kCQLevelMin = 4;
const int kCQLevelMax = 63;
const int kCQLevelStep = 8;
const int kCQTargetBitrate = 2000;

class CQTest : public ::libvpx_test::EncoderTest,
    public ::libvpx_test::CodecTestWithParam<int> {
 protected:
  CQTest() : EncoderTest(GET_PARAM(0)), cq_level_(GET_PARAM(1)) {
    init_flags_ = VPX_CODEC_USE_PSNR;
  }

  virtual ~CQTest() {}

  virtual void SetUp() {
    InitializeConfig();
    SetMode(libvpx_test::kTwoPassGood);
  }

  virtual void BeginPassHook(unsigned int /*pass*/) {
    file_size_ = 0;
    psnr_ = 0.0;
    n_frames_ = 0;
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  virtual void PreEncodeFrameHook(libvpx_test::VideoSource *video,
                                  libvpx_test::Encoder *encoder) {
    if (video->frame() == 1) {
      if (cfg_.rc_end_usage == VPX_CQ) {
        encoder->Control(VP8E_SET_CQ_LEVEL, cq_level_);
      }
      encoder->Control(VP8E_SET_CPUUSED, 3);
    }
  }

  virtual void PSNRPktHook(const vpx_codec_cx_pkt_t *pkt) {
    psnr_ += pow(10.0, pkt->data.psnr.psnr[0] / 10.0);
    n_frames_++;
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {
    file_size_ += pkt->data.frame.sz;
  }

  double GetLinearPSNROverBitrate() const {
    double avg_psnr = log10(psnr_ / n_frames_) * 10.0;
    return pow(10.0, avg_psnr / 10.0) / file_size_;
  }

  int file_size() const { return file_size_; }
  int n_frames() const { return n_frames_; }

 private:
  int cq_level_;
  int file_size_;
  double psnr_;
  int n_frames_;
};

int prev_actual_bitrate = kCQTargetBitrate;
TEST_P(CQTest, LinearPSNRIsHigherForCQLevel) {
  const vpx_rational timebase = { 33333333, 1000000000 };
  cfg_.g_timebase = timebase;
  cfg_.rc_target_bitrate = kCQTargetBitrate;
  cfg_.g_lag_in_frames = 25;

  cfg_.rc_end_usage = VPX_CQ;
  libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                     timebase.den, timebase.num, 0, 30);
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  const double cq_psnr_lin = GetLinearPSNROverBitrate();
  const int cq_actual_bitrate = file_size() * 8 * 30 / (n_frames() * 1000);
  EXPECT_LE(cq_actual_bitrate, kCQTargetBitrate);
  EXPECT_LE(cq_actual_bitrate, prev_actual_bitrate);
  prev_actual_bitrate = cq_actual_bitrate;

  // try targeting the approximate same bitrate with VBR mode
  cfg_.rc_end_usage = VPX_VBR;
  cfg_.rc_target_bitrate = cq_actual_bitrate;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
  const double vbr_psnr_lin = GetLinearPSNROverBitrate();
  EXPECT_GE(cq_psnr_lin, vbr_psnr_lin);
}

VP8_INSTANTIATE_TEST_CASE(CQTest,
                          ::testing::Range(kCQLevelMin, kCQLevelMax,
                                           kCQLevelStep));
}  // namespace
