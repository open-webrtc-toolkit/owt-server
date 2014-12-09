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

#ifndef VCMInputProcessor_h
#define VCMInputProcessor_h

#include "VCMMediaProcessorHelper.h"
#include "VideoFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <WoogeenTransport.h>
#include <webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_rtcp.h>
#include <webrtc/modules/video_coding/codecs/interface/video_codec_interface.h>
#include <webrtc/modules/video_coding/main/interface/video_coding.h>
#include <webrtc/video_engine/vie_receiver.h>
#include <webrtc/video_engine/vie_sync_module.h>

namespace mcu {

class ExternalRenderer : public webrtc::VCMReceiveCallback {
public:
    ExternalRenderer(boost::shared_ptr<VideoFrameDecoder> frameHandler,
                     VideoFrameProvider* provider) 
        : m_frameHandler(frameHandler)
    {
        m_frameHandler->setInput(FRAME_FORMAT_I420, provider);
    }

    virtual ~ExternalRenderer()
    {
        m_frameHandler->unsetInput();
    }

    // Implements the webrtc::VCMReceiveCallback interface.
    virtual int32_t FrameToRender(webrtc::I420VideoFrame& frame)
    {
        m_frameHandler->onFrame(FRAME_FORMAT_I420, reinterpret_cast<unsigned char*>(&frame), 0, 0);
        return 0;
    }

private:
    boost::shared_ptr<VideoFrameDecoder> m_frameHandler;
};

class ExternalDecoder : public webrtc::VideoDecoder {
public:
    ExternalDecoder(boost::shared_ptr<VideoFrameDecoder> decoder,
                    VideoFrameProvider* provider)
        : m_frameFormat(FRAME_FORMAT_UNKNOWN)
        , m_frameDecoder(decoder)
        , m_provider(provider)
    {
        assert(provider);
    }

    virtual ~ExternalDecoder()
    {
        m_provider = nullptr;
        m_frameDecoder->unsetInput();
    }

    // Implements the webrtc::VideoDecoder interface.
    virtual int32_t InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores) 
    {
        assert(codecSettings->codecType == webrtc::kVideoCodecVP8 || codecSettings->codecType == webrtc::kVideoCodecH264);
        if (codecSettings->codecType == webrtc::kVideoCodecVP8)
            m_frameFormat = FRAME_FORMAT_VP8;
        else if (codecSettings->codecType == webrtc::kVideoCodecH264)
            m_frameFormat = FRAME_FORMAT_H264;

        if (m_frameDecoder->setInput(m_frameFormat, m_provider))
            return 0;

        return -1;
    }
    virtual int32_t Decode(const webrtc::EncodedImage& inputImage,
                           bool missingFrames,
                           const webrtc::RTPFragmentationHeader* fragmentation,
                           const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                           int64_t renderTimeMs = -1)
    {
        if (m_frameDecoder)
            m_frameDecoder->onFrame(m_frameFormat, inputImage._buffer, inputImage._length, 0);
        return 0;
    }
    virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) { return 0; }
    virtual int32_t Release() { return 0; }
    virtual int32_t Reset() { return 0; }

private:
    FrameFormat m_frameFormat;
    boost::shared_ptr<VideoFrameDecoder> m_frameDecoder;
    VideoFrameProvider* m_provider;
};

/**
 * A class to process the incoming streams by leveraging video coding module from
 * webrtc engine, which will framize and decode the frames.
 */
class TaskRunner;

class VCMInputProcessor : public erizo::MediaSink,
                          public webrtc::RtpFeedback,
                          public webrtc::VCMFrameTypeCallback,
                          public webrtc::VCMPacketRequestCallback,
                          public VideoFrameProvider {
    DECLARE_LOGGER();

public:
    VCMInputProcessor(int index, bool externalDecode = false);
    virtual ~VCMInputProcessor();

    // Implements the VideoFrameProvider interface.
    virtual void requestKeyFrame();

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

    // Implement the MediaSink interfaces.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    bool init(woogeen_base::WoogeenTransport<erizo::VIDEO>*, boost::shared_ptr<VideoFrameDecoder>, boost::shared_ptr<TaskRunner>);

    void bindAudioForSync(int32_t voiceChannelId, webrtc::VoEVideoSync*);

private:
    int m_index;
    bool m_externalDecoding;
    bool m_decoderRegistered;

    webrtc::VideoCodingModule* m_vcm;
    boost::scoped_ptr<webrtc::RemoteBitrateObserver> m_remoteBitrateObserver;
    boost::scoped_ptr<webrtc::RemoteBitrateEstimator> m_remoteBitrateEstimator;
    boost::scoped_ptr<webrtc::ViEReceiver> m_videoReceiver;
    boost::scoped_ptr<webrtc::RtpRtcp> m_rtpRtcp;
    boost::scoped_ptr<webrtc::ViESyncModule> m_avSync;
    boost::shared_ptr<webrtc::Transport> m_videoTransport;

    boost::scoped_ptr<DebugRecorder> m_recorder;
    boost::shared_ptr<VideoFrameDecoder> m_frameReceiver;
    boost::shared_ptr<webrtc::VCMReceiveCallback> m_renderer;
    boost::shared_ptr<webrtc::VideoDecoder> m_decoder;
    boost::shared_ptr<TaskRunner> m_taskRunner;
};

class DummyRemoteBitrateObserver : public webrtc::RemoteBitrateObserver {
public:
    virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs, unsigned int bitrate) { }
};

}
#endif /* VCMInputProcessor_h */
