#ifndef KINSKI_EARTH_SAMPLE_HPP
#define KINSKI_EARTH_SAMPLE_HPP
#include "app/ViewerApp.h"

namespace kinski
{
    class Earth_App : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_earth_mesh;
        std::vector<gl::Texture> m_textures;
        gl::Font m_font;
        
        Property_<std::vector<std::string> >::Ptr m_map_names;
        Property_<std::vector<std::string> >::Ptr m_shader_names;
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
    };
}// namespace kinski
#endif