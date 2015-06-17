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

#ifndef FakedVideoFrameEncoder_h
#define FakedVideoFrameEncoder_h

#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>
#include <MediaFramePipeline.h>

namespace mcu {

// A FakedVideoFrameEncoder doesn't do real encoding stuffs.
// Or in other words, it "encodes" an frame into its original format.
class FakedVideoFrameEncoder : public woogeen_base::VideoFrameEncoder {
public:
    FakedVideoFrameEncoder();
    ~FakedVideoFrameEncoder();

    bool activateOutput(int id, woogeen_base::FrameFormat, unsigned int framerate, unsigned short kbps, woogeen_base::VideoFrameConsumer*);
    void deActivateOutput(int id);

    void setBitrate(unsigned short kbps, int id = 0);
    void requestKeyFrame(int id);
    void onFrame(const woogeen_base::Frame&);

private:
    std::map<int, woogeen_base::VideoFrameConsumer*> m_consumers;
    boost::shared_mutex m_consumerMutex;
};

FakedVideoFrameEncoder::FakedVideoFrameEncoder()
{
}

FakedVideoFrameEncoder::~FakedVideoFrameEncoder()
{
    boost::unique_lock<boost::shared_mutex> lock(m_consumerMutex);
    m_consumers.clear();
}

inline void FakedVideoFrameEncoder::setBitrate(unsigned short kbps, int id)
{
}

inline void FakedVideoFrameEncoder::requestKeyFrame(int id)
{
}

inline void FakedVideoFrameEncoder::onFrame(const woogeen_base::Frame& frame)
{
    boost::shared_lock<boost::shared_mutex> lock(m_consumerMutex);
    if (!m_consumers.empty()) {
        std::map<int, woogeen_base::VideoFrameConsumer*>::iterator it = m_consumers.begin();
        for (; it != m_consumers.end(); ++it)
            it->second->onFrame(frame);
    }
}

inline bool FakedVideoFrameEncoder::activateOutput(int id, woogeen_base::FrameFormat format, unsigned int framerate, unsigned short kbps, woogeen_base::VideoFrameConsumer* consumer)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_consumerMutex);
    std::map<int, woogeen_base::VideoFrameConsumer*>::iterator it = m_consumers.find(id);
    if (it != m_consumers.end())
        return false;

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    m_consumers[id] = consumer;
    return true;
}

inline void FakedVideoFrameEncoder::deActivateOutput(int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_consumerMutex);
    std::map<int, woogeen_base::VideoFrameConsumer*>::iterator it = m_consumers.find(id);
    if (it != m_consumers.end()) {
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_consumers.erase(it);
    }
}

}
#endif
