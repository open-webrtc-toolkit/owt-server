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

#include "NodeEventRegistry.h"

using namespace v8;

NodeEventRegistry* NodeEventRegistry::New(Isolate* isolate, const Local<Function>& f)
{
    return (new NodeEventRegistry(isolate, f));
}

NodeEventRegistry* NodeEventRegistry::New(const Local<Function>& f)
{
    return New(Isolate::GetCurrent(), f);
}

NodeEventRegistry::NodeEventRegistry()
    : m_store{ Isolate::GetCurrent(), Object::New(Isolate::GetCurrent()) }
    , m_uvHandle{ reinterpret_cast<uv_async_t*>(malloc(sizeof(uv_async_t))) }
{
    if (m_uvHandle) {
        m_uvHandle->data = this;
        m_uvHandle->close_cb = closeCallback;
        uv_async_init(uv_default_loop(), m_uvHandle, callback);
    }
}

NodeEventRegistry::NodeEventRegistry(Isolate* isolate, const Local<Function>& f)
    : m_store{ Isolate::GetCurrent(), f }
    , m_uvHandle{ reinterpret_cast<uv_async_t*>(malloc(sizeof(uv_async_t))) }
{
    if (m_uvHandle) {
        m_uvHandle->data = this;
        m_uvHandle->close_cb = closeCallback;
        uv_async_init(uv_default_loop(), m_uvHandle, callback);
    }
}

NodeEventRegistry::~NodeEventRegistry()
{
    m_store.Reset();
    if (m_uvHandle)
        uv_close(reinterpret_cast<uv_handle_t*>(m_uvHandle), closeCallback);
}

void NodeEventRegistry::process(const Data& data)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    auto store = Local<Object>::New(isolate, m_store);
    if (store.IsEmpty())
        return;

    const unsigned argc = 1;
    Local<Value> argv[argc] = {
        String::NewFromUtf8(isolate, data.message.c_str())
    };
    TryCatch try_catch;

    if (store->IsFunction()) {
        Local<Function>::Cast(store)->Call(isolate->GetCurrentContext()->Global(), argc, argv);
        if (try_catch.HasCaught()) {
            node::FatalException(isolate, try_catch);
        }
        m_store.Reset();
        return;
    }

    auto val = store->Get(String::NewFromUtf8(isolate, data.event.c_str()));
    if (!val->IsFunction())
        return;
    Local<Function>::Cast(val)->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(isolate, try_catch);
    }
}

// main thread
void NodeEventRegistry::process()
{
    while (!m_buffer.empty()) {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_data = m_buffer.front();
            m_buffer.pop();
        }
        process(m_data);
    }
}

// other thread
bool NodeEventRegistry::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    if (m_uvHandle && uv_is_active(reinterpret_cast<uv_handle_t*>(m_uvHandle))) {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_buffer.push(Data{ event, data });
        }
        uv_async_send(m_uvHandle);
        return true;
    }
    return false;
}

void NodeEventRegistry::closeCallback(uv_handle_t* handle)
{
    free(handle);
}

void NodeEventRegistry::callback(uv_async_t* handle)
{
    NodeEventRegistry* registry = reinterpret_cast<NodeEventRegistry*>(handle->data);
    registry->process();
}

NodeEventedObjectWrap::NodeEventedObjectWrap()
    : NodeEventRegistry{}
{
}

NodeEventedObjectWrap::~NodeEventedObjectWrap()
{
}

void NodeEventedObjectWrap::addEventListener(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsFunction()) {
        isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, "Wrong arguments")));
        return;
    }
    NodeEventedObjectWrap* n = ObjectWrap::Unwrap<NodeEventedObjectWrap>(args.Holder());
    Local<Object>::New(isolate, n->m_store)->Set(args[0], args[1]);
}
