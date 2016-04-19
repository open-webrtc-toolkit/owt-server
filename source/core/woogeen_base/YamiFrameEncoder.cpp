/*
 * Copyright 2016 Intel Corporation All Rights Reserved.
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

#include "YamiVideoDisplay.h"
#include "YamiVideoFrame.h"
#include "MediaUtilities.h"
#include <webrtc/modules/video_coding/codecs/vp8/vp8_factory.h>
#include <VideoEncoderHost.h>

using namespace webrtc;
using namespace YamiMediaCodec;

namespace woogeen_base {

DEFINE_LOGGER(YamiFrameEncoder, "woogeen.YamiFrameEncoder");

class VideoStream : public FrameSource
{
    DECLARE_LOGGER()
public:
    VideoStream():m_dest(NULL)
    {
        memset(&m_output, 0, sizeof(m_output));
    }

    ~VideoStream()
    {
        removeVideoDestination(m_dest);
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

        SharedPtr<VADisplay> vaDisplay = YamiGetVADisplay();
        if (!vaDisplay) {
            ELOG_ERROR("get va display failed");
            return false;
        }

        NativeDisplay display;
        display.type = NATIVE_DISPLAY_VA;
        display.handle = (intptr_t)*vaDisplay;
        m_encoder->setNativeDisplay(&display);

        Encode_Status status = m_encoder->start();
        if (status != ENCODE_SUCCESS) {
            ELOG_DEBUG("start encode failed status = %d", status);
        }

        return status == ENCODE_SUCCESS;
    }
    void onFrame(const SharedPtr<::VideoFrame>& input)
    {
      ELOG_ERROR("onFrame");
        Encode_Status status = m_encoder->encode(input);
        if (status != ENCODE_SUCCESS) {
            ELOG_DEBUG("encode status = %d", status);
            return;
        }
        status = m_encoder->getOutput(&m_output);
        ELOG_DEBUG("output format = %d", m_output.format);

        Frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.format = m_format;
        frame.payload = m_output.data;
        frame.additionalInfo.video.width = m_width;
        frame.additionalInfo.video.height = m_height;

        while (status == ENCODE_SUCCESS) {
            frame.length = m_output.dataSize;
            //frame.timeStamp = m_output.timeStamp;
            static int i;
            i++;
            frame.timeStamp = i*1000/30*90;
            ELOG_DEBUG("time: %d, %lu, len = %d", frame.timeStamp, m_output.timeStamp, frame.length);
            FILE* fp = fopen("/home/webrtc/yxian/webrtc-woogeen-2.latest/a.264", "ab");
            if (fp) {
                fwrite( m_output.data, 1, frame.length, fp);
                fclose(fp);
            } else {
            ELOG_DEBUG("open failed");
            }
            deliverFrame(frame);
            //get next output
            status = m_encoder->getOutput(&m_output);
        }
    }
    void setBitrate(unsigned short kbps)
    {

    }
    void requestKeyFrame()
    {
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

    FrameDestination* m_dest;

    //encoder
    boost::shared_ptr<IVideoEncoder> m_encoder;
};


DEFINE_LOGGER(VideoStream, "woogeen.VideoStream");

YamiFrameEncoder::YamiFrameEncoder(woogeen_base::FrameFormat format, bool useSimulcast)
    : m_encodeFormat(format)
    , m_useSimulcast(useSimulcast)
    , m_id(0)
{
}

YamiFrameEncoder::~YamiFrameEncoder()
{
}

bool YamiFrameEncoder::canSimulcastFor(FrameFormat format, uint32_t width, uint32_t height)
{
    return false;
}

bool YamiFrameEncoder::isIdle()
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);
    return m_streams.empty();
}

int32_t YamiFrameEncoder::generateStream(uint32_t width, uint32_t height, woogeen_base::FrameDestination* dest)
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
        YamiVideoFrame* holder = (YamiVideoFrame*)frame.payload;
        auto input = holder->frame;
	if (!input) {
	  ELOG_ERROR("input null");
	}
        for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
            it->second->onFrame(input);
        }
        break;
    }
    case FRAME_FORMAT_VP8:
        assert(false);
        break;
    case FRAME_FORMAT_H264:
        assert(false);
    default:
        break;
    }
}


} // namespace woogeen_base

