// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef VideoFrameTranscoderImpl_h
#define VideoFrameTranscoderImpl_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>
#include <MediaUtilities.h>
#include <MediaFramePipeline.h>
#include <VideoFrameTranscoder.h>

#include <VCMFrameDecoder.h>
#include <VCMFrameEncoder.h>

#include <FFmpegFrameDecoder.h>

#include <FrameProcesser.h>

#ifdef ENABLE_MSDK
#include <MsdkFrameDecoder.h>
#include <MsdkFrameEncoder.h>
#endif

#ifdef ENABLE_SVT_HEVC_ENCODER
#include <SVTHEVCEncoder.h>
#endif

namespace mcu {

class VideoFrameTranscoderImpl : public VideoFrameTranscoder, public owt_base::FrameSource, public owt_base::FrameDestination {
public:
    VideoFrameTranscoderImpl();
    ~VideoFrameTranscoderImpl();

    bool setInput(int input, owt_base::FrameFormat, owt_base::FrameSource*);
    void unsetInput(int input);

    bool addOutput(int output,
            owt_base::FrameFormat,
            const owt_base::VideoCodecProfile profile,
            const owt_base::VideoSize&,
            const unsigned int framerateFPS,
            const unsigned int bitrateKbps,
            const unsigned int keyFrameIntervalSeconds,
            owt_base::FrameDestination*);
    void removeOutput(int output);

    void requestKeyFrame(int output);

    void drawText(const std::string& textSpec);
    void clearText();

    void onFrame(const owt_base::Frame& frame) {deliverFrame(frame);}

private:
    struct Input {
        owt_base::FrameSource* source;
        boost::shared_ptr<owt_base::VideoFrameDecoder> decoder;
    };

    struct Output {
        boost::shared_ptr<owt_base::VideoFrameProcesser> processer;
        boost::shared_ptr<owt_base::VideoFrameEncoder> encoder;
        int streamId;
    };

    std::map<int, Input> m_inputs;
    boost::shared_mutex m_inputMutex;

    std::map<int, Output> m_outputs;
    boost::shared_mutex m_outputMutex;
};

VideoFrameTranscoderImpl::VideoFrameTranscoderImpl()
{
}

VideoFrameTranscoderImpl::~VideoFrameTranscoderImpl()
{
    {
        boost::unique_lock<boost::shared_mutex> lock(m_outputMutex);
        for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it) {
            this->removeVideoDestination(it->second.processer.get());
            it->second.processer->removeVideoDestination(it->second.encoder.get());
            it->second.encoder->degenerateStream(it->second.streamId);
        }
        m_outputs.clear();
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(m_inputMutex);
        for (auto it = m_inputs.begin(); it != m_inputs.end(); ++it) {
            it->second.source->removeVideoDestination(it->second.decoder.get());
            it->second.decoder->removeVideoDestination(this);
            m_inputs.erase(it);
        }
        m_inputs.clear();
    }
}

inline bool VideoFrameTranscoderImpl::setInput(int input, owt_base::FrameFormat format, owt_base::FrameSource* source)
{
    assert(source);

    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end())
        return false;

    boost::shared_ptr<owt_base::VideoFrameDecoder> decoder;

#ifdef ENABLE_MSDK
    if (!decoder && owt_base::MsdkFrameDecoder::supportFormat(format))
        decoder.reset(new owt_base::MsdkFrameDecoder());
#endif

    if (!decoder && owt_base::VCMFrameDecoder::supportFormat(format))
        decoder.reset(new owt_base::VCMFrameDecoder(format));

    if (!decoder && owt_base::FFmpegFrameDecoder::supportFormat(format))
        decoder.reset(new owt_base::FFmpegFrameDecoder());

    if (!decoder)
        return false;

    if (decoder->init(format)) {
        decoder->addVideoDestination(this);
        source->addVideoDestination(decoder.get());
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        Input in{.source = source, .decoder = decoder};
        m_inputs[input] = in;
        return true;
    }
    return false;
}

inline void VideoFrameTranscoderImpl::unsetInput(int input)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_inputMutex);
    auto it = m_inputs.find(input);
    if (it != m_inputs.end()) {
        it->second.source->removeVideoDestination(it->second.decoder.get());
        it->second.decoder->removeVideoDestination(this);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        m_inputs.erase(it);
    }
}

inline bool VideoFrameTranscoderImpl::addOutput(int output,
                                           owt_base::FrameFormat format,
                                           const owt_base::VideoCodecProfile profile,
                                           const owt_base::VideoSize& rootSize,
                                           const unsigned int framerateFPS,
                                           const unsigned int bitrateKbps,
                                           const unsigned int keyFrameIntervalSeconds,
                                           owt_base::FrameDestination* dest)
{
    boost::shared_ptr<owt_base::VideoFrameEncoder> encoder;
    boost::shared_ptr<owt_base::VideoFrameProcesser> processer;
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    int32_t streamId = -1;

#ifdef ENABLE_MSDK
    if (!encoder && owt_base::MsdkFrameEncoder::supportFormat(format)) {
        encoder.reset(new owt_base::MsdkFrameEncoder(format, profile, false));
    }
#endif

#if ENABLE_SVT_HEVC_ENCODER
    if (!encoder && format == owt_base::FRAME_FORMAT_H265)
        encoder.reset(new owt_base::SVTHEVCEncoder(format, profile));
#endif

    if (!encoder && owt_base::VCMFrameEncoder::supportFormat(format))
        encoder.reset(new owt_base::VCMFrameEncoder(format, profile, false));

    if (!encoder)
        return false;

    streamId = encoder->generateStream(rootSize.width, rootSize.height, framerateFPS, bitrateKbps, keyFrameIntervalSeconds, dest);
    if (streamId < 0)
        return false;

    if (!processer) {
        processer.reset(new owt_base::FrameProcesser());
    }

    if (!processer->init(encoder->getInputFormat(), rootSize.width, rootSize.height, framerateFPS))
        return false;

    this->addVideoDestination(processer.get());
    processer->addVideoDestination(encoder.get());

    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    Output out{.processer = processer, .encoder = encoder, .streamId = streamId};
    m_outputs[output] = out;
    return true;
}

inline void VideoFrameTranscoderImpl::removeOutput(int32_t output)
{
    boost::upgrade_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end()) {
        it->second.encoder->degenerateStream(it->second.streamId);
        if (it->second.encoder->isIdle()) {
            this->removeVideoDestination(it->second.processer.get());
            it->second.processer->removeVideoDestination(it->second.encoder.get());
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lock);
        m_outputs.erase(output);
    }
}

inline void VideoFrameTranscoderImpl::requestKeyFrame(int output)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    auto it = m_outputs.find(output);
    if (it != m_outputs.end())
        it->second.encoder->requestKeyFrame(it->second.streamId);
}

inline void VideoFrameTranscoderImpl::drawText(const std::string& textSpec)
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it)
        it->second.processer->drawText(textSpec);
}

inline void VideoFrameTranscoderImpl::clearText()
{
    boost::shared_lock<boost::shared_mutex> lock(m_outputMutex);
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it)
        it->second.processer->clearText();
}

}
#endif
