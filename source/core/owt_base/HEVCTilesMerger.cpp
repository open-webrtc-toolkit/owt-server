// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifdef _ENABLE_HEVC_TILES_MERGER_

#include "HEVCTilesMerger.h"
#include "MediaUtilities.h"

namespace owt_base {

DEFINE_LOGGER(HEVCTilesMerger, "owt.HEVCTilesMerger");

static int filterNALs(uint8_t *data, int size, const std::vector<int> &remove_types)
{
    int remove_nals_size = 0;

    int nalu_found_length = 0;
    uint8_t* buffer_start = data;
    int buffer_length = size;
    int nalu_start_offset = 0;
    int nalu_end_offset = 0;
    int sc_len = 0;
    int nalu_type;

    while (buffer_length > 0) {
        nalu_found_length = findNALU(buffer_start, buffer_length, &nalu_start_offset, &nalu_end_offset, &sc_len);
        if (nalu_found_length < 0) {
            /* Error, should never happen */
            break;
        }

        nalu_type = (buffer_start[nalu_start_offset] & 0x7e) >> 1;
        if (find(remove_types.begin(), remove_types.end(), nalu_type) != remove_types.end()) {
            //next
            memmove(buffer_start, buffer_start + nalu_start_offset + nalu_found_length, buffer_length - nalu_start_offset - nalu_found_length);
            buffer_length -= nalu_start_offset + nalu_found_length;

            remove_nals_size += nalu_start_offset + nalu_found_length;
            continue;
        }

        buffer_start += (nalu_start_offset + nalu_found_length);
        buffer_length -= (nalu_start_offset + nalu_found_length);
    }

    return size - remove_nals_size;
}

HEVCTilesMerger::HEVCTilesMerger()
    : m_init(false)
    , m_handle(NULL)
    , m_inputHiBuffer(NULL)
    , m_inputHiBufferLength(0)
    , m_inputLowBuffer(NULL)
    , m_inputLowBufferLength(0)
    , m_outputBuffer(NULL)
    , m_outputBufferLength(0)
    , m_outputSEIBuffer(NULL)
    , m_outputBufferSEILength(0)
    , m_frameBuffer(NULL)
    , m_frameBufferLength(0)
    , m_viewport_w(960)
    , m_viewport_h(960)
    , m_viewPortFOV(80)
    , m_yaw(-90)
    , m_pitch(0)
    , m_fovUpdated(false)
    , m_enableBsDump(false)
    , m_bsDumpfp_hi_res(NULL)
    , m_bsDumpfp_low_res(NULL)
    , m_bsDumpfp(NULL)
{
}

HEVCTilesMerger::~HEVCTilesMerger()
{
    if (m_handle)
        I360SCVP_unInit(m_handle);

    if (m_inputHiBuffer)
        delete [] m_inputHiBuffer;

    if (m_inputLowBuffer)
        delete [] m_inputLowBuffer;

    if (m_outputBuffer)
        delete [] m_outputBuffer;

    if (m_outputSEIBuffer)
        delete [] m_outputSEIBuffer;

    if (m_frameBuffer)
        delete [] m_frameBuffer;
}

bool HEVCTilesMerger::init(const FrameHevcTiles& tilesFrame)
{
    memset(&m_360scvp_param, 0, sizeof(m_360scvp_param));

    m_360scvp_param.usedType = E_MERGE_AND_VIEWPORT;
    m_360scvp_param.frameWidth = tilesFrame.hi_width;
    m_360scvp_param.frameHeight = tilesFrame.hi_height;
    m_360scvp_param.frameWidthLow = tilesFrame.low_width;
    m_360scvp_param.frameHeightLow = tilesFrame.low_height;

    m_360scvp_param.paramViewPort.geoTypeInput = EGeometryType(E_SVIDEO_EQUIRECT);
    m_360scvp_param.paramViewPort.geoTypeOutput = E_SVIDEO_VIEWPORT;
    m_360scvp_param.paramViewPort.faceWidth = m_360scvp_param.frameWidth;
    m_360scvp_param.paramViewPort.faceHeight = m_360scvp_param.frameHeight;
    m_360scvp_param.paramViewPort.viewportWidth = m_viewport_w;
    m_360scvp_param.paramViewPort.viewportHeight = m_viewport_h;
    m_360scvp_param.paramViewPort.viewPortFOVH = m_viewPortFOV;
    m_360scvp_param.paramViewPort.viewPortFOVV = m_viewPortFOV;

    {
        boost::unique_lock<boost::shared_mutex> ulock(m_mutex);

        m_360scvp_param.paramViewPort.viewPortYaw = m_yaw;
        m_360scvp_param.paramViewPort.viewPortPitch = m_pitch;
        m_fovUpdated = false;
    }

    ELOG_INFO("Init hi-res %dx%d, low-res %dx%d, viewport-res %dx%d, viewport-fov %.2f(h-degree)-%.2f(v-degree), FOV(yaw %.2f, pitch %.2f)"
            , m_360scvp_param.frameWidth, m_360scvp_param.frameHeight
            , m_360scvp_param.frameWidthLow, m_360scvp_param.frameHeightLow
            , m_360scvp_param.paramViewPort.viewportWidth
            , m_360scvp_param.paramViewPort.viewportHeight
            , m_360scvp_param.paramViewPort.viewPortFOVH
            , m_360scvp_param.paramViewPort.viewPortFOVV
            , m_360scvp_param.paramViewPort.viewPortYaw
            , m_360scvp_param.paramViewPort.viewPortPitch
            );

    // hi res input
    m_inputHiBufferLength = tilesFrame.hi_width * tilesFrame.hi_height * 5;
    m_inputHiBuffer = new uint8_t[m_inputHiBufferLength];
    m_360scvp_param.pInputBitstream = m_inputHiBuffer;
    m_360scvp_param.inputBitstreamLen = 0;

    memcpy(m_360scvp_param.pInputBitstream, tilesFrame.hi_payload, tilesFrame.hi_length);
    m_360scvp_param.inputBitstreamLen = tilesFrame.hi_length;

    // low res input
    m_inputLowBufferLength = tilesFrame.low_width * tilesFrame.low_height * 5;
    m_inputLowBuffer = new uint8_t[m_inputLowBufferLength];
    m_360scvp_param.pInputLowBitstream = m_inputLowBuffer;
    m_360scvp_param.inputLowBistreamLen = m_inputLowBufferLength;

    memcpy(m_360scvp_param.pInputLowBitstream, tilesFrame.low_payload, tilesFrame.low_length);
    m_360scvp_param.inputLowBistreamLen = tilesFrame.low_length;

    // output and sei
    m_outputBufferLength = tilesFrame.hi_width * tilesFrame.hi_height * 5;
    m_outputBuffer = new uint8_t[m_outputBufferLength];
    m_360scvp_param.pOutputBitstream = m_outputBuffer;
    m_360scvp_param.outputBitstreamLen = m_outputBufferLength;

    m_outputBufferSEILength = tilesFrame.hi_width * tilesFrame.hi_height * 5;
    m_outputSEIBuffer = new uint8_t[m_outputBufferSEILength];
    m_360scvp_param.pOutputSEI = m_outputSEIBuffer;
    m_360scvp_param.outputSEILen = m_outputBufferSEILength;

    m_handle = I360SCVP_Init(&m_360scvp_param);
    if (m_handle == NULL) {
        ELOG_ERROR("Generate the vide port handle fail!");
        return false;
    }

    m_frameBufferLength = m_outputBufferLength + m_outputBufferSEILength;
    m_frameBuffer = new uint8_t[m_frameBufferLength];

    ELOG_INFO("Init OK!");
    return true;
}

HEVCTilesMerger::FrameHevcTiles::FrameHevcTiles (const Frame& frame)
{
    uint32_t width;
    uint32_t height;
    uint32_t length;

    int offset = 0;

    // hi-res
    width = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    height = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    length = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    this->hi_width = width;
    this->hi_height = height;
    this->hi_length = length;
    this->hi_payload = frame.payload + offset;

    offset += length;

    // low-res
    width = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    height = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    length = frame.payload[offset + 0]
        | frame.payload[offset + 1] << 8
        | frame.payload[offset + 2] << 16
        | frame.payload[offset + 3] << 24;
    offset += 4;

    this->low_width = width;
    this->low_height = height;
    this->low_length = length;
    this->low_payload = frame.payload + offset;

    this->isKey = frame.additionalInfo.video.isKeyFrame;
    this->timeStamp = frame.timeStamp;

    ELOG_TRACE("hi-res %dx%d, %d, low-res %dx%d, %d, %s",
            this->hi_width, this->hi_height, this->hi_length,
            this->low_width, this->low_height, this->low_length,
            this->isKey ? "key" : "delta");
}

void HEVCTilesMerger::onFrame(const Frame& frame, Frame *pOutFrame)
{
#if 0
    ELOG_INFO("onFrame(%s), %s, %dx%d, length(%d)",
            getFormatStr(frame.format),
            frame.additionalInfo.video.isKeyFrame ? "key" : "delta",
            frame.additionalInfo.video.width,
            frame.additionalInfo.video.height,
            frame.length
            );
#endif

    switch (frame.format) {
        case FRAME_FORMAT_HEVC_MCTS:
            break;
        default:
            ELOG_ERROR("Unspported video frame format %d(%s)", frame.format, getFormatStr(frame.format));
            return;
    }

    FrameHevcTiles tilesFrame(frame);

    if (!m_init) {
        if (!frame.additionalInfo.video.isKeyFrame) {
            ELOG_INFO("Need key frames");
            return;
        }

        initDump();
        init(tilesFrame);
        m_init = true;
    }

    dump(tilesFrame.hi_payload, tilesFrame.hi_length, tilesFrame.isKey, m_bsDumpfp_hi_res);
    dump(tilesFrame.low_payload, tilesFrame.low_length, tilesFrame.isKey, m_bsDumpfp_low_res);

    if (m_handle) {
        memcpy(m_360scvp_param.pInputBitstream, tilesFrame.hi_payload, tilesFrame.hi_length);
        m_360scvp_param.inputBitstreamLen = tilesFrame.hi_length;

        memcpy(m_360scvp_param.pInputLowBitstream, tilesFrame.low_payload, tilesFrame.low_length);
        m_360scvp_param.inputLowBistreamLen = tilesFrame.low_length;

        m_360scvp_param.outputBitstreamLen = 0;
        m_360scvp_param.outputSEILen = 0;

        // change viewport
        if (tilesFrame.isKey) {
            bool needSetFoV = false;

            {
                boost::unique_lock<boost::shared_mutex> ulock(m_mutex);
                if (m_fovUpdated) {
                    ELOG_DEBUG("Apply new FoV yaw %.2f -> %d, pitch %.2f -> %d"
                            , m_360scvp_param.paramViewPort.viewPortYaw
                            , m_yaw
                            , m_360scvp_param.paramViewPort.viewPortPitch
                            , m_pitch
                            );

                    m_360scvp_param.paramViewPort.viewPortYaw = m_yaw;
                    m_360scvp_param.paramViewPort.viewPortPitch = m_pitch;

                    m_fovUpdated = false;

                    needSetFoV = true;
                }
            }

            if (needSetFoV) {
                I360SCVP_setViewPort(m_handle, m_360scvp_param.paramViewPort.viewPortYaw, m_360scvp_param.paramViewPort.viewPortPitch);
            }
        }

        I360SCVP_process(&m_360scvp_param, m_handle);
        if (m_360scvp_param.outputBitstreamLen > 0) {
            if (!tilesFrame.isKey) {
                std::vector<int> ps;
                ps.push_back(32);
                ps.push_back(33);
                ps.push_back(34);

                uint32_t filted_size = filterNALs(m_360scvp_param.pOutputBitstream, m_360scvp_param.outputBitstreamLen, ps);
                if (m_360scvp_param.outputBitstreamLen != filted_size) {
                    ELOG_DEBUG("remove vps/sps/pps size: %d", m_360scvp_param.outputBitstreamLen - filted_size);

                    m_360scvp_param.outputBitstreamLen = filted_size;
                }
            }

            memcpy(m_frameBuffer, m_360scvp_param.pOutputBitstream, m_360scvp_param.outputBitstreamLen);

            //m_360scvp_param.outputSEILen = 0;
            if (m_360scvp_param.outputSEILen > 0) {
                memcpy(m_frameBuffer + m_360scvp_param.outputBitstreamLen, m_360scvp_param.pOutputSEI, m_360scvp_param.outputSEILen);
            } else {
                //ELOG_INFO("invalid SEI output bitstream len: %d\n", m_360scvp_param.outputSEILen);
            }

            Frame outFrame;
            memset(&outFrame, 0, sizeof(outFrame));

            outFrame.format = FRAME_FORMAT_H265;
            outFrame.payload = m_frameBuffer;
            outFrame.length = m_360scvp_param.outputBitstreamLen + m_360scvp_param.outputSEILen;
            outFrame.additionalInfo.video.width = m_viewport_w;
            outFrame.additionalInfo.video.height = m_viewport_h;
            outFrame.additionalInfo.video.isKeyFrame = tilesFrame.isKey;
            outFrame.timeStamp = tilesFrame.timeStamp;

            ELOG_TRACE("deliverFrame, %s, %dx%d(%s), length(%d)",
                    getFormatStr(outFrame.format),
                    outFrame.additionalInfo.video.width,
                    outFrame.additionalInfo.video.height,
                    outFrame.additionalInfo.video.isKeyFrame ? "key" : "delta",
                    outFrame.length);

            dump(outFrame.payload, outFrame.length, outFrame.additionalInfo.video.isKeyFrame, m_bsDumpfp);

            *pOutFrame = outFrame;
        } else {
            ELOG_ERROR("SCVP_process error");
        }
    } else {
        Frame outFrame;
        memset(&outFrame, 0, sizeof(outFrame));

        outFrame.format = FRAME_FORMAT_H265;
        outFrame.payload = tilesFrame.low_payload;
        outFrame.length = tilesFrame.low_length;
        outFrame.additionalInfo.video.width = tilesFrame.low_width;
        outFrame.additionalInfo.video.height = tilesFrame.low_height;
        outFrame.additionalInfo.video.isKeyFrame = tilesFrame.isKey;
        outFrame.timeStamp = tilesFrame.timeStamp;

        ELOG_TRACE("Bypass TilesMerger deliverFrame, %s, %dx%d(%s), length(%d)",
                getFormatStr(outFrame.format),
                outFrame.additionalInfo.video.width,
                outFrame.additionalInfo.video.height,
                outFrame.additionalInfo.video.isKeyFrame ? "key" : "delta",
                outFrame.length);

        dump(outFrame.payload, outFrame.length, outFrame.additionalInfo.video.isKeyFrame, m_bsDumpfp);

        *pOutFrame = outFrame;
    }
}

void HEVCTilesMerger::setFoV(int32_t yaw, int32_t pitch) {
    boost::unique_lock<boost::shared_mutex> ulock(m_mutex);
    ELOG_DEBUG("setFoV, yaw %d(%d), pitch %d", yaw, (yaw - 180), pitch);

    if (m_yaw != (yaw - 180) || m_pitch != pitch) {
        m_yaw = (yaw - 180);
        m_pitch = pitch;

        m_fovUpdated = true;
    }
}

void HEVCTilesMerger::initDump() {
    if (m_enableBsDump) {
        char dumpFileName[128];

        snprintf(dumpFileName, 128, "/tmp/stitcher-hi-res-%p.%s", this, "hevc");
        m_bsDumpfp_hi_res = fopen(dumpFileName, "wb");
        if (m_bsDumpfp_hi_res) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }

        snprintf(dumpFileName, 128, "/tmp/stitcher-low-res-%p.%s", this, "hevc");
        m_bsDumpfp_low_res = fopen(dumpFileName, "wb");
        if (m_bsDumpfp_low_res) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }

        snprintf(dumpFileName, 128, "/tmp/stitcher-output-%p.%s", this, "hevc");
        m_bsDumpfp = fopen(dumpFileName, "wb");
        if (m_bsDumpfp) {
            ELOG_DEBUG("Enable bitstream dump, %s", dumpFileName);
        } else {
            ELOG_DEBUG("Can not open dump file, %s", dumpFileName);
        }
    }
}

void HEVCTilesMerger::dump(uint8_t *buf, int len, bool isKey, FILE *fp)
{
    if (fp) {
        fwrite(&len, 1, 4, fp);
        fwrite(&isKey, 1, 1, fp);
        fwrite(buf, 1, len, fp);
        fflush(fp);
    }
}

}//namespace owt_base

#endif
