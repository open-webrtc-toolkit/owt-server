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

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/common_types.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/modules/video_processing/main/interface/video_processing.h>

using namespace webrtc;

namespace mcu {

class TaskRunner;
class VPMPool;

/**
 * This is the class to accepts the decoded frame and do some processing
 * for example, media layout mixing
 */
class VCMOutputProcessor : public webrtc::VCMPacketizationCallback,
                           public webrtc::VCMProtectionCallback,
                           public InputFrameCallback {
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
     * one particular publisher with index
     */
    virtual void handleInputFrame(webrtc::I420VideoFrame& frame, int index);

    // Implements VCMPacketizationCallback.
    virtual int32_t SendData(
        webrtc::FrameType frameType,
        uint8_t payloadType,
        uint32_t timeStamp,
        int64_t capture_time_ms,
        const uint8_t* payloadData,
        uint32_t payloadSize,
        const webrtc::RTPFragmentationHeader& fragmentationHeader,
        const webrtc::RTPVideoHeader* rtpVideoHdr);

    // Implements VideoProtectionCallback.
    virtual int ProtectionRequest(
        const FecProtectionParams* delta_fec_params,
        const FecProtectionParams* key_fec_params,
        uint32_t* sent_video_rate_bps,
        uint32_t* sent_nack_rate_bps,
        uint32_t* sent_fec_rate_bps);


    struct Layout{
        unsigned int subWidth_: 12; // assuming max is 4096
        unsigned int subHeight_: 12; // assuming max is 2160
        unsigned int div_factor_: 3; // max is 4
        bool operator==(const Layout& rhs) {
            return (this->div_factor_ == rhs.div_factor_)
                && (this->subHeight_ == rhs.subHeight_)
                && (this->subWidth_ == rhs.subWidth_);
        }
    };

private:
    int id_;

    int maxSlot_;
    Layout layout_; // current layout config;
    Layout layoutNew_; // new layout config if any;
    boost::scoped_ptr<CriticalSectionWrapper> layoutLock_;
    bool layoutFrames();
    // set the background to be black
    void clearFrame(webrtc::I420VideoFrame* frame);

    webrtc::VideoCodingModule* vcm_;
    boost::scoped_ptr<VPMPool> vpmPool_;
    boost::scoped_ptr<RtpRtcp> default_rtp_rtcp_;
    boost::scoped_ptr<DebugRecorder> recorder_;
    boost::scoped_ptr<woogeen_base::WoogeenTransport<erizo::VIDEO>> m_videoTransport;
    boost::shared_ptr<TaskRunner> taskRunner_;
    bool recordStarted_;

    /*
     * Each incoming channel will store the decoded frame in this array, and the encoding
     * thread will scan this array and compose the frames into one frame
     */
    // Delta used for translating between NTP and internal timestamps.
    int64_t delta_ntp_internal_ms_;
    boost::shared_ptr<BufferManager> bufferManager_;
    webrtc::I420VideoFrame* composedFrame_;
    webrtc::I420VideoFrame* mockFrame_;

    boost::scoped_ptr<boost::thread> encodingThread_;
    boost::asio::io_service io_service_;
    boost::scoped_ptr<boost::asio::deadline_timer> timer_;
};

/**
 * manages a pool of VPM for preprocessing the incoming I420 frame
 */
class VPMPool {
public:
	VPMPool(unsigned int size);
	~VPMPool();
	VideoProcessingModule* get(unsigned int slot);
	void update(VCMOutputProcessor::Layout& layout);
private:
	VideoProcessingModule** vpms_;
	unsigned int size_;
	VCMOutputProcessor::Layout layout_;
};

}
#endif /* VCMOutputProcessor_h */
