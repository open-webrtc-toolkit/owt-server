/*
 * Copyright 2015 Intel Corporation All Rights Reserved. 
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

#include "InternalIn.h"

namespace woogeen_base {

InternalIn::InternalIn(const std::string& protocol)
{
    if (protocol == "tcp")
        m_transport.reset(new woogeen_base::RawTransport<TCP>(this));
    else
        m_transport.reset(new woogeen_base::RawTransport<UDP>(this));

    m_transport->listenTo(0);
}

InternalIn::~InternalIn()
{
    m_transport->close();
}

unsigned int InternalIn::getListeningPort()
{
    return m_transport->getListeningPort();
}

void InternalIn::onFeedback(const FeedbackMsg& msg)
{
    char sendBuffer[512];
    sendBuffer[0] = TDT_FEEDBACK_MSG;
    memcpy(&sendBuffer[1], reinterpret_cast<char*>(const_cast<FeedbackMsg*>(&msg)), sizeof(FeedbackMsg));
    m_transport->sendData((char *)sendBuffer, sizeof(FeedbackMsg) + 1);
}

void InternalIn::onTransportData(char* buf, int len)
{
    Frame* frame = nullptr;
    switch (buf[0]) {
        case TDT_MEDIA_FRAME:
            frame = reinterpret_cast<Frame*>(buf + 1);
            frame->payload = reinterpret_cast<uint8_t*>(buf + 1 + sizeof(Frame));
            deliverFrame(*frame);
            break;
        default:
            break;
    }
}

} /* namespace woogeen_base */

