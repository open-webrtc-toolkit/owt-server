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

#ifndef MsdkFrameDecoder_h
#define MsdkFrameDecoder_h

#ifdef ENABLE_MSDK

#include "MediaFramePipeline.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>
#include <webrtc/modules/video_coding/codecs/interface/video_codec_interface.h>
#include <webrtc/video_decoder.h>

#include "mfxvideo++.h"

namespace woogeen_base {

class MsdkFrameDecoder : public VideoFrameDecoder {
    DECLARE_LOGGER();

public:
    MsdkFrameDecoder();
    ~MsdkFrameDecoder();

    static bool supportFormat(FrameFormat format) {return (format == FRAME_FORMAT_H264);}

    void onFrame(const Frame&);
    bool init(FrameFormat);

protected:
    void initDefaultParam(void);

    void updateBitstream(const Frame& frame);

    bool decHeader(mfxBitstream *pBitstream);
    void decFrame(mfxBitstream *pBitstream);


private:
    MFXVideoSession *m_session;
    MFXVideoDECODE *m_dec;

    boost::shared_ptr<mfxFrameAllocator> m_allocator;

    boost::scoped_ptr<mfxVideoParam> m_videoParam;
    boost::scoped_ptr<mfxBitstream> m_bitstream;

    boost::scoped_ptr<MsdkFramePool> m_framePool;

    bool m_ready;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkFrameDecoder_h */

