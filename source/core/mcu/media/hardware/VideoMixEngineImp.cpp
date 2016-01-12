#include "VideoMixEngineImp.h"

#include <iostream>
#include <string.h>

#define GOP_SIZE 24

const CodecType codec_type_dict[] = {CODEC_TYPE_VIDEO_VP8, CODEC_TYPE_VIDEO_AVC};

VideoMixEngineImp::VideoMixEngineImp()
    : m_state(UN_INITIALIZED)
    , m_inputIndex(0)
    , m_vppIndex(1)
    , m_outputIndex(0)
    , m_xcoder(NULL)
{
}

VideoMixEngineImp::~VideoMixEngineImp()
{
    printf("[%s]Destroy Video Mix Engine Start.\n", __FUNCTION__);

    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
    if (m_state != UN_INITIALIZED) {
        demolishPipeline();

        {
            boost::unique_lock<boost::shared_mutex> outputLock(m_outputMutex);
            for(std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end();) {
                if (it->second.stream) {
                    delete it->second.stream;
                    it->second.stream = NULL;
                }
                m_outputs.erase(it++);
            }
        }

        {
            boost::unique_lock<boost::shared_mutex> vppLock(m_vppMutex);
            m_vpps.clear();
        }

        {
            boost::unique_lock<boost::shared_mutex> inputLock(m_inputMutex);
            for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end();) {
                if (it->second.mp) {
                    delete it->second.mp;
                    it->second.mp = NULL;
                }
                m_inputs.erase(it++);
            }
        }

        m_state = UN_INITIALIZED;
    }

    printf("[%s]Destroy Video Mix Engine End.\n", __FUNCTION__);
}

bool VideoMixEngineImp::init(BackgroundColor bgColor, FrameSize frameSize)
{
    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
    if (m_state == UN_INITIALIZED) {
        VppInfo vpp;
        vpp.bgColor = bgColor;
        vpp.width = frameSize.width;
        vpp.height = frameSize.height;
        boost::unique_lock<boost::shared_mutex> vppLock(m_vppMutex);
        VppIndex index = 0;
        m_vpps[index] = vpp;
        m_state = IDLE;
        return true;
    }
    return false;
}

void VideoMixEngineImp::setBackgroundColor(const BackgroundColor* bgColor)
{
    if (m_state != IN_SERVICE)
        return;

    if (bgColor) {
        BgColor bg_color;
        bg_color.Y = bgColor->y;
        bg_color.U = bgColor->cb;
        bg_color.V = bgColor->cr;
        if (m_xcoder && !m_vpps.empty()) {
            boost::unique_lock<boost::shared_mutex> vppLock(m_vppMutex);
            for(std::map<VppIndex, VppInfo>::iterator it = m_vpps.begin(); it != m_vpps.end();) {
                if (0 == m_xcoder->SetBackgroundColor(it->second.vppHandle, &bg_color)) {
                    it->second.bgColor.y = bgColor->y;
                    it->second.bgColor.cb = bgColor->cb;
                    it->second.bgColor.cr = bgColor->cr;
                } else
                    printf("[%s]Fail to set bg color.\n", __FUNCTION__);
            }
        } else
            printf("[%s]NULL pointer.\n", __FUNCTION__);
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

void VideoMixEngineImp::setResolution(unsigned int width, unsigned int height)
{
    if (m_state != IN_SERVICE)
        return;
    boost::unique_lock<boost::shared_mutex> vppLock(m_vppMutex);
    std::map<VppIndex, VppInfo>::iterator vppIt = m_vpps.find(0);
    if (m_xcoder && vppIt != m_vpps.end()) {
        if (0 == m_xcoder->SetResolution(vppIt->second.vppHandle, width, height)) {
            vppIt->second.width = width;
            vppIt->second.height = height;
        } else
            printf("[%s]Fail to set resolution.\n", __FUNCTION__);
    } else
        printf("[%s]NULL pointer.\n", __FUNCTION__);
}

void VideoMixEngineImp::getResolution(unsigned int& width, unsigned int& height)
{
    boost::shared_lock<boost::shared_mutex> vppLock(m_vppMutex);
    std::map<VppIndex, VppInfo>::iterator vppIt = m_vpps.find(0);
    if (vppIt != m_vpps.end()) {
        width = vppIt->second.width;
        height = vppIt->second.height;
    }
}

void VideoMixEngineImp::setLayout(const CustomLayoutInfo* layoutMapping)
{
    if (m_state != IN_SERVICE)
        return;

    std::list<InputReginInfo>::const_iterator it_dec = layoutMapping->begin();
    int ret = -1;

    CustomLayout layout;
    {
        boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
        //second, set the layout information
        for(; it_dec != layoutMapping->end(); ++it_dec) {
            std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(it_dec->input);
            if (it != m_inputs.end() && it->second.decHandle != NULL) {
                Region regionInfo = {it_dec->region.left, it_dec->region.top,
                    it_dec->region.width_ratio, it_dec->region.height_ratio};
                CompRegion input = {it->second.decHandle, regionInfo};
                layout.push_back(input);
            }
        }
    }
    if (layout.size() == 0) {
        printf("[%s]No valid inputs in layoutMapping\n", __FUNCTION__);
        return;
    } else {
        boost::shared_lock<boost::shared_mutex> vppLock(m_vppMutex);
        for(std::map<VppIndex, VppInfo>::iterator it = m_vpps.begin(); it != m_vpps.end(); it++) {
            //first, set vpp as COMBO_CUSTOM mode
            ret = m_xcoder->SetComboType(COMBO_CUSTOM, it->second.vppHandle, NULL);
            if (ret != 0) {
                printf("[%s]Fail to set combo type\n", __FUNCTION__);
                return;
            }
            ret = m_xcoder->SetCustomLayout(it->second.vppHandle, &layout);
            if (ret < 0) {
                printf("[%s]Fail to set region\n", __FUNCTION__);
            }
        }
    }
}

InputIndex VideoMixEngineImp::enableInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
{
    InputIndex index = INVALID_INPUT_INDEX;

    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
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
    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_INPUT:
            break;
        case IN_SERVICE:
            ret = uninstallInput(index);
            if (ret == 0) {
                m_state = WAITING_FOR_INPUT;
                resetOutput();
            }
            break;
        case WAITING_FOR_OUTPUT:
            ret = removeInput(index);
            if (ret == 0)
                m_state = IDLE;
            break;
        default:
            break;
    }
}

void VideoMixEngineImp::pushInput(InputIndex index, unsigned char* data, int len)
{
    boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
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
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

OutputIndex VideoMixEngineImp::enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer, FrameSize frameSize)
{
    OutputIndex index = INVALID_OUTPUT_INDEX;

    if (isCodecAlreadyInUse(codec, frameSize))
        return index;
    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
    switch (m_state) {
        case UN_INITIALIZED:
            break;
        case IDLE:
        case WAITING_FOR_INPUT:
            index = scheduleOutput(codec, bitrate, consumer, frameSize);
            m_state = WAITING_FOR_INPUT;
            break;
        case WAITING_FOR_OUTPUT:
            index = scheduleOutput(codec, bitrate, consumer, frameSize);
            setupPipeline();
            m_state = IN_SERVICE;
            break;
        case IN_SERVICE:
            index = scheduleOutput(codec, bitrate, consumer, frameSize);
            installOutput(index);
            break;
        default:
            break;
    }
    return index;
}

OutputIndex VideoMixEngineImp::enableOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer)
{
    FrameSize frameSize = {0, 0};
    return enableOutput(codec, bitrate, consumer, frameSize);
}

void VideoMixEngineImp::disableOutput(OutputIndex index)
{
    int ret = -1;
    boost::unique_lock<boost::shared_mutex> stateLock(m_stateMutex);
    switch (m_state) {
        case UN_INITIALIZED:
        case IDLE:
        case WAITING_FOR_OUTPUT:
            break;
        case IN_SERVICE:
            ret = uninstallOutput(index);
            if (ret == 0) {
                m_state = WAITING_FOR_OUTPUT;
                resetInput();
            }
            break;
        case WAITING_FOR_INPUT:
            ret = removeOutput(index);
            if (ret == 0)
                m_state = IDLE;
            break;
        default:
            break;
    }
}

void VideoMixEngineImp::forceKeyFrame(OutputIndex index)
{
    if (m_state != IN_SERVICE)
        return;

    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (m_xcoder) {
            if (0 != m_xcoder->ForceKeyFrame(it->second.encHandle))
                printf("[%s]Fail to force key frame.\n", __FUNCTION__);
        } else
            printf("[%s]NULL pointer.\n", __FUNCTION__);
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

void VideoMixEngineImp::setBitrate(OutputIndex index, unsigned short bitrate)
{
    // remove the output bitrate setting limitation here
    // now stream bitrate can be set even before pipeline create/init

    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (m_xcoder) {
            if (0 != m_xcoder->SetBitrate(it->second.encHandle, bitrate)) {
                printf("[%s]Fail to set bit rate.\n", __FUNCTION__);
            } else {
                // in case: pipeline created but not initialized
                it->second.bitrate = bitrate;
            }
        } else {
            // in case: pipeline not created yet
            it->second.bitrate = bitrate;
        }
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

int VideoMixEngineImp::pullOutput(OutputIndex index, unsigned char* buf)
{
    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        Stream* stream = it->second.stream;
        void* encHandle = it->second.encHandle;
        if (encHandle && stream) {
            if (stream->GetBlockCount() > 0)
                return stream->ReadBlocks(buf, 1);

            printf("[%s]No output data.\n", __FUNCTION__);
            return 0;
        }

        printf("[%s]NULL pointer, index:%d, encHandle:%p, stream:%p.\n", \
                __FUNCTION__, index, encHandle, stream);
        return 0;
    }

    printf("[%s]Invalid input parameter.\n", __FUNCTION__);
    return 0;
}

void VideoMixEngineImp::DecodeHeaderFailEvent(void *DecHandle)
{
    boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
    for(std::map<InputIndex, InputInfo>::iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
        if (it->second.decHandle == DecHandle) {
            printf("[%s]VideoMixEngineImp::DecodeHeaderFailEvent find decode [%d].\n", __FUNCTION__, it->first);
            VideoMixEngineInput* producer = it->second.producer;
            if (producer) {
                producer->requestKeyFrame(it->first);
            }
            break;
        }
    }
}

InputIndex VideoMixEngineImp::scheduleInput(VideoMixCodecType codec, VideoMixEngineInput* producer)
{
    boost::unique_lock<boost::shared_mutex> inputLock(m_inputMutex);
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

        m_xcoder->AttachInput(&dec_cfg, NULL);
        input->decHandle = dec_cfg.DecHandle;
        input->mp = dec_cfg.inputStream;

        VideoMixEngineInput* producer = input->producer;
        if (producer)
            producer->requestKeyFrame(index);
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

void VideoMixEngineImp::installInput(InputIndex index)
{
    boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end())
        attachInput(index, &(it->second));
}

void VideoMixEngineImp::detachInput(InputInfo* input)
{
    if (input && input->decHandle && m_xcoder) {
        m_xcoder->DetachInput(input->decHandle);
        input->decHandle = NULL;
        delete input->mp;
        input->mp = NULL;
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

int VideoMixEngineImp::uninstallInput(InputIndex index)
{
    boost::upgrade_lock<boost::shared_mutex> inputLock(m_inputMutex);
    int input_size = m_inputs.size();
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end()) {
        if (input_size > 1)
            detachInput(&(it->second));
        else if (input_size == 1) {
            demolishPipeline();
            if (it->second.decHandle)
                it->second.decHandle = NULL;

            if (it->second.mp) {
                delete it->second.mp;
                it->second.mp = NULL;
            }
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> lock(inputLock);
        m_inputs.erase(it);
    }
    return m_inputs.size();
}

void VideoMixEngineImp::resetInput()
{
    boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
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
    boost::unique_lock<boost::shared_mutex> inputLock(m_inputMutex);
    std::map<InputIndex, InputInfo>::iterator it = m_inputs.find(index);
    if (it != m_inputs.end())
        m_inputs.erase(it);

    return m_inputs.size();
}

OutputIndex VideoMixEngineImp::scheduleOutput(VideoMixCodecType codec, unsigned short bitrate, VideoMixEngineOutput* consumer, FrameSize frameSize)
{
    VppIndex vppIndex = INVALID_VPP_INDEX;
    if (frameSize.width == 0 && frameSize.height == 0) {
        vppIndex = 0;
    } else {
        boost::unique_lock<boost::shared_mutex> vppLock(m_vppMutex);
        std::map<VppIndex, VppInfo>::iterator initVpp = m_vpps.find(0);
        std::map<VppIndex, VppInfo>::iterator it = m_vpps.begin();
        for (; it != m_vpps.end(); it++) {
            if (it->second.width == frameSize.width && it->second.height == frameSize.height) {
                vppIndex = it->first;
                break;
            }
        }
        if (it == m_vpps.end()) {
            vppIndex = m_vppIndex++;
            VppInfo vppInfo;
            memset(&vppInfo, 0, sizeof(vppInfo));
            vppInfo.width = frameSize.width;
            vppInfo.height = frameSize.height;
            if (initVpp != m_vpps.end()) {
                vppInfo.bgColor.cb = initVpp->second.bgColor.cb;
                vppInfo.bgColor.cr = initVpp->second.bgColor.cr;
                vppInfo.bgColor.y = initVpp->second.bgColor.y;
            }
            m_vpps[vppIndex] = vppInfo;
        }
    }

    boost::unique_lock<boost::shared_mutex> outputLock(m_outputMutex);
    OutputIndex i = m_outputIndex++;
    OutputInfo output = {codec, consumer, bitrate, vppIndex};
    m_outputs[i] = output;

    return i;
}

void VideoMixEngineImp::attachOutput(OutputInfo* output)
{
    if (output && m_xcoder) {
        EncOptions enc_cfg;
        memset(&enc_cfg, 0, sizeof(enc_cfg));
        setupOutputCfg(&enc_cfg, output);
        boost::shared_lock<boost::shared_mutex> vppLock(m_vppMutex);
        std::map<VppIndex, VppInfo>::iterator vppIt = m_vpps.find(output->vppIndex);
        if (vppIt != m_vpps.end()) {
            if (vppIt->second.vppHandle) {
                m_xcoder->AttachOutput(&enc_cfg, vppIt->second.vppHandle);
            } else {
                VppOptions vpp_cfg;
                memset(&vpp_cfg, 0, sizeof(vpp_cfg));
                setupVppCfg(&vpp_cfg, &(vppIt->second));
                m_xcoder->AttachVpp(&vpp_cfg, &enc_cfg);
                vppIt->second.vppHandle = vpp_cfg.VppHandle;
            }
            output->encHandle = enc_cfg.EncHandle;
            output->stream = enc_cfg.outputStream;
        }
    }
}

void VideoMixEngineImp::installOutput(OutputIndex index)
{
    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end())
        attachOutput(&(it->second));
}

void VideoMixEngineImp::detachOutput(OutputInfo* output)
{
    if (output && output->encHandle && m_xcoder) {
        m_xcoder->DetachOutput(output->encHandle);
        output->encHandle = NULL;
        delete output->stream;
        output->stream = NULL;
    } else
        printf("[%s]Invalid input parameter.\n", __FUNCTION__);
}

int VideoMixEngineImp::uninstallOutput(OutputIndex index)
{
    boost::upgrade_lock<boost::shared_mutex> outputLock(m_outputMutex);
    int output_size = m_outputs.size();
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end()) {
        if (output_size > 1)
            detachOutput(&(it->second));
        else if (output_size == 1) {
            demolishPipeline();
            if (it->second.encHandle)
                it->second.encHandle = NULL;

            if (it->second.stream) {
                delete it->second.stream;
                it->second.stream = NULL;
            }
        }
        boost::upgrade_to_unique_lock<boost::shared_mutex> lock(outputLock);
        m_outputs.erase(it);
    }
    return m_outputs.size();
}

void VideoMixEngineImp::resetOutput()
{
    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
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
    boost::unique_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.find(index);
    if (it != m_outputs.end())
        m_outputs.erase(it);

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

void VideoMixEngineImp::setupVppCfg(VppOptions* vpp_cfg, VppInfo* vpp)
{
    if (vpp_cfg && vpp) {
        vpp_cfg->out_width = vpp->width;
        vpp_cfg->out_height = vpp->height;
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
            //set this field to select vp8 sw encoder or vp8 hybrid encoder
            enc_cfg->swCodecPref = true;
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

    boost::shared_lock<boost::shared_mutex> inputLock(m_inputMutex);
    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    std::map<InputIndex, InputInfo>::iterator it_input = m_inputs.begin();
    std::map<OutputIndex, OutputInfo>::iterator it_output = m_outputs.begin();
    {
        boost::shared_lock<boost::shared_mutex> vppLock(m_vppMutex);
        std::map<VppIndex, VppInfo>::iterator it_vpp = m_vpps.find(0);
        if (it_input != m_inputs.end() && it_output != m_outputs.end() && it_vpp != m_vpps.end()) {
            DecOptions dec_cfg;
            memset(&dec_cfg, 0, sizeof(dec_cfg));
            setupInputCfg(&dec_cfg, &(it_input->second));

            VppOptions vpp_cfg;
            memset(&vpp_cfg, 0, sizeof(vpp_cfg));
            setupVppCfg(&vpp_cfg, &(it_vpp->second));

            EncOptions enc_cfg;
            memset(&enc_cfg, 0, sizeof(enc_cfg));
            setupOutputCfg(&enc_cfg, &(it_output->second));

            m_xcoder = MsdkXcoder::create(this);
            m_xcoder->Init(&dec_cfg, &vpp_cfg, &enc_cfg);
            m_xcoder->Start();

            it_input->second.decHandle = dec_cfg.DecHandle;
            it_input->second.mp = dec_cfg.inputStream;
            it_output->second.encHandle = enc_cfg.EncHandle;
            it_output->second.stream = enc_cfg.outputStream;
            it_vpp->second.vppHandle = vpp_cfg.VppHandle;

            VideoMixEngineInput* producer = it_input->second.producer;
            if (producer)
                producer->requestKeyFrame(it_input->first);
        }
    }

    for (++it_input; it_input != m_inputs.end(); ++it_input)
        attachInput(it_input->first, &(it_input->second));

    for (++it_output; it_output != m_outputs.end(); ++it_output)
        attachOutput(&(it_output->second));

    printf("[%s]Start Xcode pipeline.\n", __FUNCTION__);
}

void VideoMixEngineImp::demolishPipeline()
{
    if (m_xcoder) {
        m_xcoder->Stop();
        MsdkXcoder::destroy(m_xcoder);
        m_xcoder = NULL;
    }
}

bool VideoMixEngineImp::isCodecAlreadyInUse(VideoMixCodecType codec, FrameSize frameSize)
{
    VppIndex vppIndex = INVALID_VPP_INDEX;
    if (frameSize.width == 0 && frameSize.height == 0) {
        vppIndex = 0;
    } else {
        boost::shared_lock<boost::shared_mutex> vppLock(m_vppMutex);
        for(std::map<VppIndex, VppInfo>::iterator it = m_vpps.begin(); it != m_vpps.end(); it++) {
            if (it->second.width == frameSize.width && it->second.height == frameSize.height) {
                vppIndex = it->first;
                break;
            }
        }
    }
    boost::shared_lock<boost::shared_mutex> outputLock(m_outputMutex);
    for (std::map<OutputIndex, OutputInfo>::iterator it = m_outputs.begin(); it != m_outputs.end(); ++it) {
        if (it->second.codec == codec && it->second.vppIndex == vppIndex)
            return true;
    }

    return false;
}
