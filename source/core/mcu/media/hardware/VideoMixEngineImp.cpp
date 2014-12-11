#include "VideoMixEngineImp.h"

#include <string.h>
#include <iostream>
#include <webrtc/system_wrappers/interface/tick_util.h>
#include "msdk_xcoder.h"

#define GOP_SIZE 24

const CodecType codec_type_dict[] = {CODEC_TYPE_VIDEO_VP8, CODEC_TYPE_VIDEO_AVC};

VideoMixEngineImp::VideoMixEngineImp()
    : m_state(UN_INITIALIZED)
    , m_inputIndex(0)
    , m_outputIndex(0)
{
    m_xcoder = NULL;
    m_vpp = NULL;
}

VideoMixEngineImp::~VideoMixEngineImp()
{
    printf("[%s]Destroy Video Mix Engine.\n", __FUNCTION__);

    if (m_xcoder) {
        m_xcoder->Stop();
        delete m_xcoder;
        m_xcoder = NULL;
    }

    for(std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        delete it->second.stream;
        it->second.stream = NULL;
        m_outputs.erase(it);
    }

    for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        delete it->second.mp;
        it->second.mp = NULL;
        m_inputs.erase(it);
    }

    if (m_vpp) {
        delete m_vpp;
        m_vpp = NULL;
    }
}

bool VideoMixEngineImp::init(BackgroundColor bgColor, FrameSize frameSize)
{
    if (m_state == UN_INITIALIZED) {
        m_vpp = new VppInfo;
        m_vpp->bgColor = bgColor;
        m_vpp->width = frameSize.width;
        m_vpp->height = frameSize.height;
        m_state = IDLE;
        return true;
    } else
        return false;
}

void VideoMixEngineImp::setBackgroundColor(const BackgroundColor* bgColor)
{
    if (m_state == IN_SERVICE && bgColor) {
        BgColor bg_color;
        bg_color.Y = bgColor->y;
        bg_color.U = bgColor->cb;
        bg_color.V = bgColor->cr;
        if (0 == m_xcoder->SetBackgroundColor(m_vpp->vppHandle, &bg_color)) {
            m_vpp->bgColor.y = bgColor->y;
            m_vpp->bgColor.cb = bgColor->cb;
            m_vpp->bgColor.cr = bgColor->cr;
        } else {
            printf("[%s]Fail to set bg color.\n", __FUNCTION__);
        }
    }
}

void VideoMixEngineImp::setResolution(unsigned int width, unsigned int height)
{
    if (m_state == IN_SERVICE) {
        if (0 == m_xcoder->SetResolution(m_vpp->vppHandle, width, height)) {
            m_vpp->width = width;
            m_vpp->height = height;
        } else {
            printf("[%s]Fail to set resolution.\n", __FUNCTION__);
        }
    }
}

void VideoMixEngineImp::setLayout(const CustomLayoutInfo& layout)
{
    if (m_state == IN_SERVICE) {
        if (m_vpp->bgColor.y != layout.rootColor.y
            || m_vpp->bgColor.cb != layout.rootColor.cb
            ||m_vpp->bgColor.cr != layout.rootColor.cr)
            setBackgroundColor(&(layout.rootColor));

        // TODO: Currently video root size does not support changing on the fly.
        // Yet, the layout configuration does not have this restriction.
        //if (m_vpp->width != layout.rootSize.width || m_vpp->height != layout.rootSize.height)
        //setResolution(layout.rootSize.width, layout.rootSize.height);

        setRegions(layout.layoutMapping);
    }
}

void VideoMixEngineImp::setRegions(const std::map<InputIndex, RegionInfo>& layoutMapping)
{
    std::map<InputIndex, RegionInfo>::const_iterator it_dec = layoutMapping.begin();
    std::map<InputIndex, RegionInfo>::const_reverse_iterator re_it_dec = layoutMapping.rbegin();
    bool set_combo_type = false;
    int ret = -1;

    for(; it_dec != layoutMapping.end(); ++it_dec) {
        Region regionInfo;
        regionInfo.left = it_dec->second.left;
        regionInfo.top = it_dec->second.top;
        regionInfo.width_ratio = it_dec->second.relativeSize;
        regionInfo.height_ratio = it_dec->second.relativeSize;

        std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(it_dec->first);
        if (it != m_inputs.end() && it->second.decHandle != NULL) {
            if (!set_combo_type) {
                ret = m_xcoder->SetComboType(COMBO_CUSTOM, m_vpp->vppHandle, NULL);
                if (ret != 0) {
                    printf("[%s]Fail to set combo type\n", __FUNCTION__);
                    break;
                } else {
                    set_combo_type = true;
                }
            }
            if (it_dec->first == re_it_dec->first) {
                ret = m_xcoder->SetRegionInfo(m_vpp->vppHandle, it->second.decHandle, regionInfo, true);
                if (ret < 0) {
                    printf("[%s]Fail to set region, input index:%d\n", __FUNCTION__, it_dec->first);
                    break;
                } else {
                    printf("[%s]End set dynamic layout\n", __FUNCTION__);
                }
            } else {
                ret = m_xcoder->SetRegionInfo(m_vpp->vppHandle, it->second.decHandle, regionInfo, false);
                if (ret < 0) {
                    printf("[%s]Fail to set region, input index:%d\n", __FUNCTION__, it_dec->first);
                    break;
                }
            }
        }
    }
}

InputIndex VideoMixEngineImp::enableInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
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

void VideoMixEngineImp::disableInput(InputIndex index)
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

void VideoMixEngineImp::pushInput(InputIndex index, unsigned char* data, int len)
{
    if (m_state == IN_SERVICE) {
        MemPool* mp = m_inputs[index].mp;
        VideoMixCodecType codec_type = m_inputs[index].codec;
        if (mp) {
            int memPool_freeflat = mp->GetFreeFlatBufSize();
            unsigned char* memPool_wrptr = mp->GetWritePtr();
            int prefixLength = (codec_type == VCT_MIX_VP8) ? 4 : 0;
            int copySize = len + prefixLength;
            if (memPool_freeflat < copySize)
                return;

            if (codec_type == VCT_MIX_VP8)
                memcpy(memPool_wrptr, (void*)&len, prefixLength);

            memcpy(memPool_wrptr + prefixLength, data, len);
            mp->UpdateWritePtrCopyData(copySize);
        }
    }
}

OutputIndex VideoMixEngineImp::enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    OutputIndex index = INVALID_OUTPUT_INDEX;

    if (isCodecAlreadyInUse(codec))
        return index;

    switch (m_state) {
        case UN_INITIALIZED:
            break;
        case IDLE:
        case WAITING_FOR_INPUT:
            index = scheduleOutput(codec, bitrate, consumer);
            m_state = WAITING_FOR_INPUT;
            break;
        case WAITING_FOR_OUTPUT:
            index = scheduleOutput(codec, bitrate, consumer);
            setupPipeline();
            m_state = IN_SERVICE;
            break;
        case IN_SERVICE:
            index = scheduleOutput(codec, bitrate, consumer);
            installOutput(index);
            break;
        default:
            break;
    }
    return index;
}

void VideoMixEngineImp::disableOutput(OutputIndex index)
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

void VideoMixEngineImp::forceKeyFrame(OutputIndex index)
{
    if (m_state == IN_SERVICE)
        if (0 != m_xcoder->ForceKeyFrame(codec_type_dict[m_outputs[index].codec])) {
            printf("[%s]Fail to force key frame.\n", __FUNCTION__);
        }
}

void VideoMixEngineImp::setBitrate(OutputIndex index, unsigned short bitrate)
{
    if (m_state == IN_SERVICE) {
        if (0 != m_xcoder->SetBitrate(codec_type_dict[m_outputs[index].codec], bitrate)) {
            printf("[%s]Fail to set bit rate.\n", __FUNCTION__);
        } else {
            m_outputs[index].bitrate = bitrate;
        }
    }
}

int VideoMixEngineImp::pullOutput(OutputIndex index, unsigned char* buf)
{
    if (m_state == IN_SERVICE) {
        Stream* stream = m_outputs[index].stream;
        if (stream && stream->GetBlockCount() > 0)
            return stream->ReadBlocks(buf, 1);
    }

    return 0;
}

InputIndex VideoMixEngineImp::scheduleInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    InputIndex i = m_inputIndex++;
    InputInfo input = {codec, producer, NULL, NULL};
    m_inputs[i] = input;
    return i;
}

void VideoMixEngineImp::installInput(InputIndex index)
{
    if (index >= 0) {
        MemPool* memPool = new MemPool;
        memPool->init();

        DecOptions dec_cfg;
        memset(&dec_cfg, 0, sizeof(dec_cfg));
        dec_cfg.inputStream = memPool;
        dec_cfg.input_codec_type = codec_type_dict[m_inputs[index].codec];
        dec_cfg.measuremnt = NULL;
        m_xcoder->AttachInput(&dec_cfg, m_vpp->vppHandle);
        m_inputs[index].decHandle = dec_cfg.DecHandle;
        m_inputs[index].mp = memPool;

        VideoMixEngineInput* producer = m_inputs[index].producer;
        if (producer)
            producer->requestKeyFrame(index);
    }
}

void VideoMixEngineImp::uninstallInput(InputIndex index)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end() && it->second.decHandle != NULL) {
        m_xcoder->DetachInput(it->second.decHandle);
        it->second.decHandle = NULL;
        delete it->second.mp;
        it->second.mp = NULL;
        m_inputs.erase(it);
    }
}

void VideoMixEngineImp::removeInput(InputIndex index)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    m_inputs.erase(index);
    if (m_inputs.size() == 0)
        demolishPipeline();
}

OutputIndex VideoMixEngineImp::scheduleOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    OutputIndex i = m_outputIndex++;
    OutputInfo output = {codec, consumer, bitrate};
    m_outputs[i] = output;
    return i;
}

void VideoMixEngineImp::installOutput(OutputIndex index)
{
    if (index >= 0) {
        Stream* stream = new Stream;
        stream->Open();

        EncOptions enc_cfg;
        memset(&enc_cfg, 0, sizeof(enc_cfg));
        enc_cfg.outputStream = stream;
        enc_cfg.output_codec_type = codec_type_dict[m_outputs[index].codec];
        enc_cfg.bitrate = m_outputs[index].bitrate;
        enc_cfg.measuremnt = NULL;

        if (m_outputs[index].codec == VCT_MIX_H264) {
            enc_cfg.profile = 66;
            enc_cfg.numRefFrame = 1;
            enc_cfg.idrInterval= 0;
            enc_cfg.intraPeriod = GOP_SIZE;
        }

        m_xcoder->AttachOutput(&enc_cfg, m_vpp->vppHandle);
        m_outputs[index].encHandle = enc_cfg.EncHandle;
        m_outputs[index].stream = stream;
    }
}

void VideoMixEngineImp::uninstallOutput(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end() && it->second.encHandle != NULL) {
        m_xcoder->DetachOutput(it->second.encHandle);
        it->second.encHandle = NULL;
        delete it->second.stream;
        it->second.stream = NULL;
        m_outputs.erase(it);
    }
}

void VideoMixEngineImp::removeOutput(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    m_outputs.erase(index);
    if (m_outputs.size() == 0)
        demolishPipeline();
}

void VideoMixEngineImp::setupPipeline()
{
    if (m_xcoder) {
        printf("[%s]Xcode has been started.\n", __FUNCTION__);
        return;
    }

    InputInfo input = m_inputs.begin()->second;
    OutputInfo output = m_outputs.begin()->second;

    MemPool* memPool = new MemPool;
    memPool->init();

    DecOptions dec_cfg;
    memset(&dec_cfg, 0, sizeof(dec_cfg));
    dec_cfg.inputStream = memPool;
    dec_cfg.input_codec_type = codec_type_dict[input.codec];
    dec_cfg.measuremnt = NULL;

    VppOptions vpp_cfg;
    memset(&vpp_cfg, 0, sizeof(vpp_cfg));
    vpp_cfg.out_width = m_vpp->width;
    vpp_cfg.out_height = m_vpp->height;
    vpp_cfg.measuremnt = NULL;

    Stream* stream = new Stream;
    stream->Open();

    EncOptions enc_cfg;
    memset(&enc_cfg, 0, sizeof(enc_cfg));
    enc_cfg.outputStream = stream;
    enc_cfg.output_codec_type = codec_type_dict[output.codec];
    enc_cfg.bitrate = output.bitrate;
    enc_cfg.measuremnt = NULL;
    if (output.codec == VCT_MIX_H264) {
        enc_cfg.profile = 66;
        enc_cfg.numRefFrame = 1;
        enc_cfg.idrInterval= 0;
        enc_cfg.intraPeriod = GOP_SIZE;
    }

    m_xcoder = new MsdkXcoder;
    m_xcoder->Init(&dec_cfg, &vpp_cfg, &enc_cfg);
    m_xcoder->Start();

    m_inputs.begin()->second.decHandle = dec_cfg.DecHandle;
    m_inputs.begin()->second.mp = memPool;
    m_outputs.begin()->second.encHandle = enc_cfg.EncHandle;
    m_outputs.begin()->second.stream = stream;
    m_vpp->vppHandle = vpp_cfg.VppHandle;

    std::map<InputIndex, InputInfo>::iterator it_input = m_inputs.begin();
    ++it_input;
    for (; it_input != m_inputs.end(); ++it_input)
        installInput(it_input->first);

    std::map<OutputIndex, OutputInfo>::iterator it_output = m_outputs.begin();
    for (++it_output; it_output != m_outputs.end(); ++it_output)
        installOutput(it_output->first);

    printf("[%s]Start Xcode pipeline.\n", __FUNCTION__);
}

void VideoMixEngineImp::demolishPipeline()
{
    switch (m_state) {
        case IN_SERVICE:
            m_xcoder->Stop();
            delete m_xcoder;
            m_xcoder = NULL;
            m_vpp->vppHandle = NULL;
        case WAITING_FOR_INPUT:
        case WAITING_FOR_OUTPUT:
            if (m_inputs.size() == 0 && m_outputs.size() == 0)
                m_state = IDLE;
            else if (m_inputs.size() == 0)
                m_state = WAITING_FOR_INPUT;
            else if (m_outputs.size() == 0)
                m_state = WAITING_FOR_OUTPUT;
            break;
        case IDLE:
        default:
            break;
    }
}

bool VideoMixEngineImp::isCodecAlreadyInUse(VideoMixCodecType codec)
{
    for (std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->second.codec == codec)
            return true;
    }

    return false;
}
