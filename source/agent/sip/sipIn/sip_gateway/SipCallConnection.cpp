// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#define HAVE_STDBOOL_H  // Dealing with libre warning

#include "SipCallConnection.h"

#include <rtputils.h>
#include "SipGateway.h"

extern "C" {

/******************/
/* Media:         */
/******************/
/* Attention: all media data bufs sent and received through sipua CONTAIN rtp-headers. */
/*endpoint -> sipua */
extern void call_connection_tx_audio(void *call, uint8_t *data, size_t len);
extern void call_connection_tx_video(void *call, uint8_t *data, size_t len);
extern void call_connection_fir(void *call);

void call_connection_rx_audio(void* owner, uint8_t* data, size_t len)
{
    if (owner != NULL) {
        sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
        obj->onSipAudioData(reinterpret_cast<char*>(data), static_cast<int>(len));
    }
}

void call_connection_rx_video(void* owner, uint8_t* data, size_t len)
{
    if (owner != NULL) {
        sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
        obj->onSipVideoData(reinterpret_cast<char*>(data), static_cast<int>(len));
    }
}

void call_connection_rx_fir(void* owner)
{
    if (owner != NULL) {
        sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
        obj->onSipFIR();
    }
}

void call_connection_closed(void* owner) {
    if (owner != NULL) {
        sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
        obj->onConnectionClosed();
    }
}

} // extern "C"


namespace sip_gateway {

DEFINE_LOGGER(SipCallConnection, "sip.SipCallConnection");

SipCallConnection::SipCallConnection(SipGateway* gateway, const std::string& peerURI):
      m_sipCall(NULL),
      m_gateway(gateway),
      m_peerURI(peerURI),
      sequenceNumberFIR_(0)
{
    if (gateway) {
        const CallInfo *info = gateway->getCallInfoByPeerURI(peerURI);
        if (info) {
            m_audioCodec = info->audioCodec;
            m_videoCodec = info->videoCodec;
            m_sipCall = info->sipCall;
            if (m_sipCall)
                gateway->helpSetCallOwner(m_sipCall, static_cast<void*>(this));
        }
    }

    video_sink_ = NULL;
    audio_sink_ = NULL;
    fb_sink_ = NULL;
    source_fb_sink_ = this;
    sink_fb_source_ = this;
    running = true;
    ELOG_DEBUG("SipCallConnection() Codec info %s %s", m_audioCodec.c_str(), m_videoCodec.c_str());
}

SipCallConnection::~SipCallConnection()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_gateway->resetCallOwner(m_sipCall);
    running = false;
    video_sink_ = NULL;
    audio_sink_ = NULL;
    fb_sink_ = NULL;
    ELOG_DEBUG("~SipCallConnection");
}

// sink
int SipCallConnection::deliverAudioData_(std::shared_ptr<erizo::DataPacket> audio_packet)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
        call_connection_tx_audio(m_sipCall, (uint8_t*)audio_packet->data, (size_t)audio_packet->length);
    }
    return 0;
}

//sink
int SipCallConnection::deliverVideoData_(std::shared_ptr<erizo::DataPacket> video_packet)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
      call_connection_tx_video(m_sipCall, (uint8_t*)video_packet->data, (size_t)video_packet->length);

      //Some sip devices, such as Jitsi, doesn't send FIR, which leads to that
      //it will wait for quite a while until it shows the video.
      //So here trigger the onSipFIR as soon as deliver video frames to sip devices.
      //FIXME: Remove this workaround when jitsi fixes the no fir issue.
      if(sequenceNumberFIR_ == 0){
        onSipFIR();
      }
    }
    return 0;
}

int SipCallConnection::sendFirPacket()
{

    ELOG_DEBUG("sendFirPacket");
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
      call_connection_fir(m_sipCall);
    }
    return 0;
}

int SipCallConnection::sendPLI() {
    //return sendFirPacket();
    return 0;
}

int SipCallConnection::deliverFeedback_(std::shared_ptr<erizo::DataPacket> data_packet)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
       call_connection_fir(m_sipCall);
    }
    return 0;
}

void SipCallConnection::onSipAudioData(char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
        if (audio_sink_ == NULL)
            return;

        RTPHeader* head = reinterpret_cast<RTPHeader*>(data);
        if (m_audioCodec == "opus")
            head->setPayloadType(OPUS_48000_PT);
       {
            audio_sink_->deliverAudioData(std::make_shared<erizo::DataPacket>(0, data, len, erizo::AUDIO_PACKET));
       }
   }
}

void SipCallConnection::onSipVideoData(char* data, int len)
{
    //ELOG_DEBUG("SipCallConnection::onSipVideoData");
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
        if (video_sink_ == NULL)
            return;

        RTPHeader* head = reinterpret_cast<RTPHeader*>(data);
        if (m_videoCodec == "VP8") {
          head->setPayloadType(VP8_90000_PT);
        } else if (m_videoCodec == "H264") {
          head->setPayloadType(H264_90000_PT);
        }
        {
           video_sink_->deliverVideoData(std::make_shared<erizo::DataPacket>(0, data, len, erizo::VIDEO_PACKET));
        }
    }
}

void SipCallConnection::onSipFIR()
{
    ELOG_DEBUG("SipCallConnection::onSipFIR");
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
        if(fb_sink_ == NULL)
            return;
        //as the MediaSink, handle sip client fir request, deliver to FramePacketizer
        ++sequenceNumberFIR_;
        int pos = 0;
        uint8_t rtcpPacket[50];
        // add full intra request indicator
        uint8_t FMT = 4;
        rtcpPacket[pos++] = (uint8_t) 0x80 + FMT;
        rtcpPacket[pos++] = (uint8_t) 206;

        //Length of 4
        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) (4);

        // Add our own SSRC
        uint32_t* ptr = reinterpret_cast<uint32_t*>(rtcpPacket + pos);
        ptr[0] = htonl(this->getVideoSourceSSRC());
        pos += 4;

        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) 0;
        // Additional Feedback Control Information (FCI)
        uint32_t* ptr2 = reinterpret_cast<uint32_t*>(rtcpPacket + pos);
        ptr2[0] = htonl(this->getVideoSinkSSRC());
        pos += 4;

        rtcpPacket[pos++] = (uint8_t) (sequenceNumberFIR_);
        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) 0;
        rtcpPacket[pos++] = (uint8_t) 0;

        fb_sink_->deliverFeedback(
            std::make_shared<erizo::DataPacket>(0, (char *)rtcpPacket, pos, erizo::VIDEO_PACKET));
    }
}

void SipCallConnection::onConnectionClosed()
{
    ELOG_DEBUG("Enter onConnectionClosed");
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    running = false;
    sequenceNumberFIR_ = 0;
    ELOG_DEBUG("Leave onConnectionClosed");
}

void SipCallConnection::refreshVideoStream() {
    // as the source
    // source_fb_sink_->sendFirPacket();
}

void SipCallConnection::close() {
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    running = false;
    m_gateway->resetCallOwner(m_sipCall);
}

} // end of extern "C"
