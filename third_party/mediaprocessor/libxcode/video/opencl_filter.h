/* <COPYRIGHT_TAG> */
#ifdef ENABLE_VA
#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include <CL/cl.h>

#include <base/logger.h>
#include "mfxvideo++.h"

#define INIT_CL_EXT_FUNC(x) x = (x ## _fn)clGetExtensionFunctionAddress(#x);
#define SAFE_OCL_FREE(P, FREE_FUNC) { if (P) { FREE_FUNC(P); P = NULL; } }

typedef struct {
    std::string program_source;
    std::string kernelY_FuncName;
    std::string kernelUV_FuncName;
    mfxU32      format;
    cl_program  clprogram;
    cl_kernel   clkernelY;
    cl_kernel   clkernelUV;
} OCL_YUV_kernel;

class OpenCLFilter
{
public:
    virtual ~OpenCLFilter() {}

    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc) = 0;

    virtual cl_int OCLInit(mfxHDL pD3DDeviceManager) = 0;
    virtual cl_int AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name, mfxU32 format) = 0;
    virtual cl_int SelectKernel(unsigned kNo) = 0;
    virtual cl_int ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut) = 0;
    virtual unsigned char* GetImageData() = 0;
    virtual unsigned char* GetGrayImageData() = 0;
    virtual cl_int MapImage() = 0;
    virtual cl_int UnmapImage() = 0;
};

class OpenCLFilterBase: public OpenCLFilter
{
public:
	DECLARE_MLOGINSTANCE();
    OpenCLFilterBase();
    virtual ~OpenCLFilterBase();

    virtual mfxStatus SetAllocator(mfxFrameAllocator *pAlloc);

    virtual cl_int OCLInit(mfxHDL device);
    virtual cl_int AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name, mfxU32 format);
    virtual cl_int SelectKernel(unsigned kNo); // cl
    virtual cl_int ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut);
    virtual unsigned char* GetImageData();
    virtual unsigned char* GetGrayImageData();
    virtual cl_int MapImage();
    virtual cl_int UnmapImage();

protected: // functions
    virtual cl_int InitPlatform();
    virtual cl_int BuildKernels();
    virtual cl_int SetKernelArgs();
    virtual cl_int ReleaseResources();

    virtual cl_int InitDevice() = 0;
    virtual cl_int InitSurfaceSharingExtension() = 0; // vaapi, d3d9, d3d11, etc. specific
    virtual cl_int PrepareSharedSurfaces(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut) = 0;
    virtual cl_int ProcessSurface() = 0;
    virtual cl_int UpdateBackground() = 0;

    inline size_t chooseLocalSize(
        size_t globalSize, // frame width or height
        size_t preferred)  // preferred local size
    {
        size_t ret = 1;
        while ((globalSize % ret == 0) && ret <= preferred)
        {
             ret <<= 1;
        }

        return ret >> 1;
    }

protected: // variables
    bool                m_bInit;
    mfxFrameAllocator*  m_pAlloc;
    cl_platform_id      m_clplatform;
    cl_device_id        m_cldevice;
    cl_context          m_clcontext;
    cl_command_queue    m_clqueue;

    int                                 m_activeKernel;
    int                                 m_currentWidth;
    int                                 m_currentHeight;

    std::vector<OCL_YUV_kernel>         m_kernels;

    static const size_t c_shared_surfaces_num = 2; // In and Out
    static const size_t c_ocl_surface_buffers_num = 2*c_shared_surfaces_num; // YIn, UVIn, YOut, UVOut

    cl_mem              m_clbuffer[c_ocl_surface_buffers_num];

    size_t              m_GlobalWorkSizeY[2];
    size_t              m_GlobalWorkSizeUV[2];
    size_t              m_LocalWorkSizeY[2];
    size_t              m_LocalWorkSizeUV[2];

    unsigned char*      m_pbgImage;
    cl_mem              m_clbgImage;
    size_t              m_bgImageSize;

    unsigned char*      m_pImage; // Output the image data
    cl_mem              m_clImage;
    size_t              m_ImageSize;

    unsigned char*      m_pGrayImage; // Output the gray data
    cl_mem              m_clGrayImage;
    size_t              m_GrayImageSize;

    bool                m_bBackgroundUpdated;
};

std::string readFile(const char *filename);
#endif
