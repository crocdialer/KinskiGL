//
//  SyphonConnector.h
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#pragma once

#include "gl/gl.hpp"

namespace kinski{ namespace syphon{
    
    class Output
    {
     public:
        Output(){};
        Output(const std::string &theName);
        
        void publish_texture(const gl::Texture &theTexture);
        void setName(const std::string &theName);
        std::string getName();
        
     private:
        
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_obj.get() != nullptr; }
        void reset() { m_obj.reset(); }
    };
    
    struct ServerDescription
    {
        std::string name;
        std::string app_name;
    };
    
    class Input
    {
    public:
        
        static std::vector<ServerDescription> get_inputs();
        
        Input(){};
        Input(uint32_t the_index);
        
        bool has_new_frame();
        bool copy_frame(gl::Texture &tex);
        
        ServerDescription description();
        
    private:
        
        struct Obj;
        typedef std::shared_ptr<Obj> ObjPtr;
        ObjPtr m_obj;
        
        //! Emulates shared_ptr-like behavior
        operator bool() const { return m_obj.get() != nullptr; }
        void reset() { m_obj.reset(); }
    };
    
    class SyphonNotRunningException: public Exception
    {
    public:
        SyphonNotRunningException() :
        Exception("No Syphon server running"){}
    };
    
    class SyphonInputOutOfBoundsException: public Exception
    {
    public:
        SyphonInputOutOfBoundsException() :
        Exception("Requested SyphonInput out of bounds"){}
    };
    
}}//namespace