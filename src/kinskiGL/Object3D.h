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

namespace kinski { namespace gl {

    class KINSKI_API Object3D : public std::enable_shared_from_this<Object3D>
    {
    public:
        
        Object3D();
        virtual ~Object3D(){};
        
        uint32_t getID() const { return m_id; };
        inline void setPosition(const glm::vec3 &thePos) { m_transform[3].xyz() = thePos; };
        
        inline glm::vec3 position() const { return m_transform[3].xyz(); }
        inline glm::vec3& position() { return *reinterpret_cast<glm::vec3*>(&m_transform[3]);}
        inline glm::vec3 lookAt() const { return glm::normalize(-m_transform[2].xyz()); }
        inline glm::vec3 side() const { return glm::normalize(m_transform[0].xyz()); }
        inline glm::vec3 up() const { return glm::normalize(m_transform[1].xyz()); }
        void setRotation(const glm::quat &theRot);
        void setRotation(const glm::mat3 &theRot);
        glm::quat rotation() const;
        inline void setTransform(const glm::mat4 &theTrans){ m_transform = theTrans; };
        inline glm::mat4& transform() { return m_transform; };
        inline const glm::mat4& transform() const { return m_transform; };
        inline Object3DPtr parent() const {return m_parent;}
        inline std::list<Object3DPtr>& children(){return m_children;}
        inline const std::list<Object3DPtr>& children() const {return m_children;}
        
        virtual void update(float time_delta){};
        virtual AABB boundingBox() const;
        
        void accept(Visitor &theVisitor);
        
    private:
        
        static uint32_t s_idPool;
        uint32_t m_id;
        glm::mat4 m_transform;
        
        Object3DPtr m_parent;
        std::list<Object3DPtr> m_children;
    };
    
    class Visitor
    {
    public:
        Visitor();
        virtual ~Visitor();
        void visit(const gl::Object3DPtr &theNode);
        void visit(const gl::MeshPtr &theNode);
        
    private:
        std::stack<glm::mat4> m_transform_stack;
    };

}}//namespace

#endif /* defined(__kinskiGL__Object3D__) */
