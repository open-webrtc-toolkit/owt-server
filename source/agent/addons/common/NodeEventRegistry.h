// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NODEEVENTREGISTRY_H
#define NODEEVENTREGISTRY_H

#include <EventRegistry.h>
#include <memory>
#include <mutex>
#include <node.h>
#include <node_object_wrap.h>
#include <queue>
#include <string>
#include <uv.h>

// Implement ::EventRegistry interface
class NodeEventRegistry : public ::EventRegistry {
public:
    static NodeEventRegistry* New(v8::Isolate*, const v8::Local<v8::Function>&);
    static NodeEventRegistry* New(const v8::Local<v8::Function>&);

    virtual ~NodeEventRegistry();
    bool notifyAsyncEvent(const std::string& event, const std::string& data);
    bool notifyAsyncEventInEmergency(const std::string& event, const std::string& data);

protected:
    explicit NodeEventRegistry();
    explicit NodeEventRegistry(v8::Isolate*, const v8::Local<v8::Function>&);

    struct Data {
        std::string event, message;
    };
    v8::Persistent<v8::Object> m_store;

private:
    uv_async_t* m_uvHandle;
    std::mutex m_lock;
    std::deque<Data> m_buffer;
    void process();
    void process(const Data& data);
    static void closeCallback(uv_handle_t*);
    static void callback(uv_async_t*);
};

class NodeEventedObjectWrap : public node::ObjectWrap, public NodeEventRegistry {
public:
    inline static void SETUP_EVENTED_PROTOTYPE_METHODS(v8::Local<v8::FunctionTemplate> tmpl)
    {
        NODE_SET_PROTOTYPE_METHOD(tmpl, "addEventListener", addEventListener);
    }

protected:
    explicit NodeEventedObjectWrap();
    virtual ~NodeEventedObjectWrap();
    static void addEventListener(const v8::FunctionCallbackInfo<v8::Value>& args);
};

#endif
