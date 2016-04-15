// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Visitor.hpp
//
//  Created by Croc Dialer on 31/03/15.

#pragma once

#include "Mesh.hpp"
#include "Light.hpp"
#include "Camera.hpp"

namespace kinski { namespace gl {
    
    class Visitor
    {
    public:
        Visitor()
        {
            m_transform_stack.push(glm::mat4());
        }
        
        inline const std::stack<glm::mat4>& transform_stack() const {return m_transform_stack;};
        inline std::stack<glm::mat4>& transform_stack() {return m_transform_stack;};
        
        virtual void visit(Object3D &theNode)
        {
            if(!theNode.enabled()) return;
            m_transform_stack.push(m_transform_stack.top() * theNode.transform());
            for (Object3DPtr &child : theNode.children()){child->accept(*this);}
            m_transform_stack.pop();
        }
        virtual void visit(gl::Mesh &theNode){ visit(static_cast<Object3D&>(theNode)); };
        virtual void visit(gl::Light &theNode){ visit(static_cast<Object3D&>(theNode)); };
        virtual void visit(gl::Camera &theNode){ visit(static_cast<Object3D&>(theNode)); };

    protected:
        
        inline bool check_tags(const std::set<std::string> &filter_tags,
                               const std::set<std::string> &obj_tags)
        {
            for(const auto &t : obj_tags)
            {
                if(is_in(t, filter_tags)){ return true; }
            }
            return filter_tags.empty();
        }
        
    private:
        std::stack<glm::mat4> m_transform_stack;
    };
    
    template<typename T>
    class SelectVisitor : public Visitor
    {
    public:
        SelectVisitor(const std::set<std::string> &the_tags = {}, bool select_only_enabled = true):
        Visitor(),
        m_tags(the_tags),
        m_select_only_enabled(select_only_enabled){};
        
        void visit(T &theNode) override
        {
            if(theNode.enabled() || !m_select_only_enabled)
            {
                if(check_tags(m_tags, theNode.tags())){ m_objects.push_back(&theNode); }
                Visitor::visit(static_cast<gl::Object3D&>(theNode));
            }
        };
        
        void clear(){ m_objects.clear(); }
        const std::list<T*>& getObjects() const {return m_objects;};
        
    private:
        std::list<T*> m_objects;
        std::set<std::string> m_tags;
        bool m_select_only_enabled;
    };
    
}}//namespace