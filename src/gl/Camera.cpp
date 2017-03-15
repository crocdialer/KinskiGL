// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Camera.hpp"
#include "geometry_types.hpp"

namespace kinski { namespace gl{
    
    /************************* Camera ************************/
    
    glm::mat4 Camera::view_matrix() const
    {
        return glm::inverse(global_transform());
    }
    
    AABB Camera::boundingbox() const
    {
        return AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
    }
    
    /****************** OrthographicCamera *******************/
    
    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top,
                                           float near, float far):
    Camera(),
    m_left(left),
    m_right(right),
    m_bottom(bottom),
    m_top(top),
    m_near(near),
    m_far(far)
    {
        update_projection_matrix();
    }
    
    void OrthographicCamera::update_projection_matrix()
    {
        set_projection_matrix(glm::ortho(m_left, m_right, m_bottom, m_top, m_near, m_far));
    }
    
    gl::Frustum OrthographicCamera::frustum() const
    {
        return gl::Frustum(left(), right(), bottom(), top(), near(), far()).transform(transform());
        //return gl::Frustum(projection_matrix()).transform(transform());
    }
    
    void OrthographicCamera::set_size(const gl::vec2 &the_sz)
    {
        m_left = 0.f;
        m_right = the_sz.x;
        m_bottom = 0.f;
        m_top = the_sz.y;
        m_near = 0.f;
        m_far = 1.f;
        update_projection_matrix();
    }
    
    /****************** PerspectiveCamera *******************/
    
    PerspectiveCamera::PerspectiveCamera(float ascpect, float fov, float near, float far):
    Camera(),
    m_near(near),
    m_far(far),
    m_fov(fov),
    m_aspect(ascpect)
    {
        update_projection_matrix();
    }
    
    void PerspectiveCamera::update_projection_matrix()
    {
        set_projection_matrix(glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far));
    }
    
    gl::Frustum PerspectiveCamera::frustum() const
    {
        return gl::Frustum(aspect(), fov(), near(), far()).transform(transform());
    }
    
    void PerspectiveCamera::set_fov(float theFov)
    {
        m_fov = theFov;
        update_projection_matrix();
    }
    
    void PerspectiveCamera::set_aspect(float theAspect)
    {
        if(std::isnan(theAspect)){ return; }
        m_aspect = theAspect;
        update_projection_matrix();
    }
    
    void PerspectiveCamera::set_clipping(float near, float far)
    {
        m_near = near;
        m_far = far;
        update_projection_matrix();
    }
    
}}//namespace
