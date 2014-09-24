/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/encode_test_driver.h"
#include "test/video_source.h"

namespace {

class ConfigTest : public ::libvpx_test::EncoderTest,
    public ::testing::TestWithParam<enum libvpx_test::TestMode> {
 public:
  ConfigTest() : frame_count_in_(0), frame_count_out_(0), frame_count_max_(0) {}

 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(GetParam());
  }

  virtual void BeginPassHook(unsigned int /*pass*/) {
    frame_count_in_ = 0;
    frame_count_out_ = 0;
  }

  virtual void PreEncodeFrameHook(libvpx_test::VideoSource* /*video*/) {
    ++frame_count_in_;
    abort_ |= (frame_count_in_ >= frame_count_max_);
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t* /*pkt*/) {
    ++frame_count_out_;
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  unsigned int frame_count_in_;
  unsigned int frame_count_out_;
  unsigned int frame_count_max_;
};

TEST_P(ConfigTest, LagIsDisabled) {
  frame_count_max_ = 2;
  cfg_.g_lag_in_frames = 15;

  libvpx_test::DummyVideoSource video;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));

  EXPECT_EQ(frame_count_in_, frame_count_out_);
}

INSTANTIATE_TEST_CASE_P(OnePassModes, ConfigTest, ONE_PASS_TEST_MODES);
}  // namespace
