//
//  GrowthApp.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "GrowthApp.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void GrowthApp::setup()
{
    ViewerApp::setup();
    
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    
    registerProperty(m_branch_angles);
    registerProperty(m_increment);
    registerProperty(m_num_iterations);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    // our lsystem shall draw a dragon curve
    m_lsystem.set_axiom("f");
    m_lsystem.add_rule("f = f - h");
    m_lsystem.add_rule("h = f + h");
    
    LOG_INFO << m_lsystem;
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void GrowthApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::draw()
{
    gl::setMatrices(camera());
    
    if(draw_grid()){gl::drawGrid(50, 50);}
    
    if(m_mesh)
        gl::drawMesh(m_mesh);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void GrowthApp::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void GrowthApp::tearDown()
{
    LOG_PRINT<<"ciao procedural growth";
}

/////////////////////////////////////////////////////////////////

void GrowthApp::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_num_iterations ||
       theProperty == m_branch_angles ||
       theProperty == m_increment)
    {
        m_lsystem.set_branch_angles(*m_branch_angles);
        m_lsystem.set_increment(*m_increment);
        
        // iterate
        m_lsystem.iterate(*m_num_iterations);
        
        // create a mesh from our lystem geometry
        m_mesh = gl::Mesh::create(m_lsystem.create_geometry(), gl::Material::create());
    }
}
