#ifndef KINSKI_FRUSTUM_TESTER_HPP
#define KINSKI_FRUSTUM_TESTER_HPP
#include "kinskiApp/ViewerApp.h"

namespace kinski
{
    class Frustum_Tester : public ViewerApp
    {
    private:
        
        gl::MeshPtr m_point_mesh;
        gl::Font m_font;
        
        Property_<glm::mat3>::Ptr m_frustum_rotation;
        
        RangedProperty<float>::Ptr m_near;
        RangedProperty<float>::Ptr m_far;
        RangedProperty<float>::Ptr m_fov;
        
        gl::CameraPtr m_test_cam;
        gl::MeshPtr m_frustum_mesh;
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
    };
}// namespace kinski
#endif