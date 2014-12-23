#include "VideoMixEngineImp.h"

#include <string.h>
#include <iostream>
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
    printf("[%s]Destroy Video Mix Engine Start.\n", __FUNCTION__);

    Locker<Mutex> state_lock(m_state_mutex);
    if (m_state != UN_INITIALIZED) {
        demolishPipeline();

        {
            Locker<Mutex> outputs_lock(m_outputs_mutex);
            for(std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end();) {
                if (it->second.stream) {
                    delete it->second.stream;
                    it->second.stream = NULL;
                }
                m_outputs.erase(it++);
            }
        }

        {
            Locker<Mutex> inputs_lock(m_inputs_mutex);
            for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end();) {
                if (it->second.mp) {
                    delete it->second.mp;
                    it->second.mp = NULL;
                }
                m_inputs.erase(it++);
            }
        }

        if (m_vpp) {
            delete m_vpp;
            m_vpp = NULL;
        }

        m_state = UN_INITIALIZED;
    }

    printf("[%s]Destroy Video Mix Engine End.\n", __FUNCTION__);
}

bool VideoMixEngineImp::init(BackgroundColor bgColor, FrameSize frameSize)
{
    Locker<Mutex> state_lock(m_state_mutex);
    if (m_state == UN_INITIALIZED) {
        m_vpp = new VppInfo;
        if (m_vpp) {
            m_vpp->bgColor = bgColor;
            m_vpp->width = frameSize.width;
            m_vpp->height = frameSize.height;
            m_state = IDLE;
            return true;
        } else {
            return false;
        }
    } else
        return false;
}

void VideoMixEngineImp::setBackgroundColor(const BackgroundColor* bgColor)
{
    if (m_state == IN_SERVICE && bgColor) {
        setbackgroundcolor(bgColor);
    }
}

void VideoMixEngineImp::setbackgroundcolor(const BackgroundColor* bgColor)
{
    if (bgColor) {
        BgColor bg_color;
        bg_color.Y = bgColor->y;
        bg_color.U = bgColor->cb;
        bg_color.V = bgColor->cr;
        if (m_xcoder && m_vpp) {
            if (0 == m_xcoder->SetBackgroundColor(m_vpp->vppHandle, &bg_color)) {
                m_vpp->bgColor.y = bgColor->y;
                m_vpp->bgColor.cb = bgColor->cb;
                m_vpp->bgColor.cr = bgColor->cr;
            } else {
                printf("[%s]Fail to set bg color.\n", __FUNCTION__);
            }
        } else {
            printf("[%s]NULL pointer.\n", __FUNCTION__);
        }
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

void VideoMixEngineImp::setResolution(unsigned int width, unsigned int height)
{
    if (m_state == IN_SERVICE) {
        setresolution(width, height);
    }
}

void VideoMixEngineImp::setresolution(unsigned int width, unsigned int height)
{
    if (m_xcoder && m_vpp) {
        if (0 == m_xcoder->SetResolution(m_vpp->vppHandle, width, height)) {
            m_vpp->width = width;
            m_vpp->height = height;
        } else {
            printf("[%s]Fail to set resolution.\n", __FUNCTION__);
        }
    } else {
        printf("[%s]NULL pointer.\n", __FUNCTION__);
    }
}

void VideoMixEngineImp::setLayout(const CustomLayoutInfo* layout)
{
    if (m_state == IN_SERVICE) {
        if (m_vpp->bgColor.y != layout->rootColor.y
                || m_vpp->bgColor.cb != layout->rootColor.cb
                ||m_vpp->bgColor.cr != layout->rootColor.cr) {
            setbackgroundcolor(&(layout->rootColor));
        }

        // TODO: Currently video root size does not support changing on the fly.
        // Yet, the layout configuration does not have this restriction.
        //if (m_vpp->width != layout.rootSize.width || m_vpp->height != layout.rootSize.height)
        //setresolution(layout->rootSize.width, layout->rootSize.height);

        setregions(&(layout->layoutMapping));
    }
}

void VideoMixEngineImp::setregions(const std::map<InputIndex, RegionInfo>* layoutMapping)
{
    std::map<InputIndex, RegionInfo>::const_iterator it_dec = layoutMapping->begin();
    std::map<InputIndex, RegionInfo>::const_reverse_iterator re_it_dec = layoutMapping->rbegin();
    bool set_combo_type = false;
    int ret = -1;

    Locker<Mutex> inputs_lock(m_inputs_mutex);
    for(; it_dec != layoutMapping->end(); ++it_dec) {
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

            Region regionInfo = {it_dec->second.left, it_dec->second.top,
                it_dec->second.width_ratio, it_dec->second.height_ratio};
            if (it_dec->first == re_it_dec->first) {
                // TODO: Make the Region parameter constant in SetRegionInfo in VCSA
                ret = m_xcoder->SetRegionInfo(m_vpp->vppHandle, it->second.decHandle, regionInfo, true);
                if (ret < 0) {
                    printf("[%s]Fail to set region, input index:%d\n", __FUNCTION__, it_dec->first);
                    break;
                } else {
                    printf("[%s]End set dynamic layout\n", __FUNCTION__);
                }
            } else {
                // TODO: Make the Region parameter constant in SetRegionInfo in VCSA
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

    Locker<Mutex> state_lock(m_state_mutex);
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
    int ret = -1;
    Locker<Mutex> state_lock(m_state_mutex);
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_INPUT:
            break;
        case IN_SERVICE:
            ret = uninstallInput(index);
            if(ret == 0) {
                m_state = WAITING_FOR_INPUT;
                resetOutput();
            }
            break;
        case WAITING_FOR_OUTPUT:
            ret = removeInput(index);
            if(ret == 0) {
                m_state = IDLE;
            }
            break;
        default:
            break;
    }
}

void VideoMixEngineImp::pushInput(InputIndex index, unsigned char* data, int len)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end()) {
        MemPool* mp = it->second.mp;
        VideoMixCodecType codec_type = it->second.codec;
        void* decHandle = it->second.decHandle;
        if (decHandle && mp) {
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
        } else {
            printf("[%s]NULL pointer, index:%d, decHandle:%p, mempool:%p.\n", \
                __FUNCTION__, index, decHandle, mp);
        }
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

OutputIndex VideoMixEngineImp::enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    OutputIndex index = INVALID_OUTPUT_INDEX;

    if (isCodecAlreadyInUse(codec))
        return index;

    Locker<Mutex> state_lock(m_state_mutex);
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
    int ret = -1;
    Locker<Mutex> state_lock(m_state_mutex);
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_OUTPUT:
            break;
        case IN_SERVICE:
            ret = uninstallOutput(index);
            if(ret == 0) {
                m_state = WAITING_FOR_OUTPUT;
                resetInput();
            }
            break;
        case WAITING_FOR_INPUT:
            ret = removeOutput(index);
            if(ret == 0) {
                m_state = IDLE;
            }
            break;
        default:
            break;
    }
}

void VideoMixEngineImp::forceKeyFrame(OutputIndex index)
{
    if (m_state == IN_SERVICE) {
        forcekeyframe(index);
    }
}

void VideoMixEngineImp::forcekeyframe(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (m_xcoder) {
            if (0 != m_xcoder->ForceKeyFrame(codec_type_dict[it->second.codec])) {
                printf("[%s]Fail to force key frame.\n", __FUNCTION__);
            }
        } else {
            printf("[%s]NULL pointer.\n", __FUNCTION__);
        }
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

void VideoMixEngineImp::setBitrate(OutputIndex index, unsigned short bitrate)
{
    if (m_state == IN_SERVICE) {
        setbitrate(index, bitrate);
    }
}

void VideoMixEngineImp::setbitrate(OutputIndex index, unsigned short bitrate)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (m_xcoder) {
            if (0 != m_xcoder->SetBitrate(codec_type_dict[it->second.codec], bitrate)) {
                printf("[%s]Fail to set bit rate.\n", __FUNCTION__);
            } else {
                it->second.bitrate = bitrate;
            }
        } else {
            printf("[%s]NULL pointer.\n", __FUNCTION__);
        }
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

int VideoMixEngineImp::pullOutput(OutputIndex index, unsigned char* buf)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        Stream* stream = it->second.stream;
        void* encHandle = it->second.encHandle;
        if (encHandle && stream) {
            if (stream->GetBlockCount() > 0) {
                return stream->ReadBlocks(buf, 1);
            } else {
                printf("[%s]No output data.\n", __FUNCTION__);
                return 0;
            }
        } else {
            printf("[%s]NULL pointer, index:%d, encHandle:%p, stream:%p.\n", \
                __FUNCTION__, index, encHandle, stream);
            return 0;
        }
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
        return 0;
    }
}

InputIndex VideoMixEngineImp::scheduleInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    InputIndex i = m_inputIndex++;
    InputInfo input = {codec, producer, NULL, NULL};
    m_inputs[i] = input;
    return i;
}

void VideoMixEngineImp::attachInput(InputIndex index, InputInfo* input)
{
    if (input && m_xcoder) {
        DecOptions dec_cfg;
        memset(&dec_cfg, 0, sizeof(dec_cfg));
        setupInputCfg(&dec_cfg, input);

        m_xcoder->AttachInput(&dec_cfg, m_vpp->vppHandle);
        input->decHandle = dec_cfg.DecHandle;
        input->mp = dec_cfg.inputStream;

        VideoMixEngineInput* producer = input->producer;
        if (producer)
            producer->requestKeyFrame(index);
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

void VideoMixEngineImp::installInput(InputIndex index)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end()) {
        attachInput(index, &(it->second));
    }
}

void VideoMixEngineImp::detachInput(InputInfo* input)
{
    if (input && input->decHandle && m_xcoder) {
        m_xcoder->DetachInput(input->decHandle);
        input->decHandle = NULL;
        delete input->mp;
        input->mp = NULL;
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

int VideoMixEngineImp::uninstallInput(InputIndex index)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    int input_size = m_inputs.size();
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end()) {
        if (input_size > 1) {
            detachInput(&(it->second));
        } else if (input_size == 1) {
            demolishPipeline();
            if (it->second.decHandle) {
                it->second.decHandle = NULL;
            }
            if (it->second.mp) {
                delete it->second.mp;
                it->second.mp = NULL;
            }
        }
        m_inputs.erase(it);
    }
    return m_inputs.size();
}

void VideoMixEngineImp::resetInput()
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        if (it->second.mp) {
            delete it->second.mp;
            it->second.mp = NULL;
        }
        it->second.decHandle = NULL;
    }
}

int VideoMixEngineImp::removeInput(InputIndex index)
{
    Locker<Mutex> inputs_lock(m_inputs_mutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end()) {
        m_inputs.erase(it);
    }
    return m_inputs.size();
}

OutputIndex VideoMixEngineImp::scheduleOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    OutputIndex i = m_outputIndex++;
    OutputInfo output = {codec, consumer, bitrate};
    m_outputs[i] = output;
    return i;
}

void VideoMixEngineImp::attachOutput(OutputInfo* output)
{
    if (output && m_xcoder) {
        EncOptions enc_cfg;
        memset(&enc_cfg, 0, sizeof(enc_cfg));
        setupOutputCfg(&enc_cfg, output);

        m_xcoder->AttachOutput(&enc_cfg, m_vpp->vppHandle);
        output->encHandle = enc_cfg.EncHandle;
        output->stream = enc_cfg.outputStream;
    }
}

void VideoMixEngineImp::installOutput(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        attachOutput(&(it->second));
    }
}

void VideoMixEngineImp::detachOutput(OutputInfo* output)
{
    if (output && output->encHandle && m_xcoder) {
        m_xcoder->DetachOutput(output->encHandle);
        output->encHandle = NULL;
        delete output->stream;
        output->stream = NULL;
    } else {
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    }
}

int VideoMixEngineImp::uninstallOutput(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    int output_size = m_outputs.size();
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (output_size > 1) {
            detachOutput(&(it->second));
        } else if (output_size == 1) {
            demolishPipeline();
            if (it->second.encHandle) {
                it->second.encHandle = NULL;
            }
            if (it->second.stream) {
                delete it->second.stream;
                it->second.stream = NULL;
            }
        }
        m_outputs.erase(it);
    }
    return m_outputs.size();
}

void VideoMixEngineImp::resetOutput()
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    for(std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->second.stream) {
            delete it->second.stream;
            it->second.stream = NULL;
        }
        it->second.encHandle = NULL;
    }
}

int VideoMixEngineImp::removeOutput(OutputIndex index)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        m_outputs.erase(it);
    }
    return m_outputs.size();
}

void VideoMixEngineImp::setupInputCfg(DecOptions* dec_cfg, InputInfo* input)
{
    if (dec_cfg && input) {
        MemPool* memPool = new MemPool;
        if (memPool) {
            memPool->init();
            dec_cfg->inputStream = memPool;
            dec_cfg->input_codec_type = codec_type_dict[input->codec];
            dec_cfg->measuremnt = NULL;
        }
    }
}

void VideoMixEngineImp::setupVppCfg(VppOptions* vpp_cfg)
{
    if (vpp_cfg && m_vpp) {
        vpp_cfg->out_width = m_vpp->width;
        vpp_cfg->out_height = m_vpp->height;
        vpp_cfg->measuremnt = NULL;
    }
}

void VideoMixEngineImp::setupOutputCfg(EncOptions* enc_cfg, OutputInfo* output)
{
    if (enc_cfg && output) {
        Stream* stream = new Stream;
        if (stream) {
            stream->Open();
            enc_cfg->outputStream = stream;
            enc_cfg->output_codec_type = codec_type_dict[output->codec];
            enc_cfg->bitrate = output->bitrate;
            enc_cfg->numRefFrame = 1;
            enc_cfg->measuremnt = NULL;
            if (output->codec == VCT_MIX_H264) {
                enc_cfg->profile = 66;
                enc_cfg->numRefFrame = 1;
                enc_cfg->idrInterval= 0;
                enc_cfg->intraPeriod = GOP_SIZE;
            }
        }
    }
}

void VideoMixEngineImp::setupPipeline()
{
    if (m_xcoder) {
        printf("[%s]Xcode has been started.\n", __FUNCTION__);
        return;
    }

    Locker<Mutex> inputs_lock(m_inputs_mutex);
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    std::map<InputIndex, InputInfo>::iterator it_input = m_inputs.begin();
    std::map<OutputIndex, OutputInfo>::iterator it_output = m_outputs.begin();
    if (it_input != m_inputs.end() && it_output != m_outputs.end() && m_vpp) {
        DecOptions dec_cfg;
        memset(&dec_cfg, 0, sizeof(dec_cfg));
        setupInputCfg(&dec_cfg, &(it_input->second));

        VppOptions vpp_cfg;
        memset(&vpp_cfg, 0, sizeof(vpp_cfg));
        setupVppCfg(&vpp_cfg);

        EncOptions enc_cfg;
        memset(&enc_cfg, 0, sizeof(enc_cfg));
        setupOutputCfg(&enc_cfg, &(it_output->second));

        m_xcoder = new MsdkXcoder;
        m_xcoder->Init(&dec_cfg, &vpp_cfg, &enc_cfg);
        m_xcoder->Start();

        it_input->second.decHandle = dec_cfg.DecHandle;
        it_input->second.mp = dec_cfg.inputStream;
        it_output->second.encHandle = enc_cfg.EncHandle;
        it_output->second.stream = enc_cfg.outputStream;
        m_vpp->vppHandle = vpp_cfg.VppHandle;
    }

    for (++it_input; it_input != m_inputs.end(); ++it_input) {
        attachInput(it_input->first, &(it_input->second));
    }

    for (++it_output; it_output != m_outputs.end(); ++it_output) {
        attachOutput(&(it_output->second));
    }

    printf("[%s]Start Xcode pipeline.\n", __FUNCTION__);
}

void VideoMixEngineImp::demolishPipeline()
{
    if (m_xcoder) {
        m_xcoder->Stop();
        delete m_xcoder;
        m_xcoder = NULL;
        m_vpp->vppHandle = NULL;
    }
}

bool VideoMixEngineImp::isCodecAlreadyInUse(VideoMixCodecType codec)
{
    Locker<Mutex> outputs_lock(m_outputs_mutex);
    for (std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->second.codec == codec)
            return true;
    }

    return false;
}
