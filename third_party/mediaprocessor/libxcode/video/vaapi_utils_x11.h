/* <COPYRIGHT_TAG> */

#ifndef __VAAPI_UTILS_X11_H__
#define __VAAPI_UTILS_X11_H__

#if defined(LIBVA_X11_SUPPORT)

#include <va/va_x11.h>
#include "vaapi_utils.h"


class X11LibVA : public CLibVA
{
public:
    X11LibVA(void);
    virtual ~X11LibVA(void);

    void *GetXDisplay(void) { return m_display;}

protected:
    Display* m_display;

private:
    //DISALLOW_COPY_AND_ASSIGN(X11LibVA);
};

#endif // #if defined(LIBVA_X11_SUPPORT)

#endif // #ifndef __VAAPI_UTILS_X11_H__
