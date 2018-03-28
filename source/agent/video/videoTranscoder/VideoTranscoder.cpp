/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include <webrtc/base/logging.h>
#include <webrtc/system_wrappers/include/trace.h>

#include "VideoTranscoder.h"
#include "VideoFrameTranscoderImpl.h"

using namespace webrtc;
using namespace woogeen_base;

namespace mcu {

DEFINE_LOGGER(VideoTranscoder, "mcu.media.VideoTranscoder");

VideoTranscoder::VideoTranscoder(const VideoTranscoderConfig& config)
    : m_inputCount(0)
    , m_maxInputCount(1)
    , m_nextOutputIndex(0)
{
    if (ELOG_IS_TRACE_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
        rtc::LogMessage::LogTimestamps(true);

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(webrtc::kTraceAll);
    } else if (ELOG_IS_DEBUG_ENABLED()) {
        rtc::LogMessage::LogToDebug(rtc::LS_INFO);
        rtc::LogMessage::LogTimestamps(true);

        const int kTraceFilter = webrtc::kTraceNone | webrtc::kTraceTerseInfo |
            webrtc::kTraceWarning | webrtc::kTraceError |
            webrtc::kTraceCritical | webrtc::kTraceDebug |
            webrtc::kTraceInfo;

        webrtc::Trace::CreateTrace();
        webrtc::Trace::SetTraceFile(NULL, false);
        webrtc::Trace::set_level_filter(kTraceFilter);
    }

    m_freeInputIndexes.reserve(m_maxInputCount);
    for (size_t i = 0; i < m_maxInputCount; ++i)
        m_freeInputIndexes.push_back(true);

#ifdef ENABLE_MSDK
    MsdkBase *msdkBase = MsdkBase::get();
    if(msdkBase != NULL) {
        msdkBase->setConfigHevcEncoderGaccPlugin(config.useGacc);
    }
#endif

    ELOG_DEBUG("Init");

    m_frameTranscoder.reset(new VideoFrameTranscoderImpl());
}

VideoTranscoder::~VideoTranscoder()
{
    closeAll();
}

int VideoTranscoder::useAFreeInputIndex()
{
    for (size_t i = 0; i < m_freeInputIndexes.size(); ++i) {
        if (m_freeInputIndexes[i]) {
            m_freeInputIndexes[i] = false;
            return i;
        }
    }

    return -1;
}

bool VideoTranscoder::setInput(const std::string& inStreamID, const std::string& codec, woogeen_base::FrameSource* source)
{
    if (m_inputCount == m_maxInputCount) {
        ELOG_WARN("Exceeding maximum number of sources (%u), ignoring the addSource request", m_maxInputCount);
        return false;
    }

    woogeen_base::FrameFormat format = getFormat(codec);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it == m_inputs.end() || !it->second) {
        int index = useAFreeInputIndex();
        if (m_frameTranscoder->setInput(index, format, source)) {
            boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
            m_inputs[inStreamID] = index;
        }
        ++m_inputCount;
        return true;
    }

    assert("new source added with InputProcessor still available");    // should not go there
    return false;
}

void VideoTranscoder::unsetInput(const std::string& inStreamID)
{
    int index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_inputsMutex);
    auto it = m_inputs.find(inStreamID);
    if (it != m_inputs.end()) {
        index = it->second;
        m_inputs.erase(it);
    }
    lock.unlock();

    if (index >= 0) {
        m_frameTranscoder->unsetInput(index);
        m_freeInputIndexes[index] = true;
        --m_inputCount;
    }
}

bool VideoTranscoder::addOutput(
    const std::string& outStreamID
    , const std::string& codec
    , const std::string& resolution
    , const unsigned int framerateFPS
    , const unsigned int bitrateKbps
    , const unsigned int keyFrameIntervalSeconds
    , woogeen_base::FrameDestination* dest)
{
    woogeen_base::FrameFormat format = getFormat(codec);
    VideoSize vSize{0, 0};
    VideoResolutionHelper::getVideoSize(resolution, vSize);
    if (m_frameTranscoder->addOutput(m_nextOutputIndex, format, vSize, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest)) {
        boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
        m_outputs[outStreamID] = m_nextOutputIndex++;
        return true;
    }
    return false;
}

void VideoTranscoder::removeOutput(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::unique_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
        m_outputs.erase(it);
    }
    lock.unlock();

    if (index != -1) {
        m_frameTranscoder->removeOutput(index);
    }
}

void VideoTranscoder::forceKeyFrame(const std::string& outStreamID)
{
    int32_t index = -1;
    boost::shared_lock<boost::shared_mutex> lock(m_outputsMutex);
    auto it = m_outputs.find(outStreamID);
    if (it != m_outputs.end()) {
        index = it->second;
    }
    lock.unlock();

    if (index != -1) {
        m_frameTranscoder->requestKeyFrame(index);
    }
}

void VideoTranscoder::closeAll()
{
    ELOG_DEBUG("CloseAll");

    ELOG_DEBUG("Closed all media in this Transcoder");
}

}/* namespace mcu */
