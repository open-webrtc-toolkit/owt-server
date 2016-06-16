/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
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

#ifndef MsdkFrame_h
#define MsdkFrame_h

#ifdef ENABLE_MSDK

#include <boost/thread/shared_mutex.hpp>
#include <list>
#include <logger.h>

#include <webrtc/video_frame.h>

#include "MediaFramePipeline.h"

#include <mfxvideo++.h>

namespace woogeen_base {

#define MAX_DECODED_FRAME_IN_RENDERING 3

class MsdkFrame {
    DECLARE_LOGGER();

    friend class MsdkFramePool;

public:
    ~MsdkFrame();

    mfxFrameSurface1 *getSurface(void) {return &m_surface;}

    void setSyncPoint(mfxSyncPoint& syncP) {m_syncP = syncP;}
    void setSyncFlag(bool needSync) {m_needSync = needSync;}

    bool isFree(void) {return !m_surface.Data.Locked;}

    uint32_t getWidth() {return m_surface.Info.CropW;}
    uint32_t getHeight() {return m_surface.Info.CropH;}

    //set frame crop to resize
    bool reSize(uint32_t width, uint32_t height);

    bool fillFrame(uint8_t y, uint8_t u, uint8_t v);

    bool convertFrom(webrtc::I420VideoFrame& frame);
    bool convertTo(webrtc::I420VideoFrame& frame);

protected:
    MsdkFrame(boost::shared_ptr<mfxFrameAllocator> allocator, mfxFrameInfo &info, mfxMemId id);
    void sync(void);
    void *memcpy_f( void *, void *, size_t );

    bool nv12ConvertTo(mfxFrameInfo& pInfo, mfxFrameData& pData, webrtc::I420VideoFrame& frame);

    void dumpI420VideoFrameInfo(webrtc::I420VideoFrame& frame);

private:
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    MFXVideoSession *m_mainSession;

    mfxFrameSurface1 m_surface;

    mfxSyncPoint m_syncP;

    bool m_needSync;

    uint8_t *m_nv12TBuffer;
    size_t m_nv12TBufferSize;
};

class MsdkFramePool {
    DECLARE_LOGGER();

    static const uint32_t TIMEOUT = 16; //Wait for frame get free

public:
    MsdkFramePool(boost::shared_ptr<mfxFrameAllocator> allocator, mfxFrameAllocRequest &request);
    ~MsdkFramePool();

    bool init();

    uint32_t getAllocatedWidth() {return m_allocatedWidth;}
    uint32_t getAllocatedHeight() {return m_allocatedWidth;}

    bool reAllocate(uint32_t width, uint32_t height);

    boost::shared_ptr<MsdkFrame> getFreeFrame();
    boost::shared_ptr<MsdkFrame> getFrame(mfxFrameSurface1 *pSurface);

protected:
    bool waitForFrameFree(boost::shared_ptr<MsdkFrame>& frame, int timeout);
    bool allocateFrames();
    bool freeFrames();

private:
    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    mfxFrameAllocRequest m_request;
    mfxFrameAllocResponse m_response;

    uint32_t m_allocatedWidth;
    uint32_t m_allocatedHeight;

    std::list<boost::shared_ptr<MsdkFrame>> m_framePool;
};

struct MsdkFrameHolder
{
    boost::shared_ptr<MsdkFrame> frame;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkFrame_h */

