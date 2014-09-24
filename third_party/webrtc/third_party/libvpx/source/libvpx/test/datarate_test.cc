/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "third_party/googletest/src/include/gtest/gtest.h"
namespace {

class DatarateTest : public ::libvpx_test::EncoderTest,
    public ::testing::TestWithParam<enum libvpx_test::TestMode> {
 protected:
  virtual void SetUp() {
    InitializeConfig();
    SetMode(GetParam());
    ResetModel();
  }

  virtual void ResetModel() {
    last_pts_ = 0;
    bits_in_buffer_model_ = cfg_.rc_target_bitrate * cfg_.rc_buf_initial_sz;
    frame_number_ = 0;
    first_drop_ = 0;
    bits_total_ = 0;
    duration_ = 0.0;
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  virtual void PreEncodeFrameHook(::libvpx_test::VideoSource *video,
                                  ::libvpx_test::Encoder *encoder) {
    const vpx_rational_t tb = video->timebase();
    timebase_ = static_cast<double>(tb.num) / tb.den;
    duration_ = 0;
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {
    // Time since last timestamp = duration.
    vpx_codec_pts_t duration = pkt->data.frame.pts - last_pts_;

    // TODO(jimbankoski): Remove these lines when the issue:
    // http://code.google.com/p/webm/issues/detail?id=496 is fixed.
    // For now the codec assumes buffer starts at starting buffer rate
    // plus one frame's time.
    if (last_pts_ == 0)
      duration = 1;

    // Add to the buffer the bits we'd expect from a constant bitrate server.
    bits_in_buffer_model_ += duration * timebase_ * cfg_.rc_target_bitrate
        * 1000;

    /* Test the buffer model here before subtracting the frame. Do so because
     * the way the leaky bucket model works in libvpx is to allow the buffer to
     * empty - and then stop showing frames until we've got enough bits to
     * show one. As noted in comment below (issue 495), this does not currently
     * apply to key frames. For now exclude key frames in condition below. */
    bool key_frame = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? true: false;
    if (!key_frame) {
      ASSERT_GE(bits_in_buffer_model_, 0) << "Buffer Underrun at frame "
          << pkt->data.frame.pts;
    }

    const int frame_size_in_bits = pkt->data.frame.sz * 8;

    // Subtract from the buffer the bits associated with a played back frame.
    bits_in_buffer_model_ -= frame_size_in_bits;

    // Update the running total of bits for end of test datarate checks.
    bits_total_ += frame_size_in_bits ;

    // If first drop not set and we have a drop set it to this time.
    if (!first_drop_ && duration > 1)
      first_drop_ = last_pts_ + 1;

    // Update the most recent pts.
    last_pts_ = pkt->data.frame.pts;

    // We update this so that we can calculate the datarate minus the last
    // frame encoded in the file.
    bits_in_last_frame_ = frame_size_in_bits;

    ++frame_number_;
  }

  virtual void EndPassHook(void) {
    if (bits_total_) {
      const double file_size_in_kb = bits_total_ / 1000;  /* bits per kilobit */

      duration_ = (last_pts_ + 1) * timebase_;

      // Effective file datarate includes the time spent prebuffering.
      effective_datarate_ = (bits_total_ - bits_in_last_frame_) / 1000.0
          / (cfg_.rc_buf_initial_sz / 1000.0 + duration_);

      file_datarate_ = file_size_in_kb / duration_;
    }
  }

  vpx_codec_pts_t last_pts_;
  int bits_in_buffer_model_;
  double timebase_;
  int frame_number_;
  vpx_codec_pts_t first_drop_;
  int64_t bits_total_;
  double duration_;
  double file_datarate_;
  double effective_datarate_;
  int bits_in_last_frame_;
};

TEST_P(DatarateTest, BasicBufferModel) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_dropframe_thresh = 1;
  cfg_.rc_max_quantizer = 56;
  cfg_.rc_end_usage = VPX_CBR;
  // 2 pass cbr datarate control has a bug hidden by the small # of
  // frames selected in this encode. The problem is that even if the buffer is
  // negative we produce a keyframe on a cutscene. Ignoring datarate
  // constraints
  // TODO(jimbankoski): ( Fix when issue
  // http://code.google.com/p/webm/issues/detail?id=495 is addressed. )
  ::libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                       30, 1, 0, 140);

  // There is an issue for low bitrates in real-time mode, where the
  // effective_datarate slightly overshoots the target bitrate.
  // This is same the issue as noted about (#495).
  // TODO(jimbankoski/marpan): Update test to run for lower bitrates (< 100),
  // when the issue is resolved.
  for (int i = 100; i < 800; i += 200) {
    cfg_.rc_target_bitrate = i;
    ResetModel();
    ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
    ASSERT_GE(cfg_.rc_target_bitrate, effective_datarate_)
        << " The datarate for the file exceeds the target!";

    ASSERT_LE(cfg_.rc_target_bitrate, file_datarate_ * 1.3)
        << " The datarate for the file missed the target!";
  }
}

TEST_P(DatarateTest, ChangingDropFrameThresh) {
  cfg_.rc_buf_initial_sz = 500;
  cfg_.rc_max_quantizer = 36;
  cfg_.rc_end_usage = VPX_CBR;
  cfg_.rc_target_bitrate = 200;
  cfg_.kf_mode = VPX_KF_DISABLED;

  const int frame_count = 40;
  ::libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                       30, 1, 0, frame_count);

  // Here we check that the first dropped frame gets earlier and earlier
  // as the drop frame threshold is increased.

  const int kDropFrameThreshTestStep = 30;
  vpx_codec_pts_t last_drop = frame_count;
  for (int i = 1; i < 91; i += kDropFrameThreshTestStep) {
    cfg_.rc_dropframe_thresh = i;
    ResetModel();
    ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
    ASSERT_LE(first_drop_, last_drop)
        << " The first dropped frame for drop_thresh " << i
        << " > first dropped frame for drop_thresh "
        << i - kDropFrameThreshTestStep;
    last_drop = first_drop_;
  }
}

INSTANTIATE_TEST_CASE_P(AllModes, DatarateTest, ALL_TEST_MODES);
}  // namespace
