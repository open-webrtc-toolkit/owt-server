// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MsdkBase_h
#define MsdkBase_h

#ifdef ENABLE_MSDK

#include <boost/thread/shared_mutex.hpp>
#include <logger.h>

#include <mfxdefs.h>
#include <mfxvideo++.h>
#include <mfxplugin++.h>
#include <mfxvp8.h>

#ifndef MFX_VERSION
#define MFX_VERSION (MFX_VERSION_MAJOR * 1000 + MFX_VERSION_MINOR)
#endif

namespace woogeen_base {

#define ALIGN16(x) ((((x) + 15) >> 4) << 4)

enum DumpType{ MFX_DEC, MFX_VPP, MFX_ENC };

class MsdkBase {
    DECLARE_LOGGER();

public:
    ~MsdkBase();

    static MsdkBase *get(void);

    bool isSupportedDecoder(mfxU32 codecId);
    bool isSupportedEncoder(mfxU32 codecId);

    void setConfigHevcEncoderGaccPlugin(bool hevcEncoderGaccPlugin);
    bool getConfigHevcEncoderGaccPlugin();

    void setConfigMFETimeout(uint32_t MFETimeout);
    uint32_t getConfigMFETimeout();

    MFXVideoSession *createSession();
    void destroySession(MFXVideoSession *pSession);

    bool loadDecoderPlugin(uint32_t codecId, MFXVideoSession *pSession, mfxPluginUID *pluginID);
    bool loadEncoderPlugin(uint32_t codecId, MFXVideoSession *pSession, mfxPluginUID *pluginID);
    void unLoadPlugin(MFXVideoSession *pSession, mfxPluginUID *pluginID);

    boost::shared_ptr<mfxFrameAllocator> createFrameAllocator(void);
    void destroyFrameAllocator(mfxFrameAllocator *pAlloc);

    MFXVideoSession *getMainSession() {return m_mainSession;};

    static void printfFrameInfo(mfxFrameInfo *pFrameInfo);
    static void printfVideoParam(mfxVideoParam *pVideoParam, DumpType type);
    static void printfFrameAllocRequest(mfxFrameAllocRequest *pRequest);

protected:
    MsdkBase();

    bool init();
    MFXVideoSession *createSession_internal(void);

private:
    static MsdkBase *sSingleton;
    static boost::shared_mutex sSingletonLock;
    static std::vector<mfxU32> sSupportedDecoders;
    static std::vector<mfxU32> sSupportedEncoders;

    int m_fd;
    void *m_vaDisp;

    MFXVideoSession *m_mainSession;

    bool m_configHevcEncoderGaccPlugin;
    uint32_t m_configMFETimeout;
};

} /* namespace woogeen_base */

#endif /* ENABLE_MSDK */
#endif /* MsdkBase_h */

