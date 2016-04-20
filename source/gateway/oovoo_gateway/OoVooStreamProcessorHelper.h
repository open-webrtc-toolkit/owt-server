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

#ifndef OoVooStreamProcessorHelper_h
#define OoVooStreamProcessorHelper_h

#include "OoVooProtocolHeader.h"
#include <sstream>

namespace oovoo_gateway {

static const int MIXED_OOVOO_AUDIO_STREAM_ID = -1;
static const int NOT_A_OOVOO_STREAM_ID = 0;

class OoVooDataReceiver {
public:
    virtual void receiveOoVooData(const mediaPacket_t& packet) = 0;
    virtual void receiveOoVooSyncMsg(uint64_t ntp, uint64_t rtp, bool isAudio, uint32_t streamId) = 0;
    virtual ~OoVooDataReceiver() { }
};

inline std::string dumpMediaPacket(const mediaPacket_t& packet)
{
    std::ostringstream output;
    output << (packet.isAudio ? "audio" : "video") << ": length " << packet.length
        << ", streamId " << packet.streamId << ", packetId " << packet.packetId
        << ", frameId " << packet.frameId << ", fragmentId " << packet.fragmentId
        << ", resolution " << packet.resolution
        << ", isLastFragment " << packet.isLastFragment << ", isKey "
        << packet.isKey << ", timestamp " << packet.ts;
    return output.str();
}

// Magic number from the webrtc reference implementation.
const double kNtpFracPerMs = 4.294967296E6;

// Code copied from webrtc rtp_rtcp module;
// It's crazy that the Ntp timestamp in WebRTC RTCP packet has another magic!
inline int64_t NtpToMs(uint32_t ntp_secs, uint32_t ntp_frac)
{
    const double ntp_frac_ms = static_cast<double>(ntp_frac) / kNtpFracPerMs;
    return 1000 * static_cast<int64_t>(ntp_secs) +
            static_cast<int64_t>(ntp_frac_ms + 0.5);
}

inline void MsToNtp(uint64_t ms, uint32_t& ntp_secs, uint32_t& ntp_frac)
{
    ntp_secs = ms / 1000;
    ntp_frac = static_cast<uint32_t>(static_cast<double>(ms % 1000) * kNtpFracPerMs + 0.5);
}

} /* namespace oovoo_gateway */

#endif /* OoVooStreamProcessorHelper_h */
