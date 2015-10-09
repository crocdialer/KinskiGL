//
//  BlockbusterApp.cpp
//  gl
//
//  Created by Fabian on 17/03/15.
//
//

#include "BlockbusterApp.h"
#include "gl/ShaderLibrary.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void BlockbusterApp::setup()
{
    ViewerApp::setup();
    set_window_title("blockbuster");
    
    registerProperty(m_view_type);
    registerProperty(m_media_path);
    registerProperty(m_use_syphon);
    registerProperty(m_syphon_server_name);
    registerProperty(m_fbo_cam_pos);
    registerProperty(m_fbo_cam_fov);
    registerProperty(m_fbo_resolution);
    registerProperty(m_num_tiles_x);
    registerProperty(m_num_tiles_y);
    registerProperty(m_spacing_x);
    registerProperty(m_spacing_y);
    registerProperty(m_block_length);
    registerProperty(m_block_width);
    registerProperty(m_block_width_multiplier);
    registerProperty(m_border);
    registerProperty(m_mirror_img);
    registerProperty(m_use_shadows);
    registerProperty(m_depth_min);
    registerProperty(m_depth_max);
    registerProperty(m_depth_multiplier);
    registerProperty(m_depth_smooth_fall);
    registerProperty(m_depth_smooth_rise);
    registerProperty(m_poisson_radius);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    // init our application specific shaders
    init_shaders();
    
    m_psystem.opencl().init();
    m_psystem.opencl().set_sources("kernels.cl");
    m_psystem.add_kernel("texture_input");
    m_psystem.add_kernel("texture_input_alt");
    m_psystem.add_kernel("updateParticles");
    
    // openni
    m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
    m_open_ni->observeProperties();
    create_tweakbar_from_component(m_open_ni);
    
    load_settings();
    m_light_component->refresh();
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_dirty)
    {
        scene().removeObject(m_mesh);
        m_mesh = create_mesh();
        m_psystem.set_mesh(m_mesh);
        scene().addObject(m_mesh);
        m_dirty = false;
    }
    
    struct particle_params
    {
        int num_cols, num_rows, mirror, border;
        float depth_min, depth_max, multiplier;
        float smooth_fall, smooth_rise;
        float min_size, max_size;
    } p;
    p.mirror = *m_mirror_img;
    p.border = *m_border;
    p.num_cols = *m_num_tiles_x;
    p.num_rows = *m_num_tiles_y;
    p.depth_min = *m_depth_min;
    p.depth_max = *m_depth_max;
    p.multiplier = *m_depth_multiplier;
    p.smooth_fall = *m_depth_smooth_fall;
    p.smooth_rise = *m_depth_smooth_rise;
    p.min_size = *m_block_width;
    p.max_size = *m_block_width * *m_block_width_multiplier;
    
    m_psystem.set_param_buffer(&p, sizeof(particle_params));
    
    if(m_movie && m_movie->copy_frame_to_texture(textures()[TEXTURE_MOVIE], true))
    {
        m_has_new_texture = true;
    }
    
    if(m_open_ni->has_new_frame())
    {
        // get the depth+userID texture
        textures()[TEXTURE_DEPTH] = m_open_ni->get_depth_texture();
        m_has_new_texture = true;
    }
    
    if(m_has_new_texture)
    {
        m_has_new_texture = false;
        
        if(textures()[TEXTURE_MOVIE] && textures()[TEXTURE_DEPTH])
        {
            m_psystem.texture_input_alt(textures()[TEXTURE_DEPTH], textures()[TEXTURE_MOVIE]);
        }
        else if(textures()[TEXTURE_DEPTH])
        {
            m_psystem.texture_input(textures()[TEXTURE_DEPTH]);
        }
    }
    
    m_psystem.update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::draw()
{
    // draw the output texture
    if(m_fbos[0] && m_fbo_cam && *m_use_syphon)
    {
        auto tex = gl::render_to_texture(m_fbos[0],[&]
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene().render(m_fbo_cam);
        });
        textures()[TEXTURE_SYPHON] = tex;
        m_syphon.publish_texture(tex);
    }
    
    switch (*m_view_type)
    {
        case VIEW_DEBUG:
            gl::setMatrices(camera());
            if(draw_grid()){ gl::drawGrid(50, 50); }
            
            if(m_light_component->draw_light_dummies())
            {
                for (auto l : lights()){ gl::drawLight(l); }
            }
            
            scene().render(camera());
            break;
            
        case VIEW_OUTPUT:
            gl::drawTexture(textures()[TEXTURE_SYPHON], gl::windowDimension());
            break;
            
        default:
            break;
    }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto &l : lights()){ gl::drawLight(l); }
    }
    
    if(displayTweakBar()){ draw_textures(textures());}
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
    m_user_positions = { click_pos_on_ground(vec2(e.getPos())) };
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
    m_user_positions = { click_pos_on_ground(vec2(e.getPos())) };
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files)
    {
        LOG_DEBUG << f;
        
        auto ft = get_file_type(f);
        if(ft == FileType::IMAGE || ft == FileType::MOVIE)
        {
            *m_media_path = f;
        }
    }
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::tearDown()
{
    LOG_PRINT<<"ciao blockbuster";
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_media_path)
    {
        m_movie = video::MovieController::create(*m_media_path, true, true);
        textures()[TEXTURE_MOVIE] = gl::Texture();
    }
    else if(theProperty == m_block_length)
    {
        if(m_mesh)
        {
            m_mesh->material()->uniform("u_length", *m_block_length);
        }
    }
//    else if(theProperty == m_block_width)
//    {
//        if(m_mesh)
//        {
//            m_mesh->material()->uniform("u_width", *m_block_width);
//        }
//    }
    else if(theProperty == m_num_tiles_x ||
            theProperty == m_num_tiles_y ||
            theProperty == m_spacing_x ||
            theProperty == m_spacing_y ||
            theProperty == m_use_shadows)
    {
        m_dirty = true;
    }
    else if(theProperty == m_fbo_resolution ||
            theProperty == m_fbo_cam_pos ||
            theProperty == m_fbo_cam_fov)
    {
        gl::Fbo::Format fmt;
        fmt.setSamples(4);
        m_fbos[0] = gl::Fbo(m_fbo_resolution->value().x, m_fbo_resolution->value().y, fmt);
        float aspect = m_fbos[0].getAspectRatio();//m_obj_scale->value().x / m_obj_scale->value().y;
        m_fbo_cam = gl::PerspectiveCamera::create(aspect, *m_fbo_cam_fov, 5.f, 2000.f);
        m_fbo_cam->position() = *m_fbo_cam_pos;
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
    else if(theProperty == m_poisson_radius)
    {
        if(m_mesh)
            m_mesh->material()->uniform("u_poisson_radius", *m_poisson_radius);
    }
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr BlockbusterApp::create_mesh()
{
    gl::MeshPtr ret;
    gl::GeometryPtr geom = gl::Geometry::create();
    geom->setPrimitiveType(GL_POINTS);
    geom->vertices().resize(*m_num_tiles_x * *m_num_tiles_y);
    geom->normals().resize(*m_num_tiles_x * *m_num_tiles_y, vec3(0, 0, 1));
    geom->point_sizes().resize(*m_num_tiles_x * *m_num_tiles_y, 1.f);
    geom->colors().resize(*m_num_tiles_x * *m_num_tiles_y, gl::COLOR_WHITE);
    vec2 step(*m_spacing_x, *m_spacing_y), offset = - vec2(*m_num_tiles_x, *m_num_tiles_y) * step / 2.f;
    
    auto &verts = geom->vertices();
    
    for (int y = 0; y < *m_num_tiles_y; y++)
    {
        for (int x = 0; x < *m_num_tiles_x; x++)
        {
            verts[y * *m_num_tiles_x + x] = vec3(offset + vec2(x, y) * step, 0.f);
        }
    }
   
    ret = gl::Mesh::create(geom, gl::Material::create(*m_use_shadows ? m_block_shader_shadows :
                                                      m_block_shader));
    ret->material()->uniform("u_length", *m_block_length);
//    ret->material()->uniform("u_width", *m_block_width);
//    ret->material()->setBlending();
    return ret;
}

/////////////////////////////////////////////////////////////////

bool BlockbusterApp::save_settings(const std::string &path)
{
    bool ret = true;
    
    try{ Serializer::saveComponentState(m_open_ni, "openni_config.json", PropertyIO_GL()); }
    catch(Exception &e)
    {
        LOG_ERROR<<e.what();
        ret = false;
    }
    return ViewerApp::save_settings(path) && ret;
}

/////////////////////////////////////////////////////////////////

bool BlockbusterApp::load_settings(const std::string &path)
{
    bool ret = true;
    
    try{ Serializer::loadComponentState(m_open_ni, "openni_config.json", PropertyIO_GL()); }
    catch(Exception &e)
    {
        LOG_ERROR<<e.what();
        ret = false;
    }
    return ViewerApp::load_settings(path) && ret;
}

/////////////////////////////////////////////////////////////////

glm::vec3 BlockbusterApp::click_pos_on_ground(const glm::vec2 click_pos)
{
    gl::Plane ground_plane(vec3(0), vec3(0, 1, 0));
    auto ray = gl::calculateRay(camera(), click_pos);
    auto intersect = ground_plane.intersect(ray);
    vec3 ret = ray * intersect.distance;
    return ret;
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::init_shaders()
{    
    m_block_shader.loadFromData(read_file("geom_prepass.vert"),
                                phong_frag,
                                read_file("points_to_cubes.geom"));
    
    m_block_shader_shadows.loadFromData(read_file("geom_prepass.vert"),
                                        phong_shadows_frag,
                                        read_file("points_to_cubes_shadows.geom"));
}
