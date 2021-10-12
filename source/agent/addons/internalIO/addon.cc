// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "InternalServerWrapper.h"
#include "InternalClientWrapper.h"
#include "InternalConfig.h"

#include <node.h>

using namespace v8;

void InitAll(Local<Object> exports) {
  InternalServer::Init(exports);
  InternalClient::Init(exports);
  InitInternalConfig(exports);
}

NODE_MODULE(addon, InitAll)
