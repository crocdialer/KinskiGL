//
// Created by crocdialer on 4/20/18.
//

#include "imgui_util.h"
#include "gl/gl.hpp"

namespace kinski{ namespace gl{

// bool
void draw_property_ui(const Property_<bool>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::Checkbox(prop_name.c_str(), &the_property->value()))
    {
        the_property->notify_observers();
    }
}

// gl::Color
void draw_property_ui(const Property_<gl::Color>::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::ColorEdit4(prop_name.c_str(), (float*)&the_property->value()))
    {
        the_property->notify_observers();
    }
}

void draw_property_ui(const Property::Ptr &the_property)
{
    std::string prop_name = the_property->name();

    if(ImGui::TreeNode(prop_name.c_str()))
    {
        ImGui::Text(prop_name.c_str());
        ImGui::TreePop();
    }

}

void draw_component_ui(const ComponentConstPtr &the_component)
{
    ImGui::Begin(the_component->name().c_str());

    for(auto p : the_component->get_property_list())
    {
        if(p->is_of_type<bool>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<bool>>(p);
            draw_property_ui(cast_prop);
        }
        else if(p->is_of_type<gl::Color>())
        {
            auto cast_prop = std::dynamic_pointer_cast<Property_<gl::Color>>(p);
            draw_property_ui(cast_prop);
        }
        else{ draw_property_ui(p); }
    }

    ImGui::End();
}

}}
