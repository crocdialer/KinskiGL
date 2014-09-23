//
//  MovieTimeshift.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTimeshift.h"
#include "gl/ArrayTexture.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void MovieTimeshift::setup()
{
    ViewerApp::setup();

    registerProperty(m_use_camera);
    registerProperty(m_movie_speed);
    registerProperty(m_movie_path);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_movie.set_on_load_callback(bind(&MovieTimeshift::on_movie_load, this));
    
    m_custom_mat = gl::Material::create();
    
    try
    {
        auto sh = gl::createShaderFromFile("array_shader.vert", "array_shader.frag");
        m_custom_mat->setShader(sh);
    } catch (Exception &e) { LOG_ERROR << e.std::exception::what();}
    
    textures()[TEXTURE_NOISE] = create_noise_tex();
    
//    m_timer_update_noise = Timer(1.05f, io_service(), [&](){ create_noise_tex(getApplicationTime() * .01); });
//    m_timer_update_noise.set_periodic(true);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update(float timeDelta)
{
    if(m_needs_movie_refresh)
    {
        m_needs_movie_refresh = false;
        
        Stopwatch t;
        t.start();
        m_movie.copy_frames_offline(m_array_tex);
        LOG_DEBUG << "copying frames to Arraytexture took: " << t.time_elapsed() << " secs";
        
        m_custom_mat->uniform("u_num_frames", m_array_tex.getDepth());
        m_custom_mat->array_textures() = {m_array_tex};
    }
    
    // fetch data from camera, if available. then upload to array texture
    if(m_camera.is_capturing())
    {
        int w, h;
        
        if(m_camera.copy_frame(m_camera_data, &w, &h))
        {
            LOG_INFO << "received frame: " << w << " x " << h;
        }
    }
    
    textures()[TEXTURE_NOISE] = create_noise_tex(getApplicationTime() * .1);
    m_custom_mat->textures() = {textures()[TEXTURE_NOISE] };
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::draw()
{
    if(textures()[0])
    {
        gl::drawTexture(textures()[0], gl::windowDimension());
    }
    else if(m_array_tex)
    {
        gl::drawQuad(m_custom_mat, gl::windowDimension());
    }
    
    if(displayTweakBar())
    {
        draw_textures();
        
        gl::drawText2D(m_movie.get_path() + " : " +
                       kinski::as_string(m_movie.current_time(), 2) + " / " +
                       kinski::as_string(m_movie.duration(), 2),
                       fonts()[0]);
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    gl::ArrayTexture array_tex;
    
    switch (e.getCode())
    {
        case GLFW_KEY_P:
            m_movie.pause();
            break;
            
        case GLFW_KEY_B:
            m_movie.copy_frames_offline(array_tex);
            break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::got_message(const std::vector<uint8_t> &the_data)
{
    LOG_INFO << string(the_data.begin(), the_data.end());
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    *m_movie_path = files.back();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::tearDown()
{
    LOG_PRINT << "ciao movie timeshift";
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::on_movie_load()
{
    m_needs_movie_refresh = true;
}

/////////////////////////////////////////////////////////////////

gl::Texture MovieTimeshift::create_noise_tex(float seed)
{
    int w = 128, h = 128;
    float data[w * h];
    
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
        {
            data[i * h + j] = (glm::simplex( vec3(0.0125f * vec2(i, j), seed)) + 1) / 2.f;
        }
    
    gl::Texture::Format fmt;
    fmt.setInternalFormat(GL_COMPRESSED_RED_RGTC1);
//    fmt.set_mipmapping(true);
    gl::Texture noise_tex = gl::Texture (w, h, fmt);
    noise_tex.update(data, GL_RED, w, h, true);
//    noise_tex.set_anisotropic_filter(8);
    return noise_tex;
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_movie_path)
    {
        m_movie.load(*m_movie_path);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie.set_rate(*m_movie_speed);
    }
    else if(theProperty == m_use_camera)
    {
        if(*m_use_camera){ m_camera.start_capture(); }
        else{ m_camera.stop_capture(); }
    }
}