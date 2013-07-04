// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt )
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __kinskiGL__Object3D__
#define __kinskiGL__Object3D__

#include "KinskiGL.h"
#include "geometry_types.h"

namespace kinski { namespace gl {

    class KINSKI_API Object3D : public std::enable_shared_from_this<Object3D>
    {
    public:
        
        Object3D();
        virtual ~Object3D(){};
        
        uint32_t getID() const {return m_id;};
        bool enabled() const {return m_enabled;}
        void set_enabled(bool b = true){m_enabled = b;}
        inline void setPosition(const glm::vec3 &thePos) {m_transform[3].xyz() = thePos;};
        inline glm::vec3 position() const {return m_transform[3].xyz(); }
        inline glm::vec3& position() {return *reinterpret_cast<glm::vec3*>(&m_transform[3]);}
        inline glm::vec3 lookAt() const {return glm::normalize(-m_transform[2].xyz());}
        inline glm::vec3 side() const {return glm::normalize(m_transform[0].xyz());}
        inline glm::vec3 up() const {return glm::normalize(m_transform[1].xyz());}
        void setRotation(const glm::quat &theRot);
        void setRotation(const glm::mat3 &theRot);
        glm::quat rotation() const;
        
        inline glm::vec3 scale(){return glm::vec3(glm::length(m_transform[0]),
                                                  glm::length(m_transform[1]),
                                                  glm::length(m_transform[2]));};
        inline void setScale(const glm::vec3 &s){m_transform = glm::scale(m_transform, s / scale());}
        
        void setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp = glm::vec3(0, 1, 0));
        
        inline void setTransform(const glm::mat4 &theTrans) {m_transform = theTrans;}
        inline glm::mat4& transform() {return m_transform;}
        inline const glm::mat4& transform() const {return m_transform;};
        glm::mat4 global_transform() const;
        
        inline void set_parent(const Object3DPtr &the_parent){m_parent = the_parent;}
        inline Object3DPtr parent() const {return m_parent.lock();}
        
        void add_child(const Object3DPtr &the_child);
        inline std::list<Object3DPtr>& children(){return m_children;}
        inline const std::list<Object3DPtr>& children() const {return m_children;}
        
        virtual gl::AABB boundingBox() const;
        
        virtual void accept(Visitor &theVisitor);
        
    private:
        
        static uint32_t s_idPool;
        uint32_t m_id;
        bool m_enabled;
        glm::mat4 m_transform;
        std::weak_ptr<Object3D> m_parent;
        std::list<Object3DPtr> m_children;
    };
    
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
        virtual void visit(gl::Mesh &theNode){};
        virtual void visit(gl::Light &theNode){};
        virtual void visit(gl::Camera &theNode){};
        
    private:
        std::stack<glm::mat4> m_transform_stack;
    };

}}//namespace

#endif /* defined(__kinskiGL__Object3D__) */
