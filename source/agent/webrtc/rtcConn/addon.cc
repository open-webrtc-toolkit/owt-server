// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "WebRtcConnection.h"
#include "ThreadPool.h"
#include "IOThreadPool.h"
#include "MediaStream.h"

#include <dtls/DtlsSocket.h>

#include <node.h>

using namespace v8;

void InitAll(Handle<Object> exports) {
  dtls::DtlsSocketContext::Init();
  WebRtcConnection::Init(exports);
  MediaStream::Init(exports);
  ThreadPool::Init(exports);
  IOThreadPool::Init(exports);
}

NODE_MODULE(addon, InitAll)
