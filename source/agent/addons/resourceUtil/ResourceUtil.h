// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef ResourceUtil_H
#define ResourceUtil_H

#include <glib-object.h>
#include <string>
#include <logger.h>
#include <inference_engine.hpp>
#include <vpu/hddl_plugin_config.hpp>


using namespace InferenceEngine;

class ResourceUtil{
	DECLARE_LOGGER();
public:
    ResourceUtil();
    virtual ~ResourceUtil();
    float getVPUUtil();

private:
	Core ie;
};

#endif //ResourceUtil_H
