//
// Created by crocdialer on 4/20/18.
//

#include "imgui_util.h"
#include "gl/Camera.hpp"
#include "gl/Mesh.hpp"
#include "gl/Light.hpp"
#include "ImGuizmo.h"

namespace kinski{ namespace gui{

namespace
{
    char g_text_buf[512] = "\0";
}

const ImVec2& im_vec_cast(const gl::vec2 &the_vec)
{
    return *reinterpret_cast<const ImVec2*>(&the_vec);
}

const ImVec4& im_vec_cast(const gl::vec4 &the_vec)
{
    return *reinterpret_cast<const ImVec4*>(&the_vec);
}

const ImVec4 im_vec_cast(const gl::vec3 &the_vec)
{
    auto tmp = gl::vec4(the_vec, 1.f);
    return *reinterpret_cast<const ImVec4*>(&tmp);
}

// internal function signatures
void draw_light_ui(const gl::LightPtr &the_light);

// int
void draw_property_ui(const Property_<int>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(auto ranged_prop = std::dynamic_pointer_cast<RangedProperty<int>>(the_property))
    {
        if(ImGui::SliderInt(prop_name.c_str(), &ranged_prop->value(), ranged_prop->range().first,
                            ranged_prop->range().second))
        {
            the_property->notify_observers();
        }
    }
    else
    {
        if(ImGui::InputInt(prop_name.c_str(), &the_property->value(), 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            the_property->notify_observers();
        }
    }
}

void draw_property_ui(const Property_<uint32_t >::Ptr &the_property)
{
    std::string prop_name = the_property->name();
    int val = *the_property;

    if(auto ranged_prop = std::dynamic_pointer_cast<RangedProperty<uint32_t>>(the_property))
    {
        if(ImGui::SliderInt(prop_name.c_str(), &val, ranged_prop->range().first,
                            ranged_prop->range().second))
        {
            *the_property = val;
        }
    }
    else
    {
        if(ImGui::InputInt(prop_name.c_str(), &val, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            *the_property = val;
        }
    }
}

// float
void draw_property_ui(const Property_<float>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(auto ranged_prop = std::dynamic_pointer_cast<RangedProperty<float>>(the_property))
    {
        if(ImGui::SliderFloat(prop_name.c_str(), &ranged_prop->value(), ranged_prop->range().first,
                            ranged_prop->range().second))
        {
            the_property->notify_observers();
        }
    }
    else
    {
        if(ImGui::InputFloat(prop_name.c_str(), &the_property->value(), 0.f, 0.f,
                             (std::abs(the_property->value()) < 1.f) ? 5 : 2,
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            the_property->notify_observers();
        }
    }
}

// bool
void draw_property_ui(const Property_<bool>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::Checkbox(prop_name.c_str(), &the_property->value()))
    {
        the_property->notify_observers();
    }
}

// gl::vec2
void draw_property_ui(const Property_<gl::vec2>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::InputFloat2(prop_name.c_str(), &the_property->value()[0], 2,
                          ImGuiInputTextFlags_EnterReturnsTrue))
    {
        the_property->notify_observers();
    }
}

// gl::vec3
void draw_property_ui(const Property_<gl::vec3>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::InputFloat3(prop_name.c_str(), &the_property->value()[0], 2,
                          ImGuiInputTextFlags_EnterReturnsTrue))
    {
        the_property->notify_observers();
    }
}

// gl::Color (a.k.a. gl::vec4)
void draw_property_ui(const Property_<gl::Color>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::ColorEdit4(prop_name.c_str(), (float*)&the_property->value()))
    {
        the_property->notify_observers();
    }
}

// std::string
void draw_property_ui(const Property_<std::string>::Ptr &the_property)
{
    std::string prop_name = the_property->name();
    strcpy(g_text_buf, the_property->value().c_str());

    if(ImGui::InputText(prop_name.c_str(), g_text_buf, IM_ARRAYSIZE(g_text_buf),
                        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        the_property->value(g_text_buf);
    }
}

// std::vector<std::string>>
void draw_property_ui(const Property_<std::vector<std::string>>::Ptr &the_property)
{
    std::string prop_name = the_property->name();
    std::vector<std::string> &array = the_property->value();

    if(ImGui::TreeNode(prop_name.c_str()))
    {
        for(size_t i = 0; i < array.size(); ++i)
        {
            strcpy(g_text_buf, array[i].c_str());

            if(ImGui::InputText(to_string(i).c_str(), g_text_buf, IM_ARRAYSIZE(g_text_buf),
                                ImGuiInputTextFlags_EnterReturnsTrue))
            {
                array[i] = g_text_buf;
                the_property->notify_observers();
            }
        }
        ImGui::TreePop();
    }
}

// generic
void draw_property_ui(const Property::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::TreeNode(prop_name.c_str()))
    {
        ImGui::Text("%s", prop_name.c_str());
        ImGui::TreePop();
    }

}

void draw_component_ui(const ComponentConstPtr &the_component)
{
    ImGui::Begin(the_component->name().c_str());

    for(auto p : the_component->get_property_list())
    {
        // skip non-tweakable properties
        if(!p->tweakable()){ continue; }

        if(p->is_of_type<bool>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<bool>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<int>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<int>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<uint32_t>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<uint32_t>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<float>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<float>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<gl::vec2>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<gl::vec2>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<gl::vec3>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<gl::vec3>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<gl::Color>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<gl::Color>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<std::string>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<std::string>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<std::vector<std::string>>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<std::vector<std::string>>>(p);
            draw_property_ui(cast_prop);
        }
        else{ draw_property_ui(p); }
    }

    ImGui::End();
}

void draw_textures_ui(const std::vector<gl::Texture> &the_textures)
{
    for(auto &t : the_textures)
    {
        if(t)
        {
            constexpr float w = 150;
            ImVec2 sz(w, w / t.aspect_ratio());
            ImGui::Image((ImTextureID)(&t), sz);
        }
    }
}

void draw_material_ui(const gl::MaterialPtr &the_mat)
{

    // base color
    gl::Color base_color = the_mat->diffuse();
    if(ImGui::ColorEdit4("base color", (float*)&base_color))
    {
        the_mat->set_diffuse(base_color);
    }

    // roughness
    float roughness = the_mat->roughness();
    if(ImGui::SliderFloat("roughness", &roughness, 0.f, 1.f))
    {
        the_mat->set_roughness(roughness);
    }

    // metalness
    float metalness = the_mat->metalness();
    if(ImGui::SliderFloat("metalness", &metalness, 0.f, 1.f))
    {
        the_mat->set_metalness(metalness);
    }

    // ambient occlusion
    float occlusion = the_mat->occlusion();
    if(ImGui::SliderFloat("ambient occlusion", &occlusion, 0.f, 1.f))
    {
        the_mat->set_occlusion(occlusion);
    }

    // two-sided
    bool two_sided = the_mat->two_sided();
    if(ImGui::Checkbox("two-sided", &two_sided))
    {
        the_mat->set_two_sided(two_sided);
    }

    // blending
    bool blending = the_mat->blending();
    if(ImGui::Checkbox("blending", &blending))
    {
        the_mat->set_blending(blending);
    }

    // textures
    if(ImGui::TreeNode(("textures (" + to_string(the_mat->textures().size()) + ")").c_str()))
    {
        std::vector<gl::Texture> textures;
        for(const auto &p : the_mat->textures()){ textures.push_back(p.second); }
        draw_textures_ui(textures);
        ImGui::TreePop();
    }
}

void draw_materials_ui(const std::vector<gl::MaterialPtr> &the_materials)
{
    for(size_t i = 0; i < the_materials.size(); ++i)
    {
        if(ImGui::TreeNode(("material " + to_string(i)).c_str()))
        {
            draw_material_ui(the_materials[i]);
            ImGui::TreePop();
        }
    }
}

void draw_light_component_ui(const LightComponentPtr &the_component)
{
    ImGui::Begin("lights");

    for(size_t i = 0; i < the_component->lights().size(); ++i)
    {
        const auto &l = the_component->lights()[i];

        // enabled?
        bool is_enabled = l->enabled();
        if(ImGui::Checkbox((to_string(i) + ": enabled").c_str(), &is_enabled)){ l->set_enabled(is_enabled); }
        ImGui::SameLine();

        bool draw_dummy = the_component->use_dummy(i);
        if(ImGui::Checkbox((to_string(i) + ": dummy").c_str(), &draw_dummy)){ the_component->set_use_dummy(i, draw_dummy); }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, im_vec_cast(l->diffuse()));
        if(ImGui::TreeNode(("light " + to_string(i)).c_str()))
        {
            ImGui::PopStyleColor();
            draw_light_ui(l);
            ImGui::TreePop();
            ImGui::Separator();
        }
        else{ ImGui::PopStyleColor(); }
    }
    ImGui::End();
}

void draw_light_ui(const gl::LightPtr &the_light)
{
    if(ImGui::RadioButton("Directional", the_light->type() == gl::Light::DIRECTIONAL)){ the_light->set_type(gl::Light::DIRECTIONAL); }
    ImGui::SameLine();
    if(ImGui::RadioButton("Omni", the_light->type() == gl::Light::POINT)){ the_light->set_type(gl::Light::POINT); }
    ImGui::SameLine();
    if(ImGui::RadioButton("Spot", the_light->type() == gl::Light::SPOT)){ the_light->set_type(gl::Light::SPOT); }

    // intensity
    float intensity = the_light->intensity();
    if(ImGui::InputFloat("intensity", &intensity, 0.f, 0.f, (std::abs(intensity) < 1.f) ? 5 : 2))
    { the_light->set_intensity(intensity); }

    // radius
    float radius = the_light->radius();
    if(ImGui::InputFloat("radius", &radius, 0.f, 0.f, (std::abs(radius) < 1.f) ? 5 : 2))
    { the_light->set_radius(radius); }

    // color
    gl::Color color = the_light->diffuse();
    if(ImGui::ColorEdit4("color", (float*)&color)){ the_light->set_diffuse(color); }

    // shadows
    bool shadows = the_light->cast_shadow();
    if(ImGui::Checkbox("cast shadow", &shadows)){ the_light->set_cast_shadow(shadows); }

    if(the_light->type() == gl::Light::SPOT)
    {
        float spot_cutoff = the_light->spot_cutoff();
        if(ImGui::SliderFloat("spot cutoff", &spot_cutoff, 0.f, 90.f))
        {
            the_light->set_spot_cutoff(spot_cutoff);
        }

        float spot_exponent = the_light->spot_exponent();
        if(ImGui::SliderFloat("spot exponent", &spot_exponent, 0.f, 128.f))
        {
            the_light->set_spot_exponent(spot_exponent);
        }
    }

    if(the_light->type() != gl::Light::DIRECTIONAL)
    {
        auto attenuation = the_light->attenuation();

        if(ImGui::SliderFloat("attenuation", &attenuation.quadratic, 0.f, 1.f))
        {
            the_light->set_attenuation(attenuation);
        }
    }
}

void draw_object3D_ui(const gl::Object3DPtr &the_object, const gl::CameraConstPtr &the_camera)
{
    static uint32_t current_gizmo = 0;

    ImGui::Begin("selected object");

    if(!the_object){ ImGui::End(); return; }

    ImGui::Text(the_object->name().c_str());
    ImGui::Separator();

    if(ImGui::RadioButton("None", !current_gizmo)){ current_gizmo = 0; }
    ImGui::SameLine();
    if(ImGui::RadioButton("Translate", current_gizmo == ImGuizmo::TRANSLATE)){ current_gizmo = ImGuizmo::TRANSLATE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("Rotate", current_gizmo == ImGuizmo::ROTATE)){ current_gizmo = ImGuizmo::ROTATE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("Scale", current_gizmo == ImGuizmo::SCALE)){ current_gizmo = ImGuizmo::SCALE; }
    ImGui::Separator();

    glm::mat4 transform = the_object->global_transform();
    glm::vec3 position = transform[3].xyz;
    glm::vec3 rotation = glm::degrees(glm::eulerAngles(glm::quat_cast(transform)));
    glm::vec3 scale = glm::vec3(length(transform[0]), length(transform[1]), length(transform[2]));

    bool changed = ImGui::InputFloat3("position", glm::value_ptr(position), 3);
    changed = ImGui::InputFloat3("rotation", glm::value_ptr(rotation), 3, ImGuiInputTextFlags_EnterReturnsTrue) || changed;
    changed = ImGui::InputFloat3("scale", glm::value_ptr(scale), 3) || changed;

    if(changed)
    {
        auto m = glm::mat4_cast(glm::quat(glm::radians(rotation)));
        m = glm::scale(m, scale);
        m[3].xyz = position;
        transform = m;
        the_object->set_global_transform(m);
    }
    ImGui::Separator();

    // cast to gl::MeshPtr
    auto mesh = std::dynamic_pointer_cast<gl::Mesh>(the_object);

    if(mesh)
    {
        std::stringstream ss;
        ss << mesh->name() << "\nvertices: " << to_string(mesh->geometry()->vertices().size()) <<
           "\nfaces: " << to_string(mesh->geometry()->faces().size());
        ImGui::Text("%s", ss.str().c_str());
        ImGui::Separator();
        draw_materials_ui(mesh->materials());
    }
    ImGui::End();

    if(the_camera && current_gizmo)
    {
        bool is_ortho = std::dynamic_pointer_cast<const gl::OrthoCamera>(the_camera).get();
        ImGuizmo::SetOrthographic(is_ortho);

//        glm::mat4 transform = the_object->global_transform();
        ImGuizmo::Manipulate(glm::value_ptr(the_camera->view_matrix()), glm::value_ptr(the_camera->projection_matrix()),
                             ImGuizmo::OPERATION(current_gizmo), ImGuizmo::WORLD, glm::value_ptr(transform));
        the_object->set_global_transform(transform);
    }
}

void process_joystick_input(const std::vector<JoystickState> &the_joystick_states)
{
    ImGuiIO& io = ImGui::GetIO();

    // Gamepad navigation mapping [BETA]
    memset(io.NavInputs, 0, sizeof(io.NavInputs));

    if (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad)
    {
        auto &mapping = JoystickState::mapping();

        for(const auto &js : the_joystick_states)
        {
            auto analog_left = js.analog_left();
            auto dpad = js.dpad();

            io.NavInputs[ImGuiNavInput_Activate] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_A]];
            io.NavInputs[ImGuiNavInput_Cancel] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_B]];
            io.NavInputs[ImGuiNavInput_Menu] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_X]];
            io.NavInputs[ImGuiNavInput_Input] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_Y]];

            io.NavInputs[ImGuiNavInput_FocusPrev] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_BUMPER_LEFT]];
            io.NavInputs[ImGuiNavInput_FocusNext] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_BUMPER_RIGHT]];
            io.NavInputs[ImGuiNavInput_TweakSlow] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_BUMPER_LEFT]];
            io.NavInputs[ImGuiNavInput_TweakFast] = js.buttons()[mapping[JoystickState::Mapping::BUTTON_BUMPER_RIGHT]];

            io.NavInputs[ImGuiNavInput_LStickLeft] = analog_left.x < 0;
            io.NavInputs[ImGuiNavInput_LStickRight] = analog_left.x > 0;
            io.NavInputs[ImGuiNavInput_LStickUp] = analog_left.y > 0;
            io.NavInputs[ImGuiNavInput_LStickDown] = analog_left.y < 0;

            io.NavInputs[ImGuiNavInput_DpadLeft] = dpad.x < 0;
            io.NavInputs[ImGuiNavInput_DpadRight] = dpad.x > 0;
            io.NavInputs[ImGuiNavInput_DpadUp] = dpad.y < 0;
            io.NavInputs[ImGuiNavInput_DpadDown] = dpad.y > 0;

        }
        if(!the_joystick_states.empty()){ io.BackendFlags |= ImGuiBackendFlags_HasGamepad; }
        else{ io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad; }
    }
}

}}
