// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt )
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __gl__Object3D__
#define __gl__Object3D__

#include "KinskiGL.h"
#include "geometry_types.h"

namespace kinski { namespace gl {

    class KINSKI_API Object3D : public std::enable_shared_from_this<Object3D>
    {
    public:
        
        typedef std::function<void (float)> UpdateFunction;
        
        Object3D();
        virtual ~Object3D(){};
        
        uint32_t getID() const {return m_id;};
        const std::string name() const { return m_name; }
        void set_name(const std::string &the_name){ m_name = the_name; }
        
        bool enabled() const {return m_enabled;}
        void set_enabled(bool b = true){m_enabled = b;}
        bool billboard() const {return m_billboard;};
        void set_billboard(bool b) {m_billboard = b;}
        inline void setPosition(const glm::vec3 &thePos) {m_transform[3].xyz() = thePos;};
        inline glm::vec3 position() const {return m_transform[3].xyz(); }
        inline glm::vec3& position() {return *reinterpret_cast<glm::vec3*>(&m_transform[3]);}
        inline glm::vec3 lookAt() const {return glm::normalize(-m_transform[2].xyz());}
        inline glm::vec3 side() const {return glm::normalize(m_transform[0].xyz());}
        inline glm::vec3 up() const {return glm::normalize(m_transform[1].xyz());}
        void setRotation(const glm::quat &theRot);
        void setRotation(const glm::mat3 &theRot);
        void setRotation(float pitch, float yaw, float roll);
        glm::quat rotation() const;
        
        inline glm::vec3 scale(){return glm::vec3(glm::length(m_transform[0]),
                                                  glm::length(m_transform[1]),
                                                  glm::length(m_transform[2]));};

        void setScale(const glm::vec3 &s);
        inline void setScale(float s){setScale(glm::vec3(s));}
        
        void setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp = glm::vec3(0, 1, 0));
        void setLookAt(const Object3DPtr &theLookAt);
        
        inline void setTransform(const glm::mat4 &theTrans) {m_transform = theTrans;}
        inline glm::mat4& transform() {return m_transform;}
        inline const glm::mat4& transform() const {return m_transform;};
        
        void set_parent(const Object3DPtr &the_parent);
        inline Object3DPtr parent() const {return m_parent.lock();}
        
        void add_child(const Object3DPtr &the_child);
        void remove_child(const Object3DPtr &the_child);
        inline std::list<Object3DPtr>& children(){return m_children;}
        inline const std::list<Object3DPtr>& children() const {return m_children;}
        
        glm::mat4 global_transform() const;
        glm::vec3 global_position() const;
        glm::quat global_rotation() const;
        glm::vec3 global_scale() const;
        
        void set_global_transform(const glm::mat4 &transform);
        void set_global_position(const glm::vec3 &position);
        void set_global_rotation(const glm::quat &rotation);
        void set_global_scale(const glm::vec3 &scale);
        
        virtual gl::AABB boundingBox() const;
        
        /*!
         * Performs an update on this scenegraph node.
         * Triggered by gl::Scene instances during gl::Scene::update(float delta) calls
         */
        virtual void update(float time_delta)
        {
            if(m_update_function){m_update_function(time_delta);}
        };
        
        /*!
         * Provide a function object to be called on each update
         */
        void set_update_function(UpdateFunction f){ m_update_function = f;}
        
        virtual void accept(Visitor &theVisitor);
        
    private:
        
        static uint32_t s_idPool;
        
        //! unique id
        uint32_t m_id;
        
        //! user definable name
        std::string m_name;
        
        //! enabled hint, can be used by Visitors
        bool m_enabled;
        
        //! billboard hint, can be used by Visitors
        bool m_billboard;
        
        glm::mat4 m_transform;
        std::weak_ptr<Object3D> m_parent;
        std::list<Object3DPtr> m_children;
        
        UpdateFunction m_update_function;
    };

}}//namespace

#endif /* defined(__gl__Object3D__) */
