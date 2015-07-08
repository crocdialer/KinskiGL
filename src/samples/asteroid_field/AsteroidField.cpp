//
//  AsteroidField.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "AsteroidField.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void AsteroidField::setup()
{
    ViewerApp::setup();
    registerProperty(m_model_folder);
    registerProperty(m_sky_box_path);
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    m_skybox_mesh = gl::Mesh::create(gl::Geometry::createSphere(1.f, 24), gl::Material::create());
    m_skybox_mesh->material()->setDepthWrite(false);
    m_skybox_mesh->material()->setTwoSided();
    scene().addObject(m_skybox_mesh);
    
    // finally load state from file
    load_settings();
}

/////////////////////////////////////////////////////////////////

void AsteroidField::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::draw()
{
    // adapt skybox position
    m_skybox_mesh->setPosition(camera()->position());
//    gl::drawMesh(m_skybox_mesh);
    
    // draw asteroid field
    gl::setMatrices(camera());
    if(*m_draw_grid){ gl::drawGrid(50, 50); }
    
    scene().render(camera());
}

/////////////////////////////////////////////////////////////////

void AsteroidField::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void AsteroidField::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void AsteroidField::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    for(const string &f : files){ LOG_INFO << f; }
}

/////////////////////////////////////////////////////////////////

void AsteroidField::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void AsteroidField::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_model_folder)
    {
        m_proto_objects.clear();
        
        add_search_path(get_directory_part(*m_model_folder));
        for (const auto &p : get_directory_entries(*m_model_folder))
        {
            if(get_filetype(p) != FileType::FILE_MODEL){ continue; }
            auto mesh = gl::AssimpConnector::loadModel(p);
            if(mesh)
            {
                m_proto_objects.push_back(mesh);
            }
        }
    }
    else if(theProperty == m_sky_box_path)
    {
        auto &tex_vec = m_skybox_mesh->material()->textures();
        tex_vec.clear();
        try
        {
            tex_vec.push_back(gl::createTextureFromFile(*m_sky_box_path));
        }
        catch (Exception &e){ LOG_WARNING << e.what(); }
    }
}
