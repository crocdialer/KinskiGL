//
//  MovieTimeshift.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "MovieTimeshift.h"
//#include "cv/TextureIO.h"

using namespace std;
using namespace kinski;
using namespace glm;

//cv::Ptr<cv::BackgroundSubtractorMOG2> g_mog2 = cv::createBackgroundSubtractorMOG2();

/////////////////////////////////////////////////////////////////

void MovieTimeshift::setup()
{
    ViewerApp::setup();

    registerProperty(m_use_syphon);
    registerProperty(m_syphon_server_name);
    registerProperty(m_offscreen_size);
    registerProperty(m_use_camera);
    registerProperty(m_flip_image);
    registerProperty(m_cam_id);
    registerProperty(m_movie_speed);
    registerProperty(m_movie_path);
    registerProperty(m_noise_map_size);
    registerProperty(m_noise_scale_x);
    registerProperty(m_noise_scale_y);
    registerProperty(m_noise_velocity);
    registerProperty(m_noise_seed);
    registerProperty(m_use_gpu_noise);
    registerProperty(m_num_buffer_frames);
    registerProperty(m_use_bg_substract);
    registerProperty(m_mog_learn_rate);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_movie->set_on_load_callback(bind(&MovieTimeshift::on_movie_load, this));
    
    m_custom_mat = gl::Material::create();
    m_custom_mat->setBlending();
    m_custom_mat->setDepthTest(false);
    m_custom_mat->setDepthWrite(false);
    
    try
    {
        auto sh = gl::createShaderFromFile("array_shader.vert", "array_shader.frag");
        m_custom_mat->setShader(sh);
    } catch (Exception &e) { LOG_ERROR << e.what();}
    
    m_noise_gen_mat = gl::Material::create(gl::createShader(gl::SHADER_NOISE_3D));
    m_noise_gen_mat->setDepthTest(false);
    m_noise_gen_mat->setDepthWrite(false);
    
    textures()[TEXTURE_NOISE] = create_noise_tex();
    
    if(!load_settings())
    {
        m_camera->start_capture();
    }
    
    // setup remote control
    m_remote_control = RemoteControl(io_service(), {shared_from_this()});
    m_remote_control.start_listen();
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update(float timeDelta)
{
    if(m_needs_movie_refresh && !m_camera->is_capturing())
    {
        m_needs_movie_refresh = false;
        
        Stopwatch t;
        t.start();
        m_movie->copy_frames_offline(m_array_tex);
        LOG_DEBUG << "copying frames to Arraytexture took: " << t.time_elapsed() << " secs";
        
        m_custom_mat->uniform("u_num_frames", m_array_tex.getDepth());
        m_custom_mat->textures() = {m_array_tex};
    }
    
    if(m_movie->isPlaying() && m_movie->copy_frame_to_texture(textures()[TEXTURE_MOVIE]))
    {
        // got new movie frame
    }
    
    // fetch data from camera, if available. then upload to array texture
    if(m_camera->is_capturing())
    {
        int w, h;
        
        if(m_camera->copy_frame(m_camera_data, &w, &h))
        {
            LOG_TRACE << "received frame: " << w << " x " << h;
            textures()[TEXTURE_INPUT].update(&m_camera_data[0], GL_UNSIGNED_BYTE, GL_BGRA, w, h, *m_flip_image);
            
            // create a foreground image
//            cv::UMat fg_image;
            if(*m_use_bg_substract)
            {
//                fg_image = create_foreground_image(m_camera_data, w, h);
//                gl::TextureIO::updateTexture(textures()[TEXTURE_FG_IMAGE],
//                                             fg_image.getMat(cv::ACCESS_READ));
            }
            
            if(m_needs_array_refresh)
            {
                m_needs_array_refresh = false;
                
                gl::Texture::Format fmt;
                fmt.setTarget(GL_TEXTURE_3D);
//                fmt.setInternalFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
                m_array_tex = gl::Texture(w, h, *m_num_buffer_frames, fmt);
                m_array_tex.setFlipped(*m_flip_image);
                
                m_custom_mat->textures() = {m_array_tex};
                m_custom_mat->uniform("u_num_frames", m_array_tex.getDepth());
            }
            
            // upload new frame to array texture
            m_array_tex.bind();
            glTexSubImage3D(m_array_tex.getTarget(), 0, 0, 0, m_current_index, w, h, 1,
                            GL_BGRA, GL_UNSIGNED_BYTE, &m_camera_data[0]);
            m_array_tex.unbind();
            m_current_index = (m_current_index + 1) % *m_num_buffer_frames;
            m_custom_mat->uniform("u_current_index", m_current_index);
        }
    }
    
    textures()[TEXTURE_NOISE] = create_noise_tex(getApplicationTime() * *m_noise_velocity + *m_noise_seed);
    m_custom_mat->textures() = {m_array_tex, textures()[TEXTURE_NOISE]};
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::draw()
{
    if(*m_use_syphon && m_offscreen_fbo)
    {
        textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo, [this]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if(textures()[TEXTURE_MOVIE])
            {
                gl::drawTexture(textures()[TEXTURE_MOVIE], gl::windowDimension());
            }
            gl::drawQuad(m_custom_mat, gl::windowDimension());
        });
        
        m_syphon.publish_texture(textures()[TEXTURE_OUTPUT]);
        gl::drawTexture(textures()[TEXTURE_OUTPUT], gl::windowDimension());
    }
    else
    {
        if(textures()[TEXTURE_MOVIE])
        {
            gl::drawTexture(textures()[TEXTURE_MOVIE], gl::windowDimension());
        }
        gl::drawQuad(m_custom_mat, gl::windowDimension());
    }
    
    if(displayTweakBar())
    {
        draw_textures(textures());
        
        gl::drawText2D(m_movie->get_path() + " : " +
                       kinski::as_string(m_movie->current_time(), 2) + " / " +
                       kinski::as_string(m_movie->duration(), 2),
                       fonts()[0]);
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_P:
            if(m_movie->isPlaying()){ m_movie->pause(); }
            else{ m_movie->play(); }
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
    LOG_PRINT << "ciao belgium trash demo";
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::on_movie_load()
{
    m_needs_movie_refresh = true;
    m_movie->set_rate(*m_movie_speed);
}

/////////////////////////////////////////////////////////////////

gl::Texture MovieTimeshift::create_noise_tex(float seed)
{
    gl::Texture noise_tex;
    
    if(*m_use_gpu_noise)
    {
        if(!m_noise_fbo || m_noise_fbo.getSize() != m_noise_map_size->value())
        {
            m_noise_fbo = gl::Fbo(*m_noise_map_size);
        }
        m_noise_gen_mat->uniform("u_scale", vec2(*m_noise_scale_x, *m_noise_scale_y));
        m_noise_gen_mat->uniform("u_seed", seed);
        
        noise_tex = gl::render_to_texture(m_noise_fbo, [this]()
        {
            gl::drawQuad(m_noise_gen_mat, m_noise_fbo.getSize());
        });
    }
    else
    {
        int w = m_noise_map_size->value().x, h = m_noise_map_size->value().y;
        float data[w * h];
        
        for (int i = 0; i < h; i++)
        {
            for (int j = 0; j < w; j++)
            {
                data[i * h + j] = (glm::simplex( vec3(vec2(i * *m_noise_scale_x,
                                                           j * * m_noise_scale_y), seed)) + 1) / 2.f;
            }
        }
        gl::Texture::Format fmt;
        fmt.setInternalFormat(GL_RED);
        noise_tex = gl::Texture (w, h, fmt);
        noise_tex.update(data, GL_RED, w, h, true);
    }
    return noise_tex;
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::insert_data_into_array_texture(const std::vector<uint8_t> the_data,
                                                    gl::Texture &the_array_tex,
                                                    uint32_t the_width,
                                                    uint32_t the_height,
                                                    uint32_t the_index)
{

}

/////////////////////////////////////////////////////////////////

//cv::UMat MovieTimeshift::create_foreground_image(std::vector<uint8_t> &the_data, int width, int height)
//{
//    cv::UMat ret;
//    cv::UMat m = cv::Mat(height, width, CV_8UC4, &the_data[0]).getUMat(cv::ACCESS_RW);
//    static cv::UMat fg_mat;
//    g_mog2->apply(m, fg_mat, *m_mog_learn_rate);
//    blur(fg_mat, fg_mat, cv::Size(11, 11));
//    std::vector<cv::UMat> channels;
//    split(m, channels);
//    channels[3] = fg_mat;
//    merge(channels, ret);
//    return ret;
//}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_movie_path)
    {
        m_movie->load(*m_movie_path, true, true);
    }
    else if(theProperty == m_movie_speed)
    {
        m_movie->set_rate(*m_movie_speed);
    }
    else if(theProperty == m_use_camera)
    {
        if(*m_use_camera){ m_camera->start_capture(); }
        else{ m_camera->stop_capture(); }
    }
    else if(theProperty == m_num_buffer_frames)
    {
        m_needs_array_refresh = true;
    }
    else if(theProperty == m_offscreen_size)
    {
        m_offscreen_fbo = gl::Fbo(m_offscreen_size->value().x,
                                  m_offscreen_size->value().y);
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_cam_id)
    {
        m_camera = video::CameraController::create(*m_cam_id);
        if(m_camera){ m_camera->start_capture(); }
    }
    else if(theProperty == m_flip_image)
    {
        m_needs_array_refresh = true;
    }
}