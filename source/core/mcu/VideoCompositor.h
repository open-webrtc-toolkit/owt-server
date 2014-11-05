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

#ifndef VideoCompositor_h
#define VideoCompositor_h

#include "Config.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <logger.h>

namespace webrtc {
class I420VideoFrame;
class VideoProcessingModule;
class CriticalSectionWrapper;
}

namespace mcu {

class VPMPool;
class BufferManager;

/**
 * the configuration is a subset of rfc5707, VideoLayout element definition
 *    An example of a video layout with six regions is:

      +-------+---+
      |       | 2 |
      |   1   +---+
      |       | 3 |
      +---+---+---+
      | 6 | 5 | 4 |
      +---+---+---+

      <videolayout type="text/msml-basic-layout">
         <root size="CIF"/>
         <region id="1" left="0" top="0" relativesize="2/3"/>
         <region id="2" left="67%" top="0" relativesize="1/3"/>
         <region id="3" left="67%" top="33%" relativesize="1/3">
         <region id="4" left="67%" top="67%" relativesize="1/3"/>
         <region id="5" left="33%" top="67%" relativesize="1/3"/>
         <region id="6" left="0" top="67%" relativesize="1/3"/>
      </videolayout>
 */
enum VideoResolutionType
{
    cif = 0,//352x288
    vga,
    hd_720p,
    sif, //320x240
    hvga, //480x320
    r480x360,
    qcif, //176x144
    r192x144,
    hd_1080p,
    uhd_4k,
    total = uhd_4k,
};
struct VideoSize {
    int width;
    int height;
};

extern VideoSize VideoSizes[];
extern const char* VideoResString[];

struct Region {
    std::string id;
    float left; // percentage
    float top;    // percentage
    float relativesize;    //fraction
    float priority;
} ;

struct VideoLayout {
    VideoResolutionType rootsize;
    std::vector<Region> regions;
    unsigned int divFactor;    //valid for fluidLayout
    unsigned int subWidth;
    unsigned int subHeight;
};

/**
 * manages a pool of VPM for preprocessing the incoming I420 frame
 */
class VPMPool {
public:
    VPMPool(unsigned int size);
    ~VPMPool();
    webrtc::VideoProcessingModule* get(unsigned int slot);
    void update(unsigned int slot, VideoSize&);
    unsigned int size() { return m_size; }

private:
    webrtc::VideoProcessingModule** m_vpms;
    unsigned int m_size;    // total pool capacity
    std::vector<VideoSize> m_subVideSize;
};

/**
 * composite a sequence of frames into one frame based on current layout config,
 * there is a question of how many streams to be composited if there are 16 participants
 * but only 6 regions are configed. current implementation will only show 6 participants but
 * still 16 audios will be mixed. In the future, we may enable the video rotation based on VAD history
 * single thread only
 */
class SoftVideoCompositor {
    DECLARE_LOGGER();
public:
    SoftVideoCompositor(boost::shared_ptr<BufferManager>&);
    virtual bool config(VideoLayout&); //set a new layout config, but won't be effective
    virtual bool commitLayout(); //commit the new layout config
    virtual void setBackgroundColor();
    virtual webrtc::I420VideoFrame* layout(int maxSlot);
    void getLayout(VideoLayout& layout)
    {
        layout = m_currentLayout;
    }
protected:
    virtual webrtc::I420VideoFrame* fluidLayout(int maxSlot);
    virtual webrtc::I420VideoFrame* customLayout();
private:

    virtual bool validateConfig(VideoLayout& layout)
    {
    	return true;
    }

protected:
    boost::scoped_ptr<webrtc::CriticalSectionWrapper> m_configLock;
    bool m_configChanged;
    boost::scoped_ptr<VPMPool> m_vpmPool;
    boost::shared_ptr<BufferManager> m_bufferManager;
    boost::scoped_ptr<webrtc::I420VideoFrame> m_composedFrame;
    VideoLayout m_currentLayout;
    VideoLayout m_newLayout;
};

}
#endif /* VideoCompositor_h*/
