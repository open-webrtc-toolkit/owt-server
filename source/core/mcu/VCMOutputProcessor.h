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
#include "Config.h"
#include "VCMMediaProcessorHelper.h"
#include "VideoCompositor.h"
#include "VideoOutputProcessor.h"

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/video_engine/vie_encoder.h>

namespace mcu {

class TaskRunner;

/**
 * This is the class to accept the decoded frame and do some processing
 * for example, media layout mixing and encoding. It then sends out the encoded
 * data via the given WoogeenTransport. It also gives the feedback
 * to the encoder based on the feedback from the remote.
 */
class VCMOutputProcessor : public VideoOutputProcessor, public erizo::FeedbackSink, public ConfigListener, public InputFrameCallback {
    DECLARE_LOGGER();

public:
    VCMOutputProcessor(int id);
    ~VCMOutputProcessor();

    // Implements VideoOutputProcessor.
    bool init(woogeen_base::WoogeenTransport<erizo::VIDEO>*, boost::shared_ptr<TaskRunner>);
    void close();

    bool setSendVideoCodec(const webrtc::VideoCodec&);
    void onRequestIFrame();
    uint32_t sendSSRC();
    // It's not accepting external encoded frame.
    int sendFrame(char* payload, int len) { return -1; }
    erizo::FeedbackSink* feedbackSink() { return this; }

    // Implements FeedbackSink.
    int deliverFeedback(char* buf, int len);

    // Implements ConfigListener.
    void onConfigChanged();

    /**
     * Implements InputFrameCallback.
     */
    void activateInput(int index);
    void deActivateInput(int index);
    /**
     * This should be called whenever a new frame is decoded from
     * one particular publisher with the index.
     */
    void handleInputFrame(webrtc::I420VideoFrame&, int index);

private:
    void layoutTimerHandler(const boost::system::error_code&);
    bool layoutFrames();
    void updateMaxSlot(int newMaxSlot);

    std::atomic<bool> m_isClosing;

    int m_maxSlot;

    boost::scoped_ptr<SoftVideoCompositor> m_videoCompositor;
    boost::scoped_ptr<webrtc::BitrateController> m_bitrateController;
    boost::scoped_ptr<webrtc::RtcpBandwidthObserver> m_bandwidthObserver;
    boost::scoped_ptr<webrtc::ViEEncoder> m_videoEncoder;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::scoped_ptr<DebugRecorder> m_recorder;
    bool m_recordStarted;
    boost::scoped_ptr<webrtc::I420VideoFrame> m_mockFrame;

    /*
     * Each incoming channel will store the decoded frame in this array, and the encoding
     * thread will scan this array and compose the frames into one frame
     */
    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;
    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::shared_ptr<webrtc::Transport> m_videoTransport;
    boost::shared_ptr<TaskRunner> m_taskRunner;

    boost::scoped_ptr<boost::thread> m_encodingThread;
    boost::asio::io_service m_ioService;
    boost::scoped_ptr<boost::asio::deadline_timer> m_timer;
};

}
#endif /* VCMOutputProcessor_h */
