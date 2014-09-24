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
#include "test/encode_test_driver.h"
#include "test/video_source.h"
#include "third_party/googletest/src/include/gtest/gtest.h"

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
  public ::testing::TestWithParam<enum libvpx_test::TestMode> {
 protected:
  struct FrameInfo {
    FrameInfo(vpx_codec_pts_t _pts, unsigned int _w, unsigned int _h)
        : pts(_pts), w(_w), h(_h) {}

    vpx_codec_pts_t pts;
    unsigned int    w;
    unsigned int    h;
  };

  virtual void SetUp() {
    InitializeConfig();
    SetMode(GetParam());
  }

  virtual bool Continue() const {
    return !HasFatalFailure() && !abort_;
  }

  virtual void FramePktHook(const vpx_codec_cx_pkt_t *pkt) {
    if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
      const unsigned char *buf =
          reinterpret_cast<const unsigned char *>(pkt->data.frame.buf);
      const unsigned int w = (buf[6] | (buf[7] << 8)) & 0x3fff;
      const unsigned int h = (buf[8] | (buf[9] << 8)) & 0x3fff;

      frame_info_list_.push_back(FrameInfo(pkt->data.frame.pts, w, h));
    }
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

INSTANTIATE_TEST_CASE_P(OnePass, ResizeTest, ONE_PASS_TEST_MODES);
}  // namespace
