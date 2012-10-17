//
//  Camera.h
//  kinskiGL
//
//  Created by Fabian on 10/16/12.
//
//

#ifndef __kinskiGL__Camera__
#define __kinskiGL__Camera__

#include "kinskiGL.h"

namespace kinski { namespace gl{

class Camera
{
protected:
    
    glm::mat4 m_projectionMatrix;
    
    // clipping planes
    float m_near, m_far;
    
public:
    
    typedef std::shared_ptr<Camera> Ptr;
    
    Camera(float near, float far):m_near(near),
    m_far(far){};
    
    virtual void updateProjectionMatrix() = 0;
    
    glm::mat4 getProjectionMatrix() const {return m_projectionMatrix;};
};
   
class OrthographicCamera : public Camera
{
private:
    
    float m_left, m_right, m_top, m_bottom;

public:
    
    OrthographicCamera(float left, float right, float top, float bottom, float near, float far);
    
    void updateProjectionMatrix();
};
    
class PerspectiveCamera : public Camera
{
private:
    
    float m_fov;
    
    float m_aspect;
    
public:
    
    PerspectiveCamera(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 2000);
    
    void updateProjectionMatrix();
    
    void setFov(float theFov);
    float getFov() const {return m_fov;};
    
    void setAspectRatio(float theAspect);
    float getAspectRatio() const {return m_aspect;};
};

}}//namespace

#endif /* defined(__kinskiGL__Camera__) */
