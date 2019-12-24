// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SVTHEVCMCTSEncoder_h
#define SVTHEVCMCTSEncoder_h

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

class SVTHEVCMCTSEncoder : public VideoFrameEncoder, SVTHEVCEncoderdPacketListener {
    DECLARE_LOGGER();

public:
    SVTHEVCMCTSEncoder(FrameFormat format, VideoCodecProfile profile, bool useSimulcast = false);
    ~SVTHEVCMCTSEncoder();

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

    void onEncodedPacket(SVTHEVCEncoderBase *encoder, boost::shared_ptr<SVTHEVCEncodedPacket> pkt);

protected:
    static void InitEncoder(SVTHEVCMCTSEncoder*This, uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds)
    {
        This->initEncoder(width, height, frameRate, bitrateKbps, keyFrameIntervalSeconds);
    }

    bool initEncoder(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);
    bool initEncoderAsync(uint32_t width, uint32_t height, uint32_t frameRate, uint32_t bitrateKbps, uint32_t keyFrameIntervalSeconds);

    void fillPacketsDone(boost::shared_ptr<SVTHEVCEncodedPacket> hi_res_pkt, boost::shared_ptr<SVTHEVCEncodedPacket> low_res_pkt);

private:
    bool                        m_encoderReady;
    FrameDestination            *m_dest;

    uint32_t m_width_hi;
    uint32_t m_height_hi;
    uint32_t m_width_low;
    uint32_t m_height_low;
    uint32_t m_frameRate;
    uint32_t m_bitrateKbps;
    uint32_t m_keyFrameIntervalSeconds;

    uint32_t m_encodedFrameCount;

    boost::shared_ptr<SVTHEVCEncoderBase> m_hi_res_encoder;
    boost::shared_ptr<SVTHEVCEncoderBase> m_low_res_encoder;

    uint32_t m_hi_res_pending_call;
    uint32_t m_low_res_pending_call;

    boost::shared_mutex m_mutex;

    std::queue<boost::shared_ptr<SVTHEVCEncodedPacket>> m_hi_res_packet_queue;
    std::queue<boost::shared_ptr<SVTHEVCEncodedPacket>> m_low_res_packet_queue;
    boost::mutex m_queueMutex;
    boost::condition_variable m_queueCond;

    uint8_t *m_payload_buffer;
    uint32_t m_payload_buffer_length;

    boost::shared_ptr<boost::asio::io_service> m_srv;
    boost::shared_ptr<boost::asio::io_service::work> m_srvWork;
    boost::shared_ptr<boost::thread> m_thread;

    boost::thread m_sendThread;
    bool m_sendThreadExited;
};

} /* namespace owt_base */
#endif /* SVTHEVCMCTSEncoder_h */
