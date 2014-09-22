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

#ifndef OoVooOutboundStreamProcessor_h
#define OoVooOutboundStreamProcessor_h

#include "OoVooProtocolHeader.h"
#include "OoVooStreamProcessorHelper.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <logger.h>
#include <MediaDefinitions.h>
#include <webrtc/modules/rtp_rtcp/interface/rtp_header_parser.h>

namespace oovoo_gateway {

class OoVooOutboundStreamProcessor : public erizo::RTPDataReceiver {
    DECLARE_LOGGER();
public:
    OoVooOutboundStreamProcessor();
    virtual ~OoVooOutboundStreamProcessor();

    void receiveRtpData(char*, int len, erizo::DataType, uint32_t);

    void init(OoVooDataReceiver*);
    void setVideoStreamResolution(uint32_t streamId, VideoResolutionType);
    bool mapSsrcToStreamId(uint32_t ssrc, uint32_t streamId);
    uint32_t unmapSsrc(uint32_t ssrc);

private:
    void processRtcpData(char*, int len, bool isAudio);
    void processAudioData(char*, int len);
    void processVideoData(char*, int len);

    typedef struct {
        uint32_t frameId;
        uint32_t fragmentId;
        VideoResolutionType resolution;
    } AuxiliaryPacketInfo;

    AuxiliaryPacketInfo& getAuxiliaryPacketInfo(uint32_t streamId);

    std::map<uint32_t, uint32_t> m_ssrcStreamIdMap;
    boost::shared_mutex m_ssrcStreamIdMapMutex;
    // The m_auxPacketInfoMap tracks the auxiliary packet information (fragment id
    // and frame id) for a given stream, which is used when the frame ID is not
    // directly available from a single RTP packet or setting the fragment id.
    std::map<uint32_t, AuxiliaryPacketInfo> m_auxPacketInfoMap;
    boost::scoped_ptr<webrtc::RtpHeaderParser> m_rtpHeaderParser;
    mediaPacket_t m_audioPacket;
    mediaPacket_t m_videoPacket;
    boost::mutex m_audioMutex, m_videoMutex;
    OoVooDataReceiver* m_ooVooReceiver;
};

} /* namespace oovoo_gateway */

#endif /* OoVooOutboundStreamProcessor_h */
