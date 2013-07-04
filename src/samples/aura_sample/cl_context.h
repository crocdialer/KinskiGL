//
//  cl_util.h
//  kinskiGL
//
//  Created by Fabian on 5/10/13.
//
//

#ifndef __kinskiGL__cl_context__
#define __kinskiGL__cl_context__

#define __CL_ENABLE_EXCEPTIONS
#include "cl.hpp"

namespace kinski {
    
    class cl_context
    {
    public:
        void init();
        
        cl::Context& context(){return m_context;}
        cl::Device& device(){return m_device;};
        cl::CommandQueue& queue(){return m_queue;}
        cl::Program& program(){return m_program;}
        
        void set_sources(const std::string &path);
        void set_sources(const cl::Program::Sources &sources);
        
    private:
        //OpenCL standard stuff
        cl::Context m_context;
        cl::Device m_device;
        cl::CommandQueue m_queue;
        cl::Program m_program;
    };
    
// Helper function to get error string
// *********************************************************************
    const char* oclErrorString(int error);
}
#endif /* defined(__kinskiGL__cl_util__) */
