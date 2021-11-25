// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "QuicTransportServer.h"
#include "QuicTransportClient.h"
#include <node.h>

using namespace v8;

NAN_MODULE_INIT(InitAll)
{
  QuicTransportServer::init(target);
  QuicTransportClient::init(target);
}

NODE_MODULE(addon, InitAll)
