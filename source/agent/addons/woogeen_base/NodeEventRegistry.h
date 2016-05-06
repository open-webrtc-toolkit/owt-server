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
    bool notifyAsyncEvent(const std::string& event, const std::string& data, bool prompt = false);

protected:
    explicit NodeEventRegistry();
    explicit NodeEventRegistry(v8::Isolate*, const v8::Local<v8::Function>&);

    typedef struct {
        std::string event, message;
    } Data;
    v8::Persistent<v8::Object> m_store;

private:
    uv_async_t* m_uvHandle;
    std::mutex m_lock;
    std::deque<Data> m_buffer;
    Data m_data;
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
