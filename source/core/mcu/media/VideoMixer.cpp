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

#include "EncodedVideoFrameSender.h"
#include "HardwareVideoMixer.h"
#include "TaskRunner.h"
#include "VCMInputProcessor.h"
#include "VCMOutputProcessor.h"
#include "VideoCompositor.h"
#include <WoogeenTransport.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

VideoMixer::VideoMixer(erizo::RTPDataReceiver* receiver, bool hardwareAccelerated)
    : m_hardwareAccelerated(hardwareAccelerated)
    , m_participants(0)
    , m_outputReceiver(receiver)
    , m_addSourceOnDemand(false)
{
    m_taskRunner.reset(new TaskRunner());

    const VideoLayout& layout = Config::get()->getVideoLayout();

    if (m_hardwareAccelerated)
        m_frameCompositor.reset(new HardwareVideoMixer(layout));
    else
        m_frameCompositor.reset(new SoftVideoCompositor(layout));

    Config::get()->registerListener(this);

    m_taskRunner->Start();

    webrtc::Trace::CreateTrace();
    webrtc::Trace::SetTraceFile("webrtc.trace.txt");
    webrtc::Trace::set_level_filter(webrtc::kTraceAll);
}

VideoMixer::~VideoMixer()
{
    closeAll();
    Config::get()->unregisterListener(this);
    m_outputReceiver = nullptr;
}

int32_t VideoMixer::addOutput(int payloadType)
{
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end())
        return it->second->id();

    FrameFormat outputFormat = FRAME_FORMAT_UNKNOWN;
    int outputId = -1;
    switch (payloadType) {
    case VP8_90000_PT:
        outputFormat = FRAME_FORMAT_VP8;
        outputId = MIXED_VP8_VIDEO_STREAM_ID;
        break;
    case H264_90000_PT:
        outputFormat = FRAME_FORMAT_H264;
        outputId = MIXED_H264_VIDEO_STREAM_ID;
        break;
    default:
        return -1;
    }

    WoogeenTransport<erizo::VIDEO>* transport = new WoogeenTransport<erizo::VIDEO>(m_outputReceiver, nullptr);

    VideoFrameSender* output = nullptr;
    if (m_hardwareAccelerated) {
        output = new EncodedVideoFrameSender(outputId, m_frameCompositor, outputFormat, transport, m_taskRunner);
        m_frameCompositor->activateOutput(output->id(), outputFormat, 30, 500, output);
    } else {
        output = new VCMOutputProcessor(outputId, transport, m_taskRunner);
        m_frameCompositor->activateOutput(output->id(), FRAME_FORMAT_I420, 30, 500, output);
    }

    // Fetch video size.
    VideoSize rootSize = DEFAULT_VIDEO_SIZE;
    std::map<VideoResolutionType, VideoSize>::const_iterator sizeIterator = VideoSizes.find(vga);
    if (sizeIterator != VideoSizes.end())
        rootSize = sizeIterator->second;
    output->setSendCodec(outputFormat, rootSize);
    m_outputs[payloadType].reset(output);

    return output->id();
}

int32_t VideoMixer::removeOutput(int payloadType)
{
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end()) {
        VideoFrameSender* output = it->second.get();
        int32_t id = output->id();
        m_frameCompositor->deActivateOutput(id);
        m_outputs.erase(it);
        return id;
    }

    return -1;
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
        addSource(id, false, nullptr, "");
    }

    return 0;
}

int VideoMixer::deliverFeedback(char* buf, int len)
{
    // TODO: For now we just send the feedback to all of the output processors.
    // The output processor will filter out the feedback which does not belong
    // to it. In the future we may do the filtering at a higher level?
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        FeedbackSink* feedbackSink = it->second->feedbackSink();
        if (feedbackSink)
            feedbackSink->deliverFeedback(buf, len);
    }

    return len;
}

IntraFrameCallback* VideoMixer::getIFrameCallback(int payloadType)
{
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end())
        return it->second->iFrameCallback();

    return nullptr;
}

uint32_t VideoMixer::getSendSSRC(int payloadType)
{
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end())
        return it->second->sendSSRC();

    return 0;
}

void VideoMixer::onConfigChanged()
{
    ELOG_DEBUG("onConfigChanged");
    m_frameCompositor->setLayout(Config::get()->getVideoLayout());
}

/**
 * Attach a new InputStream to the mixer
 */
int32_t VideoMixer::addSource(uint32_t from, bool isAudio, FeedbackSink* feedback, const std::string&)
{
    assert(!isAudio);

    if (m_participants == MAX_VIDEO_SLOT_NUMBER) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", MAX_VIDEO_SLOT_NUMBER);
        return -1;
    }

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(from);
    if (it == m_sinksForSources.end() || !it->second) {
        int index = assignSlot(from);
        ELOG_DEBUG("addSource - assigned slot is %d", index);

        VCMInputProcessor* videoInputProcessor(new VCMInputProcessor(index, m_hardwareAccelerated));
        videoInputProcessor->init(new WoogeenTransport<erizo::VIDEO>(nullptr, feedback),
                                  m_frameCompositor,
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
