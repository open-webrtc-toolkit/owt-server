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

#ifndef FfDecoder_h
#define FfDecoder_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <webrtc/modules/audio_coding/include/audio_coding_module.h>

#include <logger.h>

#include "MediaFramePipeline.h"
#include "AudioDecoder.h"

#include "AcmDecoder.h"
#include "AcmEncoder.h"
#include "FfEncoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace mcu {
using namespace woogeen_base;
using namespace webrtc;

class FfDecoder : public AudioDecoder, public woogeen_base::FrameSource {
    DECLARE_LOGGER();

public:
    FfDecoder(const FrameFormat format);
    ~FfDecoder();

    bool init() override;
    bool getAudioFrame(AudioFrame *audioFrame) override;

    // Implements woogeen_base::FrameDestination
    void onFrame(const Frame& frame) override;

protected:
    bool initDecoder(FrameFormat format,uint32_t sampleRate, uint32_t channels);
    bool initResampler(enum AVSampleFormat inFormat, int inSampleRate, int inChannels,
        enum AVSampleFormat outSampleFormat, int outSampleRate, int outChannels);
    bool initFifo(enum AVSampleFormat sampleFmt, uint32_t sampleRate, uint32_t channels);

    bool resampleFrame(AVFrame *frame, uint8_t **pOutData, int *pOutNbSamples);
    bool addFrameToFifo(AVFrame *frame);

private:
    FrameFormat m_format;
    boost::shared_mutex m_mutex;

    bool m_valid;

    AVCodecContext *m_decCtx;
    AVFrame *m_decFrame;
    AVPacket m_packet;

    bool m_needResample;
    struct SwrContext *m_swrCtx;
    uint8_t **m_swrSamplesData;
    int m_swrSamplesLinesize;
    int m_swrSamplesCount;
    bool m_swrInitialised;

    AVAudioFifo* m_audioFifo;
    AVFrame* m_audioFrame;

    enum AVSampleFormat m_inSampleFormat;
    int m_inSampleRate;
    int m_inChannels;

    enum AVSampleFormat m_outSampleFormat;
    int m_outSampleRate;
    int m_outChannels;

    int64_t m_timestamp;
    FrameFormat m_outFormat;

    boost::shared_ptr<AudioEncoder> m_output;
    boost::shared_ptr<AudioDecoder> m_input;

    char m_errbuff[500];
    char *ff_err2str(int errRet);
};

} /* namespace mcu */

#endif /* FfDecoder_h */
