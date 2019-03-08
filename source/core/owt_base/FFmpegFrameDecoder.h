// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef FFmpegFrameDecoder_h
#define FFmpegFrameDecoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <logger.h>

#include "MediaFramePipeline.h"
#include "I420BufferManager.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace owt_base {

class FFmpegFrameDecoder : public VideoFrameDecoder {
    DECLARE_LOGGER();

public:
    FFmpegFrameDecoder();
    ~FFmpegFrameDecoder();

    static bool supportFormat(FrameFormat format) {return true;}

    void onFrame(const Frame&);
    bool init(FrameFormat);

protected:
    static int AVGetBuffer(AVCodecContext *s, AVFrame *frame, int flags);
    static void AVFreeBuffer(void* opaque, uint8_t* data);

private:
    AVCodecContext *m_decCtx;
    AVFrame *m_decFrame;

    AVPacket m_packet;

    boost::scoped_ptr<owt_base::I420BufferManager> m_bufferManager;

    char m_errbuff[500];
    char *ff_err2str(int errRet);
};

} /* namespace owt_base */

#endif /* FFmpegFrameDecoder_h */

