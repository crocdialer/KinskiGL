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
    
    mat4 getProjectionMatrix() const {return m_projectionMatrix;};
    mat4 getViewMatrix() const;
    AABB boundingBox() const;
    
    virtual gl::Frustum frustum() const = 0;
    virtual float near() const = 0;
    virtual float far() const = 0;
    
protected:

    virtual void updateProjectionMatrix() = 0;
    void setProjectionMatrix(const mat4 &theMatrix) { m_projectionMatrix = theMatrix; };
    
private:
    
    mat4 m_projectionMatrix;
};
   
class KINSKI_API OrthographicCamera : public Camera
{
public:
    
    typedef std::shared_ptr<OrthographicCamera> Ptr;
    
    static Ptr create(float left, float right, float bottom, float top,
                      float near, float far)
    {
        return Ptr(new OrthographicCamera(left, right, bottom, top, near, far));
    };
    
    virtual gl::Frustum frustum() const;
    
    float near() const {return m_near;};
    void near(float val)
    {
        m_near = val;
        updateProjectionMatrix();
    };
    float far() const {return m_far;};
    void far(float val)
    {
        m_far = val;
        updateProjectionMatrix();
    };
    inline float left() const {return m_left;};
    void left(float val)
    {
        m_left = val;
        updateProjectionMatrix();
    };
    inline float right() const {return m_right;};
    void right(float val)
    {
        m_right = val;
        updateProjectionMatrix();
    };
    inline float bottom() const {return m_bottom;};
    void bottom(float val)
    {
        m_bottom = val;
        updateProjectionMatrix();
    };
    inline float top() const {return m_top;};
    void top(float val)
    {
        m_top = val; updateProjectionMatrix();
    };

protected:
    
    void updateProjectionMatrix();
    
private:
    
    OrthographicCamera(float left, float right, float bottom, float top,
                       float near, float far);
    
    float m_left, m_right, m_bottom, m_top, m_near, m_far;
};
    
class KINSKI_API PerspectiveCamera : public Camera
{
    
public:
    
    typedef std::shared_ptr<PerspectiveCamera> Ptr;
    typedef std::shared_ptr<const PerspectiveCamera> ConstPtr;
    
    static Ptr create(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 5000)
    { return Ptr(new PerspectiveCamera(ascpect, fov, near, far)); }
    
    PerspectiveCamera(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 5000);

    virtual gl::Frustum frustum() const;
    
    void setFov(float theFov);
    float fov() const {return m_fov;};
    
    void setAspectRatio(float theAspect);
    float aspectRatio() const {return m_aspect;};
    
    void setClippingPlanes(float near, float far);
    
    float near() const {return m_near;};
    float far() const {return m_far;};

protected:
    
    void updateProjectionMatrix();
    
private:
    
    float m_near, m_far;
    float m_fov;
    float m_aspect;
};

}}//namespace

#endif /* defined(__gl__Camera__) */
