// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "NodeEventRegistry.h"
#include <list>

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
    std::lock_guard<std::mutex> lock(m_lock);
    m_store.Reset();
    if (m_uvHandle && !uv_is_closing(reinterpret_cast<uv_handle_t*>(m_uvHandle)))
        uv_close(reinterpret_cast<uv_handle_t*>(m_uvHandle), closeCallback);
    if (m_uvHandle)
        m_uvHandle->data = nullptr;
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
    TryCatch try_catch(isolate);

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

void NodeEventRegistry::process()
{
    std::list<Data> datas;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        while (true) {
            Data data;
            {
                if (!m_buffer.empty()) {
                    data = m_buffer.front();
                    m_buffer.pop_front();
                } else {
                    break;
                }
            }
            datas.push_back(data);
        }
    }
    for (std::list<Data>::iterator it = datas.begin(); it != datas.end(); ++it) {
        process(*it);
    }
}

// other thread
bool NodeEventRegistry::notifyAsyncEvent(const std::string& event, const std::string& data)
{
    if (m_uvHandle && uv_is_active(reinterpret_cast<uv_handle_t*>(m_uvHandle))) {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_buffer.push_back(Data{ event, data });
        }
        uv_async_send(m_uvHandle);
        return true;
    }
    return false;
}

// other thread
bool NodeEventRegistry::notifyAsyncEventInEmergency(const std::string& event, const std::string& data)
{
    if (m_uvHandle && uv_is_active(reinterpret_cast<uv_handle_t*>(m_uvHandle))) {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_buffer.push_front(Data{ event, data });
        }
        uv_async_send(m_uvHandle);
        return true;
    }
    return false;
}

void NodeEventRegistry::closeCallback(uv_handle_t* handle)
{
    free(handle);
    handle = nullptr;
}

void NodeEventRegistry::callback(uv_async_t* handle)
{
    if (handle && uv_is_active(reinterpret_cast<uv_handle_t*>(handle)) && handle->data) {
        NodeEventRegistry* registry = reinterpret_cast<NodeEventRegistry*>(handle->data);
        if (registry)
            registry->process();
    }
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
