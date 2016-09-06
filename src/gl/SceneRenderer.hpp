// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Renderer.hpp
//
//  Created by Fabian on 4/21/13.

#pragma once

#include "gl/gl.hpp"
#include "gl/Buffer.hpp"

namespace kinski{ namespace gl{
    
    DEFINE_CLASS_PTR(RenderBin);
    
    class KINSKI_API RenderBin
    {
    public:
        
        struct item
        {
            //! the item's mesh
            gl::Mesh *mesh;
            
            //! the item's transform in eye-coords
            mat4 transform;
        };
        
        struct light
        {
            //! a lightsource
            gl::Light *light;
            
            //! the light's transform in eye-coords
            mat4 transform;
        };
        
        struct sort_items_increasing
        {
            inline bool operator()(const item &lhs, const item &rhs)
            {return lhs.transform[3].z > rhs.transform[3].z;}
        };
        
        struct sort_items_decreasing
        {
            inline bool operator()(const item &lhs, const item &rhs)
            {return lhs.transform[3].z < rhs.transform[3].z;}
        };
        
        RenderBin(const CameraPtr &cam): camera(cam){};
        CameraPtr camera;
        std::list<item> items;
        std::list<light> lights;
    };
    
    class KINSKI_API SceneRenderer
    {
    public:
        
        typedef std::shared_ptr<SceneRenderer> Ptr;
        typedef std::shared_ptr<const SceneRenderer> ConstPtr;
        
        enum UniformBufferIndex {LIGHT_UNIFORM_BUFFER = 0, MATRIX_UNIFORM_BUFFER = 1,
            SHADOW_UNIFORM_BUFFER = 2};
        enum UniformBlockBinding {MATERIAL_BLOCK = 0, LIGHT_BLOCK = 1, MATRIX_BLOCK = 2,
            SHADOW_BLOCK = 3};
        
        SceneRenderer();
        virtual ~SceneRenderer(){};
        
        uint32_t render_scene(const gl::SceneConstPtr &the_scene, const CameraPtr &the_cam,
                              const std::set<std::string> &the_tags = {});
        
        void set_light_uniforms(MaterialPtr &the_mat, const std::list<RenderBin::light> &light_list);
        void update_uniform_buffers(const std::list<RenderBin::light> &light_list);
        void update_uniform_buffer_matrices(const mat4 &model_view,
                                            const mat4 &projection);
        
        void set_shadowmap_size(const vec2 &the_size);
        std::vector<gl::Fbo>& shadow_fbos() { return m_shadow_fbos; }
        std::vector<gl::CameraPtr>& shadow_cams() { return m_shadow_cams; }
        void set_shadow_pass(bool b){ m_shadow_pass = b; }
        
    private:
        
        RenderBinPtr cull(const gl::SceneConstPtr &the_scene, const CameraPtr &theCamera,
                          const std::set<std::string> &the_tags = {}) const;
        
        void render(const RenderBinPtr &theBin);
        
        void draw_sorted_by_material(const CameraPtr &cam, const std::list<RenderBin::item> &item_list,
                                     const std::list<RenderBin::light> &light_list);
        
        gl::Buffer m_uniform_buffer[3];
        
        // shadow params
        int m_num_shadow_lights;
        std::vector<gl::Fbo> m_shadow_fbos;
        std::vector<gl::CameraPtr> m_shadow_cams;
        bool m_shadow_pass;
        
        void update_uniform_buffer_shadows(const mat4 &the_transform);
    };
    
}}// namespace