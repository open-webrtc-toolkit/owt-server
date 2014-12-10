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

#ifndef HardwareVideoFrameMixer_h
#define HardwareVideoFrameMixer_h

#include "JobTimer.h"
#include "VideoFramePipeline.h"

#include "hardware/VideoMixEngine.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <map>
#include <set>
#include <string>

namespace mcu {

class HardwareVideoFrameMixerInput : public VideoMixEngineInput {
public:
    HardwareVideoFrameMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                            FrameFormat inFormat,
                            VideoFrameProvider* provider);
    virtual ~HardwareVideoFrameMixerInput();

    void push(unsigned char* payload, int len);

    InputIndex index() { return m_index; }

    virtual void requestKeyFrame(InputIndex index);

private:
    InputIndex m_index;
    VideoFrameProvider* m_provider;
    boost::shared_ptr<VideoMixEngine> m_engine;
};

class HardwareVideoFrameMixerOutput : public VideoMixEngineOutput, public JobTimerListener {
public:
    HardwareVideoFrameMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                            FrameFormat outFormat,
                            unsigned int framerate,
                            unsigned short bitrate,
                            VideoFrameConsumer* receiver);
    virtual ~HardwareVideoFrameMixerOutput();

    void setBitrate(unsigned short bitrate);
    void requestKeyFrame();

    // Implement the JobTimerListener interface.
    virtual void onTimeout();

    virtual void notifyFrameReady(OutputIndex index);

private:
    unsigned char m_esBuffer[1024 * 1024];
    FrameFormat m_outFormat;
    OutputIndex m_index;
    boost::shared_ptr<VideoMixEngine> m_engine;
    VideoFrameConsumer* m_receiver;

    unsigned int m_frameRate;
    unsigned int m_outCount;
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

class HardwareVideoFrameMixer : public VideoFrameMixer {
    DECLARE_LOGGER();
public:
    HardwareVideoFrameMixer();
    virtual ~HardwareVideoFrameMixer();

    bool activateInput(int slot, FrameFormat, VideoFrameProvider*);
    void deActivateInput(int slot);
    void pushInput(int slot, unsigned char* payload, int len);

    void setLayout(const VideoLayout&);

    void setBitrate(int id, unsigned short bitrate);
    void requestKeyFrame(int id);
    bool activateOutput(int id, FrameFormat, unsigned int framerate, unsigned short bitrate, VideoFrameConsumer*);
    void deActivateOutput(int id);

private:
    bool onSlotNumberChanged(uint32_t newSlotNum);

    CustomLayoutInfo m_currentLayout;
    boost::shared_ptr<VideoMixEngine> m_engine;
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>> m_inputs;
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>> m_outputs;
};

}
#endif
