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

#ifndef VCMMediaProcessor_h
#define VCMMediaProcessor_h

#include "BufferManager.h"

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/common_types.h>
#include <webrtc/modules/rtp_rtcp/interface/fec_receiver.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_payload_registry.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/modules/video_processing/main/interface/video_processing.h>
#include <webrtc/system_wrappers/interface/thread_wrapper.h>

using namespace webrtc;

namespace mcu {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.  Each publisher will be
 * served by one VCMInputProcessor.
 * This class more or less is working as the vie_receiver class
 */
class ACMInputProcessor;
class AVSyncModule;
class AVSyncTaskRunner;
class DebugRecorder;

class InputFrameCallback {
public:
    virtual void handleInputFrame(webrtc::I420VideoFrame&, int index) = 0;
};

class VCMInputProcessor : public erizo::RTPDataReceiver,
                          public RtpData,
                          public VCMReceiveCallback {
    DECLARE_LOGGER();

public:
    VCMInputProcessor(int index);
    virtual ~VCMInputProcessor();

    // Implements the webrtc::VCMReceiveCallback interface.
    virtual int32_t FrameToRender(webrtc::I420VideoFrame&);

    // Implements the webrtc::RtpData interfaces.
    virtual int32_t OnReceivedPayloadData(
        const uint8_t* payloadData,
        const uint16_t payloadSize,
        const WebRtcRTPHeader*);

    virtual bool OnRecoveredPacket(const uint8_t* packet, int packet_length) { return false; }

    // Implements the erizo::RTPDataReceiver interface.
    virtual void receiveRtpData(char* rtpdata, int len, erizo::DataType type = erizo::VIDEO, uint32_t streamId = 0);

    bool init(boost::shared_ptr<BufferManager>, boost::shared_ptr<InputFrameCallback>, boost::shared_ptr<AVSyncTaskRunner>);
    void close();

    bool setReceiveVideoCodec(const VideoCodec&);
    void bindAudioInputProcessor(boost::shared_ptr<ACMInputProcessor>);
    int channelId() { return index_;}

private:
    int index_; //the index number of this publisher
    VideoCodingModule* vcm_;
    boost::scoped_ptr<RtpHeaderParser> rtp_header_parser_;
    boost::scoped_ptr<RTPPayloadRegistry> rtp_payload_registry_;
    boost::scoped_ptr<RtpReceiver> rtp_receiver_;
    boost::scoped_ptr<RtpRtcp> rtp_rtcp_;
    boost::scoped_ptr<AVSyncModule> avSync_;

    boost::shared_ptr<ACMInputProcessor> aip_;
    boost::shared_ptr<InputFrameCallback> frameReadyCB_;
    boost::shared_ptr<BufferManager> bufferManager_;
    boost::shared_ptr<AVSyncTaskRunner> taskRunner_;

    boost::scoped_ptr<DebugRecorder> recorder_;
};

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

    bool init(webrtc::Transport*, boost::shared_ptr<BufferManager>);
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

private:
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

    int id_;

    int maxSlot_;
    Layout layout_; // current layout config;
    Layout layoutNew_; // new layout config if any;
    boost::scoped_ptr<CriticalSectionWrapper> layoutLock_;
    bool layoutFrames();

    webrtc::VideoCodingModule* vcm_;
    webrtc::VideoProcessingModule* vpm_;
    boost::scoped_ptr<RtpRtcp> default_rtp_rtcp_;
    boost::scoped_ptr<DebugRecorder> recorder_;
    bool recordStarted_;

    /*
     * Each incoming channel will store the decoded frame in this array, and the encoding
     * thread will scan this array and compose the frames into one frame
     */
    boost::shared_ptr<BufferManager> bufferManager_;
    webrtc::I420VideoFrame* composedFrame_;
    webrtc::I420VideoFrame* mockFrame_;

    boost::scoped_ptr<boost::thread> encodingThread_;
    boost::asio::io_service io_service_;
    boost::scoped_ptr<boost::asio::deadline_timer> timer_;
};

}
#endif /* VCMMediaProcessor_h */
