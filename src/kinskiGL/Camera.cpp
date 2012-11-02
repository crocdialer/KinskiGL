//
//  Camera.cpp
//  kinskiGL
//
//  Created by Fabian on 10/16/12.
//
//

#include "Camera.h"

namespace kinski { namespace gl{
    
    /****************** OrthographicCamera *******************/
    
    OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom,
                                           float near, float far):
    m_near(near),
    m_far(far),
    m_left(left),
    m_right(right),
    m_top(top),
    m_bottom(bottom)
    {
        updateProjectionMatrix();
    }
    
    void OrthographicCamera::updateProjectionMatrix()
    {
        m_projectionMatrix = glm::ortho(m_left, m_right, m_bottom, m_top, m_near, m_far);
    }
    
    /****************** PerspectiveCamera *******************/
    
    PerspectiveCamera::PerspectiveCamera(float ascpect, float fov, float near, float far):
    m_near(near),
    m_far(far),
    m_fov(fov),
    m_aspect(ascpect)
    {
    
    }
    
    void PerspectiveCamera::updateProjectionMatrix()
    {
        m_projectionMatrix = glm::perspective(m_fov, m_aspect, m_near, m_far);
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
    
}}//namespace