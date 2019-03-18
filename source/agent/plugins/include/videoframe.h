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

#ifndef AnalyticsVideoFrame_h
#define AnalyticsVideoFrame_h

#include "scoped_refptr.h"
#include "rvadefs.h"
#include "video_frame_buffer.h"

namespace ics {
namespace analytics {

class VideoFrame {
 public:
  VideoFrame(const ics::analytics::scoped_refptr<VideoFrameBuffer>& buffer, rvaU64 frame_id); 
  
  ~VideoFrame();
  
  // Support move and copy
  VideoFrame (const VideoFrame&);
  VideoFrame (VideoFrame&&);
  VideoFrame& operator=(const VideoFrame&);
  VideoFrame& operator=(VideoFrame&&);

  rvaU32 width() const;
  rvaU32 height() const;

  rvaU64 frame_id() const;
  
  ics::analytics::scoped_refptr<VideoFrameBuffer> video_frame_buffer() const;

  bool is_native_frame() const {
    return video_frame_buffer()->type() == VideoFrameBuffer::Type::kNative;
  }

 private:
  ics::analytics::scoped_refptr<VideoFrameBuffer> video_frame_buffer_;
  rvaU64 frame_id_;
};

}
}
#endif
