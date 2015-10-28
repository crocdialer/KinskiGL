//
//  3dViewer.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#ifndef __gl__3dViewer__
#define __gl__3dViewer__

#include "app/ViewerApp.h"
#include "app/LightComponent.h"
#include "gl/Fbo.h"

#include "video/MovieController.h"

namespace kinski
{
    glm::mat4 create_projector_matrix(gl::Camera::Ptr eye, gl::Camera::Ptr projector);
    
    class ModelViewer : public ViewerApp
    {
    private:
        
        enum TextureEnum{TEXTURE_DIFFUSE = 0, TEXTURE_SHADOWMAP = 1, TEXTURE_MOVIE = 2};
        
        gl::MeshPtr m_mesh;
        
        LightComponent::Ptr m_light_component;
        
        std::vector<gl::Fbo> m_fbos{4};
        
        gl::PerspectiveCamera::Ptr m_projector;
        gl::MeshPtr m_projector_mesh;
        video::MovieControllerPtr m_movie = video::MovieController::create();
        
        gl::MaterialPtr m_draw_depth_mat;
        
        gl::PerspectiveCamera::Ptr create_camera_from_viewport();
        
        Property_<std::string>::Ptr m_model_path = Property_<std::string>::create("Model path", "");
        Property_<std::string>::Ptr m_movie_path = Property_<std::string>::create("Movie path", "");
        
    public:
        
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void resize(int w ,int h) override;
        void keyPress(const KeyEvent &e) override;
        void keyRelease(const KeyEvent &e) override;
        void mousePress(const MouseEvent &e) override;
        void mouseRelease(const MouseEvent &e) override;
        void mouseMove(const MouseEvent &e) override;
        void mouseDrag(const MouseEvent &e) override;
        void mouseWheel(const MouseEvent &e) override;
        void got_message(const std::vector<uint8_t> &the_message) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
    };
}// namespace kinski

#endif /* defined(__gl__3dViewer__) */
