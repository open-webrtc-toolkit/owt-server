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

#include "MediaRecording.h"
#include "MediaUtilities.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <logger.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mcu {

class MediaRecorder {
    DECLARE_LOGGER();

public:
    MediaRecorder(MediaRecording* videoRecording, MediaRecording* audioRecording, const std::string& recordPath, int snapshotInterval);
    virtual ~MediaRecorder();

    bool startRecording();
    void stopRecording();

private:
    bool initRecordContext();
    void recordLoop();
    void writeVideoFrame(EncodedFrame& encoded_frame);
    void writeAudioFrame(EncodedFrame& encoded_frame);

    MediaRecording* m_videoRecording;
    MediaRecording* m_audioRecording;

    bool m_recording;
    std::string m_recordPath;
    AVFormatContext* m_recordContext;
    AVStream* m_recordVideoStream;
    AVStream* m_recordAudioStream;
    int64_t m_recordStartTime;
    long long m_firstVideoTimestamp;
    long long m_firstAudioTimestamp;

    boost::scoped_ptr<MediaFrameQueue> m_videoQueue;
    boost::scoped_ptr<MediaFrameQueue> m_audioQueue;
    boost::thread m_recordThread;
};

} /* namespace mcu */

#endif /* MediaRecorder_h */
