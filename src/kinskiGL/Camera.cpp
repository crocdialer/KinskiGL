// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Camera.h"
#include "geometry_types.h"

namespace kinski { namespace gl{
    
    /************************* Camera ************************/
    
    glm::mat4 Camera::getViewMatrix() const
    {
        return glm::inverse(transform());
    }
    
    AABB Camera::boundingBox() const
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
        updateProjectionMatrix();
    }
    
    void OrthographicCamera::updateProjectionMatrix()
    {
        setProjectionMatrix(glm::ortho(m_left, m_right, m_bottom, m_top, m_near, m_far));
    }
    
    gl::Frustum OrthographicCamera::frustum() const
    {
        return gl::Frustum(left(), right(), bottom(), top(), near(), far()).transform(transform());
        //return gl::Frustum(getProjectionMatrix()).transform(transform());
    }
    
    /****************** PerspectiveCamera *******************/
    
    PerspectiveCamera::PerspectiveCamera(float ascpect, float fov, float near, float far):
    Camera(),
    m_near(near),
    m_far(far),
    m_fov(fov),
    m_aspect(ascpect)
    {
        updateProjectionMatrix();
    }
    
    void PerspectiveCamera::updateProjectionMatrix()
    {
        setProjectionMatrix(glm::perspective(m_fov, m_aspect, m_near, m_far));
    }
    
    gl::Frustum PerspectiveCamera::frustum() const
    {
        return gl::Frustum(aspectRatio(), fov(), near(), far()).transform(transform());
    }
    
    void PerspectiveCamera::setFov(float theFov)
    {
        m_fov = theFov;
        updateProjectionMatrix();
    }
    
    void PerspectiveCamera::setAspectRatio(float theAspect)
    {
        m_aspect = theAspect;
        updateProjectionMatrix();
    }
    
    void PerspectiveCamera::setClippingPlanes(float near, float far)
    {
        m_near = near;
        m_far = far;
        updateProjectionMatrix();
    }
    
}}//namespace