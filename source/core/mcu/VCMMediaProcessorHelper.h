/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
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

#ifndef VCMMediaProcessorHelper_h
#define VCMMediaProcessorHelper_h

#include <webrtc/common_video/interface/i420_video_frame.h>
#include <webrtc/modules/video_coding/main/interface/video_coding_defines.h>
#include <webrtc/system_wrappers/interface/critical_section_wrapper.h>
#include <webrtc/system_wrappers/interface/scoped_ptr.h>

namespace mcu {

class InputFrameCallback {
public:
    virtual void handleInputFrame(webrtc::I420VideoFrame&, int index) = 0;
};

class DebugRecorder {
 public:
  DebugRecorder()
      : cs_(webrtc::CriticalSectionWrapper::CreateCriticalSection()), file_(NULL) {}

  ~DebugRecorder() { Stop(); }

  int Start(const char* file_name_utf8) {
    webrtc::CriticalSectionScoped cs(cs_.get());
    if (file_)
      fclose(file_);
    file_ = fopen(file_name_utf8, "wb");
    if (!file_)
      return VCM_GENERAL_ERROR;
    return VCM_OK;
  }

  void Stop() {
    webrtc::CriticalSectionScoped cs(cs_.get());
    if (file_) {
      fclose(file_);
      file_ = NULL;
    }
  }

  void Add(const webrtc::I420VideoFrame& frame) {
    webrtc::CriticalSectionScoped cs(cs_.get());
    if (file_)
      PrintI420VideoFrame(frame, file_);
  }

  int PrintI420VideoFrame(const webrtc::I420VideoFrame& frame, FILE* file) {
    if (file == NULL)
      return -1;
    if (frame.IsZeroSize())
      return -1;
    for (int planeNum = 0; planeNum < webrtc::kNumOfPlanes; ++planeNum) {
      int width = (planeNum ? (frame.width() + 1) / 2 : frame.width());
      int height = (planeNum ? (frame.height() + 1) / 2 : frame.height());
      webrtc::PlaneType plane_type = static_cast<webrtc::PlaneType>(planeNum);
      const uint8_t* plane_buffer = frame.buffer(plane_type);
      for (int y = 0; y < height; y++) {
        if (fwrite(plane_buffer, 1, width, file) !=
            static_cast<unsigned int>(width)) {
          return -1;
        }
        plane_buffer += frame.stride(plane_type);
      }
    }
    return 0;
  }

 private:
  webrtc::scoped_ptr<webrtc::CriticalSectionWrapper> cs_;
  FILE* file_ GUARDED_BY(cs_);
};

}

#endif
