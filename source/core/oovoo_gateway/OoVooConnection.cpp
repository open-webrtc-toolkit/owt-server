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

#include "OoVooConnection.h"

using namespace woogeen_base;

namespace oovoo_gateway {

DEFINE_LOGGER(OoVooConnection, "ooVoo.OoVooConnection");

OoVooConnection::OoVooConnection(boost::shared_ptr<OoVooProtocolStack> ooVoo)
{
    ELOG_WARN("OoVooConnection constructor");
    m_tcpListener.reset(new AVSTransportListener<TCP>(ooVoo));
    m_tcpTransport.reset(new RawTransport<TCP>(m_tcpListener.get(), false));
    m_udpListener.reset(new AVSTransportListener<UDP>(ooVoo));
    m_udpTransport.reset(new RawTransport<UDP>(m_udpListener.get(), false));
}

OoVooConnection::~OoVooConnection()
{
    ELOG_DEBUG("OoVooConnection Destructor");
}

void OoVooConnection::connect(const std::string& ip, uint32_t port, bool isUdp)
{
    if (isUdp)
        m_udpTransport->createConnection(ip, port);
    else
        m_tcpTransport->createConnection(ip, port);
}

int OoVooConnection::sendData(const char* buf, int len, bool isUdp)
{
    if (isUdp) {
        ELOG_TRACE("Sending UDP message to AVS, length %d", len);
        m_udpTransport->sendData(buf, len);
    } else {
        ELOG_DEBUG("Sending TCP message to AVS, length %d", len);
        m_tcpTransport->sendData(buf, len);
    }
    return len;
}

void OoVooConnection::close()
{
    m_udpTransport->close();
    m_tcpTransport->close();
}

DEFINE_TEMPLATE_LOGGER(template<Protocol prot>, AVSTransportListener<prot>, "ooVoo.OoVooConnection");

template<Protocol prot>
void AVSTransportListener<prot>::onTransportData(char* buf, int len)
{
    // This means we received data from AVS.
    switch (prot) {
    case UDP:
        ELOG_TRACE("Receiving UDP message from AVS, length %d", len);
        break;
    case TCP:
        ELOG_DEBUG("Receiving TCP message from AVS, length %d", len);
        break;
    default:
        break;
    }
    m_ooVoo->dataAvailable(buf, len, prot == UDP);
}

template<Protocol prot>
void AVSTransportListener<prot>::onTransportError()
{
    ELOG_WARN("Error with %s connection to AVS", prot == UDP ? "UDP" : "TCP");
    m_ooVoo->avsConnectionFail(prot == UDP);
}

template<Protocol prot>
void AVSTransportListener<prot>::onTransportConnected()
{
    ELOG_DEBUG("%s connection established with AVS", prot == UDP ? "UDP" : "TCP");
    m_ooVoo->avsConnectionSuccess(prot == UDP);
}

}
/* namespace oovoo_gateway */
