//
//  cl_context.cpp
//  gl
//
//  Created by Fabian on 7/1/13.
//
//

#include "cl_context.h"
#include "crocore/filesystem.hpp"

#if defined(linux) || defined(__linux) || defined(__linux__)
  #include <GL/glx.h>
#elif defined (__APPLE__) || defined(MACOSX)
#include <OpenGL/CGLCurrent.h>
#endif

using namespace std;

namespace kinski
{
    void cl_context::init()
    {
        // OpenCL
        try
        {
            // Get available platforms
            vector<cl::Platform> platforms;
            cl::Platform::get(&platforms);
            LOG_INFO<<platforms.front().getInfo<CL_PLATFORM_VERSION>();
            
            // context sharing is OS specific
#if defined (__APPLE__) || defined(MACOSX)
            CGLContextObj curCGLContext = CGLGetCurrentContext();
            CGLShareGroupObj curCGLShareGroup = CGLGetShareGroup(curCGLContext);
            
            cl_context_properties properties[] =
            {
                CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                (cl_context_properties)curCGLShareGroup,
                0
            };
#elif defined WIN32
            cl_context_properties properties[] =
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                0
            };
#else
            cl_context_properties properties[] =
            {
                CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
                CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
                CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
                0
            };
#endif
            
            m_context = cl::Context(CL_DEVICE_TYPE_GPU, properties);
            
            // Get a list of devices on this platform
            vector<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
            m_device = devices[0];
            
            // Create a command queue and use the first device
            m_queue = cl::CommandQueue(m_context, m_device);
            
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
        }
    }
    
    void cl_context::set_sources(const std::string &path)
    {
        try
        {
            cl::Program::Sources sources;
            std::string file_content = crocore::fs::read_file(path);
            sources.push_back(std::make_pair(file_content.c_str(), file_content.size() + 1));
            set_sources(sources);
        }
        catch(crocore::fs::FileNotFoundException &e){LOG_ERROR<<e.what();}
    }
    
    void cl_context::set_sources(const cl::Program::Sources &sources)
    {
        try
        {
            // Make program of the source code in the context
            m_program = cl::Program(m_context, sources);
            
            // Build program for these specific devices
            m_program.build();
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            LOG_ERROR << "Build Status: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(m_device);
            LOG_ERROR << "Build Options:\t" << m_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(m_device);
            LOG_ERROR << "Build Log:\t " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device);
        }
    }
    
    const char* oclErrorString(int error)
    {
        static const char* errorString[] = {
            "CL_SUCCESS",
            "CL_DEVICE_NOT_FOUND",
            "CL_DEVICE_NOT_AVAILABLE",
            "CL_COMPILER_NOT_AVAILABLE",
            "CL_MEM_OBJECT_ALLOCATION_FAILURE",
            "CL_OUT_OF_RESOURCES",
            "CL_OUT_OF_HOST_MEMORY",
            "CL_PROFILING_INFO_NOT_AVAILABLE",
            "CL_MEM_COPY_OVERLAP",
            "CL_IMAGE_FORMAT_MISMATCH",
            "CL_IMAGE_FORMAT_NOT_SUPPORTED",
            "CL_BUILD_PROGRAM_FAILURE",
            "CL_MAP_FAILURE",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "CL_INVALID_VALUE",
            "CL_INVALID_DEVICE_TYPE",
            "CL_INVALID_PLATFORM",
            "CL_INVALID_DEVICE",
            "CL_INVALID_CONTEXT",
            "CL_INVALID_QUEUE_PROPERTIES",
            "CL_INVALID_COMMAND_QUEUE",
            "CL_INVALID_HOST_PTR",
            "CL_INVALID_MEM_OBJECT",
            "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
            "CL_INVALID_IMAGE_SIZE",
            "CL_INVALID_SAMPLER",
            "CL_INVALID_BINARY",
            "CL_INVALID_BUILD_OPTIONS",
            "CL_INVALID_PROGRAM",
            "CL_INVALID_PROGRAM_EXECUTABLE",
            "CL_INVALID_KERNEL_NAME",
            "CL_INVALID_KERNEL_DEFINITION",
            "CL_INVALID_KERNEL",
            "CL_INVALID_ARG_INDEX",
            "CL_INVALID_ARG_VALUE",
            "CL_INVALID_ARG_SIZE",
            "CL_INVALID_KERNEL_ARGS",
            "CL_INVALID_WORK_DIMENSION",
            "CL_INVALID_WORK_GROUP_SIZE",
            "CL_INVALID_WORK_ITEM_SIZE",
            "CL_INVALID_GLOBAL_OFFSET",
            "CL_INVALID_EVENT_WAIT_LIST",
            "CL_INVALID_EVENT",
            "CL_INVALID_OPERATION",
            "CL_INVALID_GL_OBJECT",
            "CL_INVALID_BUFFER_SIZE",
            "CL_INVALID_MIP_LEVEL",
            "CL_INVALID_GLOBAL_WORK_SIZE",
        };
        
        const int errorCount = sizeof(errorString) / sizeof(errorString[0]);
        
        const int index = -error;
        
        return (index >= 0 && index < errorCount) ? errorString[index] : "";
        
    }
}
