//
//  SyphonConnector.cpp
//  gl
//
//  Created by Fabian on 5/3/13.
//
//

#include "SyphonConnector.h"
#include "gl/Texture.hpp"
#include "gl/Buffer.hpp"
#import <Syphon/Syphon.h>

namespace kinski{ namespace syphon{

    struct OutputImpl
    {
        SyphonServer* m_syphon_server;

        OutputImpl(const std::string theName)
        {
            m_syphon_server = [[SyphonServer alloc] initWithName:[NSString stringWithUTF8String:theName.c_str()]
                                                    context:CGLGetCurrentContext() options:nil];
        }
        ~OutputImpl()
        {
            [m_syphon_server dealloc];
        }
    };
    
    Output::Output(const std::string &theName):m_impl(new OutputImpl(theName)){}
    
    void Output::publish_texture(const gl::Texture &theTexture)
    {
        if(!theTexture){ return; }
        if(!m_impl) throw SyphonNotRunningException();
        NSRect rect = NSMakeRect(0, 0, theTexture.width(), theTexture.height());
        NSSize size = NSMakeSize(theTexture.width(), theTexture.height());
        
        // syphon won't be nice and change the viewport settings, so save them
        gl::SaveViewPort sv;

        [m_impl->m_syphon_server publishFrameTexture:theTexture.id()
                                textureTarget:theTexture.target()
                                imageRegion:rect
                                textureDimensions:size flipped:theTexture.flipped()];
    }
    
    void Output::setName(const std::string &theName)
    {
        if(!m_impl) throw SyphonNotRunningException();
        [m_impl->m_syphon_server setName:[NSString stringWithUTF8String:theName.c_str()]];
    }
    
    std::string Output::getName()
    {
        if(!m_impl) throw SyphonNotRunningException();
        return [m_impl->m_syphon_server.name UTF8String];
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    ServerDescription convert_description(NSDictionary *the_desc)
    {
        // These are the keys we can use in the server description dictionary.
        NSString* name = [the_desc objectForKey:SyphonServerDescriptionNameKey];
        NSString* appName = [the_desc objectForKey:SyphonServerDescriptionAppNameKey];
        return {[name UTF8String], [appName UTF8String]};
    }
    
    std::vector<ServerDescription> Input::get_inputs()
    {
        std::vector<ServerDescription> ret;
        
        for(NSDictionary *description in [[SyphonServerDirectory sharedDirectory] servers])
        {
            ret.push_back(convert_description(description));
        }
        return ret;
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    struct InputImpl
    {
        SyphonClient *m_syphon_client;
        SyphonImage *m_image;

        InputImpl(uint32_t the_index):
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
        ~InputImpl()
        {
            [m_syphon_client stop];
            [m_syphon_client dealloc];
            if(m_image){ [m_image release]; }
        }
    };
    
    Input::Input(uint32_t the_index):
    m_impl(new InputImpl(the_index))
    {
    
    }
    
    bool Input::has_new_frame()
    {
        return m_impl && [m_impl->m_syphon_client hasNewFrame];
    }
    
    bool Input::copy_frame(gl::Texture &tex)
    {
        if(!has_new_frame()) return false;
        
        // release the last SyphonImage object, if any
        if(m_impl->m_image){ [m_impl->m_image release]; }
        
        // Get a new frame from the client
        m_impl->m_image = [m_impl->m_syphon_client newFrameImageForContext:CGLGetCurrentContext()];
        KINSKI_CHECK_GL_ERRORS();
        
        tex = gl::Texture(GL_TEXTURE_RECTANGLE,
                          [m_impl->m_image textureName],
                          [m_impl->m_image textureSize].width,
                          [m_impl->m_image textureSize].height, true);
        return true;
    }
    
    ServerDescription Input::description()
    {
        return convert_description(m_impl->m_syphon_client.serverDescription);
    }
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
}}//namespace
