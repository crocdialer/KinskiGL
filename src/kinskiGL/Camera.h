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
public:
    
    typedef std::shared_ptr<Camera> Ptr;
    
    Camera():Object3D(){};
    
    void setLookAt(const glm::vec3 &theLookAt, const glm::vec3 &theUp = glm::vec3(0,1,0));
    
    glm::mat4 getProjectionMatrix() const {return m_projectionMatrix;};
    
    glm::mat4 getViewMatrix() const;
    
    virtual ~Camera(){};

protected:

    void setProjectionMatrix(const glm::mat4 &theMatrix) { m_projectionMatrix = theMatrix; };
    
private:
    
    glm::mat4 m_projectionMatrix;
};
   
class OrthographicCamera : public Camera
{
public:
    
    typedef std::shared_ptr<OrthographicCamera> Ptr;
    
    OrthographicCamera(float left = 0, float right = 1, float top = 1, float bottom = 0,
                       float near = 0, float far = 1000);
    
    inline float near() const {return m_near;};
    inline float far() const {return m_far;};
    inline float left() const {return m_left;};
    inline float right() const {return m_right;};
    inline float bottom() const {return m_bottom;};
    inline float top() const {return m_top;};
    
private:
    
    float m_near, m_far, m_left, m_right, m_top, m_bottom;
};
    
class PerspectiveCamera : public Camera
{
    
public:
    
    typedef std::shared_ptr<PerspectiveCamera> Ptr;
    
    PerspectiveCamera(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 2000);

    void setFov(float theFov);
    float fov() const {return m_fov;};
    
    void setAspectRatio(float theAspect);
    float aspectRatio() const {return m_aspect;};
    
    void setClippingPlanes(float near, float far);
    
    float near() const {return m_near;};
    float far() const {return m_far;};
    
private:
    
    float m_near, m_far;
    
    float m_fov;
    
    float m_aspect;
};

}}//namespace

#endif /* defined(__kinskiGL__Camera__) */
