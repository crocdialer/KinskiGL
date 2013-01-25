//
//  Camera.cpp
//  kinskiGL
//
//  Created by Fabian on 10/16/12.
//
//

#include "Camera.h"
#include "geometry_types.h"

namespace kinski { namespace gl{
    
    /************************* Camera ************************/
    
    glm::mat4 Camera::getViewMatrix() const
    {
        return glm::inverse(transform());
    }
    
    void Camera::setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp)
    {
        setTransform( glm::inverse(glm::lookAt(position(), theLookAt, theUp)) );
    }
    
    /****************** OrthographicCamera *******************/
    
    OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom,
                                           float near, float far):
    Camera(),
    m_near(near),
    m_far(far),
    m_left(left),
    m_right(right),
    m_top(top),
    m_bottom(bottom)
    {
        setProjectionMatrix(glm::ortho(m_left, m_right, m_bottom, m_top, m_near, m_far));
    }
    
    gl::Frustum OrthographicCamera::frustum() const
    {
        return gl::Frustum(transform(), left(), right(), bottom(),
                    top(), near(), far());
    }
    
    /****************** PerspectiveCamera *******************/
    
    PerspectiveCamera::PerspectiveCamera(float ascpect, float fov, float near, float far):
    Camera(),
    m_near(near),
    m_far(far),
    m_fov(fov),
    m_aspect(ascpect)
    {
        setProjectionMatrix(glm::perspective(m_fov, m_aspect, m_near, m_far));
    }
    
    gl::Frustum PerspectiveCamera::frustum() const
    {
        return gl::Frustum(transform(), fov(), near(), far());
    }
    
    void PerspectiveCamera::setFov(float theFov)
    {
        m_fov = theFov;
        setProjectionMatrix(glm::perspective(m_fov, m_aspect, m_near, m_far));
    }
    
    void PerspectiveCamera::setAspectRatio(float theAspect)
    {
        m_aspect = theAspect;
        setProjectionMatrix(glm::perspective(m_fov, m_aspect, m_near, m_far));
    }
    
    void PerspectiveCamera::setClippingPlanes(float near, float far)
    {
        m_near = near;
        m_far = far;
        setProjectionMatrix(glm::perspective(m_fov, m_aspect, m_near, m_far));
    }
    
}}//namespace