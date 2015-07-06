/* <COPYRIGHT_TAG> */

#ifndef __VAAPI_UTILS_H__
#define __VAAPI_UTILS_H__

#ifdef LIBVA_SUPPORT

#include <va/va.h>
#include "mfxdefs.h"

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    void operator=(const TypeName&)

class CLibVA
{
public:
    virtual ~CLibVA(void) {};

    VADisplay GetVADisplay(void) { return m_va_dpy; }

protected:
    CLibVA(void) :
        m_va_dpy(0)
    {}
    VADisplay m_va_dpy;

private:
    DISALLOW_COPY_AND_ASSIGN(CLibVA);
};

CLibVA* CreateLibVA(void);

mfxStatus va_to_mfx_status(VAStatus va_res);

#endif // #ifdef LIBVA_SUPPORT

#endif // #ifndef __VAAPI_UTILS_H__
