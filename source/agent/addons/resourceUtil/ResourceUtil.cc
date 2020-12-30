// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "ResourceUtil.h"
#include <iostream>
#include <string>
#include <vector>

using namespace InferenceEngine;

DEFINE_LOGGER(ResourceUtil, "mcu.media.ResourceUtil");

ResourceUtil::ResourceUtil() {
}

ResourceUtil::~ResourceUtil() {
}

float ResourceUtil::getVPUUtil() { 

    std::vector<float> v = ie.GetMetric("HDDL", VPU_HDDL_METRIC(DEVICE_UTILIZATION));
    float sum=0, avg=0;
    for(float n : v) {
            sum +=n;
    }
    avg = sum/v.size();

    return avg;
}

