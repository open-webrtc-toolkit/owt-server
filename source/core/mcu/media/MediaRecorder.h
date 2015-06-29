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

#ifndef MediaRecorder_h
#define MediaRecorder_h

#include <boost/thread.hpp>
#include <logger.h>
#include <MediaMuxer.h>
#include <MediaUtilities.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mcu {

class MediaRecorder : public woogeen_base::MediaMuxer {
    DECLARE_LOGGER();

public:
    MediaRecorder(const std::string& recordUrl, int snapshotInterval);
    ~MediaRecorder();

    // MediaMuxer interface
    bool setMediaSource(woogeen_base::FrameDispatcher* videoDispatcher, woogeen_base::FrameDispatcher* audioDispatcher);
    void removeMediaSource();
    void onFrame(const woogeen_base::Frame&);

private:
    bool init();
    void close();
    bool initRecordContext();
    void recordLoop();
    void writeVideoFrame(woogeen_base::EncodedFrame& encoded_frame);
    void writeAudioFrame(woogeen_base::EncodedFrame& encoded_frame);

    woogeen_base::FrameDispatcher* m_videoSource;
    woogeen_base::FrameDispatcher* m_audioSource;
    AVStream* m_videoStream;
    AVStream* m_audioStream;
    AVFormatContext* m_context;

    int32_t m_videoId, m_audioId;

    std::string m_recordPath;
    // FIXME: snapshot interval for the future usage
    int m_snapshotInterval;
    int64_t m_recordStartTime;
};

} /* namespace mcu */

#endif /* MediaRecorder_h */
