// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif
#include "SipGateway.h"
#include "SipCallConnection.h"

#include <node.h>

extern "C" int sipua_mod_init();

using namespace v8;

void InitAll(Handle<Object> exports) {
  SipGateway::Init(exports);
  SipCallConnection::Init(exports);
  sipua_mod_init();
}

NODE_MODULE(addon, InitAll)
