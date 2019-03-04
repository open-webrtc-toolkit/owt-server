// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "AudioMixerWrapper.h"
#include <node.h>

using namespace v8;

NODE_MODULE(addon, AudioMixer::Init)
