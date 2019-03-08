// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkFrameDecoder_h
#define MsdkFrameDecoder_h

#ifdef ENABLE_MSDK

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <logger.h>

#include "MediaFramePipeline.h"

#include "MsdkFrame.h"
#include "MsdkBase.h"

namespace owt_base {

class MsdkFrameDecoder : public VideoFrameDecoder {
    DECLARE_LOGGER();

public:
    MsdkFrameDecoder();
    ~MsdkFrameDecoder();

    static bool supportFormat(FrameFormat format);

    void onFrame(const Frame&);
    bool init(FrameFormat);

protected:
    void initDefaultParam(void);

    bool allocateFrames(void);

    void updateBitstream(const Frame& frame);

    bool decHeader(mfxBitstream *pBitstream);
    void decFrame(mfxBitstream *pBitstream);

    void flushOutput(void);
    bool resetDecoder(void);

    void dump(uint8_t *buf, int len);

private:
    MFXVideoSession *m_session;
    MFXVideoDECODE *m_dec;

    std::deque<uint32_t> m_timeStamps;

    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    boost::scoped_ptr<mfxVideoParam> m_videoParam;
    boost::scoped_ptr<mfxBitstream> m_bitstream;

    uint32_t m_decBsOffset;

    boost::shared_ptr<MsdkFramePool> m_framePool;

    uint8_t m_statDetectHeaderFrameCount;

    bool m_ready;

    mfxPluginUID m_pluginID;

    bool m_enableBsDump;
    FILE *m_bsDumpfp;
};

} /* namespace owt_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkFrameDecoder_h */

