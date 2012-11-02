//
//  Camera.h
//  kinskiGL
//
//  Created by Fabian on 10/16/12.
//
//

#ifndef __kinskiGL__Camera__
#define __kinskiGL__Camera__

#include "Object3D.h"

namespace kinski { namespace gl{

class Camera : public Object3D
{
protected:
    
    glm::mat4 m_projectionMatrix;
    
public:
    
    typedef std::shared_ptr<Camera> Ptr;
    
    Camera(){};
    
    virtual void updateProjectionMatrix() = 0;
    
    glm::mat4 getProjectionMatrix() const {return m_projectionMatrix;};
    
    glm::mat4 getViewMatrix() const {return glm::mat4();};
    
    virtual ~Camera(){};
};
   
class OrthographicCamera : public Camera
{
private:
    
    float m_near, m_far, m_left, m_right, m_top, m_bottom;

public:
    
    OrthographicCamera(float left = 0, float right = 1, float top = 1, float bottom = 0,
                       float near = 0, float far = 1000);
    
    void updateProjectionMatrix();
};
    
class PerspectiveCamera : public Camera
{
private:
    
    float m_near, m_far;
    
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
