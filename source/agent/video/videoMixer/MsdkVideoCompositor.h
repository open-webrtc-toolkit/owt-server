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

#ifndef MsdkVideoCompositor_h
#define MsdkVideoCompositor_h

#ifdef ENABLE_MSDK

#include "VideoFrameMixer.h"
#include "VideoLayout.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <JobTimer.h>
#include <logger.h>
#include <MediaFramePipeline.h>
#include <vector>
#include <set>

#include "mfxvideo++.h"
#include "MsdkBase.h"

namespace mcu {
class VppInput;

/**
 * composite a sequence of frames into one frame based on current layout config,
 * there is a question of how many streams to be composited if there are 16 participants
 * but only 6 regions are configed. current implementation will only show 6 participants but
 * still 16 audios will be mixed. In the future, we may enable the video rotation based on VAD history.
 */
class MsdkVideoCompositor : public VideoFrameCompositor,
                            public woogeen_base::JobTimerListener {
    DECLARE_LOGGER();
    enum LayoutSolutionState{UN_INITIALIZED = 0, CHANGING, IN_WORK};

public:
    MsdkVideoCompositor(uint32_t maxInput, VideoSize rootSize, YUVColor bgColor, bool crop);
    ~MsdkVideoCompositor();

    bool activateInput(int input);
    void deActivateInput(int input);
    void pushInput(int input, const woogeen_base::Frame& frame);
    void updateRootSize(VideoSize& rootSize);
    void updateBackgroundColor(YUVColor& bgColor);
    void updateLayoutSolution(LayoutSolution& solution);

    void onTimeout();

protected:
    void initDefaultParam(void);
    void updateParam(void);

    void init(void);

    void fillFrame(mfxFrameSurface1 *pSurface, uint8_t y, uint8_t u, uint8_t v);

    bool isSolutionChanged ();

    void generateFrame();
    bool commitLayout(); // Commit the new layout config..clear()
    boost::shared_ptr<MsdkFrame> layout();
    boost::shared_ptr<MsdkFrame> customLayout();

private:
    uint32_t m_maxInput;
    bool m_crop;

    VideoSize m_compositeSize;
    VideoSize m_newCompositeSize;

    YUVColor m_bgColor;
    YUVColor m_newBgColor;

    LayoutSolution m_currentLayout;
    LayoutSolution m_newLayout;

    LayoutSolutionState m_solutionState;

    boost::scoped_ptr<mfxVideoParam> m_videoParam;
    boost::scoped_ptr<mfxExtVPPComposite> m_extVppComp;

    std::vector<mfxVPPCompInputStream> m_compInputStreams;

    // Delta used for translating between NTP and internal timestamps.
    int64_t m_ntpDelta;

    MFXVideoSession *m_session;
    //mfxFrameAllocator *m_allocator;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    MFXVideoVPP *m_vpp;

    std::vector<boost::shared_ptr<VppInput>> m_inputs;

    boost::scoped_ptr<MsdkFramePool> m_framePool;

    boost::shared_ptr<MsdkFrame> m_defaultInputFrame;
    boost::scoped_ptr<MsdkFramePool> m_defaultInputFramePool;

    boost::scoped_ptr<woogeen_base::JobTimer> m_jobTimer;

    boost::shared_mutex m_mutex;
};

}

#endif /* ENABLE_MSDK */
#endif /* MsdkVideoCompositor_h*/
