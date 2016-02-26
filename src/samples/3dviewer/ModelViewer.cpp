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

namespace
{
    const std::string tag_ground_plane = "ground_plane";
};

/////////////////////////////////////////////////////////////////

void ModelViewer::setup()
{
    ViewerApp::setup();
    
    register_property(m_model_path);
    register_property(m_use_lighting);
    register_property(m_use_ground_plane);
    register_property(m_use_bones);
    register_property(m_display_bones);
    register_property(m_animation_index);
    register_property(m_animation_speed);
    register_property(m_cube_map_folder);
    observe_properties();
    
    // create our UI
    add_tweakbar_for_component(shared_from_this());
    add_tweakbar_for_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
    
    // add groundplane
    auto ground_mesh = gl::Mesh::create(gl::Geometry::createPlane(400, 400),
                                        gl::Material::create(gl::create_shader(gl::ShaderType::PHONG_SHADOWS)));
    ground_mesh->transform() = glm::rotate(mat4(), -glm::half_pi<float>(), gl::X_AXIS);
    ground_mesh->add_tag(tag_ground_plane);
    
    scene().addObject(ground_mesh);
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    if(m_mesh && m_dirty_shader)
    {
        m_dirty_shader  = false;
        
        bool use_bones = m_mesh->geometry()->hasBones() && *m_use_bones;
        gl::Shader shader;
        
        if(use_bones)
        {
            shader = gl::create_shader(*m_use_lighting ? gl::ShaderType::PHONG_SKIN_SHADOWS :
                                                        gl::ShaderType::UNLIT_SKIN, false);
        }
        else
        {
            shader = gl::create_shader(*m_use_lighting ? gl::ShaderType::PHONG_SHADOWS :
                                                        gl::ShaderType::UNLIT, false);
        }
        
        for(auto &mat : m_mesh->materials())
        {
            mat->setShader(shader);
            mat->setBlending();
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::set_matrices(camera());
    if(draw_grid()){ gl::draw_grid(50, 50); }
    
    if(m_light_component->draw_light_dummies())
    {
        for (auto l : lights()){ gl::draw_light(l); }
    }
    
    scene().render(camera());
    
    if(m_mesh && *m_display_bones) // slow!
    {
        // crunch bone data
        vector<vec3> skel_points;
        vector<string> bone_names;
        build_skeleton(m_mesh->rootBone(), skel_points, bone_names);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, camera()->getViewMatrix() * m_mesh->global_transform());
        
        // draw bone data
        gl::draw_lines(skel_points, gl::COLOR_DARK_RED, 5.f);
        
        for(const auto &p : skel_points)
        {
            vec3 p_trans = (m_mesh->global_transform() * vec4(p, 1.f)).xyz();
            vec2 p2d = gl::project_point_to_screen(p_trans, camera());
            gl::draw_circle(p2d, 5.f, false);
        }
    }
    
    std::vector<gl::Texture> comb_texs;
    
    // draw texture map(s)
    if(displayTweakBar() && m_mesh)
    {
        for(auto &mat : m_mesh->materials())
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
                    dropped_textures.push_back(gl::create_texture_from_file(f, true, false));
                }
                catch (Exception &e) { LOG_WARNING << e.what();}
                if(scene().pick(gl::calculate_ray(camera(), vec2(e.getX(), e.getY()))))
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

void ModelViewer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);
    
    if(theProperty == m_model_path)
    {
        add_search_path(get_directory_part(*m_model_path));
        scene().removeObject(m_mesh);
        load_asset(*m_model_path);
        scene().addObject(m_mesh);
    }
    else if(theProperty == m_use_bones){ m_dirty_shader = true; }
    else if(theProperty == m_use_lighting){ m_dirty_shader = true; }
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
              cube_planes.push_back(gl::create_texture_from_file(f));
          }
          m_cube_map = gl::create_cube_texture(cube_planes);
        }
    }
    else if(theProperty == m_use_ground_plane)
    {
        auto objs = scene().get_objects_by_tag(tag_ground_plane);
        
        for(auto &o : objs)
        {
            o->set_enabled(*m_use_ground_plane);
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
            
            try { t = gl::create_texture_from_file(the_path, true, true); }
            catch (Exception &e) { LOG_WARNING << e.what(); }
            
            if(t)
            {
                auto geom = gl::Geometry::createPlane(t.getWidth(), t.getHeight(), 100, 100);
                auto mat = gl::Material::create();
                mat->addTexture(t);
                m = gl::Mesh::create(geom, mat);
                m->transform() = rotate(mat4(), 90.f, gl::Y_AXIS);
            }
            break;
            
        default:
            break;
    }
    
    if(m)
    {
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
        m->set_animation_speed(*m_animation_speed);
        m_mesh = m;
        m_dirty_shader = true;
        return true;
    }
    return false;
}
