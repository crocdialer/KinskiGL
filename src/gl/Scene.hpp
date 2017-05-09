// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Mesh.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "SceneRenderer.hpp"

namespace kinski { namespace gl {
    
    DEFINE_CLASS_PTR(Scene);
    
    class KINSKI_API Scene : public std::enable_shared_from_this<Scene>
    {
    public:
        
        static ScenePtr create();
        
        virtual void update(float time_delta);
        void render(const CameraPtr &theCamera, const std::set<std::string> &the_tags = {}) const;
        Object3DPtr pick(const Ray &ray, bool high_precision = false,
                         const std::set<std::string> &the_tags = {}) const;
    
        void add_object(const Object3DPtr &the_object);
        void remove_object(const Object3DPtr &the_object);
        void clear();
        
        std::vector<gl::Object3DPtr> get_objects_by_tag(const std::string &the_tag) const;
        
        inline const Object3DPtr& root() const {return m_root;};
        inline Object3DPtr& root() {return m_root;};
        uint32_t num_visible_objects() const {return m_num_visible_objects;};
        
        const gl::MeshPtr& skybox() const { return m_skybox; }
        void set_skybox(const gl::Texture& t);

        void set_renderer(SceneRendererPtr the_renderer){ m_renderer = the_renderer; }
        SceneRendererPtr renderer(){ return m_renderer; }
    private:
        
        Scene();
        gl::MeshPtr m_skybox;
        mutable uint32_t m_num_visible_objects;
        mutable gl::SceneRendererPtr m_renderer;
        Object3DPtr m_root;
    };
    
}}//namespace
