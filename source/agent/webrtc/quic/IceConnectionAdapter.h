/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WEBRTC_ICECONNECTIONADAPTER_H_
#define WEBRTC_ICECONNECTIONADAPTER_H_

#include "rtc_base/third_party/sigslot/sigslot.h"
#include <IceConnection.h>

// This interface is not complete.
class IceConnectionInterface {
    virtual int sendPacket(const char* data, int length) = 0;
};

// IceConnectionAdapter could also be a class inherited from IceConnection.
// However, it will lose licode's benifits for supporting both libnice and
// nIcer.
class IceConnectionAdapter : public erizo::IceConnectionListener,
                             public IceConnectionInterface {
    DECLARE_LOGGER();

public:
    explicit IceConnectionAdapter(erizo::IceConnection* iceConnection);
    virtual int sendPacket(const char* data, int length) override;

    // Sigslots.
    sigslot::signal5<IceConnectionAdapter*, const char*, size_t, const int64_t, int> signalReadPacket;
    sigslot::signal1<IceConnectionAdapter*> signalReadyToSend;

protected:
    virtual void onPacketReceived(erizo::packetPtr packet);
    virtual void onCandidate(const erizo::CandidateInfo& candidate, erizo::IceConnection* conn);
    virtual void updateIceState(erizo::IceState state, erizo::IceConnection* conn);

private:
    erizo::IceConnection* m_iceConnection;
    erizo::IceConnectionListener* m_originListener;
};

#endif