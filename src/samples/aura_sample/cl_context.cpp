//
//  cl_context.cpp
//  kinskiGL
//
//  Created by Fabian on 7/1/13.
//
//

#include "cl_context.h"
#include "kinskiCore/Definitions.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

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
            
            m_context = cl::Context( CL_DEVICE_TYPE_GPU, properties);
            
            // Get a list of devices on this platform
            vector<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
            m_device = devices[0];
            
            // Create a command queue and use the first device
            m_queue = cl::CommandQueue(m_context, devices[0]);
            
            // Read source file
            std::string sourceCode = kinski::readFile("kernels.cl");
            
            // Read source file(s)
//            cl::Program::Sources sources;
//            std::list<std::string> cl_files = getDirectoryEntries(".", false, "cl");
//            std::list<std::string>::iterator it = cl_files.begin();
//            for (; it != cl_files.end(); ++it)
//            {
//                std::string file_content = kinski::readFile(*it);
//                sources.push_back(std::make_pair(file_content.c_str(), file_content.size() + 1));
//            }
            
            // Make program of the source code in the context
            m_program = cl::Program(m_context, sourceCode);
            
            // Build program for these specific devices
            m_program.build();
            
//            m_particleKernel = cl::Kernel(m_program, "updateParticles");
//            m_imageKernel = cl::Kernel(m_program, "set_colors_from_image");
        }
        catch(cl::Error &error)
        {
            LOG_ERROR << error.what() << "(" << oclErrorString(error.err()) << ")";
            LOG_ERROR << "Build Status: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(m_device);
            LOG_ERROR << "Build Options:\t" << m_program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(m_device);
            LOG_ERROR << "Build Log:\t " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device);
        }
    }
}
