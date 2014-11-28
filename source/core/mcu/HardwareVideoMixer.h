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

#ifndef HardwareVideoMixer_h
#define HardwareVideoMixer_h

#include "JobTimer.h"
#include "VideoMixerInterface.h"

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <VideoMixEngine.h>

namespace mcu {

class HardwareVideoMixerInput : public VideoInputProducer {
public:
    HardwareVideoMixerInput(boost::shared_ptr<VideoMixEngine> engine,
                            FrameFormat inFormat,
                            VideoMixInProvider* provider);
    virtual ~HardwareVideoMixerInput();

    void push(unsigned char* payload, int len);

    virtual void requestKeyFrame(InputIndex index);

private:
    InputIndex m_index;
    VideoMixInProvider* m_provider;
    boost::shared_ptr<VideoMixEngine> m_engine;
};

class HardwareVideoMixerOutput : public VideoOutputConsumer,
                                 public JobTimerListener {
public:
    HardwareVideoMixerOutput(boost::shared_ptr<VideoMixEngine> engine,
                            FrameFormat outFormat,
                            std::string name,
                            unsigned int framerate,
                            unsigned short bitrate,
                            VideoMixOutReceiver* receiver);
    virtual ~HardwareVideoMixerOutput();

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
    VideoMixOutReceiver* m_receiver;

    unsigned int m_frameRate;
    unsigned int m_outCount;
    boost::scoped_ptr<JobTimer> m_jobTimer;
};

class HardwareVideoMixer : public VideoMixerInterface {
    enum MixerState{UN_INITIALIZED, IN_SERVICE}; // states shall be deprecated if VCSA supports init();
public:
    HardwareVideoMixer();
    virtual ~HardwareVideoMixer();

    bool activateInput(int slot, FrameFormat format, VideoMixInProvider* provider);
    void deActivateInput(int slot);
    void pushInput(int slot, unsigned char* payload, int len);

    bool activateOutput(FrameFormat format, unsigned int framerate, unsigned short bitrate, VideoMixOutReceiver* receiver);
    void deActivateOutput(FrameFormat format);

    void setLayout(struct VideoLayout&);
    void setBitrate(FrameFormat format, unsigned short bitrate);
    void requestKeyFrame(FrameFormat format);

private:
    boost::shared_ptr<VideoMixEngine> m_engine;
    std::map<int, boost::shared_ptr<HardwareVideoMixerInput>> m_inputs;
    std::map<FrameFormat, boost::shared_ptr<HardwareVideoMixerOutput>> m_outputs;
};

}
#endif
