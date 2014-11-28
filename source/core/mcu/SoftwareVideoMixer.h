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

#ifndef SoftwareVideoMixer_h
#define SoftwareVideoMixer_h

#include "VideoMixerInterface.h"
#include "JobTimer.h"

#include <atomic>
#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <logger.h>

namespace mcu {

class BufferManager;
class SoftVideoCompositor;

class SoftwareVideoMixer : public VideoMixerInterface,
                           public JobTimerListener {
public:
    SoftwareVideoMixer();
    virtual ~SoftwareVideoMixer();

    bool activateInput(int slot, FrameFormat format, VideoMixInProvider* provider);
    void deActivateInput(int slot);
    void pushInput(int slot, unsigned char* payload, int len);

    bool activateOutput(FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoMixOutReceiver* receiver);
    void deActivateOutput(FrameFormat format);

    void setLayout(struct VideoLayout&);
    void setBitrate(FrameFormat format, unsigned short bitrate);
    void requestKeyFrame(FrameFormat format);

    void onTimeout();

private:
    void generateFrame();
    void updateMaxSlot(int newMaxSlot);

private:
    int m_maxSlot;
    int64_t m_ntpDelta;
    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::scoped_ptr<SoftVideoCompositor> m_videoCompositor;
    VideoMixOutReceiver* m_receiver;
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

}
#endif
