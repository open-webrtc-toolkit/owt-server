/*
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "RTCQuicTransportBase.h"

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Value;

Nan::Persistent<Function> RTCQuicTransportBase::s_constructor;

DEFINE_LOGGER(RTCQuicTransportBase, "RTCQuicTransportBase");