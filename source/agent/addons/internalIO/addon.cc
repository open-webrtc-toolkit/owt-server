// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalServerWrapper.h"
#include "InternalClientWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  InternalServer::Init(exports);
  InternalClient::Init(exports);
}

NODE_MODULE(addon, InitAll)
