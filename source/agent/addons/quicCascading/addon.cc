// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportWrap.h"
#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  QuicTransportIn::Init(exports);
  QuicTransportOut::Init(exports);
}

NODE_MODULE(addon, InitAll)
