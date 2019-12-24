// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SVTHEVCEncoder_h
#define SVTHEVCEncoder_h

#include <queue>

#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "logger.h"
#include "MediaFramePipeline.h"
#include "SVTHEVCEncoderBase.h"

namespace owt_base {

class SVTHEVCEncoder : public VideoFrameEncoder {
    DECLARE_LOGGER();

public:
    SVTHEVCEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~SVTHEVCEncoder();

    FrameFormat getInputFormat() {return FRAME_FORMAT_I420;}

    // Implements VideoFrameEncoder.
    void onFrame(const Frame&);
    bool canSimulcast(FrameFormat format, uint32_t width, uint32_t height);
    bool isIdle();
    int32_t generateStream(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds, FrameDestination* dest);
    void degenerateStream(int32_t streamId);
    void setBitrate(unsigned short kbps, int32_t streamId);
    void requestKeyFrame(int32_t streamId);

    void sendLoop(void);

protected:
    static void InitEncoder(SVTHEVCEncoder*This, uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
    {
        This->initEncoder(width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);
    }

    bool initEncoder(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);
    bool initEncoderAsync(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);

    void fillPacketDone(boost::shared_ptr<SVTHEVCEncodedPacket> encoded_pkt);

private:
    bool                        m_encoderReady;
    FrameDestination            *m_dest;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;
    uint32_t m_keyFrameIntervalSeconds;

    boost::shared_ptr<SVTHEVCEncoderBase> m_hevc_encoder;
    uint32_t m_encoded_frame_count;

    boost::shared_mutex m_mutex;

    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread> m_thread;

    std::queue<boost::shared_ptr<SVTHEVCEncodedPacket>> m_packet_queue;
    boost::mutex m_queueMutex;
    boost::condition_variable m_queueCond;

    boost::thread m_sendThread;
    bool m_sendThreadExited;
};

} /* namespace owt_base */
#endif /* SVTHEVCEncoder_h */
