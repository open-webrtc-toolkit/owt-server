// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "VideoSwitchWrapper.h"

#include <node.h>

using namespace v8;

void InitAll(Local<Object> exports) {
  VideoSwitch::Init(exports);
}

NODE_MODULE(addon, InitAll)
