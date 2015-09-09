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

#ifndef MediaFrameCallback_h
#define MediaFrameCallback_h

#include "MediaFramePipeline.h"

#include <rtputils.h>
#include <webrtc/video/encoded_frame_callback_adapter.h>
#include <webrtc/voice_engine/include/voe_base.h>

namespace woogeen_base {

class VideoEncodedFrameCallbackAdapter : public webrtc::EncodedImageCallback {
public:
    VideoEncodedFrameCallbackAdapter(FrameConsumer* consumer)
        : m_frameConsumer(consumer)
    {
    }

    virtual ~VideoEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(const webrtc::EncodedImage& encodedImage,
                            const webrtc::CodecSpecificInfo* codecSpecificInfo,
                            const webrtc::RTPFragmentationHeader* fragmentation)
    {
        if (encodedImage._length > 0 && m_frameConsumer) {
            FrameFormat format = FRAME_FORMAT_UNKNOWN;
            switch (codecSpecificInfo->codecType) {
            case webrtc::kVideoCodecVP8:
                format = FRAME_FORMAT_VP8;
                break;
            case webrtc::kVideoCodecH264:
                format = FRAME_FORMAT_H264;
                break;
            default:
                break;
            }

            Frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.format = format;
            frame.payload = encodedImage._buffer;
            frame.length = encodedImage._length;
            frame.timeStamp = encodedImage._timeStamp;
            frame.additionalInfo.video.width = encodedImage._encodedWidth;
            frame.additionalInfo.video.height = encodedImage._encodedHeight;

            m_frameConsumer->onFrame(frame);
        }

        return 0;
    }

private:
    FrameConsumer* m_frameConsumer;
};

class AudioEncodedFrameCallbackAdapter : public webrtc::AudioEncodedFrameCallback {
public:
    AudioEncodedFrameCallbackAdapter(FrameConsumer* consumer)
        : m_frameConsumer(consumer)
    {
    }

    virtual ~AudioEncodedFrameCallbackAdapter() { }

    virtual int32_t Encoded(webrtc::FrameType frameType, uint8_t payloadType,
                            uint32_t timeStamp, const uint8_t* payloadData,
                            uint16_t payloadSize)
    {
        if (payloadSize > 0 && m_frameConsumer) {
            FrameFormat format = FRAME_FORMAT_UNKNOWN;
            Frame frame;
            memset(&frame, 0, sizeof(frame));
            switch (payloadType) {
            case PCMU_8000_PT:
                format = FRAME_FORMAT_PCMU;
                frame.additionalInfo.audio.channels = 1; // FIXME: retrieve codec info from VOE?
                frame.additionalInfo.audio.sampleRate = 8000;
                break;
            case OPUS_48000_PT:
                format = FRAME_FORMAT_OPUS;
                frame.additionalInfo.audio.channels = 2;
                frame.additionalInfo.audio.sampleRate = 48000;
                break;
            default:
                break;
            }

            frame.format = format;
            frame.payload = const_cast<uint8_t*>(payloadData);
            frame.length = payloadSize;
            frame.timeStamp = timeStamp;

            m_frameConsumer->onFrame(frame);
        }

        return 0;
    }

private:
    FrameConsumer* m_frameConsumer;
};

} /* namespace woogeen_base */

#endif /* MediaFrameCallback_h */
