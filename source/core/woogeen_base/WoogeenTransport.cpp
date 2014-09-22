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

#include "WoogeenTransport.h"

namespace woogeen_base {

WoogeenTransport::WoogeenTransport(erizo::MediaSink* sink) {
    sink_ = sink;
}

WoogeenTransport::~WoogeenTransport() {
    // TODO Auto-generated destructor stub
}

int WoogeenTransport::SendPacket(int channel, const void* data, int len) {
    sink_->deliverVideoData(reinterpret_cast<char*>(const_cast<void*>(data)), len);
    return len; // return 0 will tell rtp_sender not able to send the packet, thus impact bitrate update
}


int WoogeenTransport::SendRTCPPacket(int channel, const void* data, int len) {
    return 0;
}

}
