// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Mesh.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"

namespace kinski { namespace gl {
    
    class KINSKI_API Scene
    {
    public:
        
        Scene();
        virtual void update(float time_delta);
        void render(const CameraPtr &theCamera, const std::set<std::string> &the_tags = {}) const;
        Object3DPtr pick(const Ray &ray, bool high_precision = false) const;
        RenderBinPtr cull(const CameraPtr &theCamera, const std::set<std::string> &the_tags = {}) const;
    
        void addObject(const Object3DPtr &theObject);
        void removeObject(const Object3DPtr &theObject);
        void clear();
        
        std::vector<gl::Object3DPtr> get_objects_by_tag(const std::string &the_tag) const;
        
        inline const Object3DPtr& root() const {return m_root;};
        inline Object3DPtr& root() {return m_root;};
        uint32_t num_visible_objects() const {return m_num_visible_objects;};
        
    private:
        
        mutable uint32_t m_num_visible_objects;
        
        //TODO: find a better place to put this
        mutable gl::Renderer m_renderer;
        
        Object3DPtr m_root;
    };
    
}}//namespace