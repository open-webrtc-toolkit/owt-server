/* <COPYRIGHT_TAG> */

#include "opencl_filter.h"

#ifdef ENABLE_VA
#include <fstream>
#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using std::endl;

DEFINE_MLOGINSTANCE_CLASS(OpenCLFilterBase, "OpenCLFilterBase");
OpenCLFilterBase::OpenCLFilterBase()
{
    m_bInit              = false;
    m_pAlloc             = NULL;
    m_activeKernel       = 0;

    for(size_t i=0; i < c_ocl_surface_buffers_num;i++)
    {
        m_clbuffer[i] = NULL;
    }

    m_cldevice           = 0;
    m_clplatform         = 0;
    m_clqueue            = 0;
    m_clcontext          = 0;

    m_currentWidth       = 0;
    m_currentHeight      = 0;

    m_pImage = NULL;
    m_clImage = NULL;
    m_ImageSize = 0;

    m_pGrayImage = NULL;
    m_clGrayImage = NULL;
    m_GrayImageSize = 0;

    m_pbgImage = NULL;
    m_clbgImage = NULL;
    m_bgImageSize = 0;

    m_bBackgroundUpdated = false;
}

OpenCLFilterBase::~OpenCLFilterBase() {
    for(unsigned i=0;i<m_kernels.size();i++)
    {
        SAFE_OCL_FREE(m_kernels[i].clprogram, clReleaseProgram);
        SAFE_OCL_FREE(m_kernels[i].clkernelY, clReleaseKernel);
        SAFE_OCL_FREE(m_kernels[i].clkernelUV, clReleaseKernel);
    }

    if (m_clImage) {
        SAFE_OCL_FREE(m_clImage, clReleaseMemObject);
        m_ImageSize = 0;
    }

    if (m_clGrayImage) {
        SAFE_OCL_FREE(m_clGrayImage, clReleaseMemObject);
        m_GrayImageSize = 0;
    }

    if (m_clbgImage) {
        SAFE_OCL_FREE(m_clbgImage, clReleaseMemObject);
        m_bgImageSize = 0;
    }

    SAFE_OCL_FREE(m_clqueue,clReleaseCommandQueue);
    SAFE_OCL_FREE(m_clcontext,clReleaseContext);
}

mfxStatus OpenCLFilterBase::SetAllocator(mfxFrameAllocator *pAlloc)
{
    m_pAlloc = pAlloc;
    return MFX_ERR_NONE;
}

cl_int OpenCLFilterBase::ReleaseResources() {
    cl_int error = CL_SUCCESS;

    for(size_t i=0; i < c_ocl_surface_buffers_num; i++)
    {
        if (m_clbuffer[i])
          error = clReleaseMemObject( m_clbuffer[i] );
        if(error) {
            MLOG_ERROR("clReleaseMemObject failed. Return code: %x\n", error);
            return error;
        }
    }

    error = clFinish( m_clqueue );
    if(error)
    {
        MLOG_ERROR("clFinish failed. Return code: %x\n", error);
        return error;
    }

    for(size_t i=0;i<c_ocl_surface_buffers_num;i++)
    {
        m_clbuffer[i] = NULL;
    }

    return error;
}

cl_int OpenCLFilterBase::OCLInit(mfxHDL /*device*/)
{
    cl_int error = CL_SUCCESS;

    error = InitPlatform();
    if (error) return error;

    error = InitSurfaceSharingExtension();
    if (error) return error;

    error = InitDevice();
    if (error) return error;

    error = BuildKernels();
    if (error) return error;

    // Create a command queue
    m_clqueue = clCreateCommandQueue(m_clcontext, m_cldevice, 0, &error);
    if (error) return error;

    m_bInit = true;

    return error;
}

cl_int OpenCLFilterBase::InitPlatform()
{
    cl_int error = CL_SUCCESS;

    // Determine the number of installed OpenCL platforms
    cl_uint num_platforms = 0;
    error = clGetPlatformIDs(0, NULL, &num_platforms);
    if(error)
    {
        MLOG_ERROR("OpenCLFilter: Couldn't get platform IDs. \
                        Make sure your platform \
                        supports OpenCL and can find a proper library.\n");
        return error;
    }

    // Get all of the handles to the installed OpenCL platforms
    std::vector<cl_platform_id> platforms(num_platforms);
    error = clGetPlatformIDs(num_platforms, &platforms[0], &num_platforms);
    if(error) {
        MLOG_ERROR("OpenCLFilter: Failed to get OCL platform IDs. Return Code:%x\n", error);
        return error;
    }

    // Find the platform handle for the installed Gen driver
    const size_t max_string_size = 1024;
    char platform[max_string_size];
    for (unsigned int platform_index = 0; platform_index < num_platforms; platform_index++)
    {
        error = clGetPlatformInfo(platforms[platform_index], CL_PLATFORM_NAME, max_string_size, platform, NULL);
        if(error) return error;

        if(strcmp(platform, "Intel")) // Use only Intel platfroms
        {
            MLOG_INFO("OpenCL platform %s is used\n", platform);
            m_clplatform = platforms[platform_index];
        }
    }
    if (0 == m_clplatform)
    {
        MLOG_ERROR("OpenCLFilter: Didn't find an Intel platform!\n");
        return CL_INVALID_PLATFORM;
    }

    return error;
}

cl_int OpenCLFilterBase::BuildKernels()
{
    MLOG_INFO("OpenCLFilter: Reading and compiling OCL kernels\n");

    cl_int error = CL_SUCCESS;

    char buildOptions[] = "-I. -Werror -cl-fast-relaxed-math";

    for(unsigned int i = 0; i < m_kernels.size(); i++)
    {
        // Create a program object from the source file
        const char *program_source_buf = m_kernels[i].program_source.c_str();

        MLOG_INFO("OpenCLFilter: program source:%x\n", program_source_buf);
        m_kernels[i].clprogram = clCreateProgramWithSource(m_clcontext, 1, &program_source_buf, NULL, &error);
        if(error) {
            MLOG_ERROR("OpenCLFilter: clCreateProgramWithSource failed. Return code: %x\n", error);
            return error;
        }

        // Build OCL kernel
        error = clBuildProgram(m_kernels[i].clprogram, 1, &m_cldevice, buildOptions, NULL, NULL);
        if (error == CL_BUILD_PROGRAM_FAILURE)
        {
            size_t buildLogSize = 0;
            cl_int logStatus = clGetProgramBuildInfo (m_kernels[i].clprogram, m_cldevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
            std::vector<char> buildLog(buildLogSize + 1);
            logStatus = clGetProgramBuildInfo (m_kernels[i].clprogram, m_cldevice, CL_PROGRAM_BUILD_LOG, buildLogSize, &buildLog[0], NULL);
            MLOG_ERROR("build program failed here\n");
            return error;
        }
        else if (error)
            return error;

        // Create the kernel objects
        m_kernels[i].clkernelY = clCreateKernel(m_kernels[i].clprogram, m_kernels[i].kernelY_FuncName.c_str(), &error);
        if (error) {
            MLOG_ERROR("OpenCLFilter: clCreateKernel failed. Return code: %x\n", error);
            return error;
        }

        MLOG_INFO("Y Function build sucessfully\n");
        m_kernels[i].clkernelUV = clCreateKernel(m_kernels[i].clprogram, m_kernels[i].kernelUV_FuncName.c_str(), &error);
        if (error) {
            MLOG_ERROR("OpenCLFilter: clCreateKernel failed. Return code: %x\n", error);
            return error;
        }
    }

    MLOG_INFO("OpenCLFilter: %d kernels built\n", m_kernels.size());

    return error;
}

cl_int OpenCLFilterBase::AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name, mfxU32 format)
{
    OCL_YUV_kernel kernel;
    kernel.program_source = std::string(filename);
    kernel.kernelY_FuncName = std::string(kernelY_name);
    kernel.kernelUV_FuncName = std::string(kernelUV_name);
    kernel.format = format;
    kernel.clprogram = 0;
    kernel.clkernelY = kernel.clkernelUV = 0;
    m_kernels.push_back(kernel);
    return CL_SUCCESS;
}

cl_int OpenCLFilterBase::SelectKernel(unsigned kNo)
{
    if(m_bInit)
    {
        if(kNo < m_kernels.size())
        {
            if(m_kernels[m_activeKernel].format != m_kernels[kNo].format)
                ReleaseResources(); // Kernel format changed, OCL buffers must be released & recreated

            m_activeKernel = kNo;

            return CL_SUCCESS;
        }
        else {
            return CL_INVALID_PROGRAM;
        }
    }
    else
        return CL_DEVICE_NOT_FOUND;
}

cl_int OpenCLFilterBase::SetKernelArgs()
{
    cl_int error = CL_SUCCESS;

    // set kernelY parameters
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelY, 0, sizeof(cl_mem), &m_clbuffer[0]);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    // set kernelUV parameters
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelY, 1, sizeof(cl_mem), &m_clbuffer[1]);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelY, 2, sizeof(cl_mem), &m_clImage);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelUV, 0, sizeof(cl_mem), &m_clbuffer[0]);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelUV, 1, sizeof(cl_mem), &m_clbuffer[1]);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelUV, 2, sizeof(cl_mem), &m_clGrayImage);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }
    // set back ground parameters
    error = clSetKernelArg(m_kernels[m_activeKernel+1].clkernelY, 0, sizeof(cl_mem), &m_clbgImage);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %d\n", error);
        return error;
    }

    // set kernel Y parameters
    error = clSetKernelArg(m_kernels[m_activeKernel+1].clkernelY, 1, sizeof(cl_mem), &m_clbuffer[0]);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }

    error = clSetKernelArg(m_kernels[m_activeKernel+1].clkernelY, 2, sizeof(cl_mem), &m_clGrayImage);
    if(error) {
        MLOG_ERROR("clSetKernelArg failed. Return code: %x\n", error);
        return error;
    }
        return error;
}

cl_int OpenCLFilterBase::MapImage()
{
    cl_int error = CL_SUCCESS;
    m_pImage = (unsigned char*)clEnqueueMapBuffer(m_clqueue, m_clImage, CL_TRUE, CL_MAP_READ, 0, m_ImageSize, 0, NULL, NULL, &error);
    if (error) {
        MLOG_ERROR("Map buffer failed, Return code %d\n", error);
    }

    m_pGrayImage = (unsigned char*)clEnqueueMapBuffer(m_clqueue, m_clGrayImage, CL_TRUE, CL_MAP_READ, 0, m_GrayImageSize, 0, NULL, NULL, &error);
    if (error) {
        MLOG_ERROR("Map gray buffer failed, Return code %d\n", error);
    }

    m_pbgImage = (unsigned char*)clEnqueueMapBuffer(m_clqueue, m_clbgImage, CL_TRUE, CL_MAP_READ, 0, m_bgImageSize, 0, NULL, NULL, &error);
    if (error) {
        MLOG_ERROR("Map background buffer failed, Return code %d\n", error);
    }
    return error;
}

cl_int OpenCLFilterBase::UnmapImage()
{
    cl_int error = CL_SUCCESS;
    error = clEnqueueUnmapMemObject(m_clqueue, m_clImage, m_pImage, 0, NULL, NULL);
    if (error) {
        MLOG_ERROR("Unmap buffer failed, Return code %x\n", error);
    }

    error = clEnqueueUnmapMemObject(m_clqueue, m_clGrayImage, m_pGrayImage, 0, NULL, NULL);
    if (error) {
        MLOG_ERROR("Unmap gray buffer failed, Return code %x\n", error);
    }

    error = clEnqueueUnmapMemObject(m_clqueue, m_clbgImage, m_pbgImage, 0, NULL, NULL);
    if (error) {
        MLOG_ERROR("Unmap background buffer failed, Return code %x\n", error);
    }
    return error;
}

unsigned char* OpenCLFilterBase::GetImageData()
{
    return m_pImage;
}

unsigned char* OpenCLFilterBase::GetGrayImageData()
{
    return m_pGrayImage;
    //return m_pbgImage;
}

cl_int OpenCLFilterBase::ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut)
{
    cl_int error = CL_SUCCESS;

    if(pSurfOut == NULL) {
        if (m_ImageSize == 0) {
            //Create the image buffer for output
            m_ImageSize = width * height * 3;
            m_clImage = clCreateBuffer(m_clcontext, CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, m_ImageSize, NULL, &error);
            if (error) {
                MLOG_ERROR("create cl image failed\n");
            }
        }

        if (m_GrayImageSize == 0) {
            m_GrayImageSize = width * height;
            m_clGrayImage = clCreateBuffer(m_clcontext, CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, m_GrayImageSize, NULL, &error);
            if (error) {
                MLOG_ERROR("create gray cl image failed\n");
            }
        }
    }

    if (m_bgImageSize == 0) {
        m_bgImageSize = width * height;
        // back ground image should be created in GPU side.
        m_clbgImage = clCreateBuffer(m_clcontext, CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR, m_bgImageSize, NULL, &error);
        if (error) {
            MLOG_ERROR("create gray cl image failed\n");
        }
    }

    error = PrepareSharedSurfaces(width, height, pSurfIn, pSurfOut);
    if (error) return error;

    error = ProcessSurface();
    if (error) return error;

    error = UpdateBackground();
    if (error) return error;

    error =  ReleaseResources();
    return error;
}

#if defined(_WIN32) || defined(_WIN64)

std::string readFile(const char *filename)
{
    std::ifstream input(filename, std::ios::in | std::ios::binary);
    if(!input.good())
    {
        // look in folder with executable
        input.clear();
        const size_t module_length = 1024;
        char module_name[module_length];
        GetModuleFileNameA(0, module_name, module_length);
        char *p = strrchr(module_name, '\\');
        if (p)
        {
            strncpy_s(p + 1, module_length - (p + 1 - module_name), filename, _TRUNCATE);
            input.open(module_name, std::ios::binary);
        }
    }

    if (!input)
        return std::string("Error_opening_file_\"") + std::string(filename) + std::string("\"");

    input.seekg(0, std::ios::end);
    std::vector<char> program_source(static_cast<int>(input.tellg().seekpos()));
    input.seekg(0);

    input.read(&program_source[0], program_source.size());

    return std::string(program_source.begin(), program_source.end());
}

#else

std::string readFile(const char *filename)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("ERROR:Failed to open kernel file '%s'\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    if (fileSize == 0) {
        printf("ERROR: File to check size ");
    }
    rewind(fp);

    int stringSize = fileSize+1;
    std::string program(stringSize, '\n');
    size_t res = fread(&program[0], sizeof(char), fileSize, fp);
    if (res != fileSize) {
        printf("ERROR: failed to read file '%s'\n", filename);
        fclose(fp);
        return NULL;
    }

    program[fileSize] = '\0';
    fclose(fp);
    return program;
}

#endif // #if defined(_WIN32) || defined(_WIN64)
#endif
