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

#include "VideoFrameMixer.h"
#include "VideoLayout.h"

#include "hardware/VideoMixEngine.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <JobTimer.h>
#include <logger.h>
#include <map>
#include <MediaFramePipeline.h>
#include <set>
#include <string>

namespace mcu {

class HardwareVideoFrameMixerInput : public VideoMixEngineInput {
public:
    HardwareVideoFrameMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                                 woogeen_base::FrameFormat inFormat,
                                 woogeen_base::VideoFrameProvider* provider);
    virtual ~HardwareVideoFrameMixerInput();

    void push(unsigned char* payload, int len);

    InputIndex index() { return m_index; }

    virtual void requestKeyFrame(InputIndex index);

private:
    InputIndex m_index;
    woogeen_base::VideoFrameProvider* m_provider;
    boost::shared_ptr<VideoMixEngine> m_engine;
};

class HardwareVideoFrameMixerOutput : public VideoMixEngineOutput, public woogeen_base::JobTimerListener {
public:
    HardwareVideoFrameMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                                  woogeen_base::FrameFormat outFormat,
                                  unsigned int framerate,
                                  unsigned short bitrate,
                                  woogeen_base::VideoFrameConsumer* receiver);
    virtual ~HardwareVideoFrameMixerOutput();

    void setBitrate(unsigned short bitrate);
    void requestKeyFrame();

    // Implement the JobTimerListener interface.
    void onTimeout();

    void notifyFrameReady(OutputIndex index);
    woogeen_base::VideoFrameConsumer* receiver() { return m_receiver; }

private:
    unsigned char m_esBuffer[1024 * 1024];
    woogeen_base::FrameFormat m_outFormat;
    OutputIndex m_index;
    boost::shared_ptr<VideoMixEngine> m_engine;
    woogeen_base::VideoFrameConsumer* m_receiver;

    unsigned int m_frameRate;
    unsigned int m_outCount;
    boost::scoped_ptr<woogeen_base::JobTimer> m_jobTimer;
};

class VideoLayoutProcessor;
class HardwareVideoFrameMixer : public VideoFrameMixer {
    DECLARE_LOGGER();

public:
    HardwareVideoFrameMixer(VideoSize rootSize, YUVColor bgColor);
    virtual ~HardwareVideoFrameMixer();

    bool activateInput(int input, woogeen_base::FrameFormat, woogeen_base::VideoFrameProvider*);
    void deActivateInput(int input);
    void pushInput(int input, unsigned char* payload, int len);

    int32_t addFrameConsumer(const std::string& name, woogeen_base::FrameFormat, woogeen_base::VideoFrameConsumer*);
    void removeFrameConsumer(int32_t id);
    void setBitrate(unsigned short kbps, int id);
    void requestKeyFrame(int id);

    void updateRootSize(VideoSize& rootSize);
    void updateBackgroundColor(YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

private:
    boost::shared_ptr<VideoMixEngine> m_engine;
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>> m_inputs;
    boost::shared_mutex m_inputMutex;
    std::list<boost::shared_ptr<HardwareVideoFrameMixerOutput>> m_outputs;
    boost::shared_mutex m_outputMutex;
};

} /* namespace mcu */
#endif /* HardwareVideoFrameMixer_h */
