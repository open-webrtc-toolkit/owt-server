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

#ifndef AudioFrameWriter_h
#define AudioFrameWriter_h

#include <string>

#include <logger.h>

#include <webrtc/modules/include/module_common_types.h>

#include "MediaFramePipeline.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace woogeen_base {

class AudioFrameWriter {
    DECLARE_LOGGER();

public:
    AudioFrameWriter(const std::string& name);
    ~AudioFrameWriter();

    void write(const Frame& frame);
    void write(const webrtc::AudioFrame *audioFrame);

protected:
    FILE *getAudioFp(const webrtc::AudioFrame *audioFrame);

    void writeCompressedFrame(const Frame& frame);
    bool addAudioStream(FrameFormat format, uint32_t sampleRate, uint32_t channels);

private:
    std::string m_name;

    FILE *m_fp;
    uint32_t m_index;

    int32_t m_sampleRate;
    int32_t m_channels;

    AVFormatContext *m_context;
    AVStream *m_audioStream;
};

}

#endif // AudioFrameWriter_h
