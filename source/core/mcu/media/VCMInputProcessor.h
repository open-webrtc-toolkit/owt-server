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

#include "VideoFrameMixer.h"
#include "VideoInputProcessorBase.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <VCMFrameConstructor.h>
#include <VideoFramePipeline.h>

namespace mcu {

class ExternalRenderer : public webrtc::VCMReceiveCallback {
public:
    ExternalRenderer(int index,
                     boost::shared_ptr<VideoFrameMixer> frameHandler,
                     woogeen_base::VideoFrameProvider* provider,
                     InputProcessorCallback* initCallback)
        : m_index(index)
        , m_frameHandler(frameHandler)
    {
        assert(provider);
        assert(initCallback);
        m_frameHandler->activateInput(index, woogeen_base::FRAME_FORMAT_I420, provider);
        initCallback->onInputProcessorInitOK(index);
    }

    virtual ~ExternalRenderer()
    {
        m_frameHandler->deActivateInput(m_index);
    }

    // Implements the webrtc::VCMReceiveCallback interface.
    virtual int32_t FrameToRender(webrtc::I420VideoFrame& frame)
    {
        m_frameHandler->pushInput(m_index, reinterpret_cast<unsigned char*>(&frame), 0);
        return 0;
    }

private:
    int m_index;
    boost::shared_ptr<VideoFrameMixer> m_frameHandler;
};

class ExternalDecoder : public webrtc::VideoDecoder {
public:
    ExternalDecoder(int index,
                    boost::shared_ptr<VideoFrameMixer> frameHandler,
                    woogeen_base::VideoFrameProvider* provider,
                    InputProcessorCallback* initCallback)
        : m_index(index)
        , m_frameHandler(frameHandler)
        , m_provider(provider)
        , m_initCallback(initCallback)
    {
        assert(provider);
        assert(initCallback);
    }

    virtual ~ExternalDecoder()
    {
        m_provider = nullptr;
        m_frameHandler->deActivateInput(m_index);
    }

    // Implements the webrtc::VideoDecoder interface.
    virtual int32_t InitDecode(const webrtc::VideoCodec* codecSettings, int32_t numberOfCores) 
    {
        woogeen_base::FrameFormat frameFormat = woogeen_base::FRAME_FORMAT_UNKNOWN;
        assert(codecSettings->codecType == webrtc::kVideoCodecVP8 || codecSettings->codecType == webrtc::kVideoCodecH264);
        if (codecSettings->codecType == webrtc::kVideoCodecVP8)
            frameFormat = woogeen_base::FRAME_FORMAT_VP8;
        else if (codecSettings->codecType == webrtc::kVideoCodecH264)
            frameFormat = woogeen_base::FRAME_FORMAT_H264;

        if (m_frameHandler->activateInput(m_index, frameFormat, m_provider)) {
            m_initCallback->onInputProcessorInitOK(m_index);
            return 0;
        }

        return -1;
    }
    virtual int32_t Decode(const webrtc::EncodedImage& inputImage,
                           bool missingFrames,
                           const webrtc::RTPFragmentationHeader* fragmentation,
                           const webrtc::CodecSpecificInfo* codecSpecificInfo = NULL,
                           int64_t renderTimeMs = -1)
    {
        m_frameHandler->pushInput(m_index, inputImage._buffer, inputImage._length);
        return 0;
    }
    virtual int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) { return 0; }
    virtual int32_t Release() { return 0; }
    virtual int32_t Reset() { return 0; }

private:
    int m_index;
    boost::shared_ptr<VideoFrameMixer> m_frameHandler;
    woogeen_base::VideoFrameProvider* m_provider;
    InputProcessorCallback* m_initCallback;
};

class VCMInputProcessor : public erizo::MediaSink,
                          public woogeen_base::VideoFrameProvider {
public:
    VCMInputProcessor(int index, bool externalDecoding = false);
    virtual ~VCMInputProcessor();

    // Implements the MediaSink interface.
    int deliverAudioData(char*, int len);
    int deliverVideoData(char*, int len);

    // Implements the VideoFrameProvider interface.
    virtual void setBitrate(unsigned short kbps, int id = 0);
    virtual void requestKeyFrame(int id = 0);

    bool init(woogeen_base::WebRTCTransport<erizo::VIDEO>*, boost::shared_ptr<VideoFrameMixer>, boost::shared_ptr<woogeen_base::WebRTCTaskRunner>, InputProcessorCallback*);

    void bindAudioForSync(int32_t voiceChannelId, webrtc::VoEVideoSync*);

private:
    int m_index;
    boost::scoped_ptr<woogeen_base::VCMFrameConstructor> m_frameConstructor;
    bool m_externalDecoding;
};

}
#endif /* VCMInputProcessor_h */
