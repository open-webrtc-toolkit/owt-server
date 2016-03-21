/* <COPYRIGHT_TAG> */

#ifndef __VAAPI_ALLOCATOR_H__
#define __VAAPI_ALLOCATOR_H__


#include <stdlib.h>
#include <va/va.h>

#include "base/logger.h"
#include "base_allocator.h"

// VAAPI Allocator internal Mem ID
struct vaapiMemId
{
    VASurfaceID* m_surface;
    VAImage      m_image;
    // variables for VAAPI Allocator inernal color convertion
    unsigned int m_fourcc;
    mfxU8*       m_sys_buffer;
    mfxU8*       m_va_buffer;
};
/*
struct vaapiAllocatorParams : mfxAllocatorParams
{
    VADisplay m_dpy;
};
*/
class vaapiFrameAllocator: public BaseFrameAllocator
{
public:
	DECLARE_MLOGINSTANCE();
    vaapiFrameAllocator();
    virtual ~vaapiFrameAllocator();

    virtual mfxStatus Init(VADisplay *dpy);
    virtual mfxStatus Close();
protected:
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);

    VADisplay m_dpy;
};


#endif // __VAAPI_ALLOCATOR_H__
