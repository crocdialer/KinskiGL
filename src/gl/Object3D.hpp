// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "gl/gl.hpp"
#include "geometry_types.hpp"

namespace kinski { namespace gl {

    class KINSKI_API Object3D : public std::enable_shared_from_this<Object3D>
    {
    public:
        
        using update_fn_t = std::function<void (float)>;
        
        static Object3DPtr create(const std::string &the_name = "");

        virtual ~Object3D(){};
        
        inline uint32_t get_id() const {return m_id;};
        inline void set_id(uint32_t the_id) { m_id = the_id; };
        inline const std::string name() const { return m_name; }
        inline void set_name(const std::string &the_name){ m_name = the_name; }
        inline const std::set<std::string>& tags() const { return m_tags; };
        inline std::set<std::string>& tags() { return m_tags; };
        inline bool has_tag(const std::string& the_tag) const { return m_tags.count(the_tag); };
        
        void add_tag(const std::string& the_tag, bool recursive = false);
        void remove_tag(const std::string& the_tag, bool recursive = false);
        
        inline bool enabled() const {return m_enabled;}
        void set_enabled(bool b = true){m_enabled = b;}
        bool billboard() const {return m_billboard;};
        void set_billboard(bool b) {m_billboard = b;}
        inline void set_position(const vec3 &thePos) { position() = thePos; };
        inline vec3 position() const {return m_transform[3].xyz(); }
        inline vec3& position() {return *reinterpret_cast<vec3*>(&m_transform[3].x);}
        inline vec3 lookAt() const {return normalize(-m_transform[2].xyz());}
        inline vec3 side() const {return normalize(m_transform[0].xyz());}
        inline vec3 up() const {return normalize(m_transform[1].xyz());}
        void set_rotation(const quat &theRot);
        void set_rotation(const mat3 &theRot);
        void set_rotation(float pitch, float yaw, float roll);
        quat rotation() const;
        
        inline vec3 scale(){return vec3(length(m_transform[0]),
                                        length(m_transform[1]),
                                        length(m_transform[2]));};

        void set_scale(const vec3 &s);
        inline void set_scale(float s){set_scale(vec3(s));}
        
        void set_look_at(const vec3 &theLookAt, const vec3 &theUp = vec3(0, 1, 0));
        void set_look_at(const Object3DPtr &theLookAt);
        
        inline void set_transform(const mat4 &theTrans) {m_transform = theTrans;}
        inline mat4& transform() {return m_transform;}
        inline const mat4& transform() const {return m_transform;};
        
        void set_parent(const Object3DPtr &the_parent);
        inline Object3DPtr parent() const {return m_parent.lock();}
        
        void add_child(const Object3DPtr &the_child);
        void remove_child(const Object3DPtr &the_child, bool recursive = false);
        inline std::list<Object3DPtr>& children(){return m_children;}
        inline const std::list<Object3DPtr>& children() const {return m_children;}
        
        mat4 global_transform() const;
        vec3 global_position() const;
        quat global_rotation() const;
        vec3 global_scale() const;
        
        void set_global_transform(const mat4 &transform);
        void set_global_position(const vec3 &position);
        void set_global_rotation(const quat &rotation);
        void set_global_scale(const vec3 &scale);
        
        virtual gl::AABB aabb() const;
        virtual gl::OBB obb() const;
        
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
        void set_update_function(update_fn_t f){ m_update_function = f;}
        
        virtual void accept(Visitor &theVisitor);

    protected:
        Object3D();

    private:

        static uint32_t s_id_pool;
        
        //! unique id
        uint32_t m_id;
        
        //! set of tags
        std::set<std::string> m_tags;
        
        //! user definable name
        std::string m_name;
        
        //! enabled hint, can be used by Visitors
        bool m_enabled;
        
        //! billboard hint, can be used by Visitors
        bool m_billboard;
        
        mat4 m_transform;
        std::weak_ptr<Object3D> m_parent;
        std::list<Object3DPtr> m_children;
        
        update_fn_t m_update_function;
    };

}}//namespace
