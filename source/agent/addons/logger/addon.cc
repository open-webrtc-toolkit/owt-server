// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include <nan.h>
// include log4cxx header files
#include <log4cxx/propertyconfigurator.h>

using Nan::GetFunction;
using Nan::New;
using Nan::Set;

// Configure using the "log4cxx.properties" file
NAN_METHOD(configure) {
  // expect a number as the first argument
  log4cxx::PropertyConfigurator::configure("log4cxx.properties");
}

// Expose synchronous and asynchronous access to our
// Estimate() function
NAN_MODULE_INIT(InitAll) {
  Set(target, New<v8::String>("configure").ToLocalChecked(),
    GetFunction(New<v8::FunctionTemplate>(configure)).ToLocalChecked());
}

NODE_MODULE(addon, InitAll)