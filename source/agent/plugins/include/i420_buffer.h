/*
 * Copyright 2017 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#ifndef I420Buffer_h
#define I420Buffer_h

#include <memory>
#include "video_frame_buffer.h"
#include "aligned_malloc.h"

namespace ics {
namespace analytics {

class I420Buffer : public I420BufferInterface {
 public:
  static ics::analytics::scoped_refptr<I420Buffer> Create(int width, int height);
  static ics::analytics::scoped_refptr<I420Buffer> Create(int width,
                                                              int height,
                                                              int stride_y,
                                                              int stride_u,
                                                              int stride_v);
  
  static ics::analytics::scoped_refptr<I420Buffer> Copy(/*const */I420BufferInterface& buffer);
  
  static ics::analytics::scoped_refptr<I420Buffer> Copy(
      int width, int height,
      /*const */uint8_t* data_y, int stride_y,
      /*const */uint8_t* data_u, int stride_u,
      /*const */uint8_t* data_v, int stride_v);

  void InitializeData();
  
  int width() const override;
  int height() const override;
#if 0
  const uint8_t* DataY() const override;
  const uint8_t* DataU() const override;
  const uint8_t* DataV() const override;
#endif

  uint8_t* DataY() override;
  uint8_t* DataU() override;
  uint8_t* DataV() override;

  int StrideY() const override;
  int StrideU() const override;
  int StrideV() const override;

  uint8_t* MutableDataY();
  uint8_t* MutableDataU();
  uint8_t* MutableDataV();
 protected:
  I420Buffer(int width, int height);
  I420Buffer(int width, int height, int stride_y, int stride_u, int stride_v);

  ~I420Buffer() override;
 private:
  const int width_;
  const int height_;
  const int stride_y_;
  const int stride_u_;
  const int stride_v_;
  /*const */std::unique_ptr<uint8_t, AlignedFreeDeleter> data_;
};

}
}

#endif
