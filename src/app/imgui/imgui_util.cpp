//
// Created by crocdialer on 4/20/18.
//

#include "imgui_util.h"
#include "gl/Mesh.hpp"

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
            ImGui::Image(reinterpret_cast<ImTextureID>(t.id()), sz);
        }
    }
}

void draw_lights_ui(const std::vector<gl::LightPtr> &the_lights)
{

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

void draw_mesh_ui(const gl::MeshPtr &the_mesh)
{
    if(!the_mesh){ return; }
    ImGui::Begin("mesh");
    std::stringstream ss;
    ss << the_mesh->name() << "\nvertices: " << to_string(the_mesh->geometry()->vertices().size()) <<
          "\nfaces: " << to_string(the_mesh->geometry()->faces().size());
    ImGui::Text("%s", ss.str().c_str());
    ImGui::Separator();

    draw_materials_ui(the_mesh->materials());
    ImGui::End();
}

}}
