// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef KINSKI_GL_CAMERA_H_
#define KINSKI_GL_CAMERA_H_

#include "Object3D.h"

namespace kinski { namespace gl{

class KINSKI_API Camera : public Object3D
{
public:
    
    typedef std::shared_ptr<Camera> Ptr;
    typedef std::shared_ptr<const Camera> ConstPtr;
    
    Camera():Object3D(){};
    virtual ~Camera(){};
    
    glm::mat4 getProjectionMatrix() const {return m_projectionMatrix;};
    glm::mat4 getViewMatrix() const;
    AABB boundingBox() const;
    
    virtual gl::Frustum frustum() const = 0;
    virtual float near() const = 0;
    virtual float far() const = 0;
    
protected:

    void setProjectionMatrix(const glm::mat4 &theMatrix) { m_projectionMatrix = theMatrix; };
    
private:
    
    glm::mat4 m_projectionMatrix;
};
   
class KINSKI_API OrthographicCamera : public Camera
{
public:
    
    typedef std::shared_ptr<OrthographicCamera> Ptr;
    
    OrthographicCamera(float left = 0, float right = 1, float bottom = 0, float top = 1,
                       float near = 0, float far = 1000);
    
    virtual gl::Frustum frustum() const;
    
    float near() const {return m_near;};
    float far() const {return m_far;};
    inline float left() const {return m_left;};
    inline float right() const {return m_right;};
    inline float bottom() const {return m_bottom;};
    inline float top() const {return m_top;};
    
private:
    
    float m_left, m_right, m_bottom, m_top, m_near, m_far;
};
    
class KINSKI_API PerspectiveCamera : public Camera
{
    
public:
    
    typedef std::shared_ptr<PerspectiveCamera> Ptr;
    typedef std::shared_ptr<const PerspectiveCamera> ConstPtr;
    
    PerspectiveCamera(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 5000);

    virtual gl::Frustum frustum() const;
    
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
