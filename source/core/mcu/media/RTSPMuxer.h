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

#ifndef RTSPMuxer_h
#define RTSPMuxer_h

#include "MediaMuxing.h"
#include "MediaUtilities.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

namespace mcu {

class RTSPMuxer : public woogeen_base::MediaMuxer {
    DECLARE_LOGGER();
public:
    RTSPMuxer(const std::string&, woogeen_base::MediaMuxing*, woogeen_base::MediaMuxing*);
    virtual ~RTSPMuxer();
    // MediaMuxer interface
    bool start();
    void stop();

private:
    woogeen_base::MediaMuxing* m_videoSink;
    woogeen_base::MediaMuxing* m_audioSink;
    AVFormatContext* m_context;
    AVStream* m_videoStream;
    AVStream* m_audioStream;
    int32_t m_videoId, m_audioId;
    std::string m_uri;
    void addVideoStream(enum AVCodecID codec_id);
    void addAudioStream(enum AVCodecID codec_id);
    int writeVideoFrame(uint8_t*, size_t, int);
    int writeAudioFrame(uint8_t*, size_t, int);
    AVFrame* allocAudioFrame();
    void loop();
};

}

#endif // RTSPMuxer_h
