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

#ifndef WoogeenTransport_h
#define WoogeenTransport_h

#include <MediaDefinitions.h>
#include <webrtc/common_types.h>

namespace woogeen_base {

class WoogeenVideoTransport : public webrtc::Transport {
public:
    WoogeenVideoTransport(erizo::RTPDataReceiver*);
    virtual ~WoogeenVideoTransport();

    virtual int SendPacket(int channel, const void* data, int len);
    virtual int SendRTCPPacket(int channel, const void* data, int len);

private:
    erizo::RTPDataReceiver* m_receiver;
};

class WoogeenAudioTransport : public webrtc::Transport {
public:
    WoogeenAudioTransport(erizo::RTPDataReceiver*);
    virtual ~WoogeenAudioTransport();

    virtual int SendPacket(int channel, const void* data, int len);
    virtual int SendRTCPPacket(int channel, const void* data, int len);

private:
    erizo::RTPDataReceiver* m_receiver;
};

}
#endif /* WoogeenTransport_h */
