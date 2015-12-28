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

#ifndef VideoFrameConstructor_h
#define VideoFrameConstructor_h

#include "DebugRecorder.h"
#include "WebRTCTaskRunner.h"
#include "WebRTCTransport.h"
#include "MediaFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/codecs/interface/video_codec_interface.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/video_engine/vie_receiver.h>
#include <webrtc/video_engine/vie_sync_module.h>

namespace woogeen_base {

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.
 */
class VideoFrameConstructor : public erizo::MediaSink,
                              public FrameSource,
                              public webrtc::VideoDecoder,
                              public webrtc::RtpFeedback,
                              public webrtc::VCMFrameTypeCallback,
                              public webrtc::VCMPacketRequestCallback {
    DECLARE_LOGGER();

public:
    VideoFrameConstructor(erizo::FeedbackSink* feedback);
    virtual ~VideoFrameConstructor();

    // Implements the webrtc::VCMPacketRequestCallback interface.
    virtual int32_t ResendPackets(const uint16_t* sequenceNumbers, uint16_t length);

    // Implements the webrtc::VCMFrameTypeCallback interface.
    virtual int32_t RequestKeyFrame();

    // Implements the webrtc::RtpFeedback interface.
    virtual int32_t OnInitializeDecoder(
        const int32_t id,
        const int8_t payload_type,
        const char payload_name[RTP_PAYLOAD_NAME_SIZE],
        const int frequency,
        const uint8_t channels,
        const uint32_t rate);
    virtual void OnIncomingSSRCChanged(const int32_t id, const uint32_t ssrc);
    virtual void OnIncomingCSRCChanged(const int32_t id, const uint32_t CSRC, const bool added) { }
    virtual void ResetStatistics(uint32_t ssrc);

    // Implements the webrtc::VideoDecoder interface.
    virtual int32_t InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores);
    virtual int32_t Decode(const webrtc::EncodedImage& inputImage,
                           bool missingFrames,
                           const webrtc::RTPFragmentationHeader* fragmentation,
                           const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                           int64_t renderTimeMs = -1);
    virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) { return 0; }
    virtual int32_t Release() { return 0; }
    virtual int32_t Reset() { return 0; }


    // Implements the MediaSink interfaces.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    // Implements the FrameSource interfaces.
    void onFeedback(const FeedbackMsg& msg);

    void syncWithAudio(int32_t voiceChannelId, webrtc::VoEVideoSync*);
    bool setBitrate(uint32_t kbps);

private:
    bool init();

    FrameFormat m_format;
    uint32_t m_ssrc;

    webrtc::VideoCodingModule* m_vcm;
    boost::scoped_ptr<webrtc::RemoteBitrateObserver> m_remoteBitrateObserver;
    boost::scoped_ptr<webrtc::RemoteBitrateEstimator> m_remoteBitrateEstimator;
    boost::scoped_ptr<webrtc::ViEReceiver> m_videoReceiver;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::scoped_ptr<webrtc::ViESyncModule> m_avSync;
    boost::shared_ptr<webrtc::Transport> m_videoTransport;

    boost::scoped_ptr<DebugRecorder> m_recorder;
    boost::shared_ptr<WebRTCTaskRunner> m_taskRunner;
};

class DummyRemoteBitrateObserver : public webrtc::RemoteBitrateObserver {
public:
    virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs, unsigned int bitrate) { }
};

}
#endif /* VideoFrameConstructor_h */
