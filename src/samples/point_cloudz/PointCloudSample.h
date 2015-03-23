//
//  EmptySample.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__EmptySample__
#define __gl__EmptySample__

#include "app/ViewerApp.h"

// OpenCL
#include "opencl/cl_context.h"

// OpenNI
#include "openni/OpenNIConnector.h"

//Syphon
#include "syphon/SyphonConnector.h"

#include "gl/Fbo.h"

namespace kinski
{
    class PointCloudSample : public ViewerApp
    {
    private:
        
        gl::Font m_font;
        std::vector<gl::Texture> m_textures{4};
        
        // output via Syphon
        syphon::Output m_syphon;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
            Property_<std::string>::create("syphon server name", "point clouder");
        
        // OpenNI interface
        gl::OpenNIConnector::Ptr m_open_ni;
        gl::OpenNIConnector::UserList m_user_list;
        gl::PerspectiveCamera::Ptr m_depth_cam;
        gl::MeshPtr m_depth_cam_mesh;
        RangedProperty<float>::Ptr m_depth_cam_x, m_depth_cam_y, m_depth_cam_z;
        Property_<glm::vec3>::Ptr m_depth_cam_look_dir;
        RangedProperty<float>::Ptr m_depth_cam_scale;
        
        gl::Scene m_debug_scene;
        gl::PerspectiveCamera::Ptr m_free_camera;
        gl::MeshPtr m_free_camera_mesh;
        gl::Fbo m_fbo;
        Property_<glm::vec2>::Ptr m_fbo_size;
        RangedProperty<float>::Ptr m_fbo_cam_distance;
        
        // pointcloud mesh
        gl::MeshPtr m_point_cloud;
        
    public:
        
        void setup();
        void update(float timeDelta);
        void draw();
        void resize(int w ,int h);
        void keyPress(const KeyEvent &e);
        void keyRelease(const KeyEvent &e);
        void mousePress(const MouseEvent &e);
        void mouseRelease(const MouseEvent &e);
        void mouseMove(const MouseEvent &e);
        void mouseDrag(const MouseEvent &e);
        void mouseWheel(const MouseEvent &e);
        void got_message(const std::vector<uint8_t> &the_message);
        void tearDown();
        void updateProperty(const Property::ConstPtr &theProperty);
    };
}// namespace kinski

#endif /* defined(__gl__EmptySample__) */
