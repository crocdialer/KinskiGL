//
//  GrowthApp.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "GrowthApp.h"
//#include "AssimpConnector.h"

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
    
    registerProperty(m_arduino_device_name);
    registerProperty(m_branch_angles);
    registerProperty(m_branch_randomness);
    registerProperty(m_increment);
    registerProperty(m_increment_randomness);
    registerProperty(m_diameter);
    registerProperty(m_diameter_shrink);
    registerProperty(m_cap_bias);
    registerProperty(m_num_iterations);
    registerProperty(m_max_index);
    registerProperty(m_axiom);
    
    for(auto rule : m_rules)
        registerProperty(rule);
    
    registerProperty(m_animate_growth);
    registerProperty(m_animation_time);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    try
    {
//        m_bounding_mesh = gl::AssimpConnector::loadModel("tree01.dae");
//        m_bounding_mesh = gl::Mesh::create(gl::Geometry::createBox(vec3(15, 40, 15)),
//                                           gl::Material::create());
//        
//        m_bounding_mesh = gl::Mesh::create(gl::Geometry::createSphere(60.f, 32),
//                                           gl::Material::create(gl::createShader(gl::SHADER_PHONG)));

//        m_bounding_mesh->setPosition(vec3(0, 0, 160));
//        m_bounding_mesh->material()->setWireframe();
//        m_bounding_mesh->material()->setDiffuse(gl::COLOR_WHITE);
//        scene().addObject(m_bounding_mesh);
        
        // load shader
        m_lsystem_shader = gl::createShaderFromFile("shader_01.vert",
                                                    "shader_01.frag",
                                                    "shader_01.geom");
        
        m_textures[0] = gl::createTextureFromFile("mask.png", true, false, 4);
        
//        m_movie.load("~/Desktop/l_system_animation/vid1.mov", true, true);
    }
    catch(Exception &e){LOG_ERROR << e.what();}
    
    load_settings();
    
    // light component
    m_light_component.reset(new LightComponent());
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto &light : lights()){ scene().addObject(light); }
}

/////////////////////////////////////////////////////////////////

void GrowthApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // parse arduino input
    if(m_serial.isInitialized())
    {
        for(string line : m_serial.read_lines())
        {
            parse_line(line);
        }
        
        for (const auto &m : m_analog_in)
        {
            LOG_DEBUG << m.description() << ": " << m.last_value();
        }
    }
    
    // movie playback, get a new frame if available
    if(m_movie.copy_frame_to_texture(m_textures[0])){}
    
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
    
    scene().render(camera());
//    gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_mesh->transform());
//    gl::drawMesh(m_mesh);
    
    for(auto light : lights())
    {
        gl::drawLight(light);
    }
    
    // draw texture map(s)
    if(displayTweakBar())
    {
        float w = (windowSize()/6.f).x;
        float h = m_textures[0].getHeight() * w / m_textures[0].getWidth();
        glm::vec2 offset(getWidth() - w - 10, 10);
        glm::vec2 step(0, h + 10);
        
        for (const gl::Texture &t : m_textures)
        {
            if(!t) continue;
            
            float h = t.getHeight() * w / t.getWidth();
            drawTexture(t, vec2(w, h), offset);
            gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                           as_string(t.getHeight()), m_font, glm::vec4(1),
                           offset);
            offset += step;
        }
        
        // draw analog input meter
        float val = m_analog_in[0].last_value();
        
        gl::drawQuad(gl::COLOR_DARK_RED,
                     vec2(70, windowSize().y * val),
                     vec2(windowSize().x - 150, windowSize().y * (1 - val)));
        
        // draw fps string
        gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                       vec4(vec3(1) - clear_color().xyz(), 1.f),
                       glm::vec2(windowSize().x - 110, windowSize().y - 70));
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

            case GLFW_KEY_4:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(17.55, 20.0, 18.41);
                *m_num_iterations = 8;
                *m_axiom = "FFq";
                *m_rules[0] = "q=Fp[&/+p]F[^\\-p]";
                *m_rules[1] = "F=[--&p]q";
                *m_rules[2] = "p=FF[^^^-q][\\\\+q]";
                *m_rules[3] = "";
                break;
                
            case GLFW_KEY_5:
                // our lsystem shall draw something else ...
                *m_branch_angles = vec3(15.f);
                *m_num_iterations = 10;
                *m_axiom = "FA";
                *m_rules[0] = "A=^F+ F - B\\\\FB&&B";
                *m_rules[1] = "B=[^F/////A][&&A]";
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
       theProperty == m_branch_randomness ||
       theProperty == m_increment ||
       theProperty == m_increment_randomness ||
       theProperty == m_diameter ||
       theProperty == m_diameter_shrink)
    {
        m_dirty_lsystem = true;
    }
    else if(theProperty == m_max_index)
    {
        if(m_mesh)
            m_mesh->entries().front().numdices = *m_max_index;
    }
    else if(m_growth_animation && theProperty == m_animate_growth)
    {
        if(*m_animate_growth){m_growth_animation->start();}
        else {m_growth_animation->stop();}
    }
    else if(theProperty == m_animation_time)
    {
//        *m_animation_time
    }
    else if(theProperty == m_cap_bias)
    {
        if(m_mesh)
        {
            m_mesh->material()->uniform("u_cap_bias", *m_cap_bias);
        }
    }
    else if(theProperty == m_arduino_device_name)
    {
        if(m_arduino_device_name->value().empty())
            m_serial.setup(0, 57600);
        else
            m_serial.setup(*m_arduino_device_name, 57600);
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
    m_lsystem.set_branch_randomness(*m_branch_randomness);
    m_lsystem.set_increment(*m_increment);
    m_lsystem.set_increment_randomness(*m_increment_randomness);
    m_lsystem.set_diameter(*m_diameter);
    m_lsystem.set_diameter_shrink_factor(*m_diameter_shrink);
    
    // iterate
    m_lsystem.iterate(*m_num_iterations);
    
    // geometry constraints
//    auto poop = [=](const vec3& p) -> bool
//    {
//        return gl::is_point_inside_mesh(p, m_bounding_mesh);
//    };
//    m_lsystem.set_position_check(poop);
    
    // create a mesh from our lsystem geometry
    scene().removeObject(m_mesh);
    m_mesh = m_lsystem.create_mesh();
    m_mesh->position() -= m_mesh->boundingBox().center();
    scene().addObject(m_mesh);
    
    // add our shader
    for (auto m : m_mesh->materials())
    {
        m->setShader(m_lsystem_shader);
        m->addTexture(m_textures[0]);
        m->setBlending();
        m->setDepthTest(false);
        m->setDepthWrite(false);
        
//        m->setTwoSided();
//        m->setWireframe();
    }
//    m_mesh->materials()[0]->setShader(gl::createShader(gl::SHADER_UNLIT));
//    m_mesh->materials()[0]->textures().clear();
    
    uint32_t min = 0, max = m_mesh->entries().front().numdices - 1;
    m_max_index->setRange(min, max);
    
    // animation
    m_growth_animation = animation::create(m_max_index, min, max, *m_animation_time);
    if(!*m_animate_growth)
        m_growth_animation->stop();
    
    m_growth_animation->set_loop();
}

void GrowthApp::parse_line(const std::string &line)
{
    std::istringstream ss(line);
    int parsed_index = -1;
    
    vector<string> tokens = split(line);
    
    // return if number of tokens doesn´t match or our prefix is not found
    if(tokens.size() < 2 || tokens[0].find(m_input_prefix) == string::npos) return;
    
    parsed_index = string_as<int>(tokens[0].substr(m_input_prefix.size()));
    
    // return if parsed index is out of bounds
    if(parsed_index < 0 || parsed_index >= m_analog_in.size()) return;
    
    m_analog_in[parsed_index].push(string_as<int>(tokens[1]) / 1023.f);
}
