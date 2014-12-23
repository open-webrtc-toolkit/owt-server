#include "VideoMixEngine.h"
#include "VideoMixEngineImp.h"


VideoMixEngine::VideoMixEngine()
    : m_imp(NULL)
{
    m_imp = new VideoMixEngineImp();
}

VideoMixEngine::~VideoMixEngine()
{
    delete m_imp;
}

bool VideoMixEngine::init(BackgroundColor bgColor, FrameSize frameSize)
{
    return m_imp->init(bgColor, frameSize);
}

void VideoMixEngine::setBackgroundColor(const BackgroundColor* bgColor)
{
    m_imp->setBackgroundColor(bgColor);
}

void VideoMixEngine::setResolution(unsigned int width, unsigned int height)
{
    m_imp->setResolution(width, height);
}

void VideoMixEngine::setLayout(const CustomLayoutInfo* layout)
{
    m_imp->setLayout(layout);
}

InputIndex VideoMixEngine::enableInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
{
    if (codec != VCT_MIX_VP8 && codec != VCT_MIX_H264)
        return INVALID_INPUT_INDEX;

    return m_imp->enableInput(codec, producer);
}

void VideoMixEngine::disableInput(InputIndex index)
{
    m_imp->disableInput(index);
}

void VideoMixEngine::pushInput(InputIndex index, unsigned char* data, int len)
{
    m_imp->pushInput(index, data, len);
}

OutputIndex VideoMixEngine::enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    if (codec != VCT_MIX_VP8 && codec != VCT_MIX_H264)
        return INVALID_OUTPUT_INDEX;

    return m_imp->enableOutput(codec, bitrate, consumer);
}

void VideoMixEngine::disableOutput(OutputIndex index)
{
    m_imp->disableOutput(index);
}

void VideoMixEngine::forceKeyFrame(OutputIndex index)
{
    m_imp->forceKeyFrame(index);
}

void VideoMixEngine::setBitrate(OutputIndex index, unsigned short bitrate)
{
    m_imp->setBitrate(index, bitrate);
}

int VideoMixEngine::pullOutput(OutputIndex index, unsigned char* buf)
{
    return m_imp->pullOutput(index, buf);
}
