// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkFrame_h
#define MsdkFrame_h

#ifdef ENABLE_MSDK

#include <list>

#include <boost/thread/shared_mutex.hpp>

#include <webrtc/api/video/i420_buffer.h>

#include <mfxvideo++.h>

#include "logger.h"
#include "MediaFramePipeline.h"

namespace woogeen_base {

#define dumpMsdkFrameInfo(name, frame) ELOG_TRACE("%s-(%p)frame use_count %ld, locked_count %d", name, frame.get(), frame.use_count(), frame->getLockedCount())

#define MAX_DECODED_FRAME_IN_RENDERING 3

class MsdkFrame {
    DECLARE_LOGGER();

    static const uint32_t TIMEOUT = 16; //Wait for frame get free
public:
    MsdkFrame(uint32_t width, uint32_t height, boost::shared_ptr<mfxFrameAllocator> allocator);
    MsdkFrame(mfxFrameInfo &info, mfxMemId &id, boost::shared_ptr<mfxFrameAllocator> allocator);
    ~MsdkFrame();

    bool init();

    mfxFrameSurface1 *getSurface(void) {return &m_surface;}

    void setSyncPoint(mfxSyncPoint& syncP) {m_syncP = syncP;}
    void setSyncFlag(bool needSync) {m_needSync = needSync;}

    bool isFree(void) {return !m_surface.Data.Locked;}
    int getLockedCount(void) {return m_surface.Data.Locked;}

    uint32_t getWidth() {return m_surface.Info.Width;}
    uint32_t getHeight() {return m_surface.Info.Height;}

    uint32_t getVideoWidth() {return m_surface.Info.CropW;}
    uint32_t getVideoHeight() {return m_surface.Info.CropH;}

    uint32_t getCropX() {return m_surface.Info.CropX;}
    uint32_t getCropY() {return m_surface.Info.CropY;}
    uint32_t getCropW() {return m_surface.Info.CropW;}
    uint32_t getCropH() {return m_surface.Info.CropH;}

    //set frame crop to resize
    bool setCrop(uint32_t cropX, uint32_t cropY, uint32_t cropW, uint32_t cropH);

    /*
    (16,    128,    128)    //black
    (235,   128,    128)    //white
    (82,    90,     240)    //red
    (144,   54,     34)     //green
    (41,    240,    110)    //blue
    */
    bool fillFrame(uint8_t y, uint8_t u, uint8_t v);

    bool convertFrom(webrtc::VideoFrameBuffer *buffer);
    bool convertTo(webrtc::I420Buffer *buffer);

    void sync(void);

protected:
    bool nv12ConvertTo(mfxFrameInfo& pInfo, mfxFrameData& pData, webrtc::I420Buffer *buffer);

private:
    mfxFrameAllocRequest m_request;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    mfxFrameAllocResponse m_response;
    mfxFrameSurface1 m_surface;
    bool m_valid;
    bool m_externalAlloc;

    MFXVideoSession *m_mainSession;
    mfxSyncPoint m_syncP;
    bool m_needSync;

    uint8_t *m_nv12TBuffer;
    size_t m_nv12TBufferSize;
};

class MsdkFramePool {
    DECLARE_LOGGER();

public:
    MsdkFramePool(const uint32_t width, const uint32_t height, const int32_t count, boost::shared_ptr<mfxFrameAllocator> allocator);
    MsdkFramePool(mfxFrameAllocRequest &request, boost::shared_ptr<mfxFrameAllocator> allocator);

    ~MsdkFramePool();

    boost::shared_ptr<MsdkFrame> getFreeFrame();
    boost::shared_ptr<MsdkFrame> getFrame(mfxFrameSurface1 *pSurface);

    void dumpInfo();

private:
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    mfxFrameAllocResponse m_response;

    std::list<boost::shared_ptr<MsdkFrame>> m_framePool;
    boost::shared_mutex m_mutex;
};

enum MsdkCmd {
    MsdkCmd_NONE,
    MsdkCmd_DEC_FLUSH,
};

struct MsdkFrameHolder
{
    boost::shared_ptr<MsdkFrame> frame;

    MsdkCmd cmd;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkFrame_h */

