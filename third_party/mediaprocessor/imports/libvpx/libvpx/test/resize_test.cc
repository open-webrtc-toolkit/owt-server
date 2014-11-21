/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <climits>
#include <vector>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/codec_factory.h"
#include "test/encode_test_driver.h"
#include "test/i420_video_source.h"
#include "test/video_source.h"
#include "test/util.h"

namespace {

const unsigned int kInitialWidth = 320;
const unsigned int kInitialHeight = 240;

unsigned int ScaleForFrameNumber(unsigned int frame, unsigned int val) {
  if (frame < 10)
    return val;
  if (frame < 20)
    return val / 2;
  if (frame < 30)
    return val * 2 / 3;
  if (frame < 40)
    return val / 4;
  if (frame < 50)
    return val * 7 / 8;
  return val;
}

class ResizingVideoSource : public ::libvpx_test::DummyVideoSource {
 public:
  ResizingVideoSource() {
    SetSize(kInitialWidth, kInitialHeight);
    limit_ = 60;
  }

 protected:
  virtual void Next() {
    ++frame_;
    SetSize(ScaleForFrameNumber(frame_, kInitialWidth),
            ScaleForFrameNumber(frame_, kInitialHeight));
    FillFrame();
  }
};

class ResizeTest : public ::libvpx_test::EncoderTest,
  public ::libvpx_test::CodecTestWithParam<libvpx_test::TestMode> {
 protected:
  ResizeTest() : EncoderTest(GET_PARAM(0)) {}

  struct FrameInfo {
    FrameInfo(vpx_codec_pts_t _pts, unsigned int _w, unsigned int _h)
        : pts(_pts), w(_w), h(_h) {}

    vpx_codec_pts_t pts;
    unsigned int    w;
    unsigned int    h;
  };

  virtual void SetUp() {
    InitializeConfig();
    SetMode(GET_PARAM(1));
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  virtual void DecompressedFrameHook(const vpx_image_t &img,
                                     vpx_codec_pts_t pts) {
    frame_info_list_.push_back(FrameInfo(pts, img.d_w, img.d_h));
  }

  std::vector< FrameInfo > frame_info_list_;
};

TEST_P(ResizeTest, TestExternalResizeWorks) {
  ResizingVideoSource video;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));

  for (std::vector<FrameInfo>::iterator info = frame_info_list_.begin();
       info != frame_info_list_.end(); ++info) {
    const vpx_codec_pts_t pts = info->pts;
    const unsigned int expected_w = ScaleForFrameNumber(pts, kInitialWidth);
    const unsigned int expected_h = ScaleForFrameNumber(pts, kInitialHeight);

    EXPECT_EQ(expected_w, info->w)
        << "Frame " << pts << "had unexpected width";
    EXPECT_EQ(expected_h, info->h)
        << "Frame " << pts << "had unexpected height";
  }
}

class ResizeInternalTest : public ResizeTest {
 protected:
  ResizeInternalTest() : ResizeTest(), frame0_psnr_(0.0) {}

  virtual void PreEncodeFrameHook(libvpx_test::VideoSource *video,
                                  libvpx_test::Encoder *encoder) {
    if (video->frame() == 3) {
      struct vpx_scaling_mode mode = {VP8E_FOURFIVE, VP8E_THREEFIVE};
      encoder->Control(VP8E_SET_SCALEMODE, &mode);
    }
    if (video->frame() == 6) {
      struct vpx_scaling_mode mode = {VP8E_NORMAL, VP8E_NORMAL};
      encoder->Control(VP8E_SET_SCALEMODE, &mode);
    }
  }

  virtual void PSNRPktHook(const vpx_codec_cx_pkt_t *pkt) {
    if (!frame0_psnr_)
      frame0_psnr_ = pkt->data.psnr.psnr[0];
    EXPECT_NEAR(pkt->data.psnr.psnr[0], frame0_psnr_, 1.0);
  }

  double frame0_psnr_;
};

TEST_P(ResizeInternalTest, TestInternalResizeWorks) {
  ::libvpx_test::I420VideoSource video("hantro_collage_w352h288.yuv", 352, 288,
                                       30, 1, 0, 10);
  init_flags_ = VPX_CODEC_USE_PSNR;
  // q picked such that initial keyframe on this clip is ~30dB PSNR
  cfg_.rc_min_quantizer = cfg_.rc_max_quantizer = 48;
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));

  for (std::vector<FrameInfo>::iterator info = frame_info_list_.begin();
       info != frame_info_list_.end(); ++info) {
    const vpx_codec_pts_t pts = info->pts;
    if (pts >= 3 && pts < 6) {
      ASSERT_EQ(282U, info->w) << "Frame " << pts << " had unexpected width";
      ASSERT_EQ(173U, info->h) << "Frame " << pts << " had unexpected height";
    } else {
      EXPECT_EQ(352U, info->w) << "Frame " << pts << " had unexpected width";
      EXPECT_EQ(288U, info->h) << "Frame " << pts << " had unexpected height";
    }
  }
}

VP8_INSTANTIATE_TEST_CASE(ResizeTest, ONE_PASS_TEST_MODES);
VP9_INSTANTIATE_TEST_CASE(ResizeInternalTest,
                          ::testing::Values(::libvpx_test::kOnePassBest));
}  // namespace
