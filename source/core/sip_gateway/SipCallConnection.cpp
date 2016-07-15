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
    sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
    obj->onSipAudioData(reinterpret_cast<char*>(data), static_cast<int>(len));
}

void call_connection_rx_video(void* owner, uint8_t* data, size_t len)
{
    sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
    obj->onSipVideoData(reinterpret_cast<char*>(data), static_cast<int>(len));
}

void call_connection_rx_fir(void* owner)
{
    sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
    obj->onSipFIR();
}

void call_connection_closed(void* owner) {
    sip_gateway::SipCallConnection* obj = static_cast<sip_gateway::SipCallConnection*>(owner);
    if (obj != NULL) {
       obj->onConnectionClosed();
    }
}

} // extern "C"


namespace sip_gateway {

DEFINE_LOGGER(SipCallConnection, "sip.SipCallConnection");

SipCallConnection::SipCallConnection(SipGateway* gateway, const std::string& peerURI):
      m_gateway(gateway),
      m_peerURI(peerURI)
{
    const CallInfo *info = gateway->getCallInfoByPeerURI(peerURI);
    m_sipCall = info->sipCall;
    m_audioCodec = info->audioCodec;
    m_videoCodec = info->videoCodec;
    if (m_sipCall)
        gateway->helpSetCallOwner(m_sipCall, static_cast<void*>(this));
    videoSink_ = NULL;
    audioSink_ = NULL;
    fbSink_ = NULL;
    sourcefbSink_ = this;
    sinkfbSource_ = this;
    running = true;
    ELOG_DEBUG("SipCallConnection() Codec info %s %s", m_audioCodec.c_str(), m_videoCodec.c_str());
}

SipCallConnection::~SipCallConnection()
{
    videoSink_ = NULL;
    audioSink_ = NULL;
    fbSink_ = NULL;
    ELOG_DEBUG("~SipCallConnection");
}

// sink
int SipCallConnection::deliverAudioData(char* buf, int len)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
        call_connection_tx_audio(m_sipCall, (uint8_t*)buf, (size_t)len);
    }
    return 0;
}

//sink
int SipCallConnection::deliverVideoData(char* buf, int len)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
      call_connection_tx_video(m_sipCall, (uint8_t*)buf, (size_t)len);
    }
    return 0;
}

int SipCallConnection::sendFirPacket()
{

    ELOG_DEBUG("sendFirPacket");
    //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
      call_connection_fir(m_sipCall);
    }
    return 0;
}

int SipCallConnection::deliverFeedback(char* buf, int len)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    if (running) {
       call_connection_fir(m_sipCall);
    }
    return 0;
}

void SipCallConnection::onSipAudioData(char* data, int len)
{
    if (running) {
        if (audioSink_ == NULL)
            return;

        RTPHeader* head = reinterpret_cast<RTPHeader*>(data);
        if (m_audioCodec == "opus")
            head->setPayloadType(OPUS_48000_PT);
       {
            //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
            audioSink_->deliverAudioData(data, len);
       }
   }
}

void SipCallConnection::onSipVideoData(char* data, int len)
{
    if (running) {
        if (videoSink_ == NULL)
            return;

        RTPHeader* head = reinterpret_cast<RTPHeader*>(data);
        if (m_videoCodec == "VP8") {
          head->setPayloadType(VP8_90000_PT);
        } else if (m_videoCodec == "H264") {
          head->setPayloadType(H264_90000_PT);
        }
        {
          //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
          videoSink_->deliverVideoData(data, len);
        }
    }
}

void SipCallConnection::onSipFIR()
{
    if (running) {
        //as the MediaSink, handle sip client fir request, deliver to FramePacketizer
        {
          //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
          fbSink_->deliverFeedback(NULL, 0);
        }
    }
}

void SipCallConnection::onConnectionClosed()
{
    ELOG_DEBUG("Enter onConnectionClosed");
    //boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    running = false;
    ELOG_DEBUG("Leave onConnectionClosed");
}

void SipCallConnection::refreshVideoStream() {
    // as the source
    // sourcefbSink_->sendFirPacket();
}

} // end of extern "C"
