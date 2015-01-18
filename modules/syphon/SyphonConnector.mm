//
//  SyphonConnector.cpp
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#include "SyphonConnector.h"
#include "gl/Texture.h"
#include "gl/Buffer.h"
#import <Syphon/Syphon.h>

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
            [m_syphon_server dealloc];
        }
    };
    
    SyphonConnector::SyphonConnector(const std::string &theName):m_obj(new Obj(theName)){}
    
    void SyphonConnector::publish_texture(const Texture &theTexture)
    {
        if(!m_obj) throw SyphonNotRunningException();
        NSRect rect = NSMakeRect(0, 0, theTexture.getWidth(), theTexture.getHeight());
        NSSize size = NSMakeSize(theTexture.getWidth(), theTexture.getHeight());
        
        // syphon won't be nice and change the viewport settings, so save them
        gl::SaveViewPort sv;

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
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    SyphonServerDescription convert_description(NSDictionary *the_desc)
    {
        // These are the keys we can use in the server description dictionary.
        NSString* name = [the_desc objectForKey:SyphonServerDescriptionNameKey];
        NSString* appName = [the_desc objectForKey:SyphonServerDescriptionAppNameKey];
        return {[name UTF8String], [appName UTF8String]};
    }
    
    std::vector<SyphonServerDescription> SyphonInput::get_inputs()
    {
        std::vector<SyphonServerDescription> ret;
        
        for(NSDictionary *description in [[SyphonServerDirectory sharedDirectory] servers])
        {
            ret.push_back(convert_description(description));
        }
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    struct SyphonInput::Obj
    {
        SyphonClient *m_syphon_client;
        SyphonImage *m_image;
        
        Obj(uint32_t the_index):
        m_image(nullptr)
        {
            NSArray *server_descriptions = [[SyphonServerDirectory sharedDirectory] servers];
            
            // index out of bounds
            if(the_index >= [server_descriptions count])
            {
                throw SyphonInputOutOfBoundsException();
            }
            
            NSDictionary *desc = [server_descriptions objectAtIndex:the_index];
            m_syphon_client = [[SyphonClient alloc] initWithServerDescription:desc
                                                                      options:nil
                                                              newFrameHandler:nil];
        }
        ~Obj()
        {
            [m_syphon_client stop];
            [m_syphon_client dealloc];
            if(m_image){ [m_image release]; }
        }
    };
    
    SyphonInput::SyphonInput(uint32_t the_index):
    m_obj(new Obj(the_index))
    {
    
    }
    
    bool SyphonInput::has_new_frame()
    {
        return m_obj && [m_obj->m_syphon_client hasNewFrame];
    }
    
    bool SyphonInput::copy_frame(gl::Texture &tex)
    {
        if(!has_new_frame()) return false;
        
        // release the last SyphonImage object, if any
        if(m_obj->m_image){ [m_obj->m_image release]; }
        
        // Get a new frame from the client
        m_obj->m_image = [m_obj->m_syphon_client newFrameImageForContext:CGLGetCurrentContext()];
        KINSKI_CHECK_GL_ERRORS();
        
        tex = gl::Texture(GL_TEXTURE_RECTANGLE,
                          [m_obj->m_image textureName],
                          [m_obj->m_image textureSize].width,
                          [m_obj->m_image textureSize].height, true);
        return true;
    }
    
    SyphonServerDescription SyphonInput::description()
    {
        return convert_description(m_obj->m_syphon_client.serverDescription);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
}}//namespace