//
//  SyphonConnector.h
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#ifndef __gl__SyphonConnector__
#define __gl__SyphonConnector__

#include "gl/KinskiGL.h"
#include "core/Component.h"

namespace kinski{ namespace gl{
    
    class SyphonConnector
    {
     public:
        SyphonConnector(){};
        SyphonConnector(const std::string &theName);
        
        void publish_texture(const Texture &theTexture);
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
    
    struct SyphonServerDescription
    {
        std::string name;
        std::string app_name;
    };
    
    class SyphonInput
    {
    public:
        
        static std::vector<SyphonServerDescription> get_inputs();
        
        SyphonInput(){};
        SyphonInput(uint32_t the_index);
        
        bool has_new_frame();
        bool copy_frame(gl::Texture &tex);
        
        SyphonServerDescription description();
        
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

#endif /* defined(__gl__SyphonConnector__) */
