//
//  SyphonConnector.cpp
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#include "SyphonConnector.h"
#include "gl/Texture.h"
#include "gl/Fbo.h"
#import <Syphon/SyphonServer.h>

namespace kinski{ namespace gl{

    struct SyphonConnector::Obj
    {
        SyphonServer* m_syphon_server;
        
        Obj(const std::string theName)
        {
            m_syphon_server = [[SyphonServer alloc] initWithName:[NSString stringWithUTF8String:theName.c_str()]
                                                    context:CGLGetCurrentContext() options:nil];
        }
        ~Obj()
        {
            [m_syphon_server dealloc];
        }
    };
    
    SyphonConnector::SyphonConnector(const std::string &theName):m_obj(new Obj(theName)){}
    
    void SyphonConnector::publish_framebuffer(const Fbo &theFbo)
    {
        if(!m_obj) throw SyphonNotRunningException();
        NSRect rect = NSMakeRect(0, 0, theFbo.getWidth(), theFbo.getHeight());
        NSSize size = NSMakeSize(theFbo.getWidth(), theFbo.getHeight());
        [m_obj->m_syphon_server publishFramebuffer:theFbo.getId() imageRegion:rect textureDimensions:size];
    }
    
    void SyphonConnector::publish_texture(const Texture &theTexture)
    {
        if(!m_obj) throw SyphonNotRunningException();
        NSRect rect = NSMakeRect(0, 0, theTexture.getWidth(), theTexture.getHeight());
        NSSize size = NSMakeSize(theTexture.getWidth(), theTexture.getHeight());
        
        // not working with OpenGL 3.2
        // using the gl3 branch of this fork helped: git://github.com/GameClay/syphon-framework.git
        [m_obj->m_syphon_server publishFrameTexture:theTexture.getId()
                                textureTarget:theTexture.getTarget()
                                imageRegion:rect
                                textureDimensions:size flipped:theTexture.isFlipped()];
    }
    
    void SyphonConnector::setName(const std::string &theName)
    {
        if(!m_obj) throw SyphonNotRunningException();
        [m_obj->m_syphon_server setName:[NSString stringWithUTF8String:theName.c_str()]];
    }
    
    std::string SyphonConnector::getName()
    {
        if(!m_obj) throw SyphonNotRunningException();
        return [m_obj->m_syphon_server.name UTF8String];
    }
    
}}//namespace