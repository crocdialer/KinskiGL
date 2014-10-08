//
//  KristallApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "KristallApp.h"

#include "Fmod_Sound.h"
#include "DrawHelper.h"

#include <random>

using namespace std;
using namespace kinski;
using namespace glm;

float KristallApp::get_volume_for_subspectrum(float from_freq, float to_freq)
{
    //TODO: no hardcoded frequency here
    float sampling_rate = 44100.f;
    
    from_freq = std::max(0.f, from_freq);
    to_freq = std::min(sampling_rate / 2.f, to_freq);
    
    float fraction_from = from_freq / (sampling_rate / 2.f);
    float fraction_to = to_freq / (sampling_rate / 2.f);
    int from_bin = (int)(m_sound_values.size() * fraction_from);
    int to_bin = (int)(m_sound_values.size() * fraction_to);
    
    if(to_bin - from_bin <= 0)
    {
    
        return 0.f;
    }
    
    // sum values
    float sum = 0;
    for(int i = from_bin; i < to_bin; i++){ sum += m_sound_values[i].last_value(); }
    
    // return arithmetic average
    return sum / (to_bin - from_bin);
}

float KristallApp::get_volume_for_freq_band(FrequencyBand bnd)
{
    auto iter = m_frequency_map.find(bnd);
    if(iter == m_frequency_map.end()) return 0.f;
    
    auto pair = iter->second;
    return get_volume_for_subspectrum(pair.first, pair.second);
}

/////////////////////////////////////////////////////////////////

void KristallApp::setup()
{
    ViewerApp::setup();
    
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    set_precise_selection(false);
    
    registerProperty(m_template_image);
    registerProperty(m_view_option);
    registerProperty(m_use_syphon);
    registerProperty(m_syphon_server_name);
    registerProperty(m_fbo_size);
    registerProperty(m_fbo_cam_height);
    registerProperty(m_fbo_cam_distance);
    registerProperty(m_fbo_fov);
    registerProperty(m_branch_angles);
    registerProperty(m_branch_randomness);
    registerProperty(m_increment);
    registerProperty(m_increment_randomness);
    registerProperty(m_diameter);
    registerProperty(m_diameter_shrink);
    registerProperty(m_cap_bias);
    registerProperty(m_split_limit);
    registerProperty(m_num_iterations);
    registerProperty(m_axiom);
    
    for(auto rule : m_rules)
        registerProperty(rule);
    
    registerProperty(m_aabb_min);
    registerProperty(m_aabb_max);
    registerProperty(m_use_bounding_mesh);
    registerProperty(m_animate_growth);
    registerProperty(m_light_elevation);
    
    // particle properties
    registerProperty(m_use_particle_simulation);
    registerProperty(m_particle_gravity);
    registerProperty(m_particle_bounce);
    
    registerProperty(m_force_multiplier);
    registerProperty(m_activity_thresh);
    registerProperty(m_burst_thresh);

    registerProperty(m_smooth_inc);
    registerProperty(m_smooth_dec);
    
    // timing
    registerProperty(m_time_growth);
    registerProperty(m_time_idle);
    registerProperty(m_time_decay);
    registerProperty(m_time_burst);
    
    // frequency adjustments
    registerProperty(m_freq_low);
    registerProperty(m_freq_mid_low);
    registerProperty(m_freq_mid_high);
    registerProperty(m_freq_high);
    
    observeProperties();
    create_tweakbar_from_component(shared_from_this());
    
    // offscreen assets
    gl::Fbo::Format fmt;
    fmt.setSamples(4);
    m_offscreen_fbos[0] = gl::Fbo(1920, 1080, fmt);
    m_offscreen_fbos[1] = gl::Fbo(1920, 1080, fmt);
    
    m_offscreen_camera.reset(new gl::PerspectiveCamera(16.f / 9, *m_fbo_fov, 600.f, 4500.f));
    m_offscreen_camera->setPosition(vec3(0, 0, 20));
    m_offscreen_camera->setLookAt(vec3(0, 0, -1));
    m_camera_mesh = gl::createFrustumMesh(m_offscreen_camera);
    try
    {
        m_compositing_mat = gl::Material::create(gl::createShaderFromFile("compositing_shader.vert",
                                                                          "compositing_shader.frag"));
    } catch (Exception &e) { LOG_ERROR << e.what(); }
    m_compositing_mat->setBlending();
    m_compositing_mat->setDepthTest(false);
    m_compositing_mat->setDepthWrite(false);
    
    m_debug_scene.addObject(m_camera_mesh);
    
    // occluder mechanism
    gl::GeometryPtr m_occluder_geom = gl::Geometry::createBox(vec3(1));
    gl::MaterialPtr m_occluder_mat = gl::Material::create();
    m_occluder_mat->setDiffuse(gl::Color(m_occluder_mat->diffuse().rgb(), .2));
    m_occluder_mat->setBlending();
    m_occluder_mat->setDepthWrite(false);
    
    for(int i = 0; i < m_occluding_meshes.size(); i++)
    {
        m_occluding_meshes[i] = gl::Mesh::create(m_occluder_geom, m_occluder_mat);
        
        // these values are overwritten by saved values, if present
        m_occluding_meshes[i]->setScale(vec3(60, 60 , 1000));
        m_occluding_meshes[i]->position() += glm::linearRand(vec3(-500, -300, 500),
                                                             vec3(500, 300, 500));
    }
    
    try
    {
        auto aabb = gl::AABB(*m_aabb_min, *m_aabb_max);
        m_bounding_mesh = gl::Mesh::create(gl::Geometry::createBox(aabb.halfExtents()),
                                           gl::Material::create());
        
        m_bounding_mesh->position() += aabb.center();
        
        // some material props
        auto &bound_mat = m_bounding_mesh->material();
        bound_mat->setDiffuse(gl::Color(bound_mat->diffuse().rgb(), .2));
        bound_mat->setBlending();
        bound_mat->setDepthWrite(false);
        
        // load shaders
        
        // split_cubes geometry shader + phong shading
        m_lsystem_shaders[0] = gl::createShaderFromFile("geom_prepass.vert",
                                                        "phong.frag",
                                                        "split_cuboids.geom");
        
        // split_cubes geometry shader + unlit shading
        m_lsystem_shaders[1] = gl::createShaderFromFile("geom_prepass.vert",
                                                        "unlit.frag",
                                                        "split_cuboids.geom");
        
        // split lines + unlit shading
        m_lsystem_shaders[2] = gl::createShaderFromFile("geom_prepass.vert",
                                                        "unlit.frag",
                                                        "split_lines.geom");
        
        m_lsystem_shaders[3] = gl::createShader(gl::SHADER_POINTS_TEXTURE);
        
        m_textures[MASK_TEX] = gl::createTextureFromFile("mask.png", true, false, 4);
        
        m_comp_assets.empty_tex = gl::DrawHelper::create_empty_tex();
        m_comp_assets.template_img = gl::createTextureFromFile(*m_template_image);
        m_comp_assets.background_tex = m_comp_assets.template_img;
        
        // our video texture
        m_movies[0].load("~/Desktop/l_system_animation/vid3.mov", true, true);
    }
    catch(Exception &e){LOG_ERROR << e.what();}
    
    // load settings, lights, materials
    load_settings();
    
    // attach shader to all materials
    materials().resize(4);
    for (auto m : materials())
    {
        m->setShader(m_lsystem_shaders[0]);
        m->addTexture(m_textures[MASK_TEX]);
        m_color_table.push_back(m->diffuse());
    }
    
    // light component
    m_light_component.reset(new LightComponent());
    m_light_component->set_lights(lights());
    create_tweakbar_from_component(m_light_component);
    
    // add lights to scene
    for (auto l : lights()){ scene().addObject(l ); }
        
    // material component
    m_material_component.reset(new MaterialComponent());
    m_material_component->set_materials(materials());
    create_tweakbar_from_component(m_material_component);
    
    // occluder component
    m_occluder_component.reset(new Object3DComponent);
    m_occluder_component->set_objects(m_occluding_meshes);
    create_tweakbar_from_component(m_occluder_component);
    
    auto rec_devices = audio::get_recording_devices();
    
    // print audio input devices
    for(audio::device rec_dev : rec_devices){ LOG_INFO << rec_dev.name; }
    
    // audio recording for fft
    m_sound_recording = std::make_shared<audio::Fmod_Sound>();
    m_sound_recording->set_loop(true);
    m_sound_recording->set_volume(0);
    m_sound_recording->record(.05);
    m_sound_recording->play();
    
    // smoothness filter mechanism
    int num_freq_bands = 256;
    m_sound_spectrum.resize(num_freq_bands);
    m_sound_values.resize(num_freq_bands);
    
    for(auto &measure : m_sound_values)
    {
        auto filter = std::make_shared<FalloffFilter<float>>();
        filter->set_increase_speed(*m_smooth_inc);
        filter->set_decrease_speed(*m_smooth_dec);
        measure.set_filter(filter);
    }
    
    // init opencl particlesystem
    m_particle_system.opencl().init();
    m_particle_system.opencl().set_sources("kernels.cl");
    m_particle_system.add_kernel("updateParticles");
    m_particle_system.add_kernel("apply_forces");
    
    // define our timer objects
    m_idle_timer = Timer(io_service(), std::bind(&KristallApp::change_gamestate, this, IDLE_PHASE));
    m_ready_timer = Timer(io_service(), std::bind(&KristallApp::change_gamestate, this, READY_PHASE));
    m_decay_timer = Timer(io_service(), std::bind(&KristallApp::change_gamestate, this, DECAY_PHASE));
    
    // add tcp remote control
    m_remote_control = RemoteControl(io_service(), {shared_from_this(), m_light_component});
    m_remote_control.start_listen();
    
    
    // start with ready gamephase
    change_gamestate(READY_PHASE);
}

/////////////////////////////////////////////////////////////////

void KristallApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
    // refresh lsystem if necessary
    if(m_dirty_lsystem){ refresh_lsystem(m_lsystem, *m_num_iterations); }
    
    if(m_growth_animation){ m_growth_animation->update(timeDelta); }
    
    // animation stuff here
    update_animations(timeDelta);
    
    // audio analysis
    if(m_sound_recording)
    {
        audio::update();
        m_sound_recording->get_spectrum(m_sound_spectrum, m_sound_spectrum.size());
        
        for(int i = 0; i < m_sound_spectrum.size(); i++)
        {
            m_sound_values[i].push(m_sound_spectrum[i]);
        }
        
        std::vector<FrequencyBand> freq_bands = {FREQ_LOW, FREQ_MID_LOW, FREQ_MID_HIGH, FREQ_HIGH};
        m_last_volumes.resize(4);
        m_last_value = 0;
        for(int i = 0; i < 4; i++)
        {
            m_last_volumes[i] = get_volume_for_freq_band(freq_bands[i]);
            m_last_value += m_last_volumes[i];
        }
        m_last_value /= m_last_volumes.size();
    }
    
    // movie textures
    for(auto &m : m_movies)
    {
        if(m.isPlaying())
        {
            m.copy_frame_to_texture(m_textures[MOVIE_TEX]);
        }
    }
    
    // particle simulation
    if(*m_use_particle_simulation)
    {
        m_particle_system.update(timeDelta);
    }
    
    // update mesh according to frequency
    if(m_mesh && !*m_use_particle_simulation)
    {
        // overall volume
        float val = clamp<float>(m_last_value * *m_force_multiplier, 0.f, 1.f);
        
        if(m_gamestate != DECAY_PHASE)
        {
            m_mesh->entries()[0].num_indices = map_value(val, 0.f, 1.f, .05f, 1.f) * m_entries[0].num_indices;
            
            // minimum activity to stop refresh timer
            if(val > *m_activity_thresh)
            {
                // activity -> stop ready timer
                m_ready_timer.expires_from_now(*m_time_idle);
            }
        }
        
        // burst mechanism
        if(val > *m_burst_thresh)
        {
            if(!m_burst_lvl)
            {
                m_burst_lvl = true;
                LOG_DEBUG << "starting burst timer: " << *m_time_burst << "secs";
                m_decay_timer.expires_from_now(*m_time_burst);
            }
        }
        else
        {
            m_decay_timer.cancel();
            m_burst_lvl = false;
        }
        
        // mid frequency -> video mix in
        for (auto m : m_mesh->materials())
        {
            m->setDiffuse(gl::Color(m->diffuse().rgb(), 1.f));
            m->uniform("u_mix_video", m_last_volumes[FREQ_MID_HIGH] * *m_force_multiplier);
            
            if(m_gamestate != DECAY_PHASE)
            {
                m->uniform("u_cap_bias",
                           (1 + 2.f * *m_force_multiplier * m_last_volumes[FREQ_HIGH]) * *m_cap_bias);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////

void KristallApp::draw()
{
    if(*m_use_syphon)
    {
        auto syphon_tex = composite_output_image();
        
        if(syphon_tex)
            m_syphon.publish_texture(syphon_tex);
        
        m_textures[OFFSCREEN_TEX_0] = syphon_tex;
    }
    
    gl::Texture tex;
    
    // render control view
    switch(*m_view_option)
    {
        case VIEW_DEBUG:
            
            gl::setMatrices(camera());
            
            // grid
            if(draw_grid()){gl::drawGrid(500, 500);}
            
            // draw our scene
            scene().render(camera());
            m_debug_scene.render(camera());
            
            // our bounding mesh
            if(wireframe())
            {
                {
                    gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
                    gl::multMatrix(gl::MODEL_VIEW_MATRIX, m_bounding_mesh->global_transform());
                    gl::drawMesh(m_bounding_mesh);
                }
                for(auto o : m_occluding_meshes)
                {
                    if(!o->enabled()) continue;
                    
                    gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
                    gl::multMatrix(gl::MODEL_VIEW_MATRIX, o->global_transform());
                    gl::drawMesh(o);
                }
            }
            
            if(m_light_component->draw_light_dummies())
            {
                for(auto light : lights())
                {
                    gl::drawLight(light);
                }
            }
            gl::drawTransform(m_lsystem.turtle_transform(), 10);
            
            // draw particle forces
            for (const auto &f : m_particle_system.forces())
            {
                glm::mat4 force_transform;
                force_transform[3] = vec4(f.xyz(), 1.f);
                gl::drawTransform(force_transform, f.w / 100.f);
            }
            break;
            
        case VIEW_OUTPUT:
            gl::drawTexture(m_textures[OFFSCREEN_TEX_0], windowSize());
            break;
            
        case VIEW_OUTPUT_DEBUG:
            gl::setMatrices(m_offscreen_camera);
            gl::drawTexture(m_textures[OFFSCREEN_TEX_0], windowSize());
            for(auto o : m_occluding_meshes)
            {
                if(!o->enabled()) continue;
                
                gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
                gl::multMatrix(gl::MODEL_VIEW_MATRIX, o->global_transform());
                gl::drawMesh(o);
            }
            break;
            
        case VIEW_TEMPLATE:
            gl::drawTexture(m_comp_assets.template_img, windowSize());
            gl::drawTexture(m_textures[OFFSCREEN_TEX_0], windowSize());
            
            gl::setMatrices(m_offscreen_camera);
            {
                gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
                gl::multMatrix(gl::MODEL_VIEW_MATRIX, m_bounding_mesh->global_transform());
                gl::drawMesh(m_bounding_mesh);
            }
            
            for(auto o : m_occluding_meshes)
            {
                if(!o->enabled()) continue;
                
                gl::ScopedMatrixPush sp(gl::MODEL_VIEW_MATRIX);
                gl::multMatrix(gl::MODEL_VIEW_MATRIX, o->global_transform());
                gl::drawMesh(o);
            }
            break;
    }
    
    
    // draw texture map(s)
    if(displayTweakBar())
    {
        float w = (windowSize()/6.f).x;
        glm::vec2 offset(getWidth() - w - 10, 10);
        
        
        for (const gl::Texture &t : m_textures)
        {
            if(!t) continue;
            
            float h = t.getHeight() * w / t.getWidth();
            glm::vec2 step(0, h + 10);
            
            drawTexture(t, vec2(w, h), offset);
            gl::drawText2D(as_string(t.getWidth()) + std::string(" x ") +
                           as_string(t.getHeight()), m_font, glm::vec4(1),
                           offset);
            offset += step;
        }
        
        // draw recording icon
        if(m_sound_recording && m_sound_recording->is_recording())
        {
            gl::drawQuad(gl::COLOR_RED,
                         vec2(70),
                         vec2(20, windowSize().y - 90.f));
        }
        
        // draw fft vals
        for (int i = 0; i < m_last_volumes.size(); i++)
        {
            float v = m_last_volumes[i];
            gl::drawQuad(gl::COLOR_ORANGE,
                         vec2(40, windowSize().y * v),
                         vec2(100 + i * 50, windowSize().y * (1 - v)));
        }
        
        // draw fps string
        gl::drawText2D(kinski::as_string(framesPerSec()), m_font,
                       vec4(vec3(1) - clear_color().xyz(), 1.f),
                       glm::vec2(windowSize().x - 110, windowSize().y - 70));
    }
}

/////////////////////////////////////////////////////////////////

void KristallApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void KristallApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    if(!displayTweakBar())
    {
        switch (e.getCode())
        {
            case GLFW_KEY_LEFT:
                *m_num_iterations -= 1;
                break;
            
            case GLFW_KEY_RIGHT:
                *m_num_iterations += 1;
                break;
                
            case GLFW_KEY_D:
                *m_view_option = (*m_view_option + 1) % 5;
                break;
                
            case GLFW_KEY_E:
                start_decay(2.5f);
                break;
                
            case GLFW_KEY_G:
                
                LOG_DEBUG << m_stop_watch.time_elapsed() << " - laps: " << m_stop_watch.laps().size();
                
                if(!m_stop_watch.running())
                    m_stop_watch.start();
                else
                    m_stop_watch.stop();
                
                break;
                
            case GLFW_KEY_L:
                
                m_stop_watch.new_lap();
                LOG_DEBUG << m_stop_watch.time_elapsed() << " - laps: " << m_stop_watch.laps().size();
                
                break;
                
            case GLFW_KEY_1:
                setup_lsystem(0);
                break;
                
            case GLFW_KEY_2:
                setup_lsystem(1);
                break;
                
            case GLFW_KEY_3:
                setup_lsystem(2);
                break;
                
            default:
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////

void KristallApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void KristallApp::got_message(const std::vector<uint8_t> &the_message)
{
    string msg = string(the_message.begin(), the_message.end());
    LOG_TRACE << msg;
}

/////////////////////////////////////////////////////////////////

void KristallApp::fileDrop(const std::vector<std::string> &files)
{
    for(auto &f : files){ LOG_DEBUG << f;}
}

/////////////////////////////////////////////////////////////////

void KristallApp::tearDown()
{
    LOG_PRINT<<"ciao procedural growth";
}

/////////////////////////////////////////////////////////////////

void KristallApp::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
    
    bool rule_changed = false;
    
    for(auto r : m_rules)
        if (theProperty == r) rule_changed = true;
        
    if(theProperty == m_axiom ||
       rule_changed ||
       theProperty == m_num_iterations ||
       theProperty == m_branch_angles ||
       theProperty == m_branch_randomness ||
       theProperty == m_increment ||
       theProperty == m_increment_randomness ||
       theProperty == m_diameter ||
       theProperty == m_diameter_shrink)
    {
        m_dirty_lsystem = true;
    }
    else if(theProperty == m_cap_bias ||
            theProperty == m_split_limit)
    {
        for (auto m : materials())
        {
            m->uniform("u_cap_bias", *m_cap_bias);
            m->uniform("u_split_limit", *m_split_limit);
        }
    }
    else if(theProperty == m_light_elevation)
    {
        set_light_animation(m_last_value);
    }
    else if(theProperty == m_use_syphon)
    {
        m_syphon = *m_use_syphon ? gl::SyphonConnector(*m_syphon_server_name) : gl::SyphonConnector();
    }
    else if(theProperty == m_syphon_server_name)
    {
        try{m_syphon.setName(*m_syphon_server_name);}
        catch(gl::SyphonNotRunningException &e){LOG_WARNING<<e.what();}
    }
    else if(theProperty == m_fbo_size || theProperty == m_fbo_cam_distance ||
            theProperty == m_fbo_cam_height || theProperty == m_fbo_fov)
    {
        m_offscreen_fbos[0] = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
        m_offscreen_fbos[1] = gl::Fbo(m_fbo_size->value().x, m_fbo_size->value().y);
        
        m_offscreen_camera->setFov(*m_fbo_fov);
        m_offscreen_camera->setAspectRatio(m_fbo_size->value().x / m_fbo_size->value().y);
        m_offscreen_camera->setPosition(vec3(0, *m_fbo_cam_height, *m_fbo_cam_distance));
        
        m_debug_scene.removeObject(m_camera_mesh);
        m_camera_mesh = gl::createFrustumMesh(m_offscreen_camera);
        m_debug_scene.addObject(m_camera_mesh);
    }
    else if(theProperty == m_particle_gravity ||
            theProperty == m_use_bounding_mesh)
    {
        m_particle_system.set_gravity(m_particle_gravity->value() * vec3(0, -1, 0));
        m_particle_system.set_use_constraints(*m_use_bounding_mesh);
        
    }
    else if(theProperty == m_aabb_min ||
            theProperty == m_aabb_max)
    {
        auto aabb = gl::AABB(*m_aabb_min, *m_aabb_max);
        m_bounding_mesh = gl::Mesh::create(gl::Geometry::createBox(aabb.halfExtents()),
                                           gl::Material::create());
        
        m_bounding_mesh->position() += aabb.center();

        // some material props
        auto &bound_mat = m_bounding_mesh->material();
        bound_mat->setDiffuse(gl::Color(bound_mat->diffuse().rgb(), .2));
        bound_mat->setBlending();
        bound_mat->setDepthWrite(false);
    }
    else if(theProperty == m_smooth_dec ||
            theProperty == m_smooth_inc)
    {
        for(auto &measure : m_sound_values)
        {
            auto filter = std::make_shared<FalloffFilter<float>>();
            filter->set_increase_speed(*m_smooth_inc);
            filter->set_decrease_speed(*m_smooth_dec);
            measure.set_filter(filter);
        }
    }
    else if(theProperty == m_freq_low ||
            theProperty == m_freq_mid_low ||
            theProperty == m_freq_mid_high ||
            theProperty == m_freq_high )
    {
        m_frequency_map =
        {
            {FREQ_LOW, std::pair<float, float>(0.f, *m_freq_low)},
            {FREQ_MID_LOW, std::pair<float, float>(*m_freq_low, *m_freq_mid_low)},
            {FREQ_MID_HIGH, std::pair<float, float>(*m_freq_mid_low, *m_freq_mid_high)},
            {FREQ_HIGH, std::pair<float, float>(*m_freq_mid_high, *m_freq_high)}
        };
    }
    else if(theProperty == m_template_image)
    {
        try{ m_comp_assets.template_img = gl::createTextureFromFile(*m_template_image);
        } catch (FileNotFoundException &e){ LOG_ERROR << e.what(); }
    }
}

void KristallApp::update_animations(float time_delta)
{
    for(auto anim : m_animations)
    {
        if(anim)
        {
            anim->update(time_delta);
        }
    }
}

void KristallApp::setup_lsystem(int the_index)
{
    switch(the_index)
    {
    
        case 0:
        // our lsystem shall draw a dragon curve
        *m_branch_angles = vec3(90);
        *m_axiom = "&(180)F";
        *m_num_iterations = 11;
        *m_rules[0] = "F = F - H";
        *m_rules[1] = "H = F + H";
        *m_rules[2] = "";
        *m_rules[3] = "";
        break;
        
        case 1:
        // our lsystem shall draw something else ...
        *m_branch_angles = vec3(90);
        *m_num_iterations = 3;
        *m_axiom = "&(180)-L";
        *m_rules[0] = "L=LF+RFR+FL-F-LFLFL-FRFR+";
        *m_rules[1] = "R=-LFLF+RFRFR+F+RF-LFL-FR";
        *m_rules[2] = "";
        *m_rules[3] = "";
        break;
        
        case 2:
        // our lsystem shall draw something else ...
        *m_branch_angles = vec3(60);
        *m_num_iterations = 4;
        *m_axiom = "&(180)F";
        *m_rules[0] = "F=F+G++G-F--FF-G+";
        *m_rules[1] = "G=-F+GG++G+F--F-G";
        *m_rules[2] = "";
        *m_rules[3] = "";
        break;
    }
}

void KristallApp::refresh_lsystem(LSystem &the_lsystem, int num_iterations, float sz)
{
    m_dirty_lsystem = false;
    
    the_lsystem.set_axiom(*m_axiom);
    the_lsystem.rules().clear();
    
    for(auto r : m_rules)
        the_lsystem.add_rule(*r);
    
    the_lsystem.set_branch_angles(*m_branch_angles);
    the_lsystem.set_branch_randomness(*m_branch_randomness);
    the_lsystem.set_increment(*m_increment);
    the_lsystem.set_increment_randomness(*m_increment_randomness);
    the_lsystem.set_diameter(kinski::map_value<float>(sz, 0.f, 1.f, *m_diameter / 2.f, *m_diameter));
    the_lsystem.set_diameter_shrink_factor(*m_diameter_shrink);
    
    // iterate
    the_lsystem.iterate(num_iterations);
    
    the_lsystem.set_max_random_tries(20);
    
    // add a position check functor
    if(*m_use_bounding_mesh)
    {
        the_lsystem.set_position_check([&](const glm::vec3& p) -> bool
        {
            if(!gl::is_point_inside_mesh(p, m_bounding_mesh)) return false;
            
            for(auto o : m_occluding_meshes)
            {
                if(!o->enabled()) continue;
                if(gl::is_point_inside_mesh(p, o)) return false;
            }
            return true;
        });
    }
    // add an empty functor (clear position check)
    else
    {
        the_lsystem.set_position_check(LSystem::PositionCheckFunctor());
    }
    
    // generate a random spawn position
//    bool pos_valid = false;
//    vec3 rnd_spawn;
//    while (!pos_valid)
//    {
//        rnd_spawn = 0.8f * glm::linearRand(m_aabb_min->value(), m_aabb_max->value());
//        pos_valid = true;
//        
//        // check against occluder
//        for(auto o : m_occluding_meshes)
//        {
//            if(o->enabled() && gl::is_point_inside_mesh(rnd_spawn, o))
//            {
//                pos_valid = false;
//                break;
//            }
//        }
//    }
//    the_lsystem.turtle_transform()[3] = vec4(rnd_spawn, 1);
    
    // create a mesh from our lsystem geometry
    scene().removeObject(m_mesh);
    m_mesh = the_lsystem.create_mesh();
    m_entries = m_mesh->entries();
    
    scene().addObject(m_mesh);
    
    // set materials
//    m_animations[MESH_FADE].reset();
    m_mesh->materials().clear();
    int mat_index = kinski::random(0, 100) % materials().size();
    
    for (int i = 0; i < materials().size(); i++)
    {
        m_mesh->materials().push_back(gl::Material::create(m_lsystem_shaders[0]));
        *m_mesh->materials()[i] = *materials()[i];
        
        const gl::Color &c = m_color_table[mat_index];
        m_mesh->materials()[i]->setDiffuse(c);
        m_mesh->materials()[i]->setAmbient(c);
        m_mesh->materials()[i]->uniform("u_cap_bias", *m_cap_bias);
        m_mesh->materials()[i]->uniform("u_split_limit", *m_split_limit);
        m_mesh->materials()[i]->addTexture(m_textures[MOVIE_TEX]);
    }

    // update the opencl ParticleSystem for our mesh
    m_particle_system.set_mesh(m_mesh);
    
    // gravity
    m_particle_system.set_gravity(m_particle_gravity->value() * vec3(0, -1, 0));
    
    //bounce
    m_particle_system.set_bouncyness(*m_particle_bounce);
    
    // simulation AABB
    m_particle_system.set_aabb(gl::AABB(*m_aabb_min, *m_aabb_max));
    
    // create some random forces for decay-explosion
    m_particle_system.forces().clear();
    auto aabb = m_mesh->geometry()->boundingBox();
    
    for(int i = 0; i < 5; i++)
    {
        float intensity = sz * 13000000.f; //kinski::random(120000.f, 600000.f);
        auto force = vec4(glm::linearRand(aabb.min, aabb.max), intensity);
        m_particle_system.forces().push_back(force);
    }
    *m_use_particle_simulation = false;
    
    m_ready_timer.expires_from_now(*m_time_idle);
}

void KristallApp::start_decay(float fade_time)
{
    m_ready_timer.cancel();
    
    gl::Color decay_color = gl::Color(0.392, 0.584, 0.929, .5);
    float cap_growth_factor = 4.f;
    
    std::vector<gl::Color> diffuse, ambient;
    float cap_bias = *m_cap_bias;
    
    for (auto m : m_mesh->materials())
    {
        diffuse.push_back(m->diffuse());
        ambient.push_back(m->ambient());
    }
        
    auto anim = std::make_shared<animation::Animation>(fade_time, 0,[=](float progress)
    {
        for (int i = 0; i < m_mesh->materials().size(); i++)
        {
            m_mesh->materials()[i]->setDiffuse(glm::mix(diffuse[i], decay_color, progress));
            m_mesh->materials()[i]->setAmbient(glm::mix(ambient[i], decay_color, progress));
            m_mesh->materials()[i]->uniform("u_cap_bias", kinski::mix(cap_bias,
                                                                      cap_growth_factor * cap_bias,
                                                                      progress));
        }
    });
    
    m_ready_timer.cancel();
    
    anim->set_finish_callback([&]()
    {
        *m_use_particle_simulation = true;
        
        // fade out mesh
        m_animations[MESH_FADE] = std::make_shared<animation::Animation>(*m_time_idle, 0.f,
        [&](float progress)
        {
            if(m_mesh)
            {
                for (auto &m : m_mesh->materials())
                { m->setDiffuse(gl::Color(m->diffuse().rgb(), 1.f - progress)); }
            }
        });
        m_animations[MESH_FADE]->set_finish_callback([&]()
        {
            change_gamestate(READY_PHASE);
        });
        m_animations[MESH_FADE]->set_ease_function(animation::EaseOutSine());
        m_animations[MESH_FADE]->start();
    });
    anim->start();
    m_animations[DECAY] = anim;
}

gl::Texture KristallApp::composite_output_image()
{
    // render the output image offscreen
    return gl::render_to_texture(scene(), m_offscreen_fbos[0], m_offscreen_camera);;
}

void KristallApp::change_gamestate(GameState the_state)
{
    // our current state
    switch (the_state)
    {
        case READY_PHASE:
            LOG_DEBUG << "READY_PHASE";
            
            setup_lsystem(round(kinski::random(0.f, 3.f)));
            refresh_lsystem(m_lsystem, *m_num_iterations);
            
            // light animation
            set_light_animation(0.f);
            
            break;
        
        case DECAY_PHASE:
            LOG_DEBUG << "DECAY_PHASE";
            start_decay(2.5f);
            // light animation
            set_light_animation(1.f);
            break;
            
        case IDLE_PHASE:
            LOG_DEBUG << "IDLE";
            if(m_gamestate != READY_PHASE) return;
            fade_in_background();
            break;
            
        default:
            break;
    }
    m_gamestate = the_state;
}

void KristallApp::set_light_animation(float force)
{
    auto bounds = m_bounding_mesh->boundingBox();
    
    force += .1;
    
    auto p0 = sphericalRand(glm::length(bounds.halfExtents()));
    if(p0.z < 0) p0.z = -p0.z;
    auto p1 = sphericalRand(glm::length(bounds.halfExtents()));
    if(p1.z < 0) p1.z = -p1.z;
    
    m_animations[LIGHT_0] = animation::create(&lights()[LIGHT_0]->position(), p0, p1,
                                                      *m_light_elevation / force);
    m_animations[LIGHT_0]->set_loop(animation::LOOP_BACK_FORTH);
    m_animations[LIGHT_0]->set_ease_function(animation::EaseInOutSine());
    m_animations[LIGHT_0]->start();
    
    m_animations[LIGHT_1] = animation::create(&lights()[LIGHT_1]->position(),
                                              vec3(0, 0, 200) + 2.f * bounds.min,
                                              vec3(0, 0, 200) + 2.f * bounds.max,
                                              *m_light_elevation / force);
    m_animations[LIGHT_1]->set_loop(animation::LOOP_BACK_FORTH);
    m_animations[LIGHT_1]->set_ease_function(animation::EaseInOutSine());
    m_animations[LIGHT_1]->start();
}

void KristallApp::fade_in_background()
{

}

void KristallApp::save_settings(const std::string &path)
{
    ViewerApp::save_settings(path);
    
    std::list<Component::Ptr> object_components;
    for (int i = 0; i < m_occluding_meshes.size(); i++)
    {
        Object3DComponent::Ptr tmp(new Object3DComponent());
        tmp->set_name("Object " + as_string(i));
        tmp->set_objects(m_occluding_meshes);
        tmp->set_index(i);
        object_components.push_back(tmp);
    }
    try
    {
     Serializer::saveComponentState(object_components, "objects.json", PropertyIO_GL());
    }
    catch(Exception &e){LOG_ERROR<<e.what();}
}

void KristallApp::load_settings(const std::string &path)
{
    ViewerApp::load_settings(path);
    
    std::list<Component::Ptr> object_components;

    for (int i = 0; i < m_occluding_meshes.size(); i++)
    {
        Object3DComponent::Ptr tmp(new Object3DComponent());
        tmp->set_name("Object " + as_string(i));
        tmp->set_objects(m_occluding_meshes, false);
        tmp->set_index(i);
        tmp->observeProperties();
        object_components.push_back(tmp);
    }
    try
    {
        Serializer::loadComponentState(object_components, "objects.json", PropertyIO_GL());
    }
    catch(Exception &e){LOG_ERROR<<e.what();}
}

void create_lsystems()
{

}