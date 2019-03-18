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

#ifndef VideoFrameBuffer_h 
#define VideoFrameBuffer_h

#include "stdint.h"
#include "refcount.h"
#include "scoped_refptr.h"

namespace ics {
namespace analytics {

class I420BufferInterface;

/// Base class for frame buffers of different types of pixel formats and
//  storage.
class VideoFrameBuffer : public ics::analytics::RefCountInterface {
 public:
  enum class Type {
    kNative,
    kI420,
  };
 
  virtual Type type() const = 0;

  virtual int width() const = 0;

  virtual int height() const = 0;

  virtual ics::analytics::scoped_refptr<I420BufferInterface> ToI420() = 0;

  ics::analytics::scoped_refptr<I420BufferInterface> GetI420(); 
  ics::analytics::scoped_refptr<const I420BufferInterface> GetI420() const; 
 protected:
  ~VideoFrameBuffer() override {}
};

/// This interface represents Type::kI420
class PlanarYuvBuffer : public VideoFrameBuffer {
 public:
  virtual int ChromaWidth() const = 0;
  virtual int ChromaHeight() const = 0;

  virtual /*const*/ uint8_t* DataY() /*const*/ = 0;
  virtual /*const*/ uint8_t* DataU() /*const*/ = 0;
  virtual /*const*/ uint8_t* DataV() /*const*/ = 0;
  
  virtual int StrideY() const = 0;
  virtual int StrideU() const = 0;
  virtual int StrideV() const = 0;

 protected:
  ~PlanarYuvBuffer() override {}
};

class I420BufferInterface : public PlanarYuvBuffer {
 public:
  Type type() const final;
  
  int ChromaWidth() const final;
  int ChromaHeight() const final;

  ics::analytics::scoped_refptr<I420BufferInterface> ToI420() final;

 protected:
  ~I420BufferInterface() override {}
};

}
}

#endif
