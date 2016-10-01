// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  Renderer.cpp
//
//  Created by Fabian on 4/21/13.

#include "Visitor.hpp"
#include "Mesh.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Fbo.hpp"
#include "Scene.hpp"
#include "SceneRenderer.hpp"

namespace kinski{ namespace gl{

    using std::pair;
    using std::map;
    using std::list;
    using namespace glm;
    
    class CullVisitor : public Visitor
    {
    public:
        CullVisitor(const CameraPtr &theCamera, const std::set<std::string> &the_tags):
        Visitor(),
        m_frustum(theCamera->frustum()),
        m_tags(the_tags),
        m_render_bin(new gl::RenderBin(theCamera))
        {
            transform_stack().push(theCamera->getViewMatrix());
        }
        
        RenderBinPtr get_render_bin() const {return m_render_bin;}
        
        void visit(Mesh &theNode) override
        {
            if(!theNode.enabled() || !check_tags(m_tags, theNode.tags())) return;
            
            glm::mat4 model_view = transform_stack().top() * theNode.transform();
            gl::AABB boundingBox = theNode.boundingBox();
            //            boundingBox.transform(theNode.global_transform());
            
            if(m_frustum.intersect(boundingBox))
            {
                RenderBin::item item;
                item.mesh = &theNode;
                item.transform = model_view;
                m_render_bin->items.push_back(item);
            }
            // super class provides node traversing and transform accumulation
            Visitor::visit(static_cast<gl::Object3D&>(theNode));
        }
        
        void visit(Light &theNode) override
        {
            //TODO: only collect lights that actually affect the scene (e.g. point-light radi)
            if(theNode.enabled() && check_tags(m_tags, theNode.tags()))
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
        std::set<std::string> m_tags;
        RenderBinPtr m_render_bin;
    };
    
    SceneRenderer::SceneRenderer()
    {
        m_shadow_fbos.resize(4);
        m_shadow_cams.resize(4);
    }
    
    uint32_t SceneRenderer::render_scene(const gl::SceneConstPtr &the_scene,
                                         const CameraPtr &the_cam,
                                         const std::set<std::string> &the_tags)
    {
        // shadow passes
        SelectVisitor<Light> lv;
        the_scene->root()->accept(lv);

        float extents = 2.f * glm::length(the_scene->root()->boundingBox().halfExtents());

        uint32_t i = 0;
        set_shadowmap_size(glm::vec2(1024));

        for(gl::Light *l : lv.get_objects())
        {
            if(l->enabled() && l->cast_shadow())
            {
                if(i >= shadow_fbos().size())
                {
                    LOG_WARNING << "too many lights with active shadows";
                    break;
                }
                set_shadow_pass(true);
                shadow_cams()[i] = gl::create_shadow_camera(l, extents);

                // offscreen render shadow map here
                gl::render_to_texture(shadow_fbos()[i], [&]()
                {
                    glClear(GL_DEPTH_BUFFER_BIT);
                    render(cull(the_scene, shadow_cams()[i]));
                });
                i++;
                set_shadow_pass(false);
            }
        }

        // skybox drawing
        if(the_scene->skybox())
        {
            gl::set_projection(the_cam);
            mat4 m = the_cam->getViewMatrix();
            m[3] = vec4(0, 0, 0, 1);
            gl::load_matrix(gl::MODEL_VIEW_MATRIX, m);
            gl::draw_mesh(the_scene->skybox());
        }

        // forward render pass
        auto render_bin = cull(the_scene, the_cam, the_tags);
        render(render_bin);
        
        // return number of rendered objects
        return render_bin->items.size();
    }
    
    RenderBinPtr SceneRenderer::cull(const gl::SceneConstPtr &the_scene,
                                     const CameraPtr &theCamera,
                                     const std::set<std::string> &the_tags) const
    {
        CullVisitor cull_visitor(theCamera, the_tags);
        the_scene->root()->accept(cull_visitor);
        RenderBinPtr ret = cull_visitor.get_render_bin();
//        m_num_visible_objects = ret->items.size();
        return ret;
    }
    
    void SceneRenderer::render(const RenderBinPtr &theBin)
    {        
        std::list<RenderBin::item> opaque_items, blended_items;
        for (auto &item :theBin->items)
        {
            bool opaque = true;
            for (const auto &material : item.mesh->materials())
            {
                if(!material->opaque())
                {
                    opaque = false;
                    break;
                }
            }
            if(opaque){opaque_items.push_back(item);}
            else{blended_items.push_back(item);}
        }
        
        //sort by distance to camera
        opaque_items.sort(RenderBin::sort_items_increasing());
        blended_items.sort(RenderBin::sort_items_decreasing());
        
        m_num_shadow_lights = 0;
        
        for(const RenderBin::light &l : theBin->lights)
        {
            if(l.light->cast_shadow()){ m_num_shadow_lights++; }
        }
        
        // update uniform buffers (global light settings)
        update_uniform_buffers(theBin->lights);
        
        // draw our stuff
        draw_sorted_by_material(theBin->camera, opaque_items, theBin->lights);
        draw_sorted_by_material(theBin->camera, blended_items, theBin->lights);
    }
    
    void SceneRenderer::draw_sorted_by_material(const CameraPtr &cam, const list<RenderBin::item> &item_list,
                                           const list<RenderBin::light> &light_list)
    {
        KINSKI_CHECK_GL_ERRORS();
//        typedef map<pair<Material*, Geometry*>, list<RenderBin::item> > MatMeshMap;
//        MatMeshMap mat_mesh_map;
        
        for (const RenderBin::item &item : item_list)
        {
            Mesh *m = item.mesh;
            
            const glm::mat4 &modelView = item.transform;
            mat4 mvp_matrix = cam->getProjectionMatrix() * modelView;
            mat3 normal_matrix = glm::inverseTranspose(glm::mat3(modelView));
            
            for(auto &mat : m->materials())
            {
                mat->uniform("u_modelViewMatrix", modelView);
                mat->uniform("u_modelViewProjectionMatrix", mvp_matrix);
                mat->uniform("u_normalMatrix", normal_matrix);

                if(!m_shadow_pass && m_num_shadow_lights)
                {
                    std::vector<glm::mat4> shadow_matrices;
                    char buf[32];
                    for(int i = 0; i < m_num_shadow_lights; i++)
                    {
                        if(!m_shadow_cams[i]) break;
                        int tex_unit = mat->textures().size() + i;
                        shadow_matrices.push_back(m_shadow_cams[i]->getProjectionMatrix() *
                                                  m_shadow_cams[i]->getViewMatrix() * m->global_transform());
                        m_shadow_fbos[i].getDepthTexture().bind(tex_unit);
                        sprintf(buf, "u_shadow_map[%d]", i);
                        mat->uniform(buf, tex_unit);
                    }
                    mat->uniform("u_shadow_matrices", shadow_matrices);
                    mat->uniform("u_shadow_map_size", m_shadow_fbos[0].getSize());
//                    mat->uniform("u_poisson_radius", 3.f);
                }
                
                // update uniform buffers for matrices and shadows
                update_uniform_buffer_shadows(m->global_transform());
                
                if(m->geometry()->hasBones())
                {
                    mat->uniform("u_bones", m->boneMatrices());
                }
                
                // lighting parameters
#if !defined(KINSKI_GLES)
                GLint block_index = mat->shader().getUniformBlockIndex("LightBlock");
                
                if(block_index >= 0)
                {
                    glUniformBlockBinding(mat->shader().getHandle(), block_index, LIGHT_BLOCK);
                    KINSKI_CHECK_GL_ERRORS();
                }
                
                block_index = mat->shader().getUniformBlockIndex("MaterialBlock");
                
                if(block_index >= 0)
                {
                    glUniformBlockBinding(mat->shader().getHandle(), block_index, MATERIAL_BLOCK);
                    KINSKI_CHECK_GL_ERRORS();
                }
                
//                block_index = mat->shader().getUniformBlockIndex("MatrixBlock");
//                glUniformBlockBinding(mat->shader().getHandle(), block_index, MATRIX_BLOCK);
                
//                block_index = mat->shader().getUniformBlockIndex("ShadowBlock");
//                glUniformBlockBinding(mat->shader().getHandle(), block_index, SHADOW_BLOCK);
#else
                set_light_uniforms(mat, light_list);
#endif
            }
            gl::apply_material(m->material());
            
#ifndef KINSKI_NO_VAO
            m->bind_vertex_array();
#else
            m->bindVertexPointers();
#endif
            
            KINSKI_CHECK_GL_ERRORS();
            
            if(m->geometry()->hasIndices())
            {
#ifndef KINSKI_GLES
                if(!m->entries().empty())
                {
                    for (uint32_t i = 0; i < m->entries().size(); i++)
                    {
                        // skip disabled entries
                        if(!m->entries()[i].enabled) continue;
                        
                        uint32_t primitive_type = m->entries()[i].primitive_type;
                        primitive_type = primitive_type ? : m->geometry()->primitiveType();
                        
                        int mat_index = clamp<int>(m->entries()[i].material_index,
                                                   0,
                                                   m->materials().size() - 1);
                        m->bind_vertex_array(mat_index);
                        apply_material(m->materials()[mat_index]);
                        KINSKI_CHECK_GL_ERRORS();
                        
                        glDrawElementsBaseVertex(primitive_type,
                                                 m->entries()[i].num_indices,
                                                 m->geometry()->indexType(),
                                                 BUFFER_OFFSET(m->entries()[i].base_index
                                                               * sizeof(m->geometry()->indexType())),
                                                 m->entries()[i].base_vertex);
                    }
                }
                else
#endif
                {
                    glDrawElements(m->geometry()->primitiveType(),
                                   m->geometry()->indices().size(), m->geometry()->indexType(),
                                   BUFFER_OFFSET(0));
                }
            }
            else
            {
                glDrawArrays(m->geometry()->primitiveType(), 0,
                             m->geometry()->vertices().size());
            }
            KINSKI_CHECK_GL_ERRORS();
        }
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(0);
#endif
    }
    
    void SceneRenderer::set_light_uniforms(MaterialPtr &the_mat,
                                           const list<RenderBin::light> &light_list)
    {
        int light_count = 0;
        
        for (const auto &light : light_list)
        {
            std::string light_str = std::string("u_lights") + "[" + to_string(light_count) + "]";
            
            the_mat->uniform(light_str + ".type", (int)light.light->type());
            the_mat->uniform(light_str + ".position", light.transform[3].xyz());
            the_mat->uniform(light_str + ".diffuse", light.light->diffuse());
            the_mat->uniform(light_str + ".ambient", light.light->ambient());
            the_mat->uniform(light_str + ".specular", light.light->specular());
            
            // point + spot
            if(light.light->type() > 0)
            {
                the_mat->uniform(light_str + ".constantAttenuation", light.light->attenuation().constant);
                the_mat->uniform(light_str + ".linearAttenuation", light.light->attenuation().linear);
                the_mat->uniform(light_str + ".quadraticAttenuation", light.light->attenuation().quadratic);
                
                if(light.light->type() == Light::SPOT)
                {
                    the_mat->uniform(light_str + ".spotDirection", glm::normalize(-light.transform[2].xyz()));
                    the_mat->uniform(light_str + ".spotCutoff", light.light->spot_cutoff());
                    the_mat->uniform(light_str + ".spotCosCutoff", cosf(glm::radians(light.light->spot_cutoff())));
                    the_mat->uniform(light_str + ".spotExponent", light.light->spot_exponent());
                }
            }
            light_count++;
        }
        the_mat->uniform("u_numLights", light_count);
    }
    
    void SceneRenderer::update_uniform_buffers(const std::list<RenderBin::light> &light_list)
    {
#ifndef KINSKI_GLES
        struct lightstruct_std140
        {
            vec3 position;
            int type;
            vec4 diffuse;
            vec4 ambient;
            vec4 specular;
            vec3 spotDirection;
            float spotCosCutoff;
            float spotExponent;
            float constantAttenuation;
            float linearAttenuation;
            float quadraticAttenuation;
        };
        
        if(!m_uniform_buffer[LIGHT_UNIFORM_BUFFER])
        {
            m_uniform_buffer[LIGHT_UNIFORM_BUFFER] = gl::Buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
        }
        
        std::vector<lightstruct_std140> light_structs;
        
        // update light uniform buffer
        for(const RenderBin::light &l : light_list)
        {
            lightstruct_std140 buf;
            buf.type = (int)l.light->type();
            buf.position = l.transform[3].xyz();
            buf.diffuse = l.light->diffuse();
            buf.ambient = l.light->ambient();
            buf.specular = l.light->specular();
            buf.constantAttenuation = l.light->attenuation().constant;
            buf.linearAttenuation = l.light->attenuation().linear;
            buf.quadraticAttenuation = l.light->attenuation().quadratic;
            buf.spotDirection = glm::normalize(-vec3(l.transform[2].xyz()));
            buf.spotCosCutoff = cosf(glm::radians(l.light->spot_cutoff()));
            buf.spotExponent = l.light->spot_exponent();
            light_structs.push_back(buf);
        }
        int num_lights = light_list.size();
        int num_bytes = sizeof(lightstruct_std140) * light_structs.size() + 16;
        uint8_t buf[num_bytes];
        memcpy(buf, &num_lights, 4);
        memcpy(buf + 16, &light_structs[0], sizeof(lightstruct_std140) * light_structs.size());
        m_uniform_buffer[LIGHT_UNIFORM_BUFFER].set_data(buf, num_bytes);
        
        glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK, m_uniform_buffer[LIGHT_UNIFORM_BUFFER].id());
#endif
    }
    
    void SceneRenderer::update_uniform_buffer_matrices(const glm::mat4 &model_view,
                                                       const glm::mat4 &projection)
    {
//#ifndef KINSKI_GLES
//        
//        struct matrixstruct_std140
//        {
//            mat4 modelViewMatrix;
//            mat4 modelViewProjectionMatrix;
//            mat3 normalMatrix;
//        };
//        
//        if(!m_uniform_buffer[MATRIX_UNIFORM_BUFFER])
//        {
//            m_uniform_buffer[MATRIX_UNIFORM_BUFFER] = gl::Buffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
//        }
//        
//        matrixstruct_std140 matrix_struct;
//        matrix_struct.modelViewMatrix = model_view;
//        matrix_struct.modelViewProjectionMatrix = projection * model_view;
//        matrix_struct.normalMatrix = glm::inverseTranspose(glm::mat3(model_view));
//        m_uniform_buffer[MATRIX_UNIFORM_BUFFER].setData(&matrix_struct, sizeof(matrix_struct));
//        
//        glBindBufferBase(GL_UNIFORM_BUFFER, MATRIX_BLOCK, m_uniform_buffer[MATRIX_UNIFORM_BUFFER].id());
//#endif
    }
    
    void SceneRenderer::set_shadowmap_size(const glm::vec2 &the_size)
    {
#ifndef KINSKI_GLES
        gl::Fbo::Format fmt;
        fmt.setNumColorBuffers(0);
        
        for(size_t i = 0; i < m_shadow_fbos.size(); i++)
        {
            if(!m_shadow_fbos[i] || m_shadow_fbos[i].getSize() != the_size)
            {
                m_shadow_fbos[i] = gl::Fbo(the_size.x, the_size.y, fmt);
            }
        }
#endif
    }
    
    void SceneRenderer::update_uniform_buffer_shadows(const glm::mat4 &the_transform)
    {
#ifndef KINSKI_GLES

#endif
    }
}}