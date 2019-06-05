// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "MediaFramePipeline.h"

namespace owt_base {

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

void FrameSource::addDataDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_data_dests_mutex);
    m_data_dests.push_back(dest);
    lock.unlock();
    dest->setDataSource(this);
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

void FrameSource::removeDataDestination(FrameDestination* dest)
{
    boost::unique_lock<boost::shared_mutex> lock(m_data_dests_mutex);
    m_data_dests.remove(dest);
    lock.unlock();
    dest->unsetDataSource();
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
    } else if (isDataFrame(frame)) {
        boost::shared_lock<boost::shared_mutex> lock(m_video_dests_mutex);
        for (auto it = m_data_dests.begin(); it != m_data_dests.end(); ++it) {
            (*it)->onFrame(frame);
        }
    }else {
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
    lock.unlock();
    onVideoSourceChanged();
}

void FrameDestination::setDataSource(FrameSource* src){
        boost::unique_lock<boost::shared_mutex> lock(m_data_src_mutex);
    m_data_src = src;
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

void FrameDestination::unsetDataSource()
{
    boost::unique_lock<boost::shared_mutex> lock(m_data_src_mutex);
    m_data_src = nullptr;
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

