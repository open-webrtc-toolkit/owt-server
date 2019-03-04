// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkScaler_h
#define MsdkScaler_h

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <logger.h>

#include "MsdkFrame.h"

namespace woogeen_base {

class MsdkScaler {
    DECLARE_LOGGER();

public:
    MsdkScaler();
    ~MsdkScaler();

    bool convert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame);
    bool convert(MsdkFrame *srcMsdkFrame, int srcCropX, int srcCropY, int srcCropW, int srcCropH
            , MsdkFrame *dstMsdkFrame, int dstCropX, int dstCropY, int dstCropW, int dstCropH);

protected:
    void initVppParam();
    bool createVpp();
    void destroyVpp();

    void updateVppParam(int inWidth, int inHeight, int inCropX, int inCropY, int inCropW, int inCropH,
            int outWidth, int outHeight, int outCropX, int outCropY, int outCropW, int outCropH);

    bool setupVpp(int inWidth, int inHeight, int inCropX, int inCropY, int inCropW, int inCropH,
            int outWidth, int outHeight, int outCropX, int outCropY, int outCropW, int outCropH);
    bool doVppConvert(MsdkFrame *srcMsdkFrame, MsdkFrame *dstMsdkFrame);

private:
    MFXVideoSession *m_session;
    MFXVideoVPP *m_vpp;
    boost::shared_ptr<mfxFrameAllocator> m_allocator;
    boost::scoped_ptr<mfxVideoParam> m_vppParam;
};

} /* namespace woogeen_base */

#endif /* MsdkScaler_h */

