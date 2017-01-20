/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

#include "YamiFrameEncoder.h"

#include "MediaUtilities.h"
#include <va/va_compat.h>
#include "VideoDisplay.h"
#include "VideoPostProcessHost.h"
#include "YamiVideoInputManager.h"
#include "YamiVideoFrame.h"
#include <VideoEncoderHost.h>

using namespace YamiMediaCodec;

namespace woogeen_base {

DEFINE_LOGGER(YamiFrameEncoder, "woogeen.YamiFrameEncoder");

class VideoStream : public FrameSource
{
    DECLARE_LOGGER()
public:
    VideoStream()
        : m_requestKeyFrame(false)
        , m_frameCount(0)
        , m_dest(nullptr)
    {
        memset(&m_output, 0, sizeof(m_output));
    }

    ~VideoStream()
    {
        removeVideoDestination(m_dest);
    }

    void onFeedback(const FeedbackMsg& msg) {
        if (msg.type == VIDEO_FEEDBACK) {
            if (msg.cmd == REQUEST_KEY_FRAME)
                requestKeyFrame();
            else if (msg.cmd == SET_BITRATE)
                setBitrate(msg.data.kbps);
        }
    }

    bool init(FrameFormat format, uint32_t width, uint32_t height, FrameDestination* dest)
    {
        if (!dest) {
            ELOG_DEBUG("null FrameDestination.");
            return false;
        }
        m_dest = dest;
        addVideoDestination(dest);
        m_format = format;
        switch (format) {
            case FRAME_FORMAT_VP8:
                m_encoder.reset(createVideoEncoder(YAMI_MIME_VP8), releaseVideoEncoder);
                ELOG_DEBUG("Created VP8 encoder.");
                break;
            case FRAME_FORMAT_H264:
                m_encoder.reset(createVideoEncoder(YAMI_MIME_H264), releaseVideoEncoder);
                ELOG_DEBUG("Created H.264 encoder.");
                break;
            case FRAME_FORMAT_H265:
                m_encoder.reset(createVideoEncoder(YAMI_MIME_H265), releaseVideoEncoder);
                ELOG_DEBUG("Created H.265 encoder.");
                break;
        default:
            ELOG_ERROR("Unspported video frame format %d", format);
            return false;
        }
        m_width = width;
        m_height = height;
        uint32_t size = m_width * m_height * 3 / 2;
        m_data.resize(size);
        m_output.data = &m_data[0];
        m_output.bufferSize = size;

        setEncoderParams();

        boost::shared_ptr<VADisplay> vaDisplay = GetVADisplay();
        if (!vaDisplay) {
            ELOG_ERROR("get va display failed");
            return false;
        }

        NativeDisplay display;
        display.type = NATIVE_DISPLAY_VA;
        display.handle = (intptr_t)*vaDisplay;
        m_encoder->setNativeDisplay(&display);

        m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
        m_vpp->setNativeDisplay(display);
        if(!m_allocator) {
          m_allocator.reset(new woogeen_base::PooledFrameAllocator(vaDisplay, 5));
          if (!m_allocator->setFormat(YAMI_FOURCC('Y', 'V', '1', '2'), m_width, m_height)) {
            ELOG_ERROR("failed to set VPP input frame format");
            return false;
          }
        }
        Encode_Status status = m_encoder->start();
        if (status != ENCODE_SUCCESS) {
            ELOG_DEBUG("start encode failed status = %d", status);
        }

        return status == ENCODE_SUCCESS;
    }
    void onFrame(const woogeen_base::Frame& input)
    {
        SharedPtr<::VideoFrame> inFrame = convert(input);

        ELOG_ERROR("onFrame, request key = %d", m_requestKeyFrame);
        inFrame->flags = m_requestKeyFrame ? VIDEO_FRAME_FLAGS_KEY : 0;
        m_requestKeyFrame = false;
        Encode_Status status = m_encoder->encode(inFrame);
        if (status != ENCODE_SUCCESS) {
            ELOG_DEBUG("encode status = %d", status);
            return;
        }
        status = m_encoder->getOutput(&m_output);

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_format;
        frame.payload = m_output.data;
        frame.additionalInfo.video.width = m_width;
        frame.additionalInfo.video.height = m_height;

        while (status == ENCODE_SUCCESS) {
            frame.length = m_output.dataSize;
            m_frameCount++;
            frame.timeStamp = m_frameCount*1000/30*90;
            ELOG_DEBUG("time: %d, %lu, len = %d", frame.timeStamp, m_output.timeStamp, frame.length);
            /*FILE* fp = fopen("/home/webrtc/yxian/webrtc-woogeen-2.latest/a.264", "ab");
            if (fp) {
                fwrite( m_output.data, 1, frame.length, fp);
                fclose(fp);
            } else {
            ELOG_DEBUG("open failed");
            }*/
            deliverFrame(frame);
            //get next output
            status = m_encoder->getOutput(&m_output);
        }
    }
    void setBitrate(unsigned short kbps)
    {
        if (!m_encoder)
            return ;
        ELOG_ERROR("set bit rate to %d", kbps);
        VideoParamsCommon encVideoParams;
        encVideoParams.size = sizeof(VideoParamsCommon);
        Encode_Status status = m_encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
        if (status != ENCODE_SUCCESS) {
            ELOG_ERROR("getParameters failed, return %d", status);
            return ;
        }
        encVideoParams.rcMode = RATE_CONTROL_CBR;
        encVideoParams.rcParams.bitRate = kbps * 1024;
        status = m_encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);
         if (status != ENCODE_SUCCESS) {
            ELOG_ERROR("setParameters failed, return %d", status);
            return ;
        }

    }
    void requestKeyFrame()
    {
        m_requestKeyFrame = true;
    }
private:
    void setEncoderParams()
    {
              //configure encoding parameters
        VideoParamsCommon encVideoParams;
        encVideoParams.size = sizeof(VideoParamsCommon);
        m_encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
        encVideoParams.resolution.width = m_width;
        encVideoParams.resolution.height = m_height;

        //frame rate parameters.
        encVideoParams.frameRate.frameRateDenom = 1;
        encVideoParams.frameRate.frameRateNum = 30; //???
        encVideoParams.rcMode = RATE_CONTROL_CBR;
        uint32_t targetKbps = calcBitrate(m_width, m_height) * (m_format == FRAME_FORMAT_VP8 ? 0.9 : 1);
        encVideoParams.rcParams.bitRate = targetKbps * 1000;
        ELOG_ERROR("(%d, %d), set bitrate to %d", m_width, m_height, encVideoParams.rcParams.bitRate);

        encVideoParams.size = sizeof(VideoParamsCommon);
        m_encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);

        VideoConfigAVCStreamFormat streamFormat;
        streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
        streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
        m_encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);
    }
private:
    //format
    uint32_t m_width;
    uint32_t m_height;
    FrameFormat m_format;

    ///output
    VideoEncOutputBuffer m_output;
    std::vector<uint8_t> m_data;

    bool m_requestKeyFrame;
    int64_t m_frameCount;

    FrameDestination* m_dest;

    //encoder
    boost::shared_ptr<IVideoEncoder> m_encoder;
    //vpp
    SharedPtr<YamiMediaCodec::IVideoPostProcess> m_vpp;
    SharedPtr<woogeen_base::PooledFrameAllocator> m_allocator;
protected:
    SharedPtr<::VideoFrame> convert(const woogeen_base::Frame& frame)
    {
      YamiVideoFrame* holder = (YamiVideoFrame*)frame.payload;
      auto input = holder->frame;

      if (m_width != frame.additionalInfo.video.width || m_height != frame.additionalInfo.video.height) {
        SharedPtr<::VideoFrame> dst = m_allocator->alloc();
        if (!dst) {
          return dst;
        }
        m_vpp->process(input, dst);
        return dst;
      } else {
        return input;
      }
    }
};


DEFINE_LOGGER(VideoStream, "woogeen.VideoStream");

YamiFrameEncoder::YamiFrameEncoder(FrameFormat format, bool useSimulcast)
    : m_encodeFormat(format)
    , m_useSimulcast(useSimulcast)
    , m_id(0)
{
}

YamiFrameEncoder::~YamiFrameEncoder()
{
}

bool YamiFrameEncoder::supportFormat(FrameFormat format)
{
    // TODO: Query the hardware/libyami capability to encode the specified format.
    return (format == FRAME_FORMAT_H264 || format == FRAME_FORMAT_H265);
}

bool YamiFrameEncoder::canSimulcast(FrameFormat format, uint32_t width, uint32_t height)
{
    return false;
}

bool YamiFrameEncoder::isIdle()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    return m_streams.empty();
}

int32_t YamiFrameEncoder::generateStream(uint32_t width, uint32_t height, uint32_t bitrateKbps, FrameDestination* dest)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    boost::shared_ptr<VideoStream> stream(new VideoStream());
    if (!stream->init(m_encodeFormat, width, height, dest)) {
        ELOG_ERROR("addFrameConsumer failed");
        return -1;
    }
    m_streams[m_id] = stream;
    return m_id++;
}

void YamiFrameEncoder::degenerateStream(int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    m_streams.erase(id);
}

void YamiFrameEncoder::setBitrate(unsigned short kbps, int id)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    auto it = m_streams.find(id);
    if (it != m_streams.end()) {
        it->second->setBitrate(kbps);
    }
}

void YamiFrameEncoder::requestKeyFrame(int id)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    auto it = m_streams.find(id);
    if (it != m_streams.end()) {
        it->second->requestKeyFrame();
    }
}

void YamiFrameEncoder::onFrame(const Frame& frame)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    switch (frame.format) {
    case FRAME_FORMAT_YAMI: {
        for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
            it->second->onFrame(frame);
        }
        break;
    }
    case FRAME_FORMAT_VP8:
        assert(false);
        break;
    case FRAME_FORMAT_H264:
        assert(false);
        break;
    case FRAME_FORMAT_H265:
        assert(false);
        break;
    default:
        break;
    }
}

} // namespace woogeen_base
