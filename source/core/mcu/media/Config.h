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

#ifndef Config_h
#define Config_h

#include <list>
#include <logger.h>
#include <stdint.h>
#include <vector>

#include "VideoLayout.h"

/**
 * A singleton class to store all the configurations for MCU
 */
namespace mcu {

class VideoLayout;

class ConfigListener {
public:
    virtual void onConfigChanged() = 0;
};

class Config
{
    DECLARE_LOGGER();
public:
    static Config* get();
    VideoLayout& getVideoLayout();

    void initVideoLayout(const std::string&, const std::string&, const std::string&, const std::string&);
    bool updateVideoLayout(uint32_t);
    void registerListener(ConfigListener*);
    void unregisterListener(ConfigListener*);

private:
    Config();
    void signalConfigChanged();

    static Config* m_config;
    VideoLayout m_currentVideoLayout;
    std::vector<VideoLayout> m_customVideoLayouts;
    std::list<ConfigListener*> m_configListeners;
};

} /* namespace mcu */
#endif /* Config_h */
