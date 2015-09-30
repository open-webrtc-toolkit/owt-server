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
#include <node.h>
#include <string>

// Implement woogeen_base::EventRegistry interface
class NodeEventRegistry: public woogeen_base::EventRegistry
{
public:
  NodeEventRegistry(const v8::Local<v8::Function>& f) {
    m_func.Reset(v8::Isolate::GetCurrent(), f);
  };
  ~NodeEventRegistry() {};
  void process(const std::string& data)
  {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = {
      v8::String::NewFromUtf8(isolate, data.c_str())
    };
    v8::TryCatch try_catch;
    v8::Local<v8::Function>::New(isolate, m_func)->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
      node::FatalException(isolate, try_catch);
    }
  };
private:
  v8::Persistent<v8::Function> m_func;
};

#endif
