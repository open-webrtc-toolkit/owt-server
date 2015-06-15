/*
 * Copyright 2014 Intel Corporation All Rights Reserved. 
 * 
 * The source code contained or described herein and all documents related to the 
 * source code ("Material") are owned by Intel Corporation or its suppliers or 
 * licensors. Title to the Material remains with Intel Corporation or its suppliers 
 * and licensors. The Material contains trade secrets and proprietary and 
 * confidential information of Intel or its suppliers and licensors. The Material 
 * is protected by worldwide copyright and trade secret laws and treaty provisions. 
 * No part of the Material may be used, copied, reproduced, modified, published, 
 * uploaded, posted, transmitted, distributed, or disclosed in any way without 
 * Intel's prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual 
 * property right is granted to or conferred upon you by disclosure or delivery of 
 * the Materials, either expressly, by implication, inducement, estoppel or 
 * otherwise. Any license under such intellectual property rights must be express 
 * and approved by Intel in writing.
 */

#include "ExternalInputGateway.h"
#include "InProcessMixer.h"
#include "OutOfProcessMixer.h"
#include "OutOfProcessMixerProxy.h"
#include "WebRTCGateway.h"
 
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

woogeen_base::Gateway* woogeen_base::Gateway::createGatewayInstance(const std::string& customParams)
{
    // FIXME: Figure out a more reliable approach to validating the input parameter.
    if (customParams != "" && customParams != "undefined") {
        boost::property_tree::ptree pt;
        std::istringstream is(customParams);
        boost::property_tree::read_json(is, pt);
        bool isMixer = pt.get<bool>("mixer", false);
 
        if (isMixer) {
            bool outOfProcess = pt.get<bool>("oop", false);
            if (outOfProcess) {
                bool isProxy = pt.get<bool>("proxy", false);
                if (!isProxy)
                    return new mcu::OutOfProcessMixer(pt.get_child("video"));

                return new mcu::OutOfProcessMixerProxy();
            }

            return new mcu::InProcessMixer(pt.get_child("video"));
        }

        bool externalInput = pt.get<bool>("externalInput", false);
        if (externalInput)
            return new mcu::ExternalInputGateway();
    }

    return new mcu::WebRTCGateway();
}
