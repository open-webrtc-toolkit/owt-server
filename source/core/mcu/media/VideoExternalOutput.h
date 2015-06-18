/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#ifndef VideoExternalOutput_h
#define VideoExternalOutput_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <MediaMuxer.h>
#include <VCMFrameConstructor.h>

namespace mcu {

class VideoExternalOutput : public erizo::MediaSink, public woogeen_base::FrameDispatcher {
public:
    VideoExternalOutput();
    virtual ~VideoExternalOutput();

    // Implements the MediaSink interfaces.
    virtual int deliverAudioData(char* buf, int len);
    virtual int deliverVideoData(char* buf, int len);

    // Implements the FrameDispatcher interfaces.
    virtual int32_t addFrameConsumer(const std::string& name, int payloadType, woogeen_base::FrameConsumer*);
    virtual void removeFrameConsumer(int32_t id);
    virtual bool getVideoSize(unsigned int& width, unsigned int& height) const;

private:
    void init();

    boost::scoped_ptr<woogeen_base::VCMFrameConstructor> m_frameConstructor;
    boost::shared_ptr<woogeen_base::WebRTCTaskRunner> m_taskRunner;
    boost::shared_ptr<woogeen_base::VideoEncodedFrameCallbackAdapter> m_encodedFrameCallback;
};

} /* namespace mcu */

#endif /* VideoExternalOutput_h */
