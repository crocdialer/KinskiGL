// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Scene.hpp"
#include "Visitor.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"
#include "SceneRenderer.hpp"
#include "Fbo.hpp"
#include "geometry_types.hpp"

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

        void visit(gl::Object3D &theNode) override
        {
            theNode.update(m_time_step);
            Visitor::visit(static_cast<gl::Object3D&>(theNode));
        };
    private:
        float m_time_step;
    };
    
    ScenePtr Scene::create()
    {
        return ScenePtr(new Scene());
    }
    
    Scene::Scene():
    m_renderer(gl::SceneRenderer::create()),
    m_root(Object3D::create("scene root"))
    {

    }
    
    void Scene::add_object(const Object3DPtr &the_object)
    {
        m_root->add_child(the_object);
    }
    
    void Scene::remove_object(const Object3DPtr &the_object)
    {
        m_root->remove_child(the_object, true);
    }
    
    void Scene::clear()
    {
        m_root = gl::Object3D::create("scene root");
        m_skybox.reset();
    }
    
    void Scene::update(float time_delta)
    {
        UpdateVisitor uv(time_delta);
        m_root->accept(uv);
    }
    
    void Scene::render(const CameraPtr &theCamera, const std::set<std::string> &the_tags) const
    {
        m_num_visible_objects = m_renderer->render_scene(shared_from_this(), theCamera, the_tags);
    }
    
    Object3DPtr Scene::pick(const Ray &ray, bool high_precision,
                            const std::set<std::string> &the_tags) const
    {
        Object3DPtr ret;
        SelectVisitor<Object3D> sv(the_tags);
        m_root->accept(sv);
        
        std::list<range_item_t> clicked_items;
        for (const auto &the_object : sv.get_objects())
        {
            if(the_object == m_root.get()){ continue; }

            gl::OBB boundingBox = the_object->obb();

            if (ray_intersection ray_hit = boundingBox.intersect(ray))
            {
                if(high_precision)
                {
                    if(const gl::Mesh *m = dynamic_cast<const gl::Mesh*>(the_object))
                    {
                        gl::Ray ray_in_object_space = ray.transform(glm::inverse(the_object->global_transform()));
                        const auto &vertices = m->geometry()->vertices();
                        const auto &indices = m->geometry()->indices();
                        
                        for(const auto &e : m->entries())
                        {
                            if(e.primitive_type && e.primitive_type != GL_TRIANGLES){ continue; }
                            
                            for(uint32_t i = 0; i < e.num_indices; i += 3)
                            {
                                gl::Triangle t(vertices[indices[i + e.base_index] + e.base_vertex],
                                               vertices[indices[i + e.base_index + 1] + e.base_vertex],
                                               vertices[indices[i + e.base_index + 2] + e.base_vertex]);
                                
                                if(ray_triangle_intersection ray_tri_hit = t.intersect(ray_in_object_space))
                                {
                                    float distance_scale = glm::length(the_object->global_scale() *
                                                                       ray_in_object_space.direction);
                                    ray_tri_hit.distance *= distance_scale;
                                    clicked_items.push_back(range_item_t(the_object, ray_tri_hit.distance));
                                    LOG_TRACE_2 << "hit distance: " << ray_tri_hit.distance;
                                    break;
                                }
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
            LOG_TRACE << "ray hit id " << ret->get_id() << " (" << clicked_items.size() << " total)";
        }
        return ret;
    }
    
    gl::Object3DPtr Scene::get_object_by_name(const std::string &the_name) const
    {
        gl::SelectVisitor<gl::Object3D> sv({}, false);
        root()->accept(sv);
        for(gl::Object3D *o : sv.get_objects())
        {
            if(o->name() == the_name){ return o->shared_from_this(); }
        }
        return nullptr;
    }
    
    std::vector<gl::Object3DPtr> Scene::get_objects_by_tags(const std::set<std::string> &the_tags) const
    {
        std::vector<gl::Object3DPtr> ret;
        gl::SelectVisitor<gl::Object3D> sv(the_tags, false);
        root()->accept(sv);
        
        for(gl::Object3D *o : sv.get_objects()){ ret.push_back(o->shared_from_this()); }
        return ret;
    }
    
    std::vector<gl::Object3DPtr> Scene::get_objects_by_tag(const std::string &the_tag) const
    {
        return get_objects_by_tags({the_tag});
    }
    
    void Scene::set_skybox(const gl::Texture& t)
    {
        if(!t){ m_skybox.reset(); return; }
        
        auto mat = gl::Material::create();
        mat->set_culling(gl::Material::CULL_FRONT);
        m_skybox = gl::Mesh::create(gl::Geometry::create_box(gl::vec3(.5f)), mat);

        switch(t.target())
        {
            case GL_TEXTURE_CUBE_MAP:
                LOG_DEBUG << "adding cubical skybox";
                mat->set_shader(gl::create_shader(gl::ShaderType::UNLIT_CUBE));
                mat->add_texture(t, Texture::Usage::ENVIROMENT);
                break;
            case GL_TEXTURE_2D:
                LOG_DEBUG << "adding panorama skybox";
#ifdef KINSKI_GLES_2
                mat->set_shader(gl::create_shader(gl::ShaderType::UNLIT_PANORAMA));
                mat->add_texture(t, Texture::Usage::ENVIROMENT);
#else
                mat->set_shader(gl::create_shader(gl::ShaderType::UNLIT_CUBE));
                mat->add_texture(gl::create_cube_texture_from_panorama(t, 1024, true, true), Texture::Usage::ENVIROMENT);
#endif
                break;
            default:
                break;
        }
    }
    
}}//namespace
