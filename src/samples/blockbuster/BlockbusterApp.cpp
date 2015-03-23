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
    registerProperty(m_media_path);
    registerProperty(m_num_tiles_x);
    registerProperty(m_num_tiles_y);
    registerProperty(m_spacing_x);
    registerProperty(m_spacing_y);
    registerProperty(m_block_length);
    registerProperty(m_block_width);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    load_settings();
    m_light_component->refresh();
    
    m_psystem.opencl().init();
    m_psystem.opencl().set_sources("kernels.cl");
    m_psystem.add_kernel("texture_input");
    
    // openni
    // OpenNI
    m_open_ni = gl::OpenNIConnector::Ptr(new gl::OpenNIConnector());
    m_open_ni->observeProperties();
    create_tweakbar_from_component(m_open_ni);
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
    
//    if(m_movie && m_movie->copy_frame_to_texture(textures()[0]))
//    {
//        m_psystem.texture_input(textures()[0]);
//    }
    
    if(m_open_ni->has_new_frame())
    {
        // get the depth+userID texture
        m_textures[0] = m_open_ni->get_depth_texture();
        m_psystem.texture_input(textures()[0]);
    }
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::draw()
{
    gl::setMatrices(camera());
    if(*m_draw_grid){ gl::drawGrid(50, 50); }
    
//    gl::drawPoints(m_user_positions);
    
    scene().render(camera());
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::drawLight(l); }
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
        
        auto ft = get_filetype(f);
        if(ft == FileType::FILE_IMAGE || ft == FileType::FILE_MOVIE)
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
        m_movie = MovieController::create(*m_media_path, true, true);
    }
    else if(theProperty == m_block_length)
    {
        if(m_mesh)
        {
            m_mesh->material()->uniform("u_length", *m_block_length);
        }
    }
    else if(theProperty == m_block_width)
    {
        if(m_mesh)
        {
            m_mesh->material()->uniform("u_width", *m_block_width);
        }
    }
    else if(theProperty == m_num_tiles_x ||
            theProperty == m_num_tiles_y ||
            theProperty == m_spacing_x ||
            theProperty == m_spacing_y)
    {
        m_dirty = true;
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
    vec2 step(*m_spacing_x, *m_spacing_y), offset = - vec2(*m_num_tiles_x, *m_num_tiles_y) * step / 2.f;
    
    auto &verts = geom->vertices();
    
    for (int y = 0; y < *m_num_tiles_y; y++)
    {
        for (int x = 0; x < *m_num_tiles_x; x++)
        {
            verts[y * *m_num_tiles_x + x].xy() = offset + vec2(x, y) * step;
        }
    }
    gl::Shader sh;
    sh.loadFromData(read_file("geom_prepass.vert").c_str(),
                    phong_frag,
                    read_file("points_to_cubes.geom").c_str());
    ret = gl::Mesh::create(geom, gl::Material::create(sh));
    ret->material()->uniform("u_length", *m_block_length);
    ret->material()->uniform("u_width", *m_block_width);
    return ret;
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
