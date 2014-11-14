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
    : m_ooVoo(ooVoo)
{
    ELOG_WARN("OoVooConnection constructor");
    m_transport.reset(new RawTransport(this));
}

OoVooConnection::~OoVooConnection()
{
    ELOG_DEBUG("OoVooConnection Destructor");
}

void OoVooConnection::connect(const std::string& ip, uint32_t port, bool isUdp)
{
    assert(m_transport);
    m_transport->createConnection(ip, port, isUdp ? UDP : TCP);
}

int OoVooConnection::sendData(const char* buf, int len, bool isUdp)
{
    assert(m_transport);
    if (isUdp) {
        ELOG_TRACE("Sending UDP message to AVS, length %d", len);
    } else {
        ELOG_DEBUG("Sending TCP message to AVS, length %d", len);
    }
    m_transport->sendData(buf, len, isUdp ? UDP : TCP);
    return len;
}

void OoVooConnection::close()
{
    assert(m_transport);
    m_transport->close();
}

void OoVooConnection::onTransportData(char* buf, int len, Protocol prot)
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

void OoVooConnection::onTransportError(Protocol prot)
{
    ELOG_WARN("Error with %s connection to AVS", prot == UDP ? "UDP" : "TCP");
    m_ooVoo->avsConnectionFail(prot == UDP);
}

void OoVooConnection::onTransportConnected(Protocol prot)
{
    ELOG_DEBUG("%s connection established with AVS", prot == UDP ? "UDP" : "TCP");
    m_ooVoo->avsConnectionSuccess(prot == UDP);
}

}
/* namespace oovoo_gateway */
