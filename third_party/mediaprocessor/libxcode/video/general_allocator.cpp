/* <COPYRIGHT_TAG> */

#include <stdarg.h>
#include "vaapi_allocator.h"

#include "general_allocator.h"

// Wrapper on standard allocator for concurrent allocation of
// D3D and system surfaces
GeneralAllocator::GeneralAllocator()
{
};
GeneralAllocator::~GeneralAllocator()
{
};
mfxStatus GeneralAllocator::Init(VADisplay *dpy)
{
    mfxStatus sts = MFX_ERR_NONE;

    m_D3DAllocator.reset(new vaapiFrameAllocator);


    sts = m_D3DAllocator.get()->Init(dpy);


    return sts;
}
mfxStatus GeneralAllocator::Close()
{
    mfxStatus sts = MFX_ERR_NONE;
    sts = m_D3DAllocator.get()->Close();


   return sts;
}

mfxStatus GeneralAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
   // return isD3DMid(mid)?m_D3DAllocator.get()->Lock(m_D3DAllocator.get(), mid, ptr):
   //                      m_SYSAllocator.get()->Lock(m_SYSAllocator.get(),mid, ptr);
   return m_D3DAllocator.get()->Lock(m_D3DAllocator.get(), mid, ptr);
}
mfxStatus GeneralAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    //return isD3DMid(mid)?m_D3DAllocator.get()->Unlock(m_D3DAllocator.get(), mid, ptr):
    //                     m_SYSAllocator.get()->Unlock(m_SYSAllocator.get(),mid, ptr);
    return m_D3DAllocator.get()->Unlock(m_D3DAllocator.get(), mid, ptr);
}

mfxStatus GeneralAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle)
{
    //return isD3DMid(mid)?m_D3DAllocator.get()->GetHDL(m_D3DAllocator.get(), mid, handle):
    //                     m_SYSAllocator.get()->GetHDL(m_SYSAllocator.get(), mid, handle);
    return m_D3DAllocator.get()->GetHDL(m_D3DAllocator.get(), mid, handle);
}

mfxStatus GeneralAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    // try to ReleaseResponse via D3D allocator
    //return isD3DMid(response->mids[0])?m_D3DAllocator.get()->Free(m_D3DAllocator.get(),response):
    //                                   m_SYSAllocator.get()->Free(m_SYSAllocator.get(), response);
    return m_D3DAllocator.get()->Free(m_D3DAllocator.get(),response);
}
mfxStatus GeneralAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (request->Type & MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET || request->Type & MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)
    {
        sts = m_D3DAllocator.get()->Alloc(m_D3DAllocator.get(), request, response);
    }
/*
*/
    return sts;
}
