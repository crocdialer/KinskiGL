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
    
    OrthoCamera::Ptr OrthoCamera::create_for_window()
    {
        return OrthoCamera::Ptr(new OrthoCamera(0.f, gl::window_dimension().x,
                                                0.f, gl::window_dimension().y,
                                                0.f, 1.f));
    }
    OrthoCamera::Ptr OrthoCamera::create(float left, float right,
                                         float bottom, float top,
                                         float near, float far)
    {
        return OrthoCamera::Ptr(new OrthoCamera(left, right, bottom, top, near, far));
    };
    
    OrthoCamera::OrthoCamera(float left, float right,
                             float bottom, float top,
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
    
    void OrthoCamera::update_projection_matrix()
    {
        set_projection_matrix(glm::ortho(m_left, m_right, m_bottom, m_top, m_near, m_far));
    }
    
    gl::Frustum OrthoCamera::frustum() const
    {
        return gl::Frustum(left(), right(), bottom(), top(), near(), far()).transform(global_transform());
        //return gl::Frustum(projection_matrix()).transform(transform());
    }
    
    void OrthoCamera::set_size(const gl::vec2 &the_sz)
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
        return gl::Frustum(aspect(), fov(), near(), far()).transform(global_transform());
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
    
    /****************** CubeCamera *******************/
    
    void CubeCamera::update_projection_matrix()
    {
        set_projection_matrix(glm::perspective(glm::radians(90.f), 1.f, m_near, m_far));
    }
    
    gl::Frustum CubeCamera::frustum() const
    {
        auto p = global_position();
        return gl::Frustum(p.x - far(), p.x + far(), p.y - far(), p.y + far(), p.z - far(), p.z + far());
    }
    
    mat4 CubeCamera::view_matrix(uint32_t the_face) const
    {
        const gl::vec3 vals[12] =
        {
            gl::X_AXIS, -gl::Y_AXIS,
            -gl::X_AXIS, -gl::Y_AXIS,
            gl::Y_AXIS, gl::Z_AXIS,
            -gl::Y_AXIS, -gl::Z_AXIS,
            gl::Z_AXIS, -gl::Y_AXIS,
            -gl::Z_AXIS, -gl::Y_AXIS
        };
        auto p = global_position();
        the_face = crocore::clamp<uint32_t>(the_face, 0, 5);
        return glm::lookAt(p, p + vals[2 * the_face], vals[2 * the_face + 1]);
    }

    std::vector<glm::mat4> CubeCamera::view_matrices() const
    {
        std::vector<glm::mat4> out_matrices(6);

        for(uint32_t i = 0; i < 6; ++i)
        {
            out_matrices[i] = view_matrix(i);
        }
        return out_matrices;
    }

}}//namespace
