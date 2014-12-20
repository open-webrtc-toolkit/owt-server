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

#include "VideoLayout.h"

#include <list>
#include <logger.h>
#include <stdint.h>
#include <vector>

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

    const VideoLayout& getVideoLayout(uint32_t id);
    void initVideoLayout(uint32_t id, const std::string&, const std::string&, const std::string&, const std::string&);
    bool updateVideoLayout(uint32_t id, uint32_t slots);
    uint32_t registerListener(ConfigListener*);
    void unregisterListener(uint32_t id);

private:
    Config();
    void signalConfigChanged(uint32_t id);
    bool validateVideoRegion(const Region&);

    static Config* m_config;
    VideoLayout m_customVideoLayouts[MAX_VIDEO_SLOT_NUMBER];
    uint32_t m_idAccumulator;
    std::map<uint32_t, VideoLayout> m_currentVideoLayouts;
    std::map<uint32_t, ConfigListener*> m_configListeners;
};

} /* namespace mcu */
#endif /* Config_h */
