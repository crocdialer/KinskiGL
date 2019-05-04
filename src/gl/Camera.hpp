// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "Object3D.hpp"

namespace kinski { namespace gl{

class Camera : public Object3D
{
public:
    
    typedef std::shared_ptr<Camera> Ptr;
    typedef std::shared_ptr<const Camera> ConstPtr;
    
    Camera():Object3D(){};
    virtual ~Camera(){};
    
    mat4 projection_matrix() const {return m_projectionMatrix;};
    mat4 view_matrix() const;
    AABB boundingbox() const;
    
    virtual gl::Frustum frustum() const = 0;
    virtual float near() const = 0;
    virtual float far() const = 0;
    
protected:

    virtual void update_projection_matrix() = 0;
    void set_projection_matrix(const mat4 &theMatrix) { m_projectionMatrix = theMatrix; };
    
private:
    
    mat4 m_projectionMatrix;
};
   
class OrthoCamera : public Camera
{
public:
    
    typedef std::shared_ptr<OrthoCamera> Ptr;
    
    static Ptr create_for_window();
    static Ptr create(float left, float right, float bottom, float top, float near, float far);
    
    virtual gl::Frustum frustum() const override;
    
    float near() const override {return m_near;};
    void near(float val)
    {
        m_near = val;
        update_projection_matrix();
    };
    float far() const override {return m_far;};
    void far(float val)
    {
        m_far = val;
        update_projection_matrix();
    };
    inline float left() const {return m_left;};
    void left(float val)
    {
        m_left = val;
        update_projection_matrix();
    };
    inline float right() const {return m_right;};
    void right(float val)
    {
        m_right = val;
        update_projection_matrix();
    };
    inline float bottom() const {return m_bottom;};
    void bottom(float val)
    {
        m_bottom = val;
        update_projection_matrix();
    };
    inline float top() const {return m_top;};
    void top(float val)
    {
        m_top = val;
        update_projection_matrix();
    };
    
    void set_size(const gl::vec2 &the_sz);

protected:
    
    void update_projection_matrix() override;
    
private:
    
    OrthoCamera(float left, float right, float bottom, float top,
                       float near, float far);
    
    float m_left, m_right, m_bottom, m_top, m_near, m_far;
};
    
class PerspectiveCamera : public Camera
{
    
public:
    
    typedef std::shared_ptr<PerspectiveCamera> Ptr;
    typedef std::shared_ptr<const PerspectiveCamera> ConstPtr;
    
    static Ptr create(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 5000)
    { return Ptr(new PerspectiveCamera(ascpect, fov, near, far)); }
    
    PerspectiveCamera(float ascpect = 4.f / 3.f, float fov = 45, float near = .1, float far = 5000);

    virtual gl::Frustum frustum() const override;
    
    void set_fov(float theFov);
    float fov() const {return m_fov;};
    
    void set_aspect(float theAspect);
    float aspect() const {return m_aspect;};
    
    void set_clipping(float near, float far);
    
    float near() const override {return m_near;};
    float far() const override {return m_far;};

protected:
    
    void update_projection_matrix() override;
    
private:
    
    float m_near, m_far;
    float m_fov;
    float m_aspect;
};
    
class CubeCamera : public Camera
{
public:
    
    typedef std::shared_ptr<CubeCamera> Ptr;
    typedef std::shared_ptr<const CubeCamera> ConstPtr;
    
    static Ptr create(float the_near, float the_far)
    {
        return Ptr(new CubeCamera(the_near, the_far));
    };
    
    virtual ~CubeCamera() = default;
    
    gl::Frustum frustum() const override;
    float near() const override { return m_near; };
    float far() const override { return m_far; };
    
    mat4 view_matrix(uint32_t the_face) const;

    std::vector<glm::mat4> view_matrices() const;
    
private:
    
    CubeCamera(float the_near, float the_far):
    Camera(),
    m_near(the_near),
    m_far(the_far)
    {
        update_projection_matrix();
    };
    void update_projection_matrix() override;
    float m_near, m_far;
};

}}//namespace
