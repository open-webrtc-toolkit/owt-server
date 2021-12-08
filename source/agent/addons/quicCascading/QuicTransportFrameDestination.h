/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef QUIC_ADDON_QUIC_TRANSPORT_FRAME_DESTINATION_H_
#define QUIC_ADDON_QUIC_TRANSPORT_FRAME_DESTINATION_H_

#include "../../core/owt_base/MediaFramePipeline.h"
#include "../common/MediaFramePipelineWrapper.h"
#include "owt/quic/web_transport_stream_interface.h"
#include <logger.h>
#include <nan.h>

// A QuicTransportFrameDestination is a hub for a single InternalIO input to multiple QuicTransport outputs.
class QuicTransportFrameDestination : public Nan::ObjectWrap {
    DECLARE_LOGGER();

public:
    explicit QuicTransportFrameDestination();
    ~QuicTransportFrameDestination();

    static NAN_MODULE_INIT(init);

private:
    static Nan::Persistent<v8::Function> s_constructor;
    static NAN_METHOD(newInstance);
};

#endif