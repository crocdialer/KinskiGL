//
//  SyphonConnector.cpp
//  kinskiGL
//
//  Created by Fabian on 5/3/13.
//
//

#include "SyphonConnector.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Fbo.h"
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
            [m_syphon_server stop];
            //[m_syphon_server release];
            [m_syphon_server dealloc];
        }
    };
    
    SyphonConnector::SyphonConnector()
    {
    
    }
    
    SyphonConnector::SyphonConnector(const std::string &theName)
    {
        setName(theName);
    }
    
    void SyphonConnector::publish_framebuffer(const Fbo &theFbo)
    {
        if(!m_obj) throw Exception("no Syphon server running");
        
        NSRect rect = NSMakeRect(0, 0, theFbo.getWidth(), theFbo.getHeight());
        NSSize size = NSMakeSize(theFbo.getWidth(), theFbo.getHeight());
        
        [m_obj->m_syphon_server publishFramebuffer:theFbo.getId() imageRegion:rect textureDimensions:size];
    }
    
    void SyphonConnector::publish_texture(const Texture &theTexture)
    {
        if(!m_obj) throw Exception("no Syphon server running");
        
        NSRect rect = NSMakeRect(0, 0, theTexture.getWidth(), theTexture.getHeight());
        NSSize size = NSMakeSize(theTexture.getWidth(), theTexture.getHeight());
        
        // not working with OpenGL 3.2
        [m_obj->m_syphon_server publishFrameTexture:theTexture.getId()
                                textureTarget:theTexture.getTarget()
                                imageRegion:rect
                                textureDimensions:size flipped:theTexture.isFlipped()];
    }
    
    void SyphonConnector::setName(const std::string &theName)
    {
        if(!m_obj){ m_obj = ObjPtr(new Obj(theName));}
        else
        {
            [m_obj->m_syphon_server setName:[NSString stringWithUTF8String:theName.c_str()]];
        }
    }
    
}}//namespace