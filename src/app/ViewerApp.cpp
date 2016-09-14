// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  ViewerApp.cpp
//
//  Created by Fabian on 3/1/13.

#include "core/Image.hpp"
#include "ViewerApp.hpp"
#include "app/LightComponent.hpp"
#include "app/MaterialComponent.hpp"

namespace kinski {

    ViewerApp::ViewerApp(int argc, char *argv[]):BaseApp(argc, argv),
    m_camera(new gl::PerspectiveCamera),
    m_scene(gl::Scene::create()),
    m_gui_camera(gl::OrthographicCamera::create(0, 1, 0, 1, 0, 1)),
    m_precise_selection(true),
    m_center_selected(false),
    m_rotation_damping (.9)
    {
        /*********** init our application properties ******************/
        m_search_paths = Property_<std::vector<std::string> >::create("File search paths",
                                                                      std::vector<std::string>());
        m_search_paths->set_tweakable(false);
        register_property(m_search_paths);

        m_logger_severity = RangedProperty<int>::create("logger severity",
                                                        (int)kinski::Severity::INFO, 0, 9);
        register_property(m_logger_severity);

        m_show_tweakbar = Property_<bool>::create("show Tweakbar", false);
        m_show_tweakbar->set_tweakable(false);
        register_property(m_show_tweakbar);

        m_hide_cursor = Property_<bool>::create("hide cursor", false);
        register_property(m_hide_cursor);
        
        m_window_size = Property_<glm::vec2>::create("window Size", gl::window_dimension());
        m_window_size->set_tweakable(false);
        register_property(m_window_size);
        
        m_fullscreen = Property_<bool>::create("fullscreen", false);
        register_property(m_fullscreen);

        m_look_at = Property_<glm::vec3>::create("look at", glm::vec3());
        register_property(m_look_at);

        m_distance = RangedProperty<float>::create("view distance", 25, 0, 7500);
        m_distance->set_tweakable(false);
        register_property(m_distance);

        m_camera_fov = Property_<float>::create("camera fov", 45.f);
        register_property(m_camera_fov);

        m_rotation = Property_<glm::mat3>::create("geometry rotation", glm::mat3());
        m_rotation->set_tweakable(false);
        register_property(m_rotation);

        m_rotation_speed = RangedProperty<float>::create("rotation speed", 0, -100, 100);
        register_property(m_rotation_speed);

        m_rotation_axis = Property_<glm::vec3>::create("rotation axis", glm::vec3(0, 1, 0));
        register_property(m_rotation_axis);

        m_draw_grid = Property_<bool>::create("draw grid", true);
        register_property(m_draw_grid);

        m_wireframe = Property_<bool>::create("wireframe", false);
        register_property(m_wireframe);

        m_drawNormals = Property_<bool>::create("normals", false);
        register_property(m_drawNormals);

        m_clear_color = Property_<glm::vec4>::create("clear color", gl::COLOR_BLACK);
        register_property(m_clear_color);

        register_function("load_settings", [this](const std::vector<std::string>&){ load_settings(); });
        register_function("save_settings", [this](const std::vector<std::string>&){ save_settings(); });
        register_function("generate_snapshot", [this](const std::vector<std::string>&){ generate_snapshot(); });
    }

    ViewerApp::~ViewerApp()
    {

    }

    void ViewerApp::setup()
    {
        set_window_title(name());
        
//        m_drag_buffer.set_capacity(5);
        
        // find font file
        std::string font_path;
        try { font_path = fs::search_file("Courier New Bold.ttf"); }
        catch (fs::FileNotFoundException &e){ LOG_DEBUG << e.what(); }
        
        if(font_path.empty())
        {
            for(auto &search_path : fs::get_search_paths())
            {
                auto font_paths = get_directory_entries(search_path, fs::FileType::FONT, true);
                if(!font_paths.empty()){ font_path = font_paths.front(); break; }
            }
        }
        
        // still no font!?
        if(font_path.empty())
        {
            LOG_WARNING << "no font-file found!\n"
            "fonts (*.ttf, *.otf, ...) can be placed next to the executable or in </usr/local/share/fonts>";
        }
        else{ fonts()[0].load(font_path, 18); }
        
        outstream_gl().set_color(gl::COLOR_WHITE);
        outstream_gl().set_font(fonts()[0]);

        gl::Shader unlit_shader = gl::create_shader(gl::ShaderType::UNLIT);

        for (int i = 0; i < 8; i++)
        {
            // lights
            auto light = std::make_shared<gl::Light>(gl::Light::POINT);
            light->set_enabled(false);
            lights().push_back(light);

            // materials
            auto material = gl::Material::create(unlit_shader);
            materials().push_back(material);
        }
        // viewer provides a directional light
        lights().front()->position() = glm::vec3(1);
        lights().front()->set_type(gl::Light::DIRECTIONAL);
        
        for(auto l : lights()){ scene()->addObject(l); }
        
        // enable observer mechanism
        observe_properties();

        // setup our light component
        m_light_component = std::make_shared<LightComponent>();
        m_light_component->set_lights(lights());

        // setup remote control
        m_remote_control = RemoteControl(main_queue().io_service(), {shared_from_this(), m_light_component});
        m_remote_control.start_listen();
    }

    void ViewerApp::update(float timeDelta)
    {
        m_camera->setAspectRatio(gl::aspect_ratio());
//        m_drag_buffer.push(glm::vec2(0));
        m_inertia *= m_rotation_damping;

        // rotation from inertia
        if(!m_mouse_down && glm::length2(m_inertia) > 0.0025)
        {
            *m_rotation = glm::mat3_cast(glm::quat(*m_rotation) *
                                         glm::quat(glm::vec3(glm::radians(-m_inertia.y),
                                                             glm::radians(-m_inertia.x), 0)));
        }
        // rotation from fixed rotationspeed
        else if(!m_mouse_down || displayTweakBar())
        {
            *m_rotation = glm::mat3( glm::rotate(glm::mat4(m_rotation->value()),
                                                 *m_rotation_speed * timeDelta,
                                                 m_rotation_axis->value()));
        }

        // update joysticks
//        for(auto &joystick : get_joystick_states())
//        {
//            float min_val = .38f, multiplier = 1.2f;
//            float x_axis = abs(joystick.axis()[0]) > min_val ? joystick.axis()[0] : 0.f;
//            float y_axis = abs(joystick.axis()[1]) > min_val ? joystick.axis()[1] : 0.f;
//
//            *m_rotation = glm::mat3(glm::rotate(glm::mat4(m_rotation->value()), multiplier * x_axis, gl::Y_AXIS));
//            *m_rotation = glm::mat3(glm::rotate(glm::mat4(m_rotation->value()), multiplier * y_axis, gl::Z_AXIS));
//
//            if(joystick.buttons()[4]){ *m_distance += 5.f; }
//            if(joystick.buttons()[5]){ *m_distance -= 5.f; }
//        }

        // update animations
        for(auto &anim : m_animations)
        {
            if(anim){ anim->update(timeDelta); }
        }

        m_scene->update(timeDelta);
    }

    void ViewerApp::mousePress(const MouseEvent &e)
    {
        m_clickPos = glm::vec2(e.getX(), e.getY());
        m_lastTransform = *m_rotation;
        m_look_at_tmp = *m_look_at;
        m_mouse_down = true;
        
        if(e.isLeft() || e.is_touch())
        {
            m_drag_buffer.clear();
            
            gl::Object3DPtr picked_obj = m_scene->pick(gl::calculate_ray(m_camera, glm::vec2(e.getX(),
                                                                                           e.getY())),
                                                      m_precise_selection);
            if(picked_obj)
            {
                LOG_TRACE << "picked id: " << picked_obj->get_id();
                if(gl::MeshPtr m = std::dynamic_pointer_cast<gl::Mesh>(picked_obj))
                {
                    m_selected_mesh = m;
                }
            }
        }

        if(e.isRight())
        {
            m_selected_mesh.reset();
        }
    }

    void ViewerApp::mouseDrag(const MouseEvent &e)
    {
        glm::vec2 mouseDiff = glm::vec2(e.getX(), e.getY()) - m_clickPos;

        if(e.isLeft() || e.is_touch())
        {

            if(e.isAltDown() || !displayTweakBar())
            {
                *m_rotation = glm::mat3_cast(glm::quat(m_lastTransform) *
                                             glm::quat(glm::vec3(glm::radians(-mouseDiff.y),
                                                                 glm::radians(-mouseDiff.x), 0)));

            }
            m_drag_buffer.push(glm::vec2(e.getX(), e.getY()) - m_dragPos);
            m_dragPos = glm::vec2(e.getX(), e.getY());
        }
        else if(e.isRight())
        {
            mouseDiff /= gl::window_dimension();
            mouseDiff *= *m_distance;
            *m_look_at = m_look_at_tmp - camera()->side() * mouseDiff.x + camera()->up() * mouseDiff.y;
        }
    }

    void ViewerApp::mouseRelease(const MouseEvent &e)
    {
        m_mouse_down = false;
        if(!displayTweakBar())
            m_inertia = kinski::mean<glm::vec2>(m_drag_buffer);
    }

    void ViewerApp::mouseWheel(const MouseEvent &e)
    {
        *m_distance -= e.getWheelIncrement().y;
    }

    void ViewerApp::keyPress(const KeyEvent &e)
    {
        BaseApp::keyPress(e);

        if(e.getCode() == Key::_SPACE)
        {
            *m_show_tweakbar = !*m_show_tweakbar;
        }

        if(!displayTweakBar())
        {
            switch (e.getCode())
            {
                case Key::_C:
                    m_center_selected = !m_center_selected;
                    break;

                case Key::_S:
                    save_settings();
                    break;

                case Key::_F:
                    set_fullscreen(!fullscreen(), windows().front()->monitor_index());
                    break;
                    
                case Key::_H:
                    *m_hide_cursor = !*m_hide_cursor;
                    break;

                case Key::_R:
                    try
                    {
                        m_inertia = glm::vec2(0);
                        m_selected_mesh.reset();
                        load_settings();
                    }catch(Exception &e)
                    {
                        LOG_WARNING << e.what();
                    }
                    break;

                case Key::_1:
                case Key::_2:
                case Key::_3:
                case Key::_4:
                case Key::_5:
                case Key::_6:
                case Key::_7:
                case Key::_8:
                case Key::_9:
//                    m_cam_index = e.getCode() - GLFW__1;
//                    LOG_DEBUG << "cam index: " << m_cam_index;
                    break;

                default:
                    break;
            }
        }
    }

    void ViewerApp::resize(int w, int h)
    {
        BaseApp::resize(w, h);
        *m_window_size = glm::vec2(w, h);
        App::set_window_size(*m_window_size);
        set_clear_color(clear_color());
    }

    // Property observer callback
    void ViewerApp::update_property(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_search_paths)
        {
            for (const auto &search_path : m_search_paths->value())
            {
                fs::add_search_path(search_path);
            }
        }
        else if(theProperty == m_logger_severity)
        {
            Logger::get()->set_severity(static_cast<Severity>(m_logger_severity->value()));
        }
        else if(theProperty == m_show_tweakbar)
        {
            displayTweakBar(*m_show_tweakbar);
        }
        else if(theProperty == m_hide_cursor)
        {
            set_cursor_visible(!*m_hide_cursor);
        }
        else if(theProperty == m_window_size)
        {
            if(!fullscreen())
            {
                gl::set_window_dimension(*m_window_size);
                m_gui_camera = gl::OrthographicCamera::create(0, gl::window_dimension().x, 0,
                                                              gl::window_dimension().y, 0, 1.f);
                
                if(!windows().empty()){ windows().front()->set_size(*m_window_size); }
            }
        }
        else if(theProperty == m_fullscreen)
        {
            if(*m_fullscreen != fullscreen())
            {
                set_fullscreen(*m_fullscreen, windows().front()->monitor_index());
            }
        }
        else if(theProperty == m_clear_color)
        {
            gl::clear_color(*m_clear_color);
            outstream_gl().set_color(glm::vec4(1.f) - m_clear_color->value());
        }
        else if(theProperty == m_camera_fov)
        {
            m_camera->setFov(*m_camera_fov);
        }
        else if(theProperty == m_distance || theProperty == m_rotation ||
                theProperty == m_look_at)
        {
            glm::vec3 look_at = *m_look_at;
            if(m_selected_mesh && m_center_selected)
                look_at = gl::OBB(m_selected_mesh->boundingBox(), m_selected_mesh->transform()).center;

            glm::mat4 tmp = glm::mat4(m_rotation->value());
            tmp[3] = glm::vec4(look_at + m_rotation->value()[2] * m_distance->value(), 1.0f);
            m_camera->transform() = tmp;
        }
    }

    bool ViewerApp::save_settings(const std::string &the_path)
    {
        App::Task t(this);
        
        std::string path_prefix = the_path.empty() ? m_default_config_path : the_path;
        path_prefix = fs::get_directory_part(path_prefix);
        
        LOG_DEBUG << "save settings to: " << path_prefix;
        
        std::list<Component::Ptr> light_components, material_components;
        for (uint32_t i = 0; i < lights().size(); i++)
        {
            LightComponent::Ptr tmp(new LightComponent());
            tmp->set_name("light_" + to_string(i));
            tmp->set_lights(lights());
            tmp->set_index(i);
            light_components.push_back(tmp);
        }
        for (uint32_t i = 0; i < materials().size(); i++)
        {
            MaterialComponent::Ptr tmp(new MaterialComponent());
            tmp->set_name("material_" + to_string(i));
            tmp->set_materials(materials());
            tmp->set_index(i);
            material_components.push_back(tmp);
        }
        try
        {
            Serializer::saveComponentState(shared_from_this(),
                                           fs::join_paths(path_prefix ,"config.json"),
                                           PropertyIO_GL());
            Serializer::saveComponentState(light_components,
                                           fs::join_paths(path_prefix ,"light_config.json"),
                                           PropertyIO_GL());
            Serializer::saveComponentState(material_components,
                                           fs::join_paths(path_prefix ,"material_config.json"),
                                           PropertyIO_GL());

        }
        catch(Exception &e)
        {
            LOG_ERROR << e.what();
            return false;
        }
        return true;
    }

    bool ViewerApp::load_settings(const std::string &the_path)
    {
        App::Task t(this);
        
        std::string path_prefix = the_path.empty() ? m_default_config_path : the_path;
        path_prefix = fs::get_directory_part(path_prefix);
        LOG_DEBUG << "load settings from: " << path_prefix;
        
        std::list<Component::Ptr> light_components, material_components;
        for (uint32_t i = 0; i < lights().size(); i++)
        {
            LightComponent::Ptr tmp(new LightComponent());
            tmp->set_name("light_" + to_string(i));
            tmp->set_lights(lights(), false);
            tmp->set_index(i);
            tmp->observe_properties();
            light_components.push_back(tmp);
        }
        for (uint32_t i = 0; i < materials().size(); i++)
        {
            MaterialComponent::Ptr tmp(new MaterialComponent());
            tmp->set_name("material_" + to_string(i));
            tmp->set_materials(materials(), false);
            tmp->set_index(i);
            tmp->observe_properties();
            material_components.push_back(tmp);
        }
        try
        {
            Serializer::loadComponentState(shared_from_this(),
                                           fs::join_paths(path_prefix , "config.json"),
                                           PropertyIO_GL());
            Serializer::loadComponentState(light_components,
                                           fs::join_paths(path_prefix , "light_config.json"),
                                           PropertyIO_GL());
            Serializer::loadComponentState(material_components,
                                           fs::join_paths(path_prefix , "material_config.json"),
                                           PropertyIO_GL());
        }
        catch(Exception &e)
        {
            LOG_ERROR<<e.what();
            for(auto p : get_property_list()){ p->notify_observers(); }
            return false;
        }

        m_light_component->refresh();

        return true;
    }

    void ViewerApp::draw_textures(const std::vector<gl::Texture> &the_textures)
    {
        float w = (gl::window_dimension() / 12.f).x;
        glm::vec2 offset(gl::window_dimension().x - w - 10, 10);

        for (const gl::Texture &t : the_textures)
        {
            if(!t) continue;

            float h = t.getHeight() * w / t.getWidth();
            glm::vec2 step(0, h + 10);

            gl::draw_texture(t, glm::vec2(w, h), offset);
            gl::draw_text_2D(to_string(t.getWidth()) + std::string(" x ") +
                           to_string(t.getHeight()), m_fonts[0], glm::vec4(1),
                           offset);
            offset += step;
        }
    }
    
    void ViewerApp::async_load_texture(const std::string &the_path,
                                       std::function<void(const gl::Texture&)> the_callback,
                                       bool mip_map,
                                       bool compress,
                                       GLfloat anisotropic_filter_lvl)
    {
        inc_task();
        background_queue().submit([this, the_path, the_callback, mip_map, compress,
                                   anisotropic_filter_lvl]()
        {
            auto img = create_image_from_file(the_path);
          
            main_queue().submit([this, img, the_callback, mip_map, compress, anisotropic_filter_lvl]()
            {
                auto tex = gl::create_texture_from_image(img, mip_map, compress,
                                                         anisotropic_filter_lvl);
                the_callback(tex);
                dec_task();
            });
        });
    }
    
    gl::Texture ViewerApp::generate_snapshot()
    {
        gl::Texture ret;

        if(!m_fbo_snapshot || m_fbo_snapshot.getSize() != gl::window_dimension())
        {
            gl::Fbo::Format fmt;
            fmt.setSamples(8);
            m_fbo_snapshot = gl::Fbo(gl::window_dimension(), fmt);
        }

        ret = gl::render_to_texture(m_fbo_snapshot, [this]()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            draw();
        });
        m_snapshot_texture = ret;
        return ret;
    }
}
