// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef HEVCTilesMerger_h
#define HEVCTilesMerger_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <deque>
#include <logger.h>

#include "MediaFramePipeline.h"

#include "360SCVPViewportAPI.h"
#include "360SCVPAPI.h"

namespace owt_base {

class HEVCTilesMerger : public FrameSource, public FrameDestination {
    DECLARE_LOGGER();

    struct FrameHevcTiles {
        uint32_t hi_width;
        uint32_t hi_height;
        uint32_t hi_length;
        uint8_t *hi_payload;

        uint32_t low_width;
        uint32_t low_height;
        uint32_t low_length;
        uint8_t *low_payload;

        bool        isKey;
        uint32_t    timeStamp;

        FrameHevcTiles () {}
        FrameHevcTiles (const Frame& frame);
    };

public:
    HEVCTilesMerger();
    ~HEVCTilesMerger();

    void onFrame(const Frame&) {}
    void onFrame(const Frame&, Frame *outFrame);

    void setFoV(int32_t yaw, int32_t pitch);

protected:
    bool init(const FrameHevcTiles& tilesFrame);
    void initDump();
    void dump(uint8_t *buf, int len, bool isKey, FILE *fp);

private:
    bool    m_init;
    void    *m_handle;
    param_360SCVP  m_360scvp_param;
    uint8_t *m_inputHiBuffer;
    int m_inputHiBufferLength;
    uint8_t *m_inputLowBuffer;
    int m_inputLowBufferLength;
    uint8_t *m_outputBuffer;
    int m_outputBufferLength;
    uint8_t *m_outputSEIBuffer;
    int m_outputBufferSEILength;
    uint8_t *m_frameBuffer;
    int m_frameBufferLength;

    uint32_t m_viewport_w;
    uint32_t m_viewport_h;
    uint32_t m_viewPortFOV;

    int32_t m_yaw;
    int32_t m_pitch;
    bool m_fovUpdated;
    boost::shared_mutex m_mutex;

    bool m_enableBsDump;
    FILE *m_bsDumpfp_hi_res;
    FILE *m_bsDumpfp_low_res;
    FILE *m_bsDumpfp;
};

} /* namespace owt_base */

#endif /* HEVCTilesMerger_h */

