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

#ifndef VideoFrameInputProcessor_h
#define VideoFrameInputProcessor_h

#include "VideoFrameMixer.h"
#include "VideoFramePipeline.h"
#include "VideoInputProcessorBase.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <VideoFramePipeline.h>

#include "webrtc/common_types.h"
#include "webrtc/video_decoder.h"

namespace mcu {

class DecodedFrameHandler : public webrtc::DecodedImageCallback {

    DECLARE_LOGGER();

public:
   DecodedFrameHandler(int index,
                       boost::shared_ptr<VideoFrameMixer> frameMixer,
                       woogeen_base::VideoFrameProvider* provider,
                       InputProcessorCallback* initCallback);

   virtual ~DecodedFrameHandler();

   virtual int32_t Decoded(webrtc::I420VideoFrame& decodedImage);

private:
    int m_index;
    boost::shared_ptr<VideoFrameMixer> m_frameMixer;
};

class RawFrameDecoder {

    DECLARE_LOGGER();

public:
   RawFrameDecoder(int index,
                   boost::shared_ptr<VideoFrameMixer> frameMixer,
                   woogeen_base::VideoFrameProvider* provider,
                   InputProcessorCallback* initCallback);

   virtual ~RawFrameDecoder();

   int32_t InitDecode(const webrtc::VideoCodec* codecSettings);

   int32_t Decode(unsigned char* payload, int len);

private:
    int m_index;
    boost::shared_ptr<VideoFrameMixer> m_frameMixer;
    woogeen_base::VideoFrameProvider* m_provider;
    InputProcessorCallback* m_initCallback;
};

class VideoFrameInputProcessor: public erizo::MediaSink,
                                public woogeen_base::VideoFrameProvider {

    DECLARE_LOGGER();

public:
    VideoFrameInputProcessor(int index, bool externalDecoding = false);
    virtual ~VideoFrameInputProcessor();

    // Implements the VideoFrameProvider interface.
    virtual void requestKeyFrame(int id = 0) {}
    virtual void setBitrate(unsigned short kbps, int id = 0) {}

    bool init(const std::string& codecName, boost::shared_ptr<VideoFrameMixer> frameReceiver, InputProcessorCallback* initCallback);

    // Implements the MediaSink interface.
    virtual int deliverAudioData(char* buf, int len);
    virtual int deliverVideoData(char* buf, int len);

private:
    int m_index;
    bool m_externalDecoding;

    boost::scoped_ptr<webrtc::VideoDecoder> m_decoder;
    boost::shared_ptr<DecodedFrameHandler> m_frameHandler;
    boost::shared_ptr<RawFrameDecoder> m_frameDecoder;
    boost::shared_ptr<VideoFrameMixer> m_frameMixer;
};

} /* namespace mcu */

#endif /* VideoFrameInputProcessor_h */
