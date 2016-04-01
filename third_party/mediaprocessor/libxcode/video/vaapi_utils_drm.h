/* <COPYRIGHT_TAG> */

#ifndef __VAAPI_UTILS_DRM_H__
#define __VAAPI_UTILS_DRM_H__

#if defined(LIBVA_DRM_SUPPORT)

#include <va/va_drm.h>
#include "vaapi_utils.h"

class MyLibVA: public CLibVA
{
};

class DRMLibVA : public CLibVA
{
public:
    DRMLibVA(void);
    virtual ~DRMLibVA(void);

protected:
    int m_fd;

private:
    DISALLOW_COPY_AND_ASSIGN(DRMLibVA);
};

#endif // #if defined(LIBVA_DRM_SUPPORT)

#endif // #ifndef __VAAPI_UTILS_DRM_H__
