//
//  MovieTimeshift.h
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#pragma once

#include "core/Timer.hpp"
#include "gl/Noise.hpp"
#include "app/ViewerApp.h"

#include "video/MovieController.h"
#include "video/CameraController.h"
#include "syphon/SyphonConnector.h"

#include "gl/Fbo.hpp"

namespace kinski
{
    class MovieTimeshift : public ViewerApp
    {
    private:
        
        enum TextureEnum { TEXTURE_INPUT = 0, TEXTURE_OUTPUT = 1, TEXTURE_FG_IMAGE = 2,
            TEXTURE_NOISE = 3, TEXTURE_MOVIE = 4};
        
        enum InputSource { INPUT_NONE = 0, INPUT_MOVIE = 1, INPUT_CAMERA = 2, INPUT_SYPHON = 3 };
        
        std::map<InputSource, std::string>
        m_input_source_names =
        {
            {INPUT_NONE, "INPUT_NONE"},
            {INPUT_MOVIE, "INPUT_MOVIE"},
            {INPUT_CAMERA, "INPUT_CAMERA"},
            {INPUT_SYPHON, "INPUT_SYPHON"}
        };
        
        bool m_input_source_changed = true;
        
        RangedProperty<int>::Ptr
        m_input_source = RangedProperty<int>::create("input source", 0, 0, 3);
        
        video::MovieControllerPtr m_movie = video::MovieController::create();
        video::CameraControllerPtr m_camera = video::CameraController::create();
        
        bool m_needs_movie_refresh = false;
        bool m_needs_array_refresh = true;
        std::vector<uint8_t> m_camera_data;
        
        gl::Texture m_array_tex;
        gl::MaterialPtr m_custom_mat;
        int m_current_index = 0;
        
        gl::Fbo m_offscreen_fbo, m_fbo_transfer;
        
        Property_<glm::vec2>::Ptr
        m_offscreen_size = Property_<glm::vec2>::create("Offscreen size", glm::vec2(800, 600)),
        m_noise_map_size = Property_<glm::vec2>::create("Noise map size", glm::vec2(128));
        
        // output via Syphon
        syphon::Output m_syphon_out;
        Property_<bool>::Ptr m_use_syphon = Property_<bool>::create("Use syphon", false);
        Property_<std::string>::Ptr m_syphon_server_name =
        Property_<std::string>::create("Syphon server name", "belgium");
        
        // syphon input
        syphon::Input m_syphon_in;
        RangedProperty<int>::Ptr
        m_syphon_in_index = RangedProperty<int>::create("syphon input", -1, -1, -1);
        
        // properties
        Property_<int>::Ptr m_cam_id = Property_<int>::create("camera id", 0);
        Property_<bool>::Ptr m_flip_image = Property_<bool>::create("flip image", true);
        Property_<string>::Ptr m_movie_path = Property_<string>::create("movie path", "");
        Property_<float>::Ptr m_movie_speed = Property_<float>::create("movie speed", 1.f);
        
        // simplex noise params
        Property_<float>::Ptr m_noise_scale_x = Property_<float>::create("noise scale x", 0.0125f);
        Property_<float>::Ptr m_noise_scale_y = Property_<float>::create("noise scale y", 0.0125f);
        Property_<float>::Ptr m_noise_velocity = Property_<float>::create("noise velocity", 0.1f);
        Property_<float>::Ptr m_noise_seed = Property_<float>::create("noise seed", 0.f);
        gl::Noise m_noise;
        
        // MOG params
        Property_<bool>::Ptr
        m_use_bg_substract = Property_<bool>::create("use background substraction", false);
        
        Property_<float>::Ptr m_mog_learn_rate = Property_<float>::create("mog learn rate", -1.f);
        
        Property_<uint32_t>::Ptr
        m_num_buffer_frames = Property_<uint32_t>::create("num buffer frames", 90);
        
        /*!
         * create a procedural simplex noise texture
         * params are read from m_noise_xxx properties (scale, velocity, seed)
         */
//        gl::Texture create_noise_tex(float seed = 0.025f);
        
        gl::Buffer m_pbo;
        
        /*!
         * set the input source to the_src
         */
        bool set_input_source(InputSource the_src);
        
        /*!
         * copy a block of bytes (interpreted as a 2D-slice) into an array texture object
         */
        bool insert_data_into_array_texture(const std::vector<uint8_t> &the_data,
                                            gl::Texture &the_array_tex,
                                            uint32_t the_width, uint32_t the_height,
                                            uint32_t the_index);
        
        /*!
         * copy a block of bytes (interpreted as a 2D-slice) into an array texture object
         */
        bool insert_texture_into_array_texture(const gl::Texture &the_texture,
                                               gl::Texture &the_array_tex,
                                               uint32_t the_index);
        
        /*!
         * create a foreground image using a background substraction algorithm (e.g. MOG)
         */
//        cv::UMat create_foreground_image(std::vector<uint8_t> &the_data, int width, int height);
        
    public:
        
        void setup() override;
        void update(float timeDelta) override;
        void draw() override;
        void got_message(const std::vector<uint8_t> &the_data) override;
        void fileDrop(const MouseEvent &e, const std::vector<std::string> &files) override;
        void tearDown() override;
        void update_property(const Property::ConstPtr &theProperty) override;
        void keyPress(const KeyEvent &e) override;
        
        void on_movie_load();
        
    };
}// namespace kinski