//
//  PointCloudSample.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "PointCloudSample.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void PointCloudSample::setup()
{
    ViewerApp::setup();
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_font(m_font);
    
    camera()->setClippingPlanes(0.1, 15000);
    
    // properties
    m_fbo_size = Property_<glm::vec2>::create("FBO size", vec2(1024, 768));
    registerProperty(m_fbo_size);
    registerProperty(m_use_syphon);
    registerProperty(m_syphon_server_name);
    
    observeProperties();
    
    m_fbo = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
    
    //create a point cloud mesh
    
    m_point_cloud = gl::Mesh::create(gl::Geometry::create(),
                                     gl::Material::create(gl::createShader(gl::SHADER_POINTS_COLOR)));
    m_point_cloud->geometry()->vertices().resize(640 * 480);
    m_point_cloud->geometry()->setPrimitiveType(GL_POINTS);
    m_point_cloud->geometry()->createGLBuffers();
    m_point_cloud->material()->setPointSize(8.f);
    m_point_cloud->material()->setPointAttenuation(0.f, 20.f, 0.f);
    
//    m_point_cloud->position() += vec3(0, 0, 1400);
//    m_point_cloud->setScale(vec3(4.f, 4.f, 1.f));
    
//    float fx = 594.21f;
//    float fy = 591.04f;
//    float a = -0.0030711f;
//    float b = 3.3309495f;
//    float cx = 339.5f;
//    float cy = 242.7f;
//    GLfloat gl_mat[16] =
//    {
//        1/fx,     0,  0, 0,
//        0,    -1/fy,  0, 0,
//        0,       0,  0, a,
//        -cx/fx, cy/fy, -1, b
//    };
//    
//    glm::mat4 m = glm::make_mat4(gl_mat);
//    m_point_cloud->setTransform(m);
    
    
    scene().addObject(m_point_cloud);
    m_debug_scene.addObject(m_point_cloud);
    
    create_tweakbar_from_component(shared_from_this());
    
    // OpenNI
    m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
    m_open_ni->observeProperties();
    create_tweakbar_from_component(m_open_ni);
    
    // load state from config file
    try
    {
        Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
    }catch(FileNotFoundException &e)
    {
        LOG_WARNING << e.what();
    }
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_open_ni->has_new_frame())
    {
        // get the depth+userID texture
        m_textures[3] = m_open_ni->get_depth_texture();
        
        // update pointcloud mesh
//        m_open_ni->update_depth_buffer(m_point_cloud->geometry()->vertexBuffer());
//        vec3 s = m_point_cloud->scale();
//        m_point_cloud->setTransform(m_depth_cam->transform());
//        m_point_cloud->setScale(s);
    }
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::draw()
{
    gl::setMatrices(camera());
    if(draw_grid()) gl::drawGrid(3600, 3600);
    
    // render scene
    scene().render(camera());
    
    if(*m_use_syphon)
    {
        // offscreen render
        m_textures[2] = gl::render_to_texture(scene(), m_fbo, camera());
        m_syphon.publish_texture(m_textures[2]);
    }
    
    if(displayTweakBar())
    {
        // draw opencv maps
        float w = (windowSize()/6.f).x;
        glm::vec2 offset(getWidth() - w - 10, 10);
        for(auto &tex : m_textures)
        {
            if(!tex) continue;
            
            float h = tex.getHeight() * w / tex.getWidth();
            glm::vec2 step(0, h + 10);
            drawTexture(tex, vec2(w, h), offset);
            gl::drawText2D(as_string(tex.getWidth()) + std::string(" x ") +
                           as_string(tex.getHeight()), m_font, glm::vec4(1),
                           offset);
            offset += step;
        }
    }
    // draw fps string
    gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                   gl::Color(vec3(1) - clear_color().xyz(), 1.f),
                   glm::vec2(windowSize().x - 115, windowSize().y - 30));
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    
    gl::Fbo::Format fmt;
    fmt.setSamples(4);
    
    m_fbo = gl::Fbo(w, h, fmt);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    int min, max;
    
    switch(e.getChar())
    {
        case GLFW_KEY_D:
//            m_debug_draw_mode->getRange(min, max);
//            *m_debug_draw_mode = (*m_debug_draw_mode + 1) % (max + 1);
            break;
        case GLFW_KEY_S:
            Serializer::saveComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
            break;
        case GLFW_KEY_R:
            Serializer::loadComponentState(m_open_ni, "ni_config.json", PropertyIO_GL());
            break;
    }
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::tearDown()
{
    LOG_PRINT<<"ciao pointcloud sample";
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
}
