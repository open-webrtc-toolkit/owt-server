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

#include "HardwareVideoFrameMixer.h"
#include "SoftVideoFrameMixer.h"
#include "VideoLayoutProcessor.h"
#include "VideoFrameInputProcessor.h"
#include <EncodedVideoFrameSender.h>
#include <WebRTCTransport.h>
#include <webrtc/modules/video_coding/codecs/vp8/vp8_factory.h>
#include <webrtc/system_wrappers/interface/trace.h>

using namespace webrtc;
using namespace woogeen_base;
using namespace erizo;

#define ENABLE_WEBRTC_TRACE 0

namespace mcu {

DEFINE_LOGGER(VideoMixer, "mcu.media.VideoMixer");

static ExtendedKey makeExtendedKey(int type, unsigned int width, unsigned int height)
{
    return static_cast<ExtendedKey>(std::make_tuple(type, width, height));
}

VideoMixer::VideoMixer(erizo::RTPDataReceiver* receiver, boost::property_tree::ptree& config)
    : m_participants(0)
    , m_outputReceiver(receiver)
    , m_maxInputCount(0)
    , m_outputKbps(0)
{
    m_hardwareAccelerated = config.get<bool>("hardware");
    m_maxInputCount = config.get<uint32_t>("maxinput");
    m_outputKbps = config.get<int>("bitrate");

    m_layoutProcessor.reset(new VideoLayoutProcessor(config));
    VideoSize rootSize;
    m_layoutProcessor->getRootSize(rootSize);
    YUVColor bgColor;
    m_layoutProcessor->getBgColor(bgColor);

    ELOG_DEBUG("Init maxInput(%u), rootSize(%u, %u), bgColor(%u, %u, %u)", m_maxInputCount, rootSize.width, rootSize.height, bgColor.y, bgColor.cb, bgColor.cr);

    m_taskRunner.reset(new woogeen_base::WebRTCTaskRunner());

    if (m_hardwareAccelerated)
        m_frameMixer.reset(new HardwareVideoFrameMixer(rootSize, bgColor));
    else
        m_frameMixer.reset(new SoftVideoFrameMixer(m_maxInputCount, rootSize, bgColor, m_taskRunner));
    m_layoutProcessor->registerConsumer(m_frameMixer);

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

int32_t VideoMixer::addOutput(int payloadType, bool nack, bool fec, const VideoSize& size)
{
    VideoSize outputSize = size;
    if (size.width == 0 || size.height == 0 || !webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter())
        m_layoutProcessor->getRootSize(outputSize);
    auto key = makeExtendedKey(payloadType, outputSize.width, outputSize.height);
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(key);
    if (it != m_outputs.end()) {
        it->second->startSend(nack, fec);
        return it->second->streamId();
    }

    woogeen_base::FrameFormat outputFormat = woogeen_base::FRAME_FORMAT_UNKNOWN;
    switch (payloadType) {
    case VP8_90000_PT:
        outputFormat = woogeen_base::FRAME_FORMAT_VP8;
        break;
    case H264_90000_PT:
        outputFormat = woogeen_base::FRAME_FORMAT_H264;
        break;
    default:
        return -1;
    }

    WebRTCTransport<erizo::VIDEO>* transport = new WebRTCTransport<erizo::VIDEO>(m_outputReceiver, nullptr);

    // EncodedVideoFrameSender employed for simulcast.
    // Software mode can also use the compound VCMOutputProcessor
    // (w/ both encoder and sender capability) which provides better QoS control.
    woogeen_base::VideoFrameSender* output = new woogeen_base::EncodedVideoFrameSender(
                                                m_frameMixer,
                                                outputFormat,
                                                m_outputKbps,
                                                transport,
                                                m_taskRunner,
                                                static_cast<uint16_t>(outputSize.width),
                                                static_cast<uint16_t>(outputSize.height));

    output->startSend(nack, fec);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_outputs[key].reset(output);

    return output->streamId();
}

int32_t VideoMixer::removeOutput(int payloadType, const VideoSize& size)
{
    VideoSize outputSize = size;
    if (size.width == 0 || size.height == 0 || !webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter())
        m_layoutProcessor->getRootSize(outputSize);
    auto key = makeExtendedKey(payloadType, outputSize.width, outputSize.height);
    boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(key);
    if (it != m_outputs.end()) {
        int32_t id = it->second->streamId();
        m_outputs.erase(it);
        return id;
    }

    return -1;
}

int32_t VideoMixer::addFrameConsumer(const std::string&/*unused*/, woogeen_base::FrameFormat format, woogeen_base::FrameConsumer* frameConsumer, const woogeen_base::MediaSpecInfo& info)
{
    int payloadType = INVALID_PT;
    switch (format) {
    case FRAME_FORMAT_VP8:
        payloadType = VP8_90000_PT;
        break;
    case FRAME_FORMAT_H264:
        payloadType = H264_90000_PT;
        break;
    default:
        break;
    }

    int32_t id = -1;
    VideoSize size {info.video.width, info.video.height};
    if (size.width == 0 || size.height == 0 || !webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter())
        m_layoutProcessor->getRootSize(size);
    id = addOutput(payloadType, true, false, size);
    if (id != -1) {
        auto key = makeExtendedKey(payloadType, size.width, size.height);
        boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
        auto it = m_outputs.find(key);
        if (it == m_outputs.end() || it->second->streamId() != id)
            return -1;
        woogeen_base::VideoFrameSender* output = m_outputs[key].get();
        output->registerPreSendFrameCallback(frameConsumer);

        // Request an IFrame explicitly, because the recorder doesn't support active I-Frame requests.
        IntraFrameCallback* iFrameCallback = output->iFrameCallback();
        if (iFrameCallback)
            iFrameCallback->handleIntraFrameRequest();
    }
    return id;
}

void VideoMixer::removeFrameConsumer(int32_t id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        if (id == it->second->streamId()) {
            it->second->deRegisterPreSendFrameCallback();
            break;
        }
    }
}

int VideoMixer::deliverFeedback(char* buf, int len)
{
    // TODO: For now we just send the feedback to all of the output processors.
    // The output processor will filter out the feedback which does not belong
    // to it. In the future we may do the filtering at a higher level?
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.begin();
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

bool VideoMixer::specifySourceRegion(uint32_t from, const std::string& regionID)
{
    int index = getInput(from);
    assert(index >= 0);

    if (index >= 0)
        return m_layoutProcessor->specifyInputRegion(index, regionID);

    ELOG_ERROR("specifySourceRegion - no such a source %d", from);
    return false;
}

std::string VideoMixer::getSourceRegion(uint32_t from)
{
    int index = getInput(from);
    assert(index >= 0);

    if (index >= 0)
        return m_layoutProcessor->getInputRegion(index);

    ELOG_ERROR("getSourceRegion - no such a source %d", from);
    return "";
}

bool VideoMixer::setResolution(const std::string& resolution)
{
    if (!m_layoutProcessor->setRootSize(resolution))
        return false;

    bool ret = true;
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.begin();
    for (; it != m_outputs.end(); ++it) {
        VideoSize outputSize;
        m_layoutProcessor->getRootSize(outputSize);
        ret &= (it->second->updateVideoSize(outputSize.width, outputSize.height));
    }

    return ret;
}

bool VideoMixer::setBackgroundColor(const std::string& color)
{
    return m_layoutProcessor->setBgColor(color);
}

IntraFrameCallback* VideoMixer::getIFrameCallback(int payloadType, const VideoSize& size)
{
    VideoSize outputSize = size;
    if (size.width == 0 || size.height == 0 || !webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter())
        m_layoutProcessor->getRootSize(outputSize);
    auto key = makeExtendedKey(payloadType, outputSize.width, outputSize.height);
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(key);
    if (it != m_outputs.end())
        return it->second->iFrameCallback();

    return nullptr;
}

uint32_t VideoMixer::getSendSSRC(int payloadType, bool nack, bool fec, const VideoSize& size)
{
    VideoSize outputSize = size;
    if (size.width == 0 || size.height == 0 || !webrtc::VP8EncoderFactoryConfig::use_simulcast_adapter())
        m_layoutProcessor->getRootSize(outputSize);
    auto key = makeExtendedKey(payloadType, outputSize.width, outputSize.height);
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(key);
    if (it != m_outputs.end())
        return it->second->sendSSRC(nack, fec);

    return 0;
}

int32_t VideoMixer::bindAudio(uint32_t id, int voiceChannelId, VoEVideoSync* voeVideoSync)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator it = m_sinksForSources.find(id);
    if (it != m_sinksForSources.end() && it->second) {
        VCMInputProcessor* input = dynamic_cast<VCMInputProcessor*>(it->second.get());
        input->bindAudioForSync(voiceChannelId, voeVideoSync);
        return 0;
    }
    return -1;
}

bool VideoMixer::setSourceBitrate(uint32_t from, uint32_t kbps)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator it = m_sinksForSources.find(from);
    if (it != m_sinksForSources.end() && it->second) {
        VCMInputProcessor* input = dynamic_cast<VCMInputProcessor*>(it->second.get());
        input->setBitrate(kbps);
        return true;
    }

    return false;
}

boost::shared_ptr<MediaSink> VideoMixer::getMediaSink(uint32_t from)
{
    boost::shared_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator it = m_sinksForSources.find(from);
    return it == m_sinksForSources.end() ? boost::shared_ptr<MediaSink>() : it->second;
}

/**
 * Attach a new InputStream to the mixer
 */
MediaSink* VideoMixer::addSource(uint32_t from,
                                 bool isAudio,
                                 DataContentType type,
                                 int payloadType,
                                 FeedbackSink* feedback,
                                 const std::string&)
{
    assert(!isAudio);

    if (m_participants == m_maxInputCount) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", m_maxInputCount);
        return nullptr;
    }

    boost::upgrade_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator it = m_sinksForSources.find(from);
    if (it == m_sinksForSources.end() || !it->second) {
        int index = assignInput(from);
        ELOG_DEBUG("addSource - assigned input index is %d", index);

        MediaSink* sink = nullptr;
        if (type == DataContentType::ENCODED_FRAME) {
            VideoFrameInputProcessor* videoInputProcessor = new VideoFrameInputProcessor(index, m_hardwareAccelerated);
            videoInputProcessor->init(payloadType, m_frameMixer, this);
            sink = videoInputProcessor;
        } else {
            assert(type == DataContentType::RTP);
            VCMInputProcessor* vcmInputProcessor(new VCMInputProcessor(index, m_hardwareAccelerated));
            vcmInputProcessor->init(new WebRTCTransport<erizo::VIDEO>(nullptr, feedback),
                                    m_frameMixer,
                                    m_taskRunner,
                                    this);
            sink = vcmInputProcessor;
        }

        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_sinksForSources[from].reset(sink);
        ++m_participants;
        return sink;
    }

    assert("new source added with InputProcessor still available");    // should not go there
    return nullptr;
}

void VideoMixer::removeSource(uint32_t from, bool isAudio)
{
    assert(!isAudio);

    boost::unique_lock<boost::shared_mutex> lock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator it = m_sinksForSources.find(from);
    if (it != m_sinksForSources.end()) {
        m_sinksForSources.erase(it);
        lock.unlock();

        int index = getInput(from);
        assert(index >= 0);
        m_sourceInputMap[index] = 0;
        m_layoutProcessor->removeInput(index);
        --m_participants;
    }
}

void VideoMixer::closeAll()
{
    ELOG_DEBUG("closeAll");

    boost::unique_lock<boost::shared_mutex> sourceLock(m_sourceMutex);
    std::map<uint32_t, boost::shared_ptr<MediaSink>>::iterator sourceItor = m_sinksForSources.begin();
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

void VideoMixer::setupNotification(std::function<void (const std::string&, const std::string&)> f)
{
    m_layoutProcessor->setupNotification(f);
    // setup others if needed.
}

}/* namespace mcu */
