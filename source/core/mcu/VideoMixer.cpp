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

#include "VideoMixer.h"

#include "BufferManager.h"
#include "TaskRunner.h"
#include "VCMInputProcessor.h"
#include "VCMOutputProcessor.h"
#include "ExternalVideoProcessor.h"
#include "HardwareVideoMixer.h"
#include "SoftwareVideoMixer.h"
#include <WoogeenTransport.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.VideoMixer");

VideoMixer::VideoMixer(erizo::RTPDataReceiver* receiver, bool hardwareAccelerated)
    : m_hardwareAccelerated(hardwareAccelerated)
    , m_participants(0)
    , m_outputReceiver(receiver)
    , m_addSourceOnDemand(false)
{
    init();
}

VideoMixer::~VideoMixer()
{
    closeAll();
    Config::get()->unregisterListener(this);
    m_outputReceiver = nullptr;
}

/**
 * init could be used for reset the state of this VideoMixer
 */
bool VideoMixer::init()
{
    webrtc::Trace::CreateTrace();
    webrtc::Trace::SetTraceFile("webrtc.trace.txt");
    webrtc::Trace::set_level_filter(webrtc::kTraceAll);

    m_taskRunner.reset(new TaskRunner());

    VideoLayout layout;
    layout.rootsize = vga;
    layout.divFactor = 1;

    if (m_hardwareAccelerated) {
        m_mixer.reset(new HardwareVideoMixer());
        m_videoOutputProcessor.reset(new ExternalVideoProcessor(MIXED_VIDEO_STREAM_ID, m_mixer, FRAME_FORMAT_VP8));
        m_videoOutputProcessor->init(new WoogeenTransport<erizo::VIDEO>(m_outputReceiver, nullptr), m_taskRunner, VideoOutputProcessor::VCT_VP8, VideoSizes[layout.rootsize]);
        m_mixer->activateOutput(FRAME_FORMAT_VP8, 30, 500, m_videoOutputProcessor.get());
    } else {
        m_mixer.reset(new SoftwareVideoMixer());
        m_videoOutputProcessor.reset(new VCMOutputProcessor(MIXED_VIDEO_STREAM_ID));
        m_videoOutputProcessor->init(new WoogeenTransport<erizo::VIDEO>(m_outputReceiver, nullptr), m_taskRunner, VideoOutputProcessor::VCT_VP8, VideoSizes[layout.rootsize]);
        m_mixer->activateOutput(FRAME_FORMAT_I420, 30, 500, m_videoOutputProcessor.get());
    }
    m_mixer->setLayout(layout);

    m_taskRunner->Start();
    Config::get()->registerListener(this);

    return true;
}

int VideoMixer::deliverAudioData(char* buf, int len) 
{
    assert(false);
    return 0;
}

#define global_ns

/**
 * Multiple sources may call this method simultaneously from different threads.
 * the incoming buffer is a rtp packet
 */
int VideoMixer::deliverVideoData(char* buf, int len)
{
    uint32_t id = 0;
    RTCPHeader* chead = reinterpret_cast<RTCPHeader*>(buf);
    uint8_t packetType = chead->getPacketType();
    assert(packetType != RTCP_Receiver_PT && packetType != RTCP_PS_Feedback_PT && packetType != RTCP_RTP_Feedback_PT);
    if (packetType == RTCP_Sender_PT)
        id = chead->getSSRC();
    else {
        global_ns::RTPHeader* head = reinterpret_cast<global_ns::RTPHeader*>(buf);
        id = head->getSSRC();
    }

    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(id);
    if (it != m_sinksForSources.end() && it->second)
        return it->second->deliverVideoData(buf, len);

    if (m_addSourceOnDemand) {
        lock.unlock();
        addSource(id, false, nullptr);
    }

    return 0;
}

int VideoMixer::deliverFeedback(char* buf, int len)
{
    FeedbackSink* feedbackSink = m_videoOutputProcessor->feedbackSink();
    if (feedbackSink)
        return feedbackSink->deliverFeedback(buf, len);

    return 0;
}

void VideoMixer::onConfigChanged()
{
    ELOG_DEBUG("onConfigChanged");
    VideoLayout* layout = Config::get()->getVideoLayout();

    m_mixer->setLayout(*layout);
}

/**
 * Attach a new InputStream to the mixer
 */
int32_t VideoMixer::addSource(uint32_t from, bool isAudio, FeedbackSink* feedback)
{
    assert(!isAudio);

    if (m_participants == BufferManager::SLOT_SIZE) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", BufferManager::SLOT_SIZE);
        return -1;
    }

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(from);
    if (it == m_sinksForSources.end() || !it->second) {
        int index = assignSlot(from);
        ELOG_DEBUG("addSource - assigned slot is %d", index);

        VCMInputProcessor* videoInputProcessor(new VCMInputProcessor(index, m_hardwareAccelerated));
        videoInputProcessor->init(new WoogeenTransport<erizo::VIDEO>(nullptr, feedback),
                                  m_mixer,
                                  m_taskRunner);

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_sinksForSources[from].reset(videoInputProcessor);
        ++m_participants;
        return 0;
    }

    assert("new source added with InputProcessor still available");    // should not go there
    return -1;
}

int32_t VideoMixer::bindAudio(uint32_t id, int voiceChannelId, VoEVideoSync* voeVideoSync)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(id);
    if (it != m_sinksForSources.end() && it->second) {
        it->second->bindAudioForSync(voiceChannelId, voeVideoSync);
        return 0;
    }
    return -1;
}

int32_t VideoMixer::removeSource(uint32_t from, bool isAudio)
{
    assert(!isAudio);

    boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(from);
    if (it != m_sinksForSources.end()) {
        m_sinksForSources.erase(it);
        lock.unlock();

        int index = getSlot(from);
        assert(index >= 0);
        m_sourceSlotMap[index] = 0;
        --m_participants;
        return 0;
    }

    return -1;
}

void VideoMixer::onRequestIFrame()
{
    ELOG_DEBUG("onRequestIFrame");
    m_videoOutputProcessor->onRequestIFrame();
}

uint32_t VideoMixer::sendSSRC()
{
    return m_videoOutputProcessor->sendSSRC();
}

void VideoMixer::closeAll()
{
    ELOG_DEBUG("closeAll");
    m_taskRunner->Stop();

    boost::unique_lock<boost::shared_mutex> sourceLock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator sourceItor = m_sinksForSources.begin();
    while (sourceItor != m_sinksForSources.end()) {
        uint32_t source = sourceItor->first;
        m_sinksForSources.erase(sourceItor++);
        int index = getSlot(source);
        assert(index >= 0);
        m_sourceSlotMap[index] = 0;
    }
    m_sinksForSources.clear();
    m_participants = 0;

    ELOG_DEBUG("Closed all media in this Mixer");
    webrtc::Trace::ReturnTrace();
}

}/* namespace mcu */
