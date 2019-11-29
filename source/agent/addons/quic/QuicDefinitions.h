/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Most classes in this file and its implementations are borrowed from Chromium/src/third_party/blink/renderer/modules/peerconnection/adapters/* with modifications.

#ifndef WEBRTC_QUICDEFINITIONS_H_
#define WEBRTC_QUICDEFINITIONS_H_

#include <memory>
#include <string>
#include <vector>

enum class QuicTransportState : uint8_t {
    New,
    Connecting,
    Connected,
    Closed,
    Failed
};

// https://w3c.github.io/webrtc-pc/#dom-rtcdtlsfingerprint
struct RTCDtlsFingerprint {
    std::string algorithm;
    std::string value;
};

// https://w3c.github.io/webrtc-quic/#dom-rtcquicparameters
struct RTCQuicParameters {
    std::string role;
    std::vector<RTCDtlsFingerprint> fingerprints;
};

#endif