/* <COPYRIGHT_TAG> */

#include "opencl_filter_va.h"

#ifdef ENABLE_VA
#if !defined(_WIN32) && !defined(_WIN64)

using std::endl;

clGetDeviceIDsFromVA_APIMediaAdapterINTEL_fn    clGetDeviceIDsFromVA_APIMediaAdapterINTEL = NULL;
clCreateFromVA_APIMediaSurfaceINTEL_fn          clCreateFromVA_APIMediaSurfaceINTEL       = NULL;
clEnqueueAcquireVA_APIMediaSurfacesINTEL_fn     clEnqueueAcquireVA_APIMediaSurfacesINTEL  = NULL;
clEnqueueReleaseVA_APIMediaSurfacesINTEL_fn     clEnqueueReleaseVA_APIMediaSurfacesINTEL  = NULL;

DEFINE_MLOGINSTANCE_CLASS(OpenCLFilterVA, "OpenCLFilterVA");
OpenCLFilterVA::OpenCLFilterVA()
{
    for(size_t i = 0; i < c_shared_surfaces_num; i++) {
        m_SharedSurfaces[i] = VA_INVALID_ID;
    }
}

OpenCLFilterVA::~OpenCLFilterVA() {
}

cl_int OpenCLFilterVA::OCLInit(mfxHDL device)
{
    if (device == NULL) {
        MLOG_ERROR("OCLInit failed, null pointer\n");
        return MFX_ERR_NULL_PTR;
    }

    m_vaDisplay = (VADisplay*)device;

    return OpenCLFilterBase::OCLInit(device);
}

cl_int OpenCLFilterVA::InitSurfaceSharingExtension()
{
    cl_int error = CL_SUCCESS;

    // Hook up the d3d sharing extension functions that we need
    INIT_CL_EXT_FUNC(clGetDeviceIDsFromVA_APIMediaAdapterINTEL);
    INIT_CL_EXT_FUNC(clCreateFromVA_APIMediaSurfaceINTEL);
    INIT_CL_EXT_FUNC(clEnqueueAcquireVA_APIMediaSurfacesINTEL);
    INIT_CL_EXT_FUNC(clEnqueueReleaseVA_APIMediaSurfacesINTEL);

    // Check for success
    if (!clGetDeviceIDsFromVA_APIMediaAdapterINTEL ||
        !clCreateFromVA_APIMediaSurfaceINTEL ||
        !clEnqueueAcquireVA_APIMediaSurfacesINTEL ||
        !clEnqueueReleaseVA_APIMediaSurfacesINTEL) {
        MLOG_ERROR("OpenCLFilter: Couldn't get all of the media sharing routines\n");
        return CL_INVALID_PLATFORM;
    }

    return error;
}

cl_int OpenCLFilterVA::InitDevice()
{
    cl_int error = CL_SUCCESS;
    MLOG_INFO("OpenCLFilter: Try to init OCL device\n");

    cl_uint nDevices = 0;
    error = clGetDeviceIDsFromVA_APIMediaAdapterINTEL(m_clplatform, CL_VA_API_DISPLAY_INTEL,
                                        m_vaDisplay, CL_PREFERRED_DEVICES_FOR_VA_API_INTEL, 1, &m_cldevice, &nDevices);
    if(error) {
        MLOG_ERROR("OpenCLFilter: clGetDeviceIDsFromVA_APIMediaAdapterINTEL failed. Return code: %x\n", error);
        return error;
    }

    if (!nDevices)
        return CL_INVALID_PLATFORM;


    // Initialize the shared context
    cl_context_properties props[] = { CL_CONTEXT_VA_API_DISPLAY_INTEL, (cl_context_properties) m_vaDisplay, CL_CONTEXT_INTEROP_USER_SYNC, 1, 0};
    m_clcontext = clCreateContext(props, 1, &m_cldevice, NULL, NULL, &error);

    if(error) {
        MLOG_ERROR("OpenCLFilter: clCreateContext failed. Return code: %x\n", error);
        return error;
    }

    MLOG_INFO("OpenCLFilter: OCL device inited\n");

    return error;
}

cl_int OpenCLFilterVA::PrepareSharedSurfaces(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut)
{
    cl_int error = CL_SUCCESS;
    mfxStatus sts = MFX_ERR_NONE;
    VASurfaceID* inputD3DSurf = NULL;
    VASurfaceID* outputD3DSurf = NULL;

    sts = m_pAlloc->GetHDL(m_pAlloc->pthis, pSurfIn, reinterpret_cast<mfxHDL*>(&inputD3DSurf));
    if(MFX_ERR_NONE != sts) { return CL_INVALID_VALUE; }

    if (pSurfOut != NULL) {
        sts = m_pAlloc->GetHDL(m_pAlloc->pthis, pSurfOut, reinterpret_cast<mfxHDL*>(&outputD3DSurf));
        if(MFX_ERR_NONE != sts) return CL_INVALID_VALUE;
    }

    if(m_bInit) {
        m_currentWidth = width;
        m_currentHeight = height;
        // Setup OpenCL buffers etc.
        if(!m_clbuffer[0]) // Initialize OCL buffers in case of new workload
        {
            if(m_kernels[m_activeKernel].format == MFX_FOURCC_NV12)
            {
                // Associate the shared buffer with the kernel object
                m_clbuffer[0] = clCreateFromVA_APIMediaSurfaceINTEL(m_clcontext, CL_MEM_READ_ONLY, inputD3DSurf, 0, &error);
                if(error) {
                    MLOG_ERROR("clCreateFromVA_APIMediaSurfaceINTEL failed. Return code: %x\n", error);
                    return error;
                }
                m_clbuffer[1] = clCreateFromVA_APIMediaSurfaceINTEL(m_clcontext, CL_MEM_READ_ONLY, inputD3DSurf, 1, &error);
                if(error) {
                    MLOG_ERROR("clCreateFromVA_APIMediaSurfaceINTEL failed. Return code: %x\n", error);
                    return error;
                }

                if (pSurfOut) {
                    m_clbuffer[2] = clCreateFromVA_APIMediaSurfaceINTEL(m_clcontext, CL_MEM_WRITE_ONLY, outputD3DSurf, 0, &error);
                    if(error) {
                        MLOG_ERROR("clCreateFromVA_APIMediaSurfaceINTEL failed. Return code: %x\n", error);
                        return error;
                    }
                    m_clbuffer[3] = clCreateFromVA_APIMediaSurfaceINTEL(m_clcontext, CL_MEM_WRITE_ONLY, outputD3DSurf, 1, &error);
                    if(error) {
                        MLOG_ERROR("clCreateFromVA_APIMediaSurfaceINTEL failed. Return code: %x\n", error);
                        return error;
                    }
                }

                // Work sizes for Y plane
                m_GlobalWorkSizeY[0] = m_currentWidth;
                m_GlobalWorkSizeY[1] = m_currentHeight;
                m_LocalWorkSizeY[0] = chooseLocalSize(m_GlobalWorkSizeY[0], 8);
                m_LocalWorkSizeY[1] = chooseLocalSize(m_GlobalWorkSizeY[1], 8);
                m_GlobalWorkSizeY[0] = m_LocalWorkSizeY[0]*(m_GlobalWorkSizeY[0]/m_LocalWorkSizeY[0]);
                m_GlobalWorkSizeY[1] = m_LocalWorkSizeY[1]*(m_GlobalWorkSizeY[1]/m_LocalWorkSizeY[1]);

                // Work size for UV plane
                m_GlobalWorkSizeUV[0] = m_currentWidth/2;
                m_GlobalWorkSizeUV[1] = m_currentHeight/2;
                m_LocalWorkSizeUV[0] = chooseLocalSize(m_GlobalWorkSizeUV[0], 8);
                m_LocalWorkSizeUV[1] = chooseLocalSize(m_GlobalWorkSizeUV[1], 8);
                m_GlobalWorkSizeUV[0] = m_LocalWorkSizeUV[0]*(m_GlobalWorkSizeUV[0]/m_LocalWorkSizeUV[0]);
                m_GlobalWorkSizeUV[1] = m_LocalWorkSizeUV[1]*(m_GlobalWorkSizeUV[1]/m_LocalWorkSizeUV[1]);

                error = SetKernelArgs();
                if (error) return error;
            } else {
                MLOG_ERROR("OpenCLFilter: Unsupported image format\n");
                return CL_INVALID_VALUE;
            }
        }
        return error;
    }
    else
        return CL_DEVICE_NOT_FOUND;
}

cl_int OpenCLFilterVA::ProcessSurface()
{
    cl_int error = CL_SUCCESS;

    if(!m_bInit)
        error = CL_DEVICE_NOT_FOUND;

    if(m_clbuffer[0] && CL_SUCCESS == error)
    {
        if(m_kernels[m_activeKernel].format == MFX_FOURCC_NV12)
        {
            cl_mem    surfaces[2] ={m_clbuffer[0],
                                    m_clbuffer[1]};

            error = clEnqueueAcquireVA_APIMediaSurfacesINTEL(m_clqueue,2,surfaces,0,NULL,NULL);
            if(error) {
                MLOG_ERROR("clEnqueueAcquireVA_APIMediaSurfacesINTEL failed. Return code: %x\n", error);
                return error;
            }

            // enqueue kernels
            error = clEnqueueNDRangeKernel(m_clqueue, m_kernels[m_activeKernel].clkernelY, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY, 0, NULL, NULL);
            if(error) {
                MLOG_ERROR("clEnqueueNDRangeKernel for Y plane failed. Return code: %x\n", error);
                return error;
            }

            if (m_bBackgroundUpdated == false) {
                error = clEnqueueNDRangeKernel(m_clqueue, m_kernels[m_activeKernel].clkernelUV, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY, 0, NULL, NULL);
                if(error) {
                    MLOG_ERROR("clEnqueueNDRangeKernel for UV plane failed. Return code: %x\n", error);
                    return error;
                }
            } else {
                error = clEnqueueNDRangeKernel(m_clqueue, m_kernels[m_activeKernel+1].clkernelY, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY, 0, NULL, NULL);
                if(error) {
                    MLOG_ERROR("clEnqueueNDRangeKernel for Y failed. Return code: %x\n", error);
                    return error;
                }
            }

            error = clEnqueueReleaseVA_APIMediaSurfacesINTEL(m_clqueue,2, surfaces,0,NULL,NULL);
            if(error) {
                MLOG_ERROR("clEnqueueReleaseVA_APIMediaSurfacesINTEL failed. Return code: %x\n", error);
                return error;
            }

            // flush & finish the command queue
            error = clFlush(m_clqueue);
            if(error) {
                MLOG_ERROR("clFlush failed. Return code: %x\n",  error);
                return error;
            }

            error = clFinish(m_clqueue);
            if(error) {
                MLOG_ERROR("clFinish failed. Return code: %x\n", error);
                return error;
            }
        }
    }
    return error;
}

cl_int OpenCLFilterVA::UpdateBackground()
{
    cl_int error = CL_SUCCESS;
    //TODO: update the background, copy it direct from image.
    if (m_bBackgroundUpdated == false) {
         if (m_GrayImageSize == m_bgImageSize) {
         // copy the data from first picture as background.
             error = clEnqueueCopyBuffer(m_clqueue, m_clGrayImage, m_clbgImage, 0, 0, m_bgImageSize, 0, NULL, NULL);
             if (error) {
                 MLOG_ERROR("Copy the first gray image as background image failed\n");
             }
             // flush & finish the command queue
             error = clFlush(m_clqueue);
             if(error) {
                 MLOG_ERROR("clFlush failed. Return code: %x\n",  error);
                 return error;
             }

             error = clFinish(m_clqueue);
             if(error) {
                 MLOG_ERROR("clFinish failed. Return code: %x\n", error);
                 return error;
             }
          } else {
              MLOG_WARNING(" the background image size isn't compatible with gray image size\n");
          }
    }

    m_bBackgroundUpdated = true;
    return error;
}
#endif // #if !defined(_WIN32) && !defined(_WIN64)
#endif
