// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INTERNALSERVERWRAPPER_H
#define INTERNALSERVERWRAPPER_H

#include "../../addons/common/MediaFramePipelineWrapper.h"
#include <InternalServer.h>
#include <logger.h>
#include <nan.h>

/*
 * Wrapper class of owt_base::InternalServer
 */
class InternalServer : public node::ObjectWrap,
                       public owt_base::InternalServer::Listener {
public:
    DECLARE_LOGGER();
    static NAN_MODULE_INIT(Init);

    owt_base::InternalServer* me;
    boost::mutex mutex;
    std::queue<std::pair<std::string, std::string>> stats_messages;

    // Implements owt_base::InternalServer::Listener
    void onConnected(const std::string& id) override;
    void onDisconnected(const std::string& id) override;

private:
    InternalServer();
    ~InternalServer();

    Nan::Callback *stats_callback_;
    uv_async_t *async_stats_;

    static Nan::Persistent<v8::Function> constructor;

    static NAN_METHOD(New);

    static NAN_METHOD(close);

    static NAN_METHOD(getListeningPort);

    static NAN_METHOD(addSource);

    static NAN_METHOD(removeSource);

    static NAUV_WORK_CB(statsCallback);
};

#endif
