// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Scene.h"
#include "Visitor.h"
#include "Mesh.h"
#include "Camera.h"
#include "Renderer.h"
#include "geometry_types.h"

using namespace std;

namespace kinski { namespace gl {
    
    struct range_item_t
    {
        Object3D* object;
        float distance;
        range_item_t(){}
        range_item_t(Object3D *obj, float d):object(obj), distance(d){}
        bool operator <(const range_item_t &other) const {return distance < other.distance;}
    };
    
    class UpdateVisitor : public Visitor
    {
    public:
        UpdateVisitor(float time_step):Visitor(), m_time_step(time_step){};

        void visit(Mesh &theNode)
        {
            theNode.update(m_time_step);
            Visitor::visit(static_cast<gl::Object3D&>(theNode));
        };
    private:
        float m_time_step;
    };
    
    class CullVisitor : public Visitor
    {
    public:
        CullVisitor(const CameraPtr &theCamera):Visitor(), m_frustum(theCamera->frustum()),
        m_render_bin(new gl::RenderBin(theCamera))
        {
            transform_stack().push(theCamera->getViewMatrix());
        }
        
        RenderBinPtr get_render_bin() const {return m_render_bin;}
        
        void visit(Mesh &theNode)
        {
            if(!theNode.enabled()) return;
            
            gl::AABB boundingBox = theNode.geometry()->boundingBox();
            //gl::Sphere s(theNode.position(), glm::length(boundingBox.halfExtents()));
            glm::mat4 model_view = transform_stack().top() * theNode.transform();
            boundingBox.transform(theNode.transform());
                    
            if (m_frustum.intersect(boundingBox))
            {
                RenderBin::item item;
                item.mesh = &theNode;
                item.transform = model_view;
                m_render_bin->items.push_back(item);
            }
            // super class provides node traversing and transform accumulation
            Visitor::visit(static_cast<gl::Object3D&>(theNode));
        }
        
        void visit(Light &theNode)
        {
            //TODO: only collect lights that actually affect the scene (e.g. point-light radi)
            if(theNode.enabled())
            {
                RenderBin::light light_item;
                light_item.light = &theNode;
                switch (theNode.type())
                {
                    case Light::DIRECTIONAL:
                        light_item.transform =
                            glm::mat4(glm::inverseTranspose(glm::mat3(transform_stack().top()))) *
                            theNode.transform();
                        break;
                        
                    case Light::POINT:
                    case Light::SPOT:
                        light_item.transform = transform_stack().top() * theNode.transform();
                        break;
                }
                
                m_render_bin->lights.push_back(light_item);
            }
            // super class provides node traversing and transform accumulation
            Visitor::visit(static_cast<gl::Object3D&>(theNode));
        }
        
        void clear(){m_render_bin->items.clear();}
        
    private:
        gl::Frustum m_frustum;
        RenderBinPtr m_render_bin;
    };
    
    Scene::Scene():
    m_root(new Object3D())
    {
    
    }
    
    void Scene::addObject(const Object3DPtr &theObject)
    {
        m_root->children().push_back(theObject);
    }
    
    void Scene::removeObject(const Object3DPtr &theObject)
    {
        m_root->children().remove(theObject);
    }
    
    void Scene::clear()
    {
        m_root.reset(new Object3D());
    }
    
    void Scene::update(float time_delta)
    {
        UpdateVisitor uv(time_delta);
        m_root->accept(uv);
    }
    
    RenderBinPtr Scene::cull(const CameraPtr &theCamera) const
    {
        CullVisitor cull_visitor(theCamera);
        m_root->accept(cull_visitor);
        RenderBinPtr ret = cull_visitor.get_render_bin();
        m_num_visible_objects = ret->items.size();
        return ret;
    }
    
    void Scene::render(const CameraPtr &theCamera) const
    {
        // shadow passes
        SelectVisitor<Light> lv;
        m_root->accept(lv);
        
        int i = 0;
        m_renderer.set_shadowmap_size(glm::vec2(1024));
        
        for(gl::Light *l : lv.getObjects())
        {
            if(l->enabled() && l->cast_shadow())
            {
                if(i >= m_renderer.shadow_fbos().size())
                {
                    LOG_WARNING << "too many lights with active shadows";
                    break;
                }
                m_renderer.set_shadow_pass(true);
                m_renderer.shadow_cams()[i] = gl::create_shadow_camera(l);
                
//                LOG_DEBUG << "rendering shadowmap: " << i;
                
                // offscreen render shadow map here
                gl::render_to_texture(m_renderer.shadow_fbos()[i], [&]()
                {
                    glClear(GL_DEPTH_BUFFER_BIT);
                    m_renderer.render(cull(m_renderer.shadow_cams()[i]));
                });
                i++;
                m_renderer.set_shadow_pass(false);
            }
        }
        
        // forward render pass
        m_renderer.render(cull(theCamera));
    }
    
    Object3DPtr Scene::pick(const Ray &ray, bool high_precision) const
    {
        Object3DPtr ret;
        SelectVisitor<Mesh> sv;
        m_root->accept(sv);
        
        std::list<range_item_t> clicked_items;
        for (const auto &the_object : sv.getObjects())
        {
            gl::OBB boundingBox (the_object->boundingBox(), the_object->global_transform());

            if (ray_intersection ray_hit = boundingBox.intersect(ray))
            {
                if(high_precision)
                {
                    if(gl::Mesh *m = the_object)
                    {
                        gl::Ray ray_in_object_space = ray.transform(glm::inverse(the_object->global_transform()));
                        const auto& vertices = m->geometry()->vertices();
                        for (const auto &face : m->geometry()->faces())
                        {
                            gl::Triangle t(vertices[face.a], vertices[face.b], vertices[face.c]);
                            
                            if(ray_triangle_intersection ray_tri_hit = t.intersect(ray_in_object_space))
                            {
                                float distance_scale = glm::length(the_object->global_scale() *
                                                                   ray_in_object_space.direction);
                                ray_tri_hit.distance *= distance_scale;
                                clicked_items.push_back(range_item_t(the_object, ray_tri_hit.distance));
                                LOG_TRACE<<"hit distance: "<<ray_tri_hit.distance;
                            }
                        }
                    }
                }
                else
                {
                    clicked_items.push_back(range_item_t(the_object, ray_hit.distance));
                }
            }
        }
        if(!clicked_items.empty())
        {
            clicked_items.sort();
            ret = clicked_items.front().object->shared_from_this();
            LOG_TRACE<<"ray hit id "<<ret->getID()<<" ("<<clicked_items.size()<<" total)";
        }
        return ret;
    }
    
}}//namespace