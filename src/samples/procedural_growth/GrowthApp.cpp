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
    registerProperty(m_max_index);
    registerProperty(m_axiom);
    
    for(auto rule : m_rules)
        registerProperty(rule);
    
    registerProperty(m_animate_growth);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void GrowthApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_dirty_lsystem) refresh_lsystem();
    
    if(m_growth_animation)
    {
        m_growth_animation->update(timeDelta);
    }
}

/////////////////////////////////////////////////////////////////

void GrowthApp::draw()
{
    gl::setMatrices(camera());
    
    if(draw_grid()){gl::drawGrid(50, 50);}
    
    if(m_mesh)
    {
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_mesh->transform());
        gl::drawMesh(m_mesh);
    }
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
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case GLFW_KEY_LEFT:
                *m_num_iterations -= 1;
                break;
            
            case GLFW_KEY_RIGHT:
                *m_num_iterations += 1;
                break;
                
            case GLFW_KEY_1:
                // our lsystem shall draw a dragon curve
                *m_branch_angles = vec3(90);
                *m_axiom = "F";
                *m_num_iterations = 14;
                *m_rules[0] = "F = F - H";
                *m_rules[1] = "H = F + H";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_2:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(90);
                *m_num_iterations = 4;
                *m_axiom = "-L";
                *m_rules[0] = "L=LF+RFR+FL-F-LFLFL-FRFR+";
                *m_rules[1] = "R=-LFLF+RFRFR+F+RF-LFL-FR";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_3:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(60);
                *m_num_iterations = 4;
                *m_axiom = "F";
                *m_rules[0] = "F=F+G++G-F--FF-G+";
                *m_rules[1] = "G=-F+GG++G+F--F-G";
                *m_rules[2] = "";
                *m_rules[3] = "";
                break;
                
            default:
                break;
        }
    }
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
    
    bool rule_changed = false;
    
    for(auto r : m_rules)
        if (theProperty == r) rule_changed = true;
        
    if(theProperty == m_axiom ||
       rule_changed ||
       theProperty == m_num_iterations ||
       theProperty == m_branch_angles ||
       theProperty == m_increment)
    {
        m_dirty_lsystem = true;
        
    }
    else if(theProperty == m_max_index)
    {
        if(m_mesh)
            m_mesh->entries().front().numdices = *m_max_index;
    }
    else if(theProperty == m_animate_growth)
    {
        if(*m_animate_growth){m_growth_animation->start();}
        else{m_growth_animation->stop();}
    }
}

void GrowthApp::refresh_lsystem()
{
    m_dirty_lsystem = false;
    
    m_lsystem.set_axiom(*m_axiom);
    m_lsystem.rules().clear();
    
    for(auto r : m_rules)
        m_lsystem.add_rule(*r);
        
    m_lsystem.set_branch_angles(*m_branch_angles);
    m_lsystem.set_increment(*m_increment);
    
    // iterate
    m_lsystem.iterate(*m_num_iterations);
    
    // create a mesh from our lystem geometry
    m_mesh = gl::Mesh::create(m_lsystem.create_geometry(), gl::Material::create());
    m_mesh->position() -= m_mesh->boundingBox().center();
    
    uint32_t min = 0, max = m_mesh->entries().front().numdices - 1;
    m_max_index->setRange(min, max);
    
    // animation
    m_growth_animation = animation::create(m_max_index, min, max, 5.f);
    if(!*m_animate_growth)
        m_growth_animation->stop();
    m_growth_animation->set_loop();
}
