//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "ModelViewer.h"
#include "AssimpConnector.h"

using namespace std;
using namespace kinski;
using namespace glm;

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();
    
    registerProperty(m_model_path);
    registerProperty(m_use_bones);
    registerProperty(m_display_bones);
    registerProperty(m_animation_index);
    registerProperty(m_animation_speed);
    registerProperty(m_cube_map_folder);
    observeProperties();
    
    // pre-create our shaders
    m_shaders[SHADER_UNLIT] = gl::createShader(gl::ShaderType::UNLIT);
    m_shaders[SHADER_UNLIT_SKIN] = gl::createShader(gl::ShaderType::UNLIT_SKIN);
    m_shaders[SHADER_PHONG] = gl::createShader(gl::ShaderType::PHONG);
    m_shaders[SHADER_PHONG_SKIN] = gl::createShader(gl::ShaderType::PHONG_SKIN);
    m_shaders[SHADER_PHONG_SHADOWS] = gl::createShader(gl::ShaderType::PHONG_SHADOWS);
    m_shaders[SHADER_PHONG_SKIN_SHADOWS] = gl::createShader(gl::ShaderType::PHONG_SKIN_SHADOWS);
    
    // create our UI
    create_tweakbar_from_component(shared_from_this());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    // add groundplane
    auto ground_mesh = gl::Mesh::create(gl::Geometry::createPlane(400, 400),
                                        gl::Material::create(m_shaders[SHADER_PHONG_SHADOWS]));
    ground_mesh->transform() = glm::rotate(mat4(), -90.f, gl::X_AXIS);
    
    scene().addObject(ground_mesh);
    
    load_settings();
    m_light_component->refresh();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::setMatrices(camera());
    if(draw_grid()){ gl::drawGrid(50, 50); }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::drawLight(l); }
    }
    
    scene().render(camera());
    
    if(m_mesh && *m_display_bones) // slow!
    {
        // crunch bone data
        vector<vec3> skel_points;
        vector<string> bone_names;
        build_skeleton(m_mesh->rootBone(), skel_points, bone_names);
        gl::loadMatrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_mesh->global_transform());
        
        // draw bone data
        gl::drawLines(skel_points, gl::COLOR_DARK_RED);
        
        for(const auto &p : skel_points)
        {
            vec3 p_trans = (m_mesh->global_transform() * vec4(p, 1.f)).xyz();
            vec2 p2d = gl::project_point_to_screen(p_trans, camera());
            gl::drawCircle(p2d, 5.f, false);
        }
    }
    
    std::vector<gl::Texture> comb_texs;
    
    // draw texture map(s)
    if(displayTweakBar() && m_mesh)
    {
        gl::MeshPtr m = m_mesh;
        if(selected_mesh()){ m = selected_mesh(); }
        
        for(auto &mat : m->materials())
        {
            comb_texs = concat_containers<gl::Texture>(mat->textures(), comb_texs);
        }
        
        draw_textures(comb_texs);
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void ModelViewer::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
{
    std::vector<gl::Texture> dropped_textures;
    
    for(const string &f : files)
    {
        LOG_INFO << f;
        
        switch (get_file_type(f))
        {
            case FileType::DIRECTORY:
                *m_cube_map_folder = f;
                break;

            case FileType::MODEL:
                *m_model_path = f;
                break;
            
            case FileType::IMAGE:
                try
                {
                    dropped_textures.push_back(gl::createTextureFromFile(f, true, false));
                }
                catch (Exception &e) { LOG_WARNING << e.what();}
                if(scene().pick(gl::calculateRay(camera(), vec2(e.getX(), e.getY()))))
                {
                    LOG_INFO << "texture drop on model";
                }
                break;
            default:
                break;
        }
    }
    if(m_mesh && !dropped_textures.empty()){ m_mesh->material()->textures() = dropped_textures; }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::tearDown()
{
    LOG_PRINT<<"ciao model viewer";
}

/////////////////////////////////////////////////////////////////

void ModelViewer::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    if(theProperty == m_model_path)
    {
        add_search_path(get_directory_part(*m_model_path));
        scene().removeObject(m_mesh);
        load_asset(*m_model_path);
        scene().addObject(m_mesh);
    }
    else if(theProperty == m_use_bones)
    {
        if(m_mesh)
        {
            bool use_bones = m_mesh->geometry()->hasBones() && *m_use_bones;
            for(auto &mat : m_mesh->materials())
            {
                mat->setShader(m_shaders[use_bones? SHADER_PHONG_SKIN_SHADOWS : SHADER_PHONG_SHADOWS]);
            }
        }
    }
    else if(theProperty == m_wireframe)
    {
        if(m_mesh)
        {
            for(auto &mat : m_mesh->materials())
            {
                mat->setWireframe(*m_wireframe);
            }
        }
    }
    else if(theProperty == m_animation_index)
    {
        if(m_mesh)
        {
            m_mesh->set_animation_index(*m_animation_index);
        }
    }
    else if(theProperty == m_animation_speed)
    {
        if(m_mesh)
        {
            m_mesh->set_animation_speed(*m_animation_speed);
        }
    }
    else if(theProperty == m_cube_map_folder)
    {
        if(kinski::is_directory(*m_cube_map_folder))
        {
          vector<gl::Texture> cube_planes;
          for(auto &f : kinski::get_directory_entries(*m_cube_map_folder, FileType::IMAGE))
          {
              cube_planes.push_back(gl::createTextureFromFile(f));
          }
          m_cube_map = gl::create_cube_texture(cube_planes);
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::build_skeleton(gl::BonePtr currentBone, vector<vec3> &points,
                                 vector<string> &bone_names)
{
    if(!currentBone) return;
    
    // read out the bone names
    bone_names.push_back(currentBone->name);
    
    for (auto child_bone : currentBone->children)
    {
        mat4 globalTransform = currentBone->worldtransform;
        mat4 childGlobalTransform = child_bone->worldtransform;
        points.push_back(globalTransform[3].xyz());
        points.push_back(childGlobalTransform[3].xyz());
        
        build_skeleton(child_bone, points, bone_names);
    }
}

/////////////////////////////////////////////////////////////////

bool ModelViewer::load_asset(const std::string &the_path)
{
    gl::MeshPtr m;
    gl::Texture t;
    
    auto asset_dir = get_directory_part(the_path);
    add_search_path(asset_dir);
    
    switch (get_file_type(the_path))
    {
        case FileType::DIRECTORY:
            for(const auto &p : get_directory_entries(the_path)){ load_asset(p); }
            break;
            
        case FileType::MODEL:
            m = gl::AssimpConnector::loadModel(the_path);
            break;
            
        case FileType::IMAGE:
            
            try { t = gl::createTextureFromFile(the_path, true, true); }
            catch (Exception &e) { LOG_WARNING << e.what(); }
            
            if(t)
            {
                auto geom = gl::Geometry::createPlane(t.getWidth(), t.getHeight(), 100, 100);
                auto mat = gl::Material::create();
                mat->addTexture(t);
                mat->setDepthWrite(false);
                m = gl::Mesh::create(geom, mat);
                m->transform() = rotate(mat4(), 90.f, gl::Y_AXIS);
            }
            break;
            
        default:
            break;
    }
    
    if(m)
    {
        bool use_bones = m->geometry()->hasBones() && *m_use_bones;
        
        for(auto &mat : m->materials())
        {
            mat->setShader(m_shaders[use_bones? SHADER_PHONG_SKIN_SHADOWS : SHADER_PHONG_SHADOWS]);
            mat->setBlending();
        }
        
        // apply scaling
        auto aabb = m->boundingBox();
        float scale_factor = 50.f / length(aabb.halfExtents());
        m->setScale(scale_factor);
        
        // look for animations for this mesh
        auto animation_folder = join_paths(asset_dir, "animations");
        
        for(const auto &f : get_directory_entries(animation_folder, FileType::MODEL))
        {
            gl::AssimpConnector::add_animations_to_mesh(f, m);
        }
        m_mesh = m;
        return true;
    }
    return false;
}
