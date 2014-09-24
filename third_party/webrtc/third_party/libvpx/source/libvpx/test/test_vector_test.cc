/*
 Copyright (c) 2012 The WebM project authors. All Rights Reserved.

 Use of this source code is governed by a BSD-style license
 that can be found in the LICENSE file in the root of the source
 tree. An additional intellectual property rights grant can be found
 in the file PATENTS.  All contributing project authors may
 be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "test/decode_test_driver.h"
#include "test/ivf_video_source.h"
extern "C" {
#include "./md5_utils.h"
#include "vpx_mem/vpx_mem.h"
}

#if defined(_MSC_VER)
#define snprintf sprintf_s
#endif

namespace {
// There are 61 test vectors in total.
const char *kTestVectors[] = {
  "vp80-00-comprehensive-001.ivf",
  "vp80-00-comprehensive-002.ivf", "vp80-00-comprehensive-003.ivf",
  "vp80-00-comprehensive-004.ivf", "vp80-00-comprehensive-005.ivf",
  "vp80-00-comprehensive-006.ivf", "vp80-00-comprehensive-007.ivf",
  "vp80-00-comprehensive-008.ivf", "vp80-00-comprehensive-009.ivf",
  "vp80-00-comprehensive-010.ivf", "vp80-00-comprehensive-011.ivf",
  "vp80-00-comprehensive-012.ivf", "vp80-00-comprehensive-013.ivf",
  "vp80-00-comprehensive-014.ivf", "vp80-00-comprehensive-015.ivf",
  "vp80-00-comprehensive-016.ivf", "vp80-00-comprehensive-017.ivf",
  "vp80-00-comprehensive-018.ivf", "vp80-01-intra-1400.ivf",
  "vp80-01-intra-1411.ivf", "vp80-01-intra-1416.ivf",
  "vp80-01-intra-1417.ivf", "vp80-02-inter-1402.ivf",
  "vp80-02-inter-1412.ivf", "vp80-02-inter-1418.ivf",
  "vp80-02-inter-1424.ivf", "vp80-03-segmentation-01.ivf",
  "vp80-03-segmentation-02.ivf", "vp80-03-segmentation-03.ivf",
  "vp80-03-segmentation-04.ivf", "vp80-03-segmentation-1401.ivf",
  "vp80-03-segmentation-1403.ivf", "vp80-03-segmentation-1407.ivf",
  "vp80-03-segmentation-1408.ivf", "vp80-03-segmentation-1409.ivf",
  "vp80-03-segmentation-1410.ivf", "vp80-03-segmentation-1413.ivf",
  "vp80-03-segmentation-1414.ivf", "vp80-03-segmentation-1415.ivf",
  "vp80-03-segmentation-1425.ivf", "vp80-03-segmentation-1426.ivf",
  "vp80-03-segmentation-1427.ivf", "vp80-03-segmentation-1432.ivf",
  "vp80-03-segmentation-1435.ivf", "vp80-03-segmentation-1436.ivf",
  "vp80-03-segmentation-1437.ivf", "vp80-03-segmentation-1441.ivf",
  "vp80-03-segmentation-1442.ivf", "vp80-04-partitions-1404.ivf",
  "vp80-04-partitions-1405.ivf", "vp80-04-partitions-1406.ivf",
  "vp80-05-sharpness-1428.ivf", "vp80-05-sharpness-1429.ivf",
  "vp80-05-sharpness-1430.ivf", "vp80-05-sharpness-1431.ivf",
  "vp80-05-sharpness-1433.ivf", "vp80-05-sharpness-1434.ivf",
  "vp80-05-sharpness-1438.ivf", "vp80-05-sharpness-1439.ivf",
  "vp80-05-sharpness-1440.ivf", "vp80-05-sharpness-1443.ivf"
};

class TestVectorTest : public libvpx_test::DecoderTest,
    public ::testing::TestWithParam<const char*> {
 protected:
  TestVectorTest() : md5_file_(NULL) {}

  virtual ~TestVectorTest() {
    if (md5_file_)
      fclose(md5_file_);
  }

  void OpenMD5File(const std::string& md5_file_name_) {
    md5_file_ = libvpx_test::OpenTestDataFile(md5_file_name_);
    ASSERT_TRUE(md5_file_) << "Md5 file open failed. Filename: "
        << md5_file_name_;
  }

  virtual void DecompressedFrameHook(const vpx_image_t& img,
                                     const unsigned int frame_number) {
    char expected_md5[33];
    char junk[128];

    // Read correct md5 checksums.
    const int res = fscanf(md5_file_, "%s  %s", expected_md5, junk);
    ASSERT_NE(res, EOF) << "Read md5 data failed";
    expected_md5[32] = '\0';

    MD5Context md5;
    MD5Init(&md5);

    // Compute and update md5 for each raw in decompressed data.
    for (int plane = 0; plane < 3; ++plane) {
      uint8_t *buf = img.planes[plane];

      for (unsigned int y = 0; y < (plane ? (img.d_h + 1) >> 1 : img.d_h);
           ++y) {
        MD5Update(&md5, buf, (plane ? (img.d_w + 1) >> 1 : img.d_w));
        buf += img.stride[plane];
      }
    }

    uint8_t md5_sum[16];
    MD5Final(md5_sum, &md5);

    char actual_md5[33];
    // Convert to get the actual md5.
    for (int i = 0; i < 16; i++) {
      snprintf(&actual_md5[i * 2], sizeof(actual_md5) - i * 2, "%02x",
               md5_sum[i]);
    }
    actual_md5[32] = '\0';

    // Check md5 match.
    ASSERT_STREQ(expected_md5, actual_md5)
        << "Md5 checksums don't match: frame number = " << frame_number;
  }

 private:
  FILE *md5_file_;
};

// This test runs through the whole set of test vectors, and decodes them.
// The md5 checksums are computed for each frame in the video file. If md5
// checksums match the correct md5 data, then the test is passed. Otherwise,
// the test failed.
TEST_P(TestVectorTest, MD5Match) {
  const std::string filename = GetParam();
  // Open compressed video file.
  libvpx_test::IVFVideoSource video(filename);

  video.Init();

  // Construct md5 file name.
  const std::string md5_filename = filename + ".md5";
  OpenMD5File(md5_filename);

  // Decode frame, and check the md5 matching.
  ASSERT_NO_FATAL_FAILURE(RunLoop(&video));
}

INSTANTIATE_TEST_CASE_P(TestVectorSequence, TestVectorTest,
                        ::testing::ValuesIn(kTestVectors));

}  // namespace
