// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALCLIENTWRAPPER_H
#define INTERNALCLIENTWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <InternalClient.h>
#include <logger.h>
#include <nan.h>

/*
 * Wrapper class of owt_base::InternalClient
 */
class InternalClient : public FrameSource,
                       public owt_base::InternalClient::Listener {
public:
    DECLARE_LOGGER();
    static NAN_MODULE_INIT(Init);

    owt_base::InternalClient* me;
    boost::mutex mutex;
    std::queue<std::string> stats_messages;

    // Implements owt_base::InternalClient::Listener
    void onConnected() override;
    void onDisconnected() override;

private:
    InternalClient();
    ~InternalClient();

    Nan::Callback *stats_callback_;
    uv_async_t *async_stats_;

    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(New);

    static NAN_METHOD(close);

    static NAN_METHOD(addDestination);

    static NAN_METHOD(removeDestination);

    static NAUV_WORK_CB(statsCallback);
};

#endif
