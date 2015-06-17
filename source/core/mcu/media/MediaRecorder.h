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

#include <boost/scoped_ptr.hpp>
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
    MediaRecorder(woogeen_base::FrameDispatcher* videoRecording, woogeen_base::FrameDispatcher* audioRecording, const std::string& recordPath, int snapshotInterval);
    ~MediaRecorder();

    // MediaMuxer interface
    bool start();
    void stop();

    void onFrame(woogeen_base::FrameFormat, unsigned char* payload, int len, unsigned int ts);

private:
    bool initRecordContext();
    void recordLoop();
    void writeVideoFrame(woogeen_base::EncodedFrame& encoded_frame);
    void writeAudioFrame(woogeen_base::EncodedFrame& encoded_frame);

    woogeen_base::FrameDispatcher* m_videoSource;
    woogeen_base::FrameDispatcher* m_audioSource;
    AVStream* m_videoStream;
    AVStream* m_audioStream;
    AVFormatContext* m_context;

    std::string m_recordPath;
    int64_t m_recordStartTime;
    int32_t m_videoId, m_audioId;
};

} /* namespace mcu */

#endif /* MediaRecorder_h */
