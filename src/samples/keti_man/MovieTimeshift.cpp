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

/////////////////////////////////////////////////////////////////

void MovieTimeshift::setup()
{
    ViewerApp::setup();
    
    register_property(m_input_source);
    register_property(m_use_syphon);
    register_property(m_syphon_server_name);
    register_property(m_offscreen_size);
    register_property(m_flip_image);
    register_property(m_cam_id);
    register_property(m_syphon_in_index);
    register_property(m_movie_speed);
    register_property(m_movie_path);
    register_property(m_noise_map_size);
    register_property(m_noise_scale_x);
    register_property(m_noise_scale_y);
    register_property(m_noise_velocity);
    register_property(m_noise_seed);
    register_property(m_num_buffer_frames);
    register_property(m_use_bg_substract);
    register_property(m_mog_learn_rate);
    
    observe_properties();
    add_tweakbar_for_component(shared_from_this());
    
    m_movie->set_on_load_callback(bind(&MovieTimeshift::on_movie_load, this));
    
    m_custom_mat = gl::Material::create();
    m_custom_mat->setBlending();
    m_custom_mat->setDepthTest(false);
    m_custom_mat->setDepthWrite(false);
    
    try
    {
        auto sh = gl::create_shader_from_file("array_shader.vert", "array_shader.frag");
        m_custom_mat->setShader(sh);
    } catch (Exception &e) { LOG_ERROR << e.what();}
    
    // init buffer as PixelBuffer
    m_pbo = gl::Buffer(GL_PIXEL_PACK_BUFFER, GL_STREAM_COPY);
    
    for(syphon::ServerDescription &sd : syphon::Input::get_inputs())
    {
        LOG_DEBUG << "Syphon inputs: " << sd.name << ": " << sd.app_name;
    }
    if(!syphon::Input::get_inputs().empty())
    {
        m_syphon_in_index->setRange(0, syphon::Input::get_inputs().size() - 1);
    }
    
    if(!load_settings())
    {
        m_camera->start_capture();
    }
}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update(float timeDelta)
{
    // check if we need to adapt the input source
    if(m_input_source_changed)
    {
        m_input_source_changed = false;
        set_input_source(InputSource(m_input_source->value()));
    }
    
    if(*m_input_source == INPUT_MOVIE && m_needs_movie_refresh)
    {
        m_needs_movie_refresh = false;
        m_movie = video::MovieController::create(*m_movie_path);
        
        Stopwatch t;
        t.start();
        
        if(m_movie->copy_frames_offline(m_array_tex))
        {
            LOG_DEBUG << "copying frames to Arraytexture took: " << t.time_elapsed() << " secs";
            
            m_custom_mat->uniform("u_num_frames", m_array_tex.getDepth());
            m_custom_mat->textures() = {m_array_tex};
        }
    }
    
    int w, h;
    bool advance_index = false;
    
    // fetch data from camera, if available. then upload to array texture
    if(m_camera && m_camera->is_capturing())
    {
        
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
            
            // insert camera raw data into array texture
            insert_data_into_array_texture(m_camera_data, m_array_tex, w, h, m_current_index);
            advance_index = true;
        }
    }
    
    // syphon
    if(*m_input_source == INPUT_SYPHON && m_syphon_in.copy_frame(textures()[TEXTURE_INPUT]))
    {
        if(!m_fbo_transfer || m_fbo_transfer.getSize() != textures()[TEXTURE_INPUT].getSize())
        {
            m_fbo_transfer = gl::Fbo(textures()[TEXTURE_INPUT].getSize());
        }
        auto tex = gl::render_to_texture(m_fbo_transfer, [&]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gl::draw_texture(textures()[TEXTURE_INPUT], m_fbo_transfer.getSize());
        });
        
        textures()[TEXTURE_INPUT] = tex;
        
        w = textures()[TEXTURE_INPUT].getWidth();
        h = textures()[TEXTURE_INPUT].getHeight();
        
        if(m_needs_array_refresh)
        {
            m_needs_array_refresh = false;
            
            gl::Texture::Format fmt;
            fmt.setTarget(GL_TEXTURE_3D);
            m_array_tex = gl::Texture(w, h, *m_num_buffer_frames, fmt);
            m_array_tex.setFlipped(!*m_flip_image);
            
            m_custom_mat->textures() = {m_array_tex};
            m_custom_mat->uniform("u_num_frames", m_array_tex.getDepth());
        }
        
        insert_texture_into_array_texture(textures()[TEXTURE_INPUT], m_array_tex, m_current_index);
        advance_index = true;
    }
    
    if(advance_index)
    {
        // advance index and set uniform variable
        m_current_index = (m_current_index + 1) % *m_num_buffer_frames;
        m_custom_mat->uniform("u_current_index", m_current_index);
    }
    
    // update procedural noise texture
    textures()[TEXTURE_NOISE] = m_noise.simplex(getApplicationTime() * *m_noise_velocity + *m_noise_seed);
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
            gl::draw_quad(m_custom_mat, gl::window_dimension());
        });
        
        m_syphon_out.publish_texture(textures()[TEXTURE_OUTPUT]);
        gl::draw_texture(textures()[TEXTURE_OUTPUT], gl::window_dimension());
    }
    else
    {
        gl::draw_quad(m_custom_mat, gl::window_dimension());
    }
    
    if(displayTweakBar())
    {
        draw_textures(textures());
        gl::draw_text_2D(m_input_source_names[InputSource(m_input_source->value())], fonts()[0]);
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

//gl::Texture MovieTimeshift::create_noise_tex(float seed)
//{
//    
//    gl::Texture noise_tex;
//
//    int w = m_noise_map_size->value().x, h = m_noise_map_size->value().y;
//    float data[w * h];
//    
//    for (int i = 0; i < h; i++)
//    {
//        for (int j = 0; j < w; j++)
//        {
//            data[i * h + j] = (glm::simplex( vec3(vec2(i * *m_noise_scale_x,
//                                                       j * * m_noise_scale_y), seed)) + 1) / 2.f;
//        }
//    }
//    gl::Texture::Format fmt;
//    fmt.setInternalFormat(GL_RED);
//    noise_tex = gl::Texture (w, h, fmt);
//    noise_tex.update(data, GL_RED, w, h, true);
//    return noise_tex;
//}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::insert_data_into_array_texture(const std::vector<uint8_t> &the_data,
                                                    gl::Texture &the_array_tex,
                                                    uint32_t the_width,
                                                    uint32_t the_height,
                                                    uint32_t the_index)
{
    // sanity check
    if(!the_array_tex || the_width != the_array_tex.getWidth() ||
       the_height != the_array_tex.getHeight() || the_index >= the_array_tex.getDepth())
    {
        return false;
    }
    
    // upload to pbo
    m_pbo.setData(the_data);
    
    // bind pbo for reading
    m_pbo.bind(GL_PIXEL_UNPACK_BUFFER);
    
    // upload new frame to array texture
    the_array_tex.bind();
    glTexSubImage3D(the_array_tex.getTarget(), 0, 0, 0, the_index, the_width, the_height, 1,
                    GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    the_array_tex.unbind();
    
    m_pbo.unbind(GL_PIXEL_UNPACK_BUFFER);
    
    return true;
}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::insert_texture_into_array_texture(const gl::Texture &the_texture,
                                                       gl::Texture &the_array_tex,
                                                       uint32_t the_index)
{
    // sanity check
    if(!the_texture || !the_array_tex || the_texture.getSize() != the_array_tex.getSize() ||
       the_index >= the_array_tex.getDepth())
    {
        return false;
    }
    
    // reserve size
    const uint32_t bytes_per_pixel = 4;
    int num_bytes = the_texture.getWidth() * the_texture.getHeight() * bytes_per_pixel;
    if(m_pbo.numBytes() != num_bytes)
    {
        m_pbo.setData(nullptr, num_bytes);
    }
    
    // download texture to pbo
    
    // bind pbo for writing
    m_pbo.bind(GL_PIXEL_PACK_BUFFER);
    
    the_texture.bind();
    glGetTexImage(the_texture.getTarget(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    the_texture.unbind();
    m_pbo.unbind(GL_PIXEL_PACK_BUFFER);
    
    // bind pbo for reading
    m_pbo.bind(GL_PIXEL_UNPACK_BUFFER);
    
    // upload new frame from pbo to array texture
    the_array_tex.bind();
    glTexSubImage3D(the_array_tex.getTarget(), 0, 0, 0, the_index, the_texture.getWidth(),
                    the_texture.getHeight(), 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    the_array_tex.unbind();
    m_pbo.unbind(GL_PIXEL_UNPACK_BUFFER);
    
    return true;
}

/////////////////////////////////////////////////////////////////

bool MovieTimeshift::set_input_source(InputSource the_src)
{
    m_needs_array_refresh = true;
    m_movie.reset();
    m_camera.reset();
    m_camera_data.clear();
    m_syphon_in = syphon::Input();
    
    switch (the_src)
    {
        case INPUT_NONE:
            break;
            
        case INPUT_MOVIE:
            m_needs_movie_refresh = true;
            break;
            
        case INPUT_CAMERA:
            m_camera = video::CameraController::create(*m_cam_id);
            m_camera->start_capture();
            break;
            
        case INPUT_SYPHON:
            try { m_syphon_in = syphon::Input(*m_syphon_in_index); }
            catch (syphon::SyphonInputOutOfBoundsException &e) { LOG_ERROR << e.what(); }
            break;
    }
    LOG_DEBUG << "input source changed to: " << m_input_source_names.at(the_src);
    return true;
}

/////////////////////////////////////////////////////////////////

//cv::UMat MovieTimeshift::create_foreground_image(std::vector<uint8_t> &the_data, int width, int height)
//{
//    static cv::Ptr<cv::BackgroundSubtractorMOG2> g_mog2 = cv::createBackgroundSubtractorMOG2();
//    static cv::UMat fg_mat;
//    
//    cv::UMat ret;
//    cv::UMat m = cv::Mat(height, width, CV_8UC4, &the_data[0]).getUMat(cv::ACCESS_RW);
//    g_mog2->apply(m, fg_mat, *m_mog_learn_rate);
//    blur(fg_mat, fg_mat, cv::Size(11, 11));
//    std::vector<cv::UMat> channels;
//    split(m, channels);
//    channels[3] = fg_mat;
//    merge(channels, ret);
//    return ret;
//}

/////////////////////////////////////////////////////////////////

void MovieTimeshift::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_input_source)
    {
        m_input_source_changed = true;
    }
    else if(theProperty == m_movie_path)
    {
        m_input_source_changed = true;
    }
    else if(theProperty == m_movie_speed)
    {
        if(m_movie){ m_movie->set_rate(*m_movie_speed); }
    }
    else if(theProperty == m_num_buffer_frames)
    {
        m_needs_array_refresh = true;
    }
    else if(theProperty == m_noise_map_size || theProperty == m_noise_scale_x ||
            theProperty == m_noise_scale_y)
    {
        m_noise.set_tex_size(*m_noise_map_size);
        m_noise.set_scale(vec2(*m_noise_scale_x, *m_noise_scale_y));
    }
    else if(theProperty == m_offscreen_size)
    {
        m_offscreen_fbo = gl::Fbo(m_offscreen_size->value().x,
                                  m_offscreen_size->value().y);
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon_out = *m_use_syphon ? syphon::Output(*m_syphon_server_name) : syphon::Output();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon_out.setName(*m_syphon_server_name);}
        catch(syphon::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_cam_id)
    {
        m_input_source_changed = true;
    }
    else if(theProperty == m_flip_image)
    {
        m_needs_array_refresh = true;
    }
}