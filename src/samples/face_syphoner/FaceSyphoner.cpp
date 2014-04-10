//
//  FaceSyphoner.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "FaceSyphoner.h"
#include "ThreshNode.h"
#include "FaceFilter.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void FaceSyphoner::setup()
{
    ViewerApp::setup();
    
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);

    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    CVProcessNode::Ptr thresh_node(new ThreshNode(-1)), face_node(new FaceFilter());
    CVCombinedProcessNode::Ptr combi_node = face_node >> thresh_node;
    combi_node->observeProperties();
    create_tweakbar_from_component(combi_node);
    m_opencv->setProcessingNode(combi_node);
    m_opencv->streamUSBCamera();
 
    load_settings();
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::update(float timeDelta)
{
    
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::draw()
{
    
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    
    switch (e.getCode())
    {
        case GLFW_KEY_LEFT:
            break;
        
        case GLFW_KEY_RIGHT:
            break;
            
        case GLFW_KEY_UP:
            break;
            
        case GLFW_KEY_DOWN:
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::got_message(const std::vector<uint8_t> &the_data)
{
    LOG_INFO << string(the_data.begin(), the_data.end());
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::tearDown()
{
    LOG_PRINT << "ciao facesyphoner";
}

/////////////////////////////////////////////////////////////////

void FaceSyphoner::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_use_syphon)
    {
        int i = 0;
        for(auto & syphon : m_syphon)
        {
            syphon = *m_use_syphon ? gl::SyphonConnector(m_syphon_server_name->value() + "_" + as_string(i))
                : gl::SyphonConnector();
            i++;
        }
    }
    else if(theProperty == m_syphon_server_name)
    {
        int i = 0;
        for(auto & syphon : m_syphon)
        {
            try{syphon.setName(m_syphon_server_name->value() + "_" + as_string(i));}
            catch(gl::SyphonNotRunningException &e){LOG_WARNING<<e.what(); break;}
            i++;
        }
        
    }
}