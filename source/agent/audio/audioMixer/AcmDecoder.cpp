// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <rtputils.h>

#include "AudioUtilities.h"
#include "AcmDecoder.h"

namespace mcu {

using namespace webrtc;
using namespace owt_base;

DEFINE_LOGGER(AcmDecoder, "mcu.media.AcmDecoder");

AcmDecoder::AcmDecoder(const FrameFormat format)
    : m_format(format)
    , m_ssrc(0)
    , m_seqNumber(0)
    , m_valid(false)
    , m_timestamp(0)
{
    AudioCodingModule::Config config;
    m_audioCodingModule.reset(AudioCodingModule::Create(config));
}

AcmDecoder::~AcmDecoder()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    int ret;

    if (!m_valid)
        return;

    ret = m_audioCodingModule->UnregisterReceiveCodec(getAudioPltype(m_format));
    if (ret != 0) {
        ELOG_ERROR_T("Error RegisterReceiveCodec");
        return;
    }

    m_format = FRAME_FORMAT_UNKNOWN;
}

bool AcmDecoder::init()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    int ret;
    struct CodecInst codec = CodecInst();

    ret = m_audioCodingModule->InitializeReceiver();
    if (ret != 0) {
        ELOG_ERROR_T("Error InitializeReceiver");
        return false;
    }

    if (!getAudioCodecInst(m_format, codec)) {
        ELOG_ERROR_T("Error fillAudioCodec(%s)", getFormatStr(m_format));
        return false;
    }

    ret = m_audioCodingModule->RegisterReceiveCodec(codec);
    if (ret != 0) {
        ELOG_ERROR_T("Error RegisterReceiveCodec(%s)", getFormatStr(m_format));
        return false;
    }

    srand((unsigned)time(0));
    m_ssrc = rand();
    m_seqNumber = 0;
    m_valid = true;

    return true;
}

bool AcmDecoder::getAudioFrame(AudioFrame* audioFrame)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    int ret;
    bool muted;

    if (!m_valid)
        return false;

    ret = m_audioCodingModule->PlayoutData10Ms(audioFrame->sample_rate_hz_, audioFrame, &muted);
    if (ret == -1) {
        ELOG_ERROR_T("getAudioFrame, fail to get raw from acm");
        return false;
    }

    if (muted) {
        return false;
    }

    return true;
}

void AcmDecoder::onFrame(const Frame& frame)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    WebRtcRTPHeader rtp_header;
    uint8_t *payload = NULL;
    size_t length = 0;
    int ret;

    if (!m_valid) {
        ELOG_ERROR_T("Not valid");
        return;
    }

    memset(&rtp_header, 0, sizeof(WebRtcRTPHeader));
    rtp_header.frameType = kAudioFrameSpeech;

    if (frame.additionalInfo.audio.isRtpPacket) {
        ::RTPHeader *head = reinterpret_cast<::RTPHeader*>(frame.payload);

        rtp_header.header.markerBit                 = head->getMarker();
        rtp_header.header.payloadType               = head->getPayloadType();
        rtp_header.header.sequenceNumber            = head->getSeqNumber();
        rtp_header.header.timestamp                 = head->getTimestamp();
        rtp_header.header.ssrc                      = head->getSSRC();
        rtp_header.header.numCSRCs                  = 0;
        rtp_header.header.paddingLength             = 0;
        rtp_header.header.headerLength              = head->getHeaderLength();
        rtp_header.header.payload_type_frequency    = frame.additionalInfo.audio.sampleRate;

        payload = frame.payload + head->getHeaderLength();
        length = frame.length - head->getHeaderLength();
    } else {
        rtp_header.header.markerBit                 = false;
        rtp_header.header.payloadType               = getAudioPltype(frame.format);
        rtp_header.header.sequenceNumber            = m_seqNumber++;
        rtp_header.header.timestamp                 = frame.timeStamp;
        rtp_header.header.ssrc                      = m_ssrc;
        rtp_header.header.numCSRCs                  = 0;
        rtp_header.header.paddingLength             = 0;
        rtp_header.header.headerLength              = 12;
        rtp_header.header.payload_type_frequency    = frame.additionalInfo.audio.sampleRate;

        payload = frame.payload;
        length = frame.length;
    }

    if (rtp_header.header.timestamp == 0) {
        // TODO: Fill a correct timestamp and check overflow.
        m_timestamp += 10*1000;
        rtp_header.header.timestamp = m_timestamp;
    }

    ELOG_TRACE_T("onFrame(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%ld), seqNum(%d), %s",
            getFormatStr(frame.format),
            frame.additionalInfo.audio.sampleRate,
            frame.additionalInfo.audio.channels,
            rtp_header.header.timestamp * 1000 / frame.additionalInfo.audio.sampleRate,
            length,
            rtp_header.header.sequenceNumber,
            frame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket"
            );

    ret = m_audioCodingModule->IncomingPacket(payload, length, rtp_header);
    if (ret != 0) {
        ELOG_ERROR_T("Fail to insert compressed into acm, format(%s), sampleRate(%d), channels(%d), timeStamp(%d), length(%ld), seqNum(%d), %s",
                getFormatStr(frame.format),
                frame.additionalInfo.audio.sampleRate,
                frame.additionalInfo.audio.channels,
                rtp_header.header.timestamp * 1000 / frame.additionalInfo.audio.sampleRate,
                length,
                rtp_header.header.sequenceNumber,
                frame.additionalInfo.audio.isRtpPacket ? "RtpPacket" : "NonRtpPacket"
                );
    }
}

} /* namespace mcu */
