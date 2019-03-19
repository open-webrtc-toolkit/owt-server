/*
 * Copyright 2017 Intel Corporation All Rights Reserved. 
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

#include <iostream>
#include <string.h>
#include "myplugin.h"
#include <unistd.h>
#include <stdlib.h>
MyPlugin::MyPlugin()
: frame_callback(nullptr)
, event_callback(nullptr) {}

rvaStatus MyPlugin::PluginInit(std::unordered_map<std::string, std::string> params) {
        std::cout << "In my plugin init." << std::endl;
        return RVA_ERR_OK;
}

rvaStatus MyPlugin::PluginClose() {
    return RVA_ERR_OK;
}

rvaStatus MyPlugin::ProcessFrameAsync(std::unique_ptr<owt::analytics::AnalyticsBuffer> buffer) {
    // fetch data from the frame and update it by clearing out some blocks of the video.
    // after update, send it back to analytics server.:
    // woogeen::analytics::VideoFrame newFrame(frame);
    //   woogeen::analytics::scoped_refptr<woogeen::analytics::I420BufferInterface> buffer = 
    //       newFrame.video_frame_buffer()->ToI420();
    //    memset(buffer->DataU(), 0, buffer->StrideU()/2);
    if (!buffer->buffer) {
        return RVA_ERR_OK;
    }
    if (buffer->width >=320 && buffer->height >=240) {
        //unsigned int time_sleep=(rand() % 100)*1000;
        //usleep(time_sleep) ;
//        std::cout<<"time sleep"<<time_sleep<<std::endl;
        memset(buffer->buffer + buffer->width * buffer->height, 28, buffer->width * buffer->height/16);
    }
    if (frame_callback) {
        frame_callback->OnPluginFrame(std::move(buffer));
    }
    return RVA_ERR_OK;
} 

// Declare the plugin 
DECLARE_PLUGIN(MyPlugin)

