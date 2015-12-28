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

class HardwareVideoFrameMixerInput : public VideoMixEngineInput, public woogeen_base::FrameDestination {
public:
    HardwareVideoFrameMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                                 woogeen_base::FrameFormat inFormat,
                                 woogeen_base::FrameSource* source);
    virtual ~HardwareVideoFrameMixerInput();

    void onFrame(const woogeen_base::Frame& frame);

    InputIndex index() { return m_index; }

    virtual void requestKeyFrame(InputIndex index);

private:
    InputIndex m_index;
    woogeen_base::FrameSource* m_source;
    boost::shared_ptr<VideoMixEngine> m_engine;
};

class HardwareVideoFrameMixerOutput : public VideoMixEngineOutput, public woogeen_base::FrameSource, public woogeen_base::JobTimerListener {
public:
    HardwareVideoFrameMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                                  woogeen_base::FrameFormat outFormat,
                                  FrameSize size,
                                  unsigned int framerate,
                                  unsigned short bitrate,
                                  woogeen_base::FrameDestination* destination);
    virtual ~HardwareVideoFrameMixerOutput();

    void setBitrate(unsigned short bitrate);
    void requestKeyFrame();

    // Implement the JobTimerListener interface.
    void onTimeout();

    //Implement woogeen_base::FrameSource
    void onFeedback(const woogeen_base::FeedbackMsg& msg);

    void notifyFrameReady(OutputIndex index);
    woogeen_base::FrameDestination* destination() { return m_destination; }

private:
    unsigned char m_esBuffer[1024 * 1024];
    woogeen_base::FrameFormat m_outFormat;
    OutputIndex m_index;
    boost::shared_ptr<VideoMixEngine> m_engine;
    woogeen_base::FrameDestination* m_destination;

    unsigned int m_frameRate;
    unsigned int m_outCount;
    boost::scoped_ptr<woogeen_base::JobTimer> m_jobTimer;
};

class HardwareVideoFrameMixer : public VideoFrameMixer {
    DECLARE_LOGGER();

public:
    HardwareVideoFrameMixer(VideoSize rootSize, YUVColor bgColor);
    virtual ~HardwareVideoFrameMixer();

    bool addInput(int input, woogeen_base::FrameFormat, woogeen_base::FrameSource*);
    void removeInput(int input);

    bool addOutput(int output, woogeen_base::FrameFormat, const VideoSize&, woogeen_base::FrameDestination*);
    void removeOutput(int output);
    void setBitrate(unsigned short kbps, int output);
    void requestKeyFrame(int output);

    void updateLayoutSolution(LayoutSolution& solution);

private:
    boost::shared_ptr<VideoMixEngine> m_engine;
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerInput>> m_inputs;
    boost::shared_mutex m_inputMutex;
    std::map<int, boost::shared_ptr<HardwareVideoFrameMixerOutput>> m_outputs;
    boost::shared_mutex m_outputMutex;
};

} /* namespace mcu */
#endif /* HardwareVideoFrameMixer_h */
