/*
 * Copyright 2017 Intel Corporation All Rights Reserved.
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

