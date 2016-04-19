/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "MediaFramePipeline.h"

namespace woogeen_base {

FrameSource::~FrameSource()
{
    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_audio_dests_mutex);
        for (auto it = m_audio_dests.begin(); it != m_audio_dests.end(); ++it) {
            (*it)->unsetAudioSource();
        }

        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_audio_dests.clear();
    }

    {
        boost::upgrade_lock<boost::shared_mutex> lock(m_video_dests_mutex);
        for (auto it = m_video_dests.begin(); it != m_video_dests.end(); ++it) {
            (*it)->unsetVideoSource();
        }

        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_video_dests.clear();
    }
}

void FrameSource::addAudioDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_audio_dests_mutex);
    m_audio_dests.push_back(dest);
    lock.unlock();
    dest->setAudioSource(this);
}

void FrameSource::addVideoDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_video_dests_mutex);
    m_video_dests.push_back(dest);
    lock.unlock();
    dest->setVideoSource(this);
}

void FrameSource::removeAudioDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_audio_dests_mutex);
    m_audio_dests.remove(dest);
    lock.unlock();
    dest->unsetAudioSource();
}

void FrameSource::removeVideoDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_video_dests_mutex);
    m_video_dests.remove(dest);
    lock.unlock();
    dest->unsetVideoSource();
}

static inline bool isAudioFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_PCM_RAW
          || frame.format == FRAME_FORMAT_PCMU
          || frame.format == FRAME_FORMAT_OPUS;
}

static inline bool isVideoFrame(const Frame& frame) {
    return frame.format == FRAME_FORMAT_I420
          || frame.format ==FRAME_FORMAT_YAMI
          || frame.format == FRAME_FORMAT_VP8
          || frame.format == FRAME_FORMAT_H264;
}

void FrameSource::deliverFrame(const Frame& frame)
{
    if (isAudioFrame(frame)) {
        boost::shared_lock<boost::shared_mutex> lock(m_audio_dests_mutex);
        for (auto it = m_audio_dests.begin(); it != m_audio_dests.end(); ++it) {
            (*it)->onFrame(frame);
        }
    } else if (isVideoFrame(frame)) {
        boost::shared_lock<boost::shared_mutex> lock(m_video_dests_mutex);
        for (auto it = m_video_dests.begin(); it != m_video_dests.end(); ++it) {
            (*it)->onFrame(frame);
        }
    } else {
        //TODO: log error here.
    }
}

//=========================================================================================

void FrameDestination::setAudioSource(FrameSource* src)
{
    boost::unique_lock<boost::shared_mutex> lock(m_audio_src_mutex);
    m_audio_src = src;
}

void FrameDestination::setVideoSource(FrameSource* src)
{
    boost::unique_lock<boost::shared_mutex> lock(m_video_src_mutex);
    m_video_src = src;
}

void FrameDestination::unsetAudioSource()
{
    boost::unique_lock<boost::shared_mutex> lock(m_audio_src_mutex);
    m_audio_src = nullptr;
}

void FrameDestination::unsetVideoSource()
{
    boost::unique_lock<boost::shared_mutex> lock(m_video_src_mutex);
    m_video_src = nullptr;
}

void FrameDestination::deliverFeedbackMsg(const FeedbackMsg& msg) {
    if (msg.type == AUDIO_FEEDBACK) {
        boost::shared_lock<boost::shared_mutex> lock(m_audio_src_mutex);
        if (m_audio_src) {
            m_audio_src->onFeedback(msg);
        }
    } else if (msg.type == VIDEO_FEEDBACK) {
        boost::shared_lock<boost::shared_mutex> lock(m_video_src_mutex);
        if (m_video_src) {
            m_video_src->onFeedback(msg);
        }
    } else {
        //TODO: log error here.
    }
}

}

