/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA
#pragma once

#if !defined(_WIN32) && !defined(_WIN64)

#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <va/va.h>

#define DCL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.h>
#include <CL/opencl.h>
#include <CL/va_ext.h>

#include <base/logger.h>
#include "opencl_filter.h"

class OpenCLFilterVA : public OpenCLFilterBase
{
public:
	DECLARE_MLOGINSTANCE();
    OpenCLFilterVA();
    virtual ~OpenCLFilterVA();
    virtual cl_int OCLInit(mfxHDL device);

protected: // functions
    cl_int InitDevice();
    cl_int InitSurfaceSharingExtension();
    cl_int PrepareSharedSurfaces(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut);
    cl_int ProcessSurface();
    cl_int UpdateBackground();

protected: // variables
    VADisplay m_vaDisplay;
    VASurfaceID m_SharedSurfaces[c_shared_surfaces_num];
};

#endif // #if !defined(_WIN32) && !defined(_WIN64)
#endif
