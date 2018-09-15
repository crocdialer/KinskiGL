//
//  ModelViewer.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include <app/imgui/ImGuizmo.h>
#include "core/Image.hpp"
#include "ModelViewer.h"
#include "assimp/assimp.hpp"

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

    register_property(m_draw_fps);
    register_property(m_model_path);
    register_property(m_use_lighting);
    register_property(m_use_deferred_render);
    register_property(m_shadow_cast);
    register_property(m_shadow_receive);
    register_property(m_use_post_process);
    register_property(m_use_fxaa);
    register_property(m_offscreen_resolution);
    register_property(m_use_normal_map);
    register_property(m_use_ground_plane);
    register_property(m_use_bones);
    register_property(m_display_bones);
    register_property(m_animation_index);
    register_property(m_animation_speed);
    register_property(m_normalmap_path);
    register_property(m_skybox_path);
    register_property(m_ground_textures);
    register_property(m_ground_plane_texture_scale);
    register_property(m_enviroment_strength);
    
    register_property(m_focal_depth);
    register_property(m_focal_length);
    register_property(m_fstop);
    register_property(m_gain);
    register_property(m_fringe);
    register_property(m_circle_of_confusion_sz);
    register_property(m_auto_focus);
    register_property(m_debug_focus);
    
    observe_properties();

    // create our remote interface
    remote_control().set_components({ shared_from_this(), m_light_component, m_warp_component });

    auto light_geom = gl::Geometry::create_sphere(2.f, 24);
    
    for(auto l : lights())
    {
        auto light_mesh = gl::Mesh::create(light_geom);
        light_mesh->material()->set_diffuse(gl::COLOR_BLACK);
        light_mesh->material()->set_emission(gl::COLOR_WHITE);
        light_mesh->material()->set_shadow_properties(gl::Material::SHADOW_NONE);
        light_mesh->set_name(l->name() + " dummy");
        l->add_child(light_mesh);
    }
    
    // add groundplane
    m_ground_mesh = gl::Mesh::create(gl::Geometry::create_plane(400, 400),
                                     gl::Material::create(gl::create_shader(gl::ShaderType::PHONG_SHADOWS)));
    m_ground_mesh->material()->set_shadow_properties(gl::Material::SHADOW_RECEIVE);
    m_ground_mesh->material()->set_roughness(0.4);
    m_ground_mesh->transform() = glm::rotate(mat4(), -glm::half_pi<float>(), gl::X_AXIS);
    m_ground_mesh->add_tag(tag_ground_plane);
    scene()->add_object(m_ground_mesh);

    load_settings();

    // initial fbo setup
    update_fbos();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update(float timeDelta)
{
    ViewerApp::update(timeDelta);

    // update window title with current fps
    set_window_title(name() + " (" + to_string(fps(), 1) + ")");

    // light selection
    for(uint32_t i = 0; i < lights().size(); ++i)
    {
        auto &l = lights()[i];

        gl::SelectVisitor<gl::Mesh> visitor;
        l->accept(visitor);
        for(auto m : visitor.get_objects()){ m->material()->set_emission(1.4f * l->diffuse()); }

//        if(selected_objects() && l == selected_objects()->parent())
//        {
//            m_light_component->set_index(i);
//            add_selected_object(l);
//        }
    }

    // construct ImGui window for this frame
    if(display_gui())
    {
        gui::draw_component_ui(shared_from_this());
        gui::draw_light_component_ui(m_light_component);
        if(*m_use_warping){ gui::draw_component_ui(m_warp_component); }

        auto obj = selected_objects().empty() ? nullptr : *selected_objects().begin();
        gui::draw_object3D_ui(obj, camera());
        gui::draw_scenegraph_ui(scene(), &m_selected_objects);

//        // draw tasks
//        auto tasks = Task::current_tasks();
//        std::stringstream ss;
//        for(auto &t : tasks){ ss << t->description() << " (" << to_string(t->duration(), 2) << " s)\n"; }
//        ImGui::Begin("Tasks");
//        ImGui::Text("%s", ss.str().c_str());
//        ImGui::End();
    }

    update_shader();
    update_fbos();

    process_joystick(timeDelta);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::draw()
{
    gl::clear();
    gl::set_matrices(camera());

    auto draw_fn = [this]()
    {
        if(draw_grid()){ gl::draw_grid(50, 50); }

        // render bones
        if(*m_display_bones){ render_bones(m_mesh, camera(), true); }

        for(auto &obj : selected_objects())
        {
            auto aabb = obj->aabb();
            gl::draw_boundingbox(aabb);
//            vec2 p2d = gl::project_point_to_screen(aabb.center() + camera()->up() * aabb.halfExtents().y, camera());
//            std::string txt_label = m_selected_object->name() + "\nvertices: " + to_string(m_selected_object->geometry()->vertices().size()) +
//                    "\nfaces: " + to_string(m_selected_object->geometry()->faces().size());
//            gl::draw_text_2D(txt_label, fonts()[0], gl::COLOR_WHITE, p2d);
        }

        // draw enabled light dummies
        m_light_component->draw_light_dummies();
    };

    if(*m_use_post_process)
    {
        auto tex = gl::render_to_texture(m_post_process_fbo, [this]()
        {
            gl::clear();
            scene()->render(camera());
        });
        m_post_process_mat->add_texture(m_post_process_fbo->texture());
        m_post_process_mat->add_texture(m_post_process_fbo->depth_texture(),
                                        gl::Texture::Usage::DEPTH);
        textures()[TEXTURE_OFFSCREEN] = tex;
    }
    if(*m_use_warping)
    {
        if(*m_use_post_process)
        {
            textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo, [this, draw_fn]()
            {
                gl::clear();
                gl::draw_quad(gl::window_dimension(), m_post_process_mat);
                draw_fn();
            });
        }
        else
        {
            textures()[TEXTURE_OUTPUT] = gl::render_to_texture(m_offscreen_fbo, [this, draw_fn]()
            {
                gl::clear();
                scene()->render(camera());
                draw_fn();
            });
        }
        
        for(uint32_t i = 0; i < m_warp_component->num_warps(); i++)
        {
            if(m_warp_component->enabled(i))
            {
                m_warp_component->render_output(i, textures()[TEXTURE_OUTPUT]);
            }
        }
    }
    else
    {
        if(*m_use_post_process)
        {
            gl::draw_quad(gl::window_dimension(), m_post_process_mat);
            draw_fn();
        }
        else
        {
            scene()->render(camera());
            draw_fn();
        }
    }

    if(*m_draw_fps)
    {
        gl::draw_text_2D(to_string(fps(), 1), fonts()[0],
                         glm::mix(gl::COLOR_OLIVE, gl::COLOR_WHITE,
                                  glm::smoothstep(0.f, 1.f, fps() / max_fps())),
                         gl::vec2(10));
    }

    // draw texture map(s)
    if(display_gui())
    {
        std::vector<gl::Texture> display_textures;

        if(*m_use_deferred_render && m_deferred_renderer->g_buffer())
        {
            uint32_t  i = 0;

            for(; i < m_deferred_renderer->g_buffer()->format().num_color_buffers; ++i)
            {
                textures()[i + 2] = m_deferred_renderer->g_buffer()->texture(i);
            }
            textures()[i + 2] = m_deferred_renderer->final_texture();

            display_textures = textures();
        }
        draw_textures(display_textures);
    }

    if(is_loading())
    {
        draw_load_indicator(gl::vec2(gl::window_dimension().x - 100, 80), 50.f);

        if(display_gui())
        {
//            auto tasks = Task::current_tasks();
//            std::stringstream ss;
//            for(auto &t : tasks){ ss << t->description() << " (" << to_string(t->duration(), 2) << " s)\n"; }
//            ImGui::Begin("Tasks");
//            ImGui::Text("%s", ss.str().c_str());
//            ImGui::End();
        }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_fbos()
{
    // check fbo
    vec2 offscreen_sz = *m_offscreen_resolution;
    vec2 sz = (offscreen_sz.x > 0 && offscreen_sz.y > 0) ?
              *m_offscreen_resolution : gl::window_dimension();

    if(*m_use_warping)
    {
        if(!m_offscreen_fbo || m_offscreen_fbo->size() != ivec2(sz) || m_dirty_g_buffer)
        {
            gl::Fbo::Format fmt;
            try{ m_offscreen_fbo = gl::Fbo::create(sz, fmt); }
            catch(Exception &e){ LOG_WARNING << e.what(); }
        }
    }
    if(*m_use_post_process)
    {
        if(!m_post_process_fbo || m_post_process_fbo->size() != ivec2(sz) || m_dirty_g_buffer)
        {
            gl::Fbo::Format fmt;
            try{ m_post_process_fbo = gl::Fbo::create(sz, fmt); }
            catch(Exception &e){ LOG_WARNING << e.what(); }
        }

        // check material
        if(!m_post_process_mat)
        {
            try
            {
                gl::ShaderPtr shader = create_shader(gl::ShaderType::DEPTH_OF_FIELD);
                m_post_process_mat = gl::Material::create(shader);
            }catch(Exception &e){ LOG_WARNING << e.what(); }
        }

        camera()->set_clipping(0.1f, 5000.f);
        m_post_process_mat->uniform("u_znear", camera()->near());
        m_post_process_mat->uniform("u_zfar", camera()->far());
        m_post_process_mat->uniform("u_focal_depth", *m_focal_depth);
        m_post_process_mat->uniform("u_focal_length", *m_focal_length);
        m_post_process_mat->uniform("u_fstop", *m_fstop);
        m_post_process_mat->uniform("u_gain", *m_gain);
        m_post_process_mat->uniform("u_fringe", *m_fringe);
        m_post_process_mat->uniform("u_debug_focus", *m_debug_focus);
        m_post_process_mat->uniform("u_auto_focus", *m_auto_focus);
        m_post_process_mat->uniform("u_circle_of_confusion_sz", *m_circle_of_confusion_sz);
    }
    
    if(m_dirty_g_buffer)
    {
//        m_deferred_renderer = gl::DeferredRenderer::create();
        if(*m_use_deferred_render){ scene()->set_renderer(m_deferred_renderer); }
        m_deferred_renderer->set_g_buffer_resolution(*m_offscreen_resolution);
        m_deferred_renderer->set_use_fxaa(*m_use_fxaa);
        m_deferred_renderer->set_enviroment_light_strength(*m_enviroment_strength);
        m_dirty_g_buffer = false;
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
    m_dirty_g_buffer = true;
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_press(const KeyEvent &e)
{
    ViewerApp::key_press(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::key_release(const KeyEvent &e)
{
    ViewerApp::key_release(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_press(const MouseEvent &e)
{
    ViewerApp::mouse_press(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_release(const MouseEvent &e)
{
    ViewerApp::mouse_release(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_move(const MouseEvent &e)
{
    ViewerApp::mouse_move(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_drag(const MouseEvent &e)
{
    ViewerApp::mouse_drag(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::mouse_wheel(const MouseEvent &e)
{
    ViewerApp::mouse_wheel(e);
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_begin(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_end(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::touch_move(const MouseEvent &e, const std::set<const Touch*> &the_touches)
{
    
}

/////////////////////////////////////////////////////////////////

void ModelViewer::file_drop(const MouseEvent &e, const std::vector<std::string> &files)
{
    std::vector<gl::Texture> dropped_textures;
    auto obj = dynamic_pointer_cast<gl::Mesh>(scene()->pick(gl::calculate_ray(camera(), e.position())));

    for(const string &f : files)
    {
        LOG_INFO << f;

        switch (fs::get_file_type(f))
        {
            case fs::FileType::MODEL:
                *m_model_path = f;
                return;
//                break;

            case fs::FileType::IMAGE:
            case fs::FileType::DIRECTORY:
            {
                *m_skybox_path = f;
            }
                break;
            default:
                break;
        }
    }
    if(obj == m_ground_mesh)
    {
        LOG_DEBUG << "texture drop on ground";
        m_ground_textures->set(files);
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::teardown()
{
    BaseApp::teardown();
    LOG_PRINT << "ciao " << name();
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_property(const Property::ConstPtr &theProperty)
{
    ViewerApp::update_property(theProperty);

    if(theProperty == m_model_path)
    {
        fs::add_search_path(fs::get_directory_part(*m_model_path));
        async_load_asset(*m_model_path, [this](gl::MeshPtr m)
        {
            if(m)
            {
                clear_selected_objects();
                scene()->remove_object(m_mesh);
                m_mesh = m;
                scene()->add_object(m_mesh);
                m_animation_index->set_range(0.f, m_mesh->animations().size() - 1);
                m_dirty_shader = true;
            }
        });
    }
    else if(theProperty == m_use_deferred_render)
    {
        if(*m_use_deferred_render && !m_deferred_renderer)
        {
            m_deferred_renderer = gl::DeferredRenderer::create();
            m_dirty_g_buffer = true;
        }
        scene()->set_renderer(*m_use_deferred_render ? m_deferred_renderer : gl::SceneRenderer::create());
    }
    else if(theProperty == m_shadow_cast ||
            theProperty == m_shadow_receive)
    {
        if(m_mesh)
        {
            for(auto &mat : m_mesh->materials())
            {
                int val = (*m_shadow_cast ? gl::Material::SHADOW_CAST : 0) |
                        (*m_shadow_receive ? gl::Material::SHADOW_RECEIVE : 0);
                mat->set_shadow_properties(val);
            }
        }
    }
    else if(theProperty == m_normalmap_path)
    {
        if(!m_normalmap_path->value().empty())
        {
            m_normal_map.reset();
            
            async_load_texture(*m_normalmap_path, [this](const gl::Texture &t)
            {
                m_normal_map = t;
                m_dirty_shader = true;
            }, true, false, 8.f);
        }
    }
    else if(theProperty == m_offscreen_resolution){ m_dirty_g_buffer = true; }
    else if(theProperty == m_use_fxaa){ m_dirty_g_buffer = true; }
    else if(theProperty == m_use_normal_map){ m_dirty_shader = true; }
    else if(theProperty == m_use_bones){ m_dirty_shader = true; }
    else if(theProperty == m_use_lighting){ m_dirty_shader = true; }
    else if(theProperty == m_wireframe)
    {
        if(m_mesh)
        {
            for(auto &mat : m_mesh->materials())
            {
                mat->set_wireframe(*m_wireframe);
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
    else if(theProperty == m_skybox_path)
    {
        async_load_texture(*m_skybox_path, [this](const gl::Texture &t)
        {
            scene()->set_skybox(t);
        }, false, false);
    }
    else if(theProperty == m_use_ground_plane)
    {
        auto objs = scene()->get_objects_by_tag(tag_ground_plane);

        for(auto &o : objs)
        {
            o->set_enabled(*m_use_ground_plane);
        }
    }
    else if(theProperty == m_ground_textures)
    {
        m_ground_mesh->material()->clear_textures();
        int i = 0;
        for(const auto &f : m_ground_textures->value())
        {
            m_ground_mesh->material()->enqueue_texture(f, 1 << i++);
        }
    }
    else if(theProperty == m_ground_plane_texture_scale)
    {
        for(auto &pair : m_ground_mesh->material()->textures())
        {
            auto t = pair.second;
            t.set_texcoord_scale(gl::vec2(*m_ground_plane_texture_scale));
            t.set_wrap(GL_REPEAT, GL_REPEAT);
        }
    }
    else if(theProperty == m_focal_length)
    {
        auto range_prop = std::dynamic_pointer_cast<RangedProperty<float>>(m_focal_depth);
        range_prop->set_range(0.f, m_focal_length->value());

        range_prop = std::dynamic_pointer_cast<RangedProperty<float>>(m_fstop);
        range_prop->set_range(0.f, m_focal_length->value());
    }
    else if(theProperty == m_enviroment_strength)
    {
        if(m_deferred_renderer){ m_deferred_renderer->set_enviroment_light_strength(*m_enviroment_strength); }
    }
}

/////////////////////////////////////////////////////////////////

void ModelViewer::build_skeleton(gl::BonePtr currentBone, const glm::mat4 start_transform,
                                 vector<gl::vec3> &points, vector<string> &bone_names)
{
    if(!currentBone) return;

    // read out the bone names
    bone_names.push_back(currentBone->name);

    for (auto child_bone : currentBone->children)
    {
        mat4 globalTransform = start_transform * currentBone->worldtransform;
        mat4 childGlobalTransform = start_transform * child_bone->worldtransform;
        points.push_back(globalTransform[3].xyz());
        points.push_back(childGlobalTransform[3].xyz());

        build_skeleton(child_bone, start_transform, points, bone_names);
    }
}

/////////////////////////////////////////////////////////////////

gl::MeshPtr ModelViewer::load_asset(const std::string &the_path)
{
    gl::MeshPtr m;

    auto asset_dir = fs::get_directory_part(the_path);
    fs::add_search_path(asset_dir);

    switch (fs::get_file_type(the_path))
    {
        case fs::FileType::DIRECTORY:
            for(const auto &p : fs::get_directory_entries(the_path)){ load_asset(p); }
            break;

        case fs::FileType::MODEL:
            m = assimp::load_model(the_path);
            break;

        case fs::FileType::IMAGE:
        
        {
            auto geom = gl::Geometry::create_plane(1.f, 1.f, 100, 100);
            auto mat = gl::Material::create();
            m = gl::Mesh::create(geom, mat);
            m->transform() = rotate(mat4(), 90.f, gl::Y_AXIS);
            
            async_load_texture(the_path, [m](const gl::Texture &t)
            {
                m->material()->add_texture(t, gl::Texture::Usage::COLOR);
                m->material()->set_two_sided();
                gl::vec3 s = m->scale();
                m->set_scale(gl::vec3(s.x * t.aspect_ratio(), s.y, 1.f));
                m->position().y += m->aabb().height() / 2.f;
            });
        }
            break;

        default:
            break;
    }

    if(m)
    {
        // apply scaling
        float scale_factor = 50.f / length(m->aabb().halfExtents());
        m->set_scale(scale_factor);
        
        // set position
        m->position().y = m->aabb().height() / 2.f - m->aabb().center().y;
        
        // look for animations for this mesh
        auto animation_folder = fs::join_paths(asset_dir, "animations");

        for(const auto &f : get_directory_entries(animation_folder, fs::FileType::MODEL))
        {
            assimp::add_animations_to_mesh(f, m);
        }
        m->set_animation_speed(*m_animation_speed);
        m->set_animation_index(*m_animation_index);
    }
    return m;
}

/////////////////////////////////////////////////////////////////

void ModelViewer::async_load_asset(const std::string &the_path,
                                   std::function<void(gl::MeshPtr)> the_completion_handler)
{
    auto task = Task::create("load asset: " + the_path);

    background_queue().submit([this, task, the_completion_handler]()
    {
        // load model on worker thread
        auto m = load_asset(*m_model_path);

        if(m)
        {
            // load and decode images on worker thread
            for(auto &mat : m->materials())
            {
                mat->set_wireframe(wireframe());
                int val = (*m_shadow_cast ? gl::Material::SHADOW_CAST : 0) |
                          (*m_shadow_receive ? gl::Material::SHADOW_RECEIVE : 0);
                mat->set_shadow_properties(val);
            }
        }

        // work on this thread done, now queue texture creation on main queue
        main_queue().submit([m, task, the_completion_handler]()
        {
            the_completion_handler(m);
        });
    });
}

/////////////////////////////////////////////////////////////////

void ModelViewer::update_shader()
{
    if(m_mesh && m_dirty_shader)
    {
        m_dirty_shader  = false;
        
        bool use_bones = m_mesh->geometry()->has_bones() && *m_use_bones;
        bool use_normal_map = *m_use_normal_map && *m_use_lighting;
        gl::ShaderPtr shader;
        gl::ShaderType type;

#if defined(KINSKI_GLES_2)
        auto lit = gl::ShaderType::GOURAUD;
        auto lit_skin = gl::ShaderType::GOURAUD_SKIN;
#else
        auto lit = gl::ShaderType::PHONG;
        auto lit_skin = gl::ShaderType::PHONG_SKIN;
#endif

        if(use_bones)
        {
            type = *m_use_lighting ? lit_skin : gl::ShaderType::UNLIT_SKIN;
        }
        else
        {
            type = *m_use_lighting ? lit : gl::ShaderType::UNLIT;
        }

        if(use_normal_map)
        {
            LOG_DEBUG << "adding normalmap: '" << m_normalmap_path->value() << "'";
            type = gl::ShaderType::PHONG_NORMALMAP;
        }

        try{ shader = gl::create_shader(type); }
        catch(Exception &e){ LOG_WARNING << e.what(); }

        auto t = (uint32_t)gl::Texture::Usage::NORMAL;

        for(auto &mat : m_mesh->materials())
        {
            if(shader){ mat->set_shader(shader); }

            if(use_normal_map && !mat->has_texture(t))
            {
                mat->enqueue_texture(*m_normalmap_path, t);
            }
        }
    }
}

void ModelViewer::render_bones(const gl::MeshPtr &the_mesh, const gl::CameraPtr &the_cam,
                               bool use_labels)
{
    if(!the_mesh || !the_cam){ return; }
    
    // crunch bone data
    vector<vec3> skel_points;
    vector<string> bone_names;
    build_skeleton(the_mesh->root_bone(), the_mesh->global_transform(), skel_points, bone_names);
    gl::load_matrix(gl::MODEL_VIEW_MATRIX, the_cam->view_matrix());
    
    // draw bones
    gl::draw_lines(skel_points, gl::COLOR_DARK_RED, 5.f);
    
    for(const auto &p : skel_points)
    {
        vec2 p2d = gl::project_point_to_screen(p, the_cam);
        gl::draw_circle(p2d, 5.f, gl::COLOR_WHITE, false);
    }
    
    if(use_labels)
    {
        // draw labels
        for(uint32_t i = 1; i < skel_points.size(); i += 2)
        {
            vec2 p2d = gl::project_point_to_screen(mix(skel_points[i], skel_points[i - 1], 0.5f), the_cam);
            gl::draw_text_2D(bone_names[i / 2], fonts()[0], gl::COLOR_WHITE, p2d);
        }
    }
}

void ModelViewer::process_joystick(float the_time_delta)
{
    for(auto &j : get_joystick_states())
    {
        if(m_mesh)
        {
            auto val = (j.trigger() + gl::vec2(1)) / 2.f;
            m_mesh->transform() = glm::rotate(m_mesh->transform(),
                                              the_time_delta * (val.y - val.x) * 7.f,
                                              gl::Y_AXIS);
            
            if(!m_mesh->animations().empty())
            {
                *m_animation_index += 2.f * the_time_delta * j.dpad().x;
                *m_animation_speed -= .5f * the_time_delta * j.dpad().y;
            }
        }
    }
}

void ModelViewer::draw_load_indicator(const gl::vec2 &the_screen_pos, float the_size)
{
    constexpr float rot_speed = -3.f;
    gl::ScopedMatrixPush mv_scope(gl::MODEL_VIEW_MATRIX), proj_scope(gl::PROJECTION_MATRIX);
    gl::set_projection(gui_camera());

    if(!m_load_indicator)
    {
        m_load_indicator = gl::Mesh::create(gl::Geometry::create_plane(1.f, 1.f));
        m_load_indicator->material()->set_diffuse(gl::COLOR_RED);
        m_load_indicator->material()->set_depth_write(false);
        m_load_indicator->material()->set_depth_test(false);
        m_load_indicator->material()->set_blending();
    }
    m_load_indicator->set_position(gl::vec3(the_screen_pos.x, gl::window_dimension().y - the_screen_pos.y, 0.f));
    m_load_indicator->set_scale(gl::vec3(the_size, the_size, 1.f));
    m_load_indicator->set_rotation(glm::rotate(glm::quat(), (float)get_application_time() * rot_speed, gl::Z_AXIS));
    gl::load_matrix(gl::MODEL_VIEW_MATRIX, m_load_indicator->transform());
    gl::draw_mesh(m_load_indicator);
}
