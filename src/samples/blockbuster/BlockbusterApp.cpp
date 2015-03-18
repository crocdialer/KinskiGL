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
    registerProperty(m_num_tiles_x);
    registerProperty(m_num_tiles_y);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_light_component = std::make_shared<LightComponent>();
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    load_settings();
    m_light_component->refresh();
    
    m_mesh = create_mesh();
    scene().addObject(m_mesh);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void BlockbusterApp::draw()
{
    gl::setMatrices(camera());
    if(*m_draw_grid){ gl::drawGrid(50, 50); }
    
    gl::drawPoints(m_user_positions);
    
    scene().render(camera());
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
    for(const string &f : files){ LOG_INFO << f; }
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
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr BlockbusterApp::create_mesh()
{
    gl::MeshPtr ret;
    gl::GeometryPtr geom = gl::Geometry::create();
    geom->setPrimitiveType(GL_POINTS);
    geom->vertices().resize(*m_num_tiles_x * *m_num_tiles_y);
    geom->normals().resize(*m_num_tiles_x * *m_num_tiles_y, vec3(0, 0, 1));
    vec2 step(10), offset = - vec2(*m_num_tiles_x, *m_num_tiles_y) * step / 2.f;
    
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
    ret->material()->uniform("u_length", 30.f);
    
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
