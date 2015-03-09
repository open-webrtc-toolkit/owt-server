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
#include "HardwareVideoFrameMixer.h"
#include "SoftVideoFrameMixer.h"
#include "TaskRunner.h"
#include "VCMOutputProcessor.h"
#include "VideoLayoutProcessor.h"
#include <WoogeenTransport.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

#define ENABLE_WEBRTC_TRACE 0

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

VideoMixer::VideoMixer(erizo::RTPDataReceiver* receiver, boost::property_tree::ptree& config)
    : m_participants(0)
    , m_outputReceiver(receiver)
    , m_maxInputCount(0)
{
    m_hardwareAccelerated = config.get<bool>("hardware");
    m_maxInputCount = config.get<uint32_t>("maxinput");

    m_layoutProcessor.reset(new VideoLayoutProcessor(config));
    VideoSize rootSize;
    m_layoutProcessor->getRootSize(rootSize);
    YUVColor bgColor;
    m_layoutProcessor->getBgColor(bgColor);

    ELOG_DEBUG("Init maxInput(%u), rootSize(%u, %u), bgColor(%u, %u, %u)", m_maxInputCount, rootSize.width, rootSize.height, bgColor.y, bgColor.cb, bgColor.cr);

    if (m_hardwareAccelerated)
        m_frameMixer.reset(new HardwareVideoFrameMixer(rootSize, bgColor));
    else
        m_frameMixer.reset(new SoftVideoFrameMixer(m_maxInputCount, rootSize, bgColor));
    m_layoutProcessor->registerConsumer(m_frameMixer);

    m_taskRunner.reset(new TaskRunner());
    m_taskRunner->Start();

#if ENABLE_WEBRTC_TRACE
    webrtc::Trace::CreateTrace();
    webrtc::Trace::SetTraceFile("webrtc.trace.txt");
    webrtc::Trace::set_level_filter(webrtc::kTraceAll);
#endif
}

VideoMixer::~VideoMixer()
{
    closeAll();

    m_taskRunner->Stop();
#if ENABLE_WEBRTC_TRACE
    webrtc::Trace::ReturnTrace();
#endif

    m_layoutProcessor->deregisterConsumer(m_frameMixer);
    m_outputReceiver = nullptr;
}

int32_t VideoMixer::addOutput(int payloadType, bool nack, bool fec)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end()) {
        it->second->startSend(nack, fec);
        return it->second->id();
    }

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
    if (m_hardwareAccelerated)
        output = new EncodedVideoFrameSender(outputId, m_frameMixer, outputFormat, transport, m_taskRunner);
    else
        output = new VCMOutputProcessor(outputId, m_frameMixer, transport, m_taskRunner);

    // Fetch video size.
    // TODO: The size should be identical to the composited video size.
    VideoSize outputSize;
    m_layoutProcessor->getRootSize(outputSize);
    output->setSendCodec(outputFormat, outputSize);
    output->startSend(nack, fec);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_outputs[payloadType].reset(output);

    return output->id();
}

int32_t VideoMixer::removeOutput(int payloadType)
{
    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end()) {
        int32_t id = it->second->id();
        m_outputs.erase(it);
        return id;
    }

    return -1;
}

void VideoMixer::startRecording(MediaFrameQueue& videoQueue, long long recordStartTime)
{
    // FIXME: Currently, only VP8_90000_PT to be recorded.
    if (addOutput(VP8_90000_PT, true, true) != -1) {
        VideoFrameSender* output = m_outputs[VP8_90000_PT].get();
        output->RegisterPostEncodeCallback(videoQueue, recordStartTime);

        // Request an IFrame explicitly, because the recorder doesn't support active I-Frame requests.
        IntraFrameCallback* iFrameCallback = output->iFrameCallback();
        if (iFrameCallback)
            iFrameCallback->handleIntraFrameRequest();
    }
}

void VideoMixer::stopRecording()
{
    // FIXME: Currently, only VP8_90000_PT to be recorded.
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(VP8_90000_PT);
    if (it != m_outputs.end())
        it->second->DeRegisterPostEncodeImageCallback();
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

    return 0;
}

int VideoMixer::deliverFeedback(char* buf, int len)
{
    // TODO: For now we just send the feedback to all of the output processors.
    // The output processor will filter out the feedback which does not belong
    // to it. In the future we may do the filtering at a higher level?
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        FeedbackSink* feedbackSink = it->second->feedbackSink();
        if (feedbackSink)
            feedbackSink->deliverFeedback(buf, len);
    }

    return len;
}

void VideoMixer::onInputProcessorInitOK(int index)
{
    ELOG_DEBUG("onInputProcessorInitOK - index: %d", index);
    m_layoutProcessor->addInput(index);
}

void VideoMixer::promoteSources(std::vector<uint32_t>& sources)
{
    std::vector<int> inputs;
    for (std::vector<uint32_t>::iterator it = sources.begin(); it != sources.end(); ++it) {
        int index = getInput(*it);
        if (index >= 0)
            inputs.push_back(index);
    }
    if (inputs.size() > 0)
        m_layoutProcessor->promoteInputs(inputs);
}

void VideoMixer::specifySourceRegion(uint32_t from, std::string& regionID)
{
    int index = getInput(from);
    assert(index >= 0);

    if (index >= 0)
        m_layoutProcessor->specifyInputRegion(index, regionID);
    else {
        ELOG_ERROR("specifySourceRegion - no such a source %d", from);
    }
}

bool VideoMixer::setResolution(const std::string& resolution)
{
    if (!m_layoutProcessor->setRootSize(resolution))
        return false;

    bool ret = true;
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        VideoSize outputSize;
        m_layoutProcessor->getRootSize(outputSize);
        ret &= (it->second->updateVideoSize(outputSize));
    }

    return ret;
}

bool VideoMixer::setBackgroundColor(const std::string& color)
{
    return m_layoutProcessor->setBgColor(color);
}

IntraFrameCallback* VideoMixer::getIFrameCallback(int payloadType)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end())
        return it->second->iFrameCallback();

    return nullptr;
}

uint32_t VideoMixer::getSendSSRC(int payloadType, bool nack, bool fec)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    std::map<int, boost::shared_ptr<VideoFrameSender>>::iterator it = m_outputs.find(payloadType);
    if (it != m_outputs.end())
        return it->second->sendSSRC(nack, fec);

    return 0;
}

/**
 * Attach a new InputStream to the mixer
 */
int32_t VideoMixer::addSource(uint32_t from, bool isAudio, FeedbackSink* feedback, const std::string&)
{
    assert(!isAudio);

    if (m_participants == m_maxInputCount) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", m_maxInputCount);
        return -1;
    }

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator it = m_sinksForSources.find(from);
    if (it == m_sinksForSources.end() || !it->second) {
        int index = assignInput(from);
        ELOG_DEBUG("addSource - assigned input index is %d", index);

        VCMInputProcessor* videoInputProcessor(new VCMInputProcessor(index, m_hardwareAccelerated));
        videoInputProcessor->init(new WoogeenTransport<erizo::VIDEO>(nullptr, feedback),
                                  m_frameMixer,
                                  m_taskRunner,
                                  this);

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

        int index = getInput(from);
        assert(index >= 0);
        m_sourceInputMap[index] = 0;
        m_layoutProcessor->removeInput(index);
        --m_participants;
        return 0;
    }

    return -1;
}

void VideoMixer::closeAll()
{
    ELOG_DEBUG("closeAll");

    boost::unique_lock<boost::shared_mutex> sourceLock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<VCMInputProcessor>>::iterator sourceItor = m_sinksForSources.begin();
    while (sourceItor != m_sinksForSources.end()) {
        uint32_t source = sourceItor->first;
        m_sinksForSources.erase(sourceItor++);
        int index = getInput(source);
        assert(index >= 0);
        m_sourceInputMap[index] = 0;
        m_layoutProcessor->removeInput(index);
    }
    m_sinksForSources.clear();
    m_participants = 0;
    sourceLock.unlock();

    boost::unique_lock<boost::shared_mutex> outputLock(m_outputMutex);
    m_outputs.clear();
    outputLock.unlock();

    ELOG_DEBUG("Closed all media in this Mixer");
}

}/* namespace mcu */
