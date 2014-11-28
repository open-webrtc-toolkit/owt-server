#include "VideoMixEngine.h"
#include <string.h>

#define GOP_SIZE 24

VideoMixEngine::VideoMixEngine()
    : m_state(UN_INITIALIZED)
    , m_inputIndex(0)
    , m_outputIndex(0)
{
    m_xcoder = new MsdkXcoder;
    m_vpp = new VppInfo;
}

VideoMixEngine::~VideoMixEngine()
{
    for(std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        uninstallOutput(it->first);
        removeOutput(it->first);
    }

    for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        uninstallInput(it->first);
        removeInput(it->first);
    }

    delete m_vpp;
    m_vpp = NULL;
    delete m_xcoder;
    m_xcoder = NULL;
}

bool VideoMixEngine::Init(BgColor bgColor, unsigned int width, unsigned int height) 
{
    if (m_state == UN_INITIALIZED) {
        m_vpp->bgColor = bgColor;
        m_vpp->width = width;
        m_vpp->height = height;
        m_state = IDLE;
        return true;
    } else
        return false;
}

void VideoMixEngine::SetBackgroundColor(BgColor bgColor)
{
    m_vpp->bgColor = bgColor;
    if (m_state == IN_SERVICE) {
        // TODO: invoke the set background color interface.
        //m_xcoder->SetBackgroundColor(m_vpp->vppHandle, bgColor);
    }
}

void VideoMixEngine::SetResolution(unsigned int width, unsigned int height)
{
    m_vpp->width = width;
    m_vpp->height = height;
    if (m_state == IN_SERVICE) {
        m_xcoder->SetResolution(m_vpp->vppHandle, width, height);
    }
}

void VideoMixEngine::SetLayout(LayoutInfo& layout){
    if (m_state == IN_SERVICE) {
        // TODO: set the actual layout information to media engine.
    }
}

InputIndex VideoMixEngine::EnableInput(CodecType codec, VideoInputProducer* producer) 
{
    InputIndex index = INVALID_INPUT_INDEX;
    switch (m_state) {
        case UN_INITIALIZED:
            break;
        case IDLE:
        case WAITING_FOR_OUTPUT:
            index = scheduleInput(codec, producer);
            m_state = WAITING_FOR_OUTPUT;
            break;
        case WAITING_FOR_INPUT:
            index = scheduleInput(codec, producer);
            setupPipeline();
            m_state = IN_SERVICE;
            break;
        case IN_SERVICE:
            index = scheduleInput(codec, producer);
            installInput(index);
            break;
        default:
            break;
    }
    return index;
}

void VideoMixEngine::DisableInput(InputIndex index) 
{
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_INPUT:
            break;
        case IN_SERVICE:
            uninstallInput(index);
        case WAITING_FOR_OUTPUT:
            removeInput(index);
            break;
        default:
            break;
    }
}

void VideoMixEngine::PushInput(InputIndex index, unsigned char* data, int len)
{
    if (m_state == IN_SERVICE && m_inputs.find(index) != m_inputs.end()) {
        int memPool_freeflat = m_inputs[index].mp->GetFreeFlatBufSize();
        unsigned char* memPool_wrptr = m_inputs[index].mp->GetWritePtr();
        int prefixLength = (m_inputs[index].codec == CODEC_TYPE_VIDEO_VP8) ? 4 : 0;
        int copySize = len + prefixLength;

        if (memPool_freeflat < copySize) {
            return;
        }

        if (m_inputs[index].codec == CODEC_TYPE_VIDEO_VP8) {
            memcpy(memPool_wrptr, (void*)&len, prefixLength);
        }

        memcpy(memPool_wrptr + prefixLength, data, len);
        m_inputs[index].mp->UpdateWritePtrCopyData(copySize);
    }
}

OutputIndex VideoMixEngine::EnableOutput(CodecType codec, const char* name, unsigned short bitrate, VideoOutputConsumer* consumer)
{
    OutputIndex index = INVALID_OUTPUT_INDEX;

    if (isCodecAlreadyInUse(codec))
        return index;

    switch (m_state) {
        case UN_INITIALIZED:
            break;
        case IDLE:
        case WAITING_FOR_INPUT:
            index = scheduleOutput(codec, name, bitrate, consumer);
            m_state = WAITING_FOR_INPUT;
            break;
        case WAITING_FOR_OUTPUT:
            index = scheduleOutput(codec, name, bitrate, consumer);                
            setupPipeline();
            m_state = IN_SERVICE;
            break;
        case IN_SERVICE:
            index = scheduleOutput(codec, name, bitrate, consumer);
            installOutput(index);
            break;
        default:
            break;
    }
    return index;
}

void VideoMixEngine::DisableOutput(OutputIndex index)
{
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_OUTPUT:
            break;
        case IN_SERVICE:
            uninstallOutput(index);
        case WAITING_FOR_INPUT:
            removeOutput(index);
            break;
        default:
            break;
    }
}

void VideoMixEngine::RequestKeyFrame(OutputIndex index)
{
    if (m_state == IN_SERVICE && m_outputs.find(index) != m_outputs.end()) {
        m_xcoder->ForceKeyFrame(m_outputs[index].codec);
    }
}

void VideoMixEngine::SetBitrate(OutputIndex index, unsigned short bitrate)
{
    if (m_state == IN_SERVICE && m_outputs.find(index) != m_outputs.end()) {
        m_outputs[index].bitrate = bitrate;
        m_xcoder->SetBitrate(m_outputs[index].codec, bitrate);
    }
}

int VideoMixEngine::PullOutput(OutputIndex index, unsigned char* buf)
{
    if (m_state == IN_SERVICE && m_outputs.find(index) != m_outputs.end()) {
        Stream* stream = m_outputs[index].stream;
        if (stream->GetBlockCount() > 0) {
            return stream->ReadBlocks(buf, 1);
        }
    }
    return 0;
}

InputIndex VideoMixEngine::scheduleInput(CodecType codec, VideoInputProducer* producer) 
{
    InputIndex i = m_inputIndex++;
    InputInfo input = {codec, producer, NULL, NULL};
    m_inputs[i] = input;
    return i;
}

void VideoMixEngine::installInput(InputIndex index) 
{
    MemPool* memPool = new MemPool;
    memPool->init();
    
    DecOptions dec_cfg = {{0}};
    dec_cfg.inputStream = memPool;
    dec_cfg.input_codec_type = m_inputs[index].codec;
    dec_cfg.measuremnt = NULL;
    m_xcoder->AttachInput(&dec_cfg, m_vpp->vppHandle);
    m_inputs[index].decHandle = dec_cfg.DecHandle;
    m_inputs[index].mp = memPool;
   
    if (m_inputs[index].producer) {
        m_inputs[index].producer->requestKeyFrame(index);
    }
}

void VideoMixEngine::uninstallInput(InputIndex index) 
{
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end() && it->second.decHandle != NULL) {
        m_xcoder->DetachInput(it->second.decHandle);
        it->second.decHandle = NULL;
        delete it->second.mp;
        it->second.mp = NULL;
    }
}

void VideoMixEngine::removeInput(InputIndex index) 
{
    m_inputs.erase(index);
    if (m_inputs.size() == 0) {
        demolishPipeline();
    }
}

OutputIndex VideoMixEngine::scheduleOutput(CodecType codec, const char* name, unsigned short bitrate, VideoOutputConsumer* consumer) 
{
    OutputIndex i = m_outputIndex++;
    OutputInfo output = {codec, consumer, name, bitrate};
    m_outputs[i] = output;
    return i;
}

void VideoMixEngine::installOutput(OutputIndex index) 
{
    Stream* stream = new Stream;
    stream->Open(m_outputs[index].name);
    
    EncOptions enc_cfg = {::CODEC_TYPE_INVALID};
    enc_cfg.outputStream = stream;
    enc_cfg.output_codec_type = m_outputs[index].codec;
    enc_cfg.bitrate = m_outputs[index].bitrate;
    enc_cfg.measuremnt = NULL;
    
    if (m_outputs[index].codec == CODEC_TYPE_VIDEO_AVC) {
        enc_cfg.profile = 66;
        enc_cfg.numRefFrame = 1;
        enc_cfg.idrInterval= 0;
        enc_cfg.intraPeriod = GOP_SIZE;
    }

    m_xcoder->AttachOutput(&enc_cfg, m_vpp->vppHandle);
    m_outputs[index].encHandle = enc_cfg.EncHandle;
    m_outputs[index].stream = stream;
}

void VideoMixEngine::uninstallOutput(OutputIndex index) 
{
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end() && it->second.encHandle != NULL) {
        m_xcoder->DetachOutput(it->second.encHandle);
        it->second.encHandle = NULL;
        delete it->second.stream;
        it->second.stream = NULL;
    }
}

void VideoMixEngine::removeOutput(OutputIndex index) 
{
    m_outputs.erase(index);
    if (m_outputs.size() == 0) {
        demolishPipeline();
    }
}

void VideoMixEngine::setupPipeline() 
{
    InputInfo input = m_inputs.begin()->second;
    OutputInfo output = m_outputs.begin()->second;

    MemPool* memPool = new MemPool;
    memPool->init();
    
    DecOptions dec_cfg = {{0}};
    dec_cfg.inputStream = memPool;
    dec_cfg.input_codec_type = input.codec;
    dec_cfg.measuremnt = NULL;

    VppOptions vpp_cfg = {0};
    vpp_cfg.out_width = m_vpp->width;
    vpp_cfg.out_height = m_vpp->height;
    vpp_cfg.measuremnt = NULL;

    Stream* stream = new Stream;
    stream->Open(output.name);
    
    EncOptions enc_cfg = {CODEC_TYPE_INVALID};
    enc_cfg.outputStream = stream;
    enc_cfg.output_codec_type = output.codec;
    enc_cfg.bitrate = output.bitrate;
    enc_cfg.measuremnt = NULL;
    if (output.codec == CODEC_TYPE_VIDEO_AVC) {
        enc_cfg.profile = 66;
        enc_cfg.numRefFrame = 1;
        enc_cfg.idrInterval= 0;
        enc_cfg.intraPeriod = GOP_SIZE;
    }

    m_xcoder->Init(&dec_cfg, &vpp_cfg, &enc_cfg);
    m_xcoder->Start();
    
    m_inputs.begin()->second.decHandle = dec_cfg.DecHandle;
    m_inputs.begin()->second.mp = memPool;
    m_outputs.begin()->second.encHandle = enc_cfg.EncHandle;
    m_outputs.begin()->second.stream = stream;
    m_vpp->vppHandle = vpp_cfg.VppHandle;
    
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin();
    ++it;
    for (; it != m_inputs.end(); ++it) {
        installInput(it->first);
    }
    
    std::map<OutputIndex, OutputInfo>::iterator it2 = m_outputs.begin();
    ++it2;
    for (; it2 != m_outputs.end(); ++it2) {
        installOutput(it2->first);
    }
}

void VideoMixEngine::demolishPipeline() 
{
    switch (m_state) {
        case IN_SERVICE:
            m_xcoder->Stop();
            m_xcoder->Join();
            m_vpp->vppHandle = NULL;
        case WAITING_FOR_INPUT:
        case WAITING_FOR_OUTPUT:
            if (m_inputs.size() == 0 && m_outputs.size() == 0) {
                m_state = IDLE;
            } else if (m_inputs.size() == 0) {
                m_state = WAITING_FOR_INPUT;
            } else if (m_outputs.size() == 0) {
                m_state = WAITING_FOR_OUTPUT;
            } else {}
            break;
        case IDLE:
        default:
            break;
    }
}

bool VideoMixEngine::isCodecAlreadyInUse(CodecType codec)
{
    for (std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->second.codec == codec) {
            return true;
        }
    }
    return false;
}
