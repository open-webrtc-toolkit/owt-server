/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


extern "C" {
#include "vpx_config.h"
#include "vp8_rtcd.h"
}
#include "test/register_state_check.h"
#include "third_party/googletest/src/include/gtest/gtest.h"

typedef void (*idct_fn_t)(short *input, unsigned char *pred_ptr,
                          int pred_stride, unsigned char *dst_ptr,
                          int dst_stride);
namespace {
class IDCTTest : public ::testing::TestWithParam<idct_fn_t>
{
  protected:
    virtual void SetUp()
    {
        int i;

        UUT = GetParam();
        memset(input, 0, sizeof(input));
        /* Set up guard blocks */
        for(i=0; i<256; i++)
            output[i] = ((i&0xF)<4&&(i<64))?0:-1;
    }

    idct_fn_t UUT;
    short input[16];
    unsigned char output[256];
    unsigned char predict[256];
};

TEST_P(IDCTTest, TestGuardBlocks)
{
    int i;

    for(i=0; i<256; i++)
        if((i&0xF) < 4 && i<64)
            EXPECT_EQ(0, output[i]) << i;
        else
            EXPECT_EQ(255, output[i]);
}

TEST_P(IDCTTest, TestAllZeros)
{
    int i;

    REGISTER_STATE_CHECK(UUT(input, output, 16, output, 16));

    for(i=0; i<256; i++)
        if((i&0xF) < 4 && i<64)
            EXPECT_EQ(0, output[i]) << "i==" << i;
        else
            EXPECT_EQ(255, output[i]) << "i==" << i;
}

TEST_P(IDCTTest, TestAllOnes)
{
    int i;

    input[0] = 4;
    REGISTER_STATE_CHECK(UUT(input, output, 16, output, 16));

    for(i=0; i<256; i++)
        if((i&0xF) < 4 && i<64)
            EXPECT_EQ(1, output[i]) << "i==" << i;
        else
            EXPECT_EQ(255, output[i]) << "i==" << i;
}

TEST_P(IDCTTest, TestAddOne)
{
    int i;

    for(i=0; i<256; i++)
        predict[i] = i;

    input[0] = 4;
    REGISTER_STATE_CHECK(UUT(input, predict, 16, output, 16));

    for(i=0; i<256; i++)
        if((i&0xF) < 4 && i<64)
            EXPECT_EQ(i+1, output[i]) << "i==" << i;
        else
            EXPECT_EQ(255, output[i]) << "i==" << i;
}

TEST_P(IDCTTest, TestWithData)
{
    int i;

    for(i=0; i<16; i++)
        input[i] = i;

    REGISTER_STATE_CHECK(UUT(input, output, 16, output, 16));

    for(i=0; i<256; i++)
        if((i&0xF) > 3 || i>63)
            EXPECT_EQ(255, output[i]) << "i==" << i;
        else if(i == 0)
            EXPECT_EQ(11, output[i]) << "i==" << i;
        else if(i == 34)
            EXPECT_EQ(1, output[i]) << "i==" << i;
        else if(i == 2 || i == 17 || i == 32)
            EXPECT_EQ(3, output[i]) << "i==" << i;
        else
            EXPECT_EQ(0, output[i]) << "i==" << i;
}

INSTANTIATE_TEST_CASE_P(C, IDCTTest,
                        ::testing::Values(vp8_short_idct4x4llm_c));
#if HAVE_MMX
INSTANTIATE_TEST_CASE_P(MMX, IDCTTest,
                        ::testing::Values(vp8_short_idct4x4llm_mmx));
#endif
}
