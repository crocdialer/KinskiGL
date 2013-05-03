//
//  SyphonConnector.h
//  kinskiGL
//
//  Created by Fabian on 5/3/13.
//
//

#ifndef __kinskiGL__SyphonConnector__
#define __kinskiGL__SyphonConnector__

#include "kinskiGL/KinskiGL.h"

namespace kinski{ namespace gl{
    
class SyphonConnector
{
 public:
    SyphonConnector();
    SyphonConnector(const std::string &theName);
    
    void publish_framebuffer(const Fbo &theFbo);
    void publish_texture(const Texture &theTexture);
    void setName(const std::string &theName);
    
 private:
    
    void open_server(const std::string &theName);
    
    struct Obj;
    typedef std::shared_ptr<Obj> ObjPtr;
    ObjPtr m_obj;
    
    //! Emulates shared_ptr-like behavior
    typedef ObjPtr SyphonConnector::*unspecified_bool_type;
    operator unspecified_bool_type() const { return ( m_obj.get() == 0 ) ? 0 : &SyphonConnector::m_obj; }
    void reset() { m_obj.reset(); }
};
    
}}//namespace

#endif /* defined(__kinskiGL__SyphonConnector__) */
