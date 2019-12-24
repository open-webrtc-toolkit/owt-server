// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SVTHEVCEncoderBase_h
#define SVTHEVCEncoderBase_h

#include <queue>

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <webrtc/api/video/video_frame.h>
#include <webrtc/api/video/video_frame_buffer.h>

#include "logger.h"
#include "MediaFramePipeline.h"

#include "svt-hevc/EbApi.h"

namespace owt_base {

class SVTHEVCEncodedPacket {
public:
    SVTHEVCEncodedPacket(EB_BUFFERHEADERTYPE* pBuffer);
    ~SVTHEVCEncodedPacket();

    uint8_t *data;
    int32_t length;
    bool isKey;
    int64_t pts;
};

class SVTHEVCEncoderBase;
class SVTHEVCEncoderdPacketListener {
public:
    virtual void onEncodedPacket(SVTHEVCEncoderBase *encoder, boost::shared_ptr<SVTHEVCEncodedPacket> pkt) = 0;
};

class SVTHEVCEncoderBase {
    DECLARE_LOGGER();

public:
    SVTHEVCEncoderBase();
    ~SVTHEVCEncoderBase();

    void setDebugDump(bool enable);

    bool init(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, uint32_t tiles_col, uint32_t tiles_row);

    bool sendFrame(const Frame& frame);
    bool sendVideoFrame(webrtc::VideoFrame *video_frame);
    bool sendVideoBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer, int64_t dts, int64_t pts);
    bool sendPicture(const uint8_t *y_plane, uint32_t y_stride,
            const uint8_t *u_plane, uint32_t u_stride,
            const uint8_t *v_plane, uint32_t v_stride,
            int64_t dts, int64_t pts);

    boost::shared_ptr<SVTHEVCEncodedPacket> getEncodedPacket(void);

    void requestKeyFrame();

    // async
    void register_listener(SVTHEVCEncoderdPacketListener *listener) {m_listener = listener;}
    void sendVideoBuffer_async(rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer, int64_t dts, int64_t pts);
    static void SendVideoBuffer_async(SVTHEVCEncoderBase *This, rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_buffer, int64_t dts, int64_t pts);

protected:
    void initDefaultParameters();
    void updateParameters(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);
    void setMCTSParameters(uint32_t tiles_col, uint32_t tiles_row);

    bool allocateBuffers();
    void deallocateBuffers();

    EB_BUFFERHEADERTYPE *getOutBuffer(void);

    void dump(uint8_t *buf, int len);

private:
    EB_COMPONENTTYPE            *m_handle;
    EB_H265_ENC_CONFIGURATION   m_encParameters;

    EB_H265_ENC_INPUT m_inputFrameBuffer;
    EB_BUFFERHEADERTYPE m_inputBuffer;
    EB_BUFFERHEADERTYPE m_outputBuffer;

    uint8_t *m_scaling_frame_buffer;

    bool m_forceIDR;

    boost::shared_mutex m_mutex;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;

    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread> m_thread;

    SVTHEVCEncoderdPacketListener *m_listener;
};

} /* namespace owt_base */
#endif /* SVTHEVCEncoderBase_h */
