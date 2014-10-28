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

#ifndef VCMOutputProcessor_h
#define VCMOutputProcessor_h

#include "BufferManager.h"
#include "VCMMediaProcessorHelper.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/video_engine/vie_encoder.h>

using namespace webrtc;

namespace mcu {

class TaskRunner;
class VPMPool;

/**
 * This is the class to accepts the decoded frame and do some processing
 * for example, media layout mixing
 */
class VCMOutputProcessor : public InputFrameCallback {
    DECLARE_LOGGER();

public:
    VCMOutputProcessor(int id);
    ~VCMOutputProcessor();

    bool init(woogeen_base::WoogeenTransport<erizo::VIDEO>*, boost::shared_ptr<BufferManager>, boost::shared_ptr<TaskRunner>);
    void close();

    void updateMaxSlot(int newMaxSlot);
    bool setSendVideoCodec(const VideoCodec& video_codec);
    void onRequestIFrame();

    void layoutTimerHandler(const boost::system::error_code& ec);

    /**
     * Implements InputFrameCallback.
     * This should be called whenever a new frame is decoded from
     * one particular publisher with the index.
     */
    virtual void handleInputFrame(webrtc::I420VideoFrame&, int index);

    struct Layout {
        unsigned int m_subWidth: 12; // assuming max is 4096
        unsigned int m_subHeight: 12; // assuming max is 2160
        unsigned int m_divFactor: 3; // max is 4
        bool operator==(const Layout& rhs)
        {
            return (this->m_divFactor == rhs.m_divFactor)
                && (this->m_subHeight == rhs.m_subHeight)
                && (this->m_subWidth == rhs.m_subWidth);
        }
    };

private:
    bool layoutFrames();
    // set the background to be black
    void clearFrame(webrtc::I420VideoFrame*);

    int m_id;
    std::atomic<bool> m_isClosing;

    int m_maxSlot;
    Layout m_currentLayout; // current layout config;
    Layout m_newLayout; // new layout config if any;
    boost::scoped_ptr<CriticalSectionWrapper> m_layoutLock;
    VideoCodec m_currentCodec;

    boost::shared_ptr<TaskRunner> m_taskRunner;
    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    boost::scoped_ptr<webrtc::ViEEncoder> m_videoEncoder;
    boost::scoped_ptr<woogeen_base::WoogeenTransport<erizo::VIDEO>> m_videoTransport;
    boost::scoped_ptr<RtpRtcp> m_rtpRtcp;
    boost::scoped_ptr<VPMPool> m_vpmPool;
    boost::scoped_ptr<DebugRecorder> m_recorder;
    bool m_recordStarted;

    /*
     * Each incoming channel will store the decoded frame in this array, and the encoding
     * thread will scan this array and compose the frames into one frame
     */
    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;
    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::scoped_ptr<webrtc::I420VideoFrame> m_composedFrame;
    webrtc::I420VideoFrame* m_mockFrame;

    boost::scoped_ptr<boost::thread> m_encodingThread;
    boost::asio::io_service m_ioService;
    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;
};

/**
 * manages a pool of VPM for preprocessing the incoming I420 frame
 */
class VPMPool {
public:
    VPMPool(unsigned int size);
    ~VPMPool();
    VideoProcessingModule* get(unsigned int slot);
    void update(VCMOutputProcessor::Layout&);

private:
    VideoProcessingModule** m_vpms;
    unsigned int m_size;
    VCMOutputProcessor::Layout m_layout;
};

}
#endif /* VCMOutputProcessor_h */
