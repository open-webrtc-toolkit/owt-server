/*
 * Copyright 2015 Intel Corporation All Rights Reserved.
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

#ifndef MsdkBase_h
#define MsdkBase_h

#ifdef ENABLE_MSDK

#include <boost/thread/shared_mutex.hpp>
#include <logger.h>

#include <mfxvideo++.h>

namespace woogeen_base {

#define printfLine      ELOG_TRACE(":%d-(%p)%s - Mark", __LINE__, this, __FUNCTION__)

#define printfFuncEnter ELOG_TRACE(":%d-(%p)%s ++++++++++ Enter", __LINE__, this, __FUNCTION__)
#define printfFuncExit  ELOG_TRACE(":%d-(%p)%s ---------- Exit", __LINE__, this, __FUNCTION__)

#define printfToDo      ELOG_TRACE(":%d-(%p)%s - Todo", __LINE__, this, __FUNCTION__)

enum DumpType{ MFX_DEC, MFX_VPP, MFX_ENC };

void printfFrameInfo(mfxFrameInfo *pFrameInfo);
void printfVideoParam(mfxVideoParam *pVideoParam, DumpType type);
void printfFrameAllocRequest(mfxFrameAllocRequest *pRequest);

class MsdkBase {
    DECLARE_LOGGER();

public:
    ~MsdkBase();

    static MsdkBase *get(void);

    MFXVideoSession *createSession(void);
    void destroySession(MFXVideoSession *pSession);

    boost::shared_ptr<mfxFrameAllocator> createFrameAllocator(void);
    void destroyFrameAllocator(mfxFrameAllocator *pAlloc);

protected:
    MsdkBase();

    bool init();
    MFXVideoSession *createSession_internal(void);

private:
    static MsdkBase *sSingleton;
    static boost::shared_mutex sSingletonLock;

    int m_fd;
    void *m_vaDisp;

    MFXVideoSession *m_mainSession;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkBase_h */

