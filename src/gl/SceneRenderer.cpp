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

const char* SceneRenderer::TAG_NO_CULL = "no_cull";

class CullVisitor : public Visitor
{
public:
    CullVisitor(const CameraPtr &theCamera, const std::set<std::string> &the_tags):
    Visitor(),
    m_frustum(theCamera->frustum()),
    m_tags(the_tags),
    m_render_bin(new gl::RenderBin(theCamera))
    {
        transform_stack().push(theCamera->view_matrix());
    }
    
    RenderBinPtr get_render_bin() const {return m_render_bin;}
    
    void visit(Mesh &theNode) override
    {
        if(!theNode.enabled() || !check_tags(m_tags, theNode.tags())) return;
        
        glm::mat4 model_view = transform_stack().top() * theNode.transform();
        gl::AABB boundingBox = theNode.aabb();

        if(m_frustum.intersect(boundingBox) || kinski::contains(theNode.tags(), gl::SceneRenderer::TAG_NO_CULL))
        {
            RenderBin::item item;
            item.mesh = std::dynamic_pointer_cast<gl::Mesh>(theNode.shared_from_this());
            item.transform = model_view;
            m_render_bin->items.push_back(item);
        }
        // super class provides node traversing and transform accumulation
        Visitor::visit(static_cast<gl::Object3D&>(theNode));
    }
    
    void visit(Light &theNode) override
    {
        if(theNode.enabled() && check_tags(m_tags, theNode.tags()))
        {
            RenderBin::light light_item;
            light_item.light = std::dynamic_pointer_cast<gl::Light>(theNode.shared_from_this());
            light_item.transform = transform_stack().top() * theNode.transform();
            
            // collect only lights that actually affect the scene
            gl::Sphere bounding_sphere(gl::vec3(theNode.transform()[3].xyz()), theNode.radius());

            if(m_frustum.intersect(bounding_sphere))
            {
                m_render_bin->lights.push_back(light_item);
            }
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

void sort_render_bin(const RenderBinPtr &the_bin,
                     std::list<RenderBin::item> &the_opaque_items,
                     std::list<RenderBin::item> &the_transparent_items)
{
    for (auto &item :the_bin->items)
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
        if(opaque){the_opaque_items.push_back(item);}
        else{the_transparent_items.push_back(item);}
    }
    
    //sort opaque items near to far
    the_opaque_items.sort([](const RenderBin::item &lhs, const RenderBin::item &rhs) -> bool
    {
        return  std::make_tuple(lhs.transform[3].z, lhs.mesh->material().get()) >
                std::make_tuple(lhs.transform[3].z, rhs.mesh->material().get());
    });
    //sort transparent items far to near
    the_transparent_items.sort([](const RenderBin::item &lhs, const RenderBin::item &rhs) -> bool
    {
        return  std::make_tuple(lhs.mesh->material().get(), lhs.transform[3].z) <
                std::make_tuple(rhs.mesh->material().get(), rhs.transform[3].z);
    });
}

SceneRendererPtr SceneRenderer::create()
{
    return SceneRendererPtr(new SceneRenderer());
}

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
    
    float extents = 2.f * glm::length(the_scene->root()->aabb().halfExtents());
    
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
            shadow_cams()[i] = gl::create_shadow_camera(l, min(extents, l->radius()));
            
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
        gl::ScopedMatrixPush mv(gl::MODEL_VIEW_MATRIX), proj(gl::PROJECTION_MATRIX);
        gl::set_projection(the_cam);
        mat4 m = the_cam->view_matrix();
        m[3] = vec4(0, 0, 0, 1);
        gl::load_matrix(gl::MODEL_VIEW_MATRIX, glm::scale(m, gl::vec3(the_cam->far() * .99f)));
        gl::draw_mesh(the_scene->skybox());
    }
    
    // forward render pass
    auto render_bin = cull(the_scene, the_cam, the_tags);

    // issue draw commands
    render(render_bin);
    
    // return number of rendered objects
    return render_bin->items.size();
}

RenderBinPtr cull(const gl::SceneConstPtr &the_scene,
                  const CameraPtr &theCamera,
                  const std::set<std::string> &the_tags)
{
    CullVisitor cull_visitor(theCamera, the_tags);
    the_scene->root()->accept(cull_visitor);
    auto bin = cull_visitor.get_render_bin();
    bin->scene = the_scene;
    return bin;
}

void SceneRenderer::render(const RenderBinPtr &theBin)
{
    std::list<RenderBin::item> opaque_items, blended_items;
    sort_render_bin(theBin, opaque_items, blended_items);
    m_num_shadow_lights = 0;
    
    for(const RenderBin::light &l : theBin->lights)
    {
        if(l.light->cast_shadow()){ m_num_shadow_lights++; }
    }
    
    // update uniform buffers (global light settings)
    update_uniform_buffers(theBin->lights);

    // make sure we start with a known state
    gl::reset_state();

    // draw our stuff
    draw_sorted_by_material(theBin->camera, opaque_items, theBin->lights);
    draw_sorted_by_material(theBin->camera, blended_items, theBin->lights);
}

void SceneRenderer::draw_sorted_by_material(const CameraPtr &cam, const list<RenderBin::item> &item_list,
                                            const list<RenderBin::light> &light_list)
{
    KINSKI_CHECK_GL_ERRORS();
    
    for (const RenderBin::item &item : item_list)
    {
        auto mesh = item.mesh;

        matrix_struct_140_t m;
        m.model_view = item.transform;
        m.model_view_projection = cam->projection_matrix() * item.transform;
        m.normal_matrix = mat4(glm::inverseTranspose(glm::mat3(item.transform)));
        m.texture_matrix = mesh->material()->texture_matrix();

#if !defined(KINSKI_GLES)
        if(!m_uniform_buffer[MATRIX_UNIFORM_BUFFER])
        {
            m_uniform_buffer[MATRIX_UNIFORM_BUFFER] = gl::Buffer(GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
        }
        m_uniform_buffer[MATRIX_UNIFORM_BUFFER].set_data(&m, sizeof(matrix_struct_140_t));
        glBindBufferBase(GL_UNIFORM_BUFFER, gl::Context::MATRIX_BLOCK, m_uniform_buffer[MATRIX_UNIFORM_BUFFER].id());
#endif
        for(auto &mat : mesh->materials())
        {
            if(!m_shadow_pass && m_num_shadow_lights)
            {
                std::vector<glm::mat4> shadow_matrices;
                char buf[32];
                for(int i = 0; i < m_num_shadow_lights; i++)
                {
                    if(!m_shadow_cams[i]) break;
                    int tex_unit = mat->textures().size() + i;
                    shadow_matrices.push_back(m_shadow_cams[i]->projection_matrix() *
                                              m_shadow_cams[i]->view_matrix() * mesh->global_transform());
                    m_shadow_fbos[i]->depth_texture().bind(tex_unit);
                    sprintf(buf, "u_shadow_map[%d]", i);
                    mat->uniform(buf, tex_unit);
                }
                mat->uniform("u_shadow_matrices", shadow_matrices);
                mat->uniform("u_shadow_map_size", m_shadow_fbos[0]->size());
                //                    mat->uniform("u_poisson_radius", 3.f);
            }

            if(mesh->geometry()->has_bones()){ mat->uniform("u_bones", mesh->bone_matrices()); }

            // lighting parameters
#if !defined(KINSKI_GLES)
            mat->shader()->uniform_block_binding("MatrixBlock", gl::Context::MATRIX_BLOCK);
            mat->shader()->uniform_block_binding("LightBlock", gl::Context::LIGHT_BLOCK);
#else
            set_light_uniforms(mat, light_list);
            mat->uniform("u_modelViewMatrix", m.model_view);
            mat->uniform("u_modelViewProjectionMatrix", m.model_view_projection);
            mat->uniform("u_normalMatrix", mat3(m.normal_matrix));
            mat->uniform("u_textureMatrix", m.texture_matrix);
#endif
        }

//        if(m->geometry()->has_dirty_buffers()){ m->geometry()->create_gl_buffers(); }
        gl::apply_material(mesh->material());
        mesh->bind_vertex_array();

        KINSKI_CHECK_GL_ERRORS();
        
        if(mesh->geometry()->has_indices())
        {
            if(!mesh->entries().empty())
            {
                std::list<uint32_t> mat_entries[mesh->materials().size()];
                
                for (uint32_t i = 0; i < mesh->entries().size(); ++i)
                {
                    uint32_t mat_index = mesh->entries()[i].material_index;
                    
                    if(mat_index < mesh->materials().size())
                        mat_entries[mat_index].push_back(i);
                }
                
                for (uint32_t i = 0; i < mesh->materials().size(); ++i)
                {
                    const auto& entry_list = mat_entries[i];
                    
                    if(!entry_list.empty())
                    {
                        mesh->bind_vertex_array(i);
                        apply_material(mesh->materials()[i]);
                    }
                    
                    for (auto entry_index : entry_list)
                    {
                        const gl::Mesh::Entry &e = mesh->entries()[entry_index];
                        
                        // skip disabled entries
                        if(!e.enabled) continue;
                        
                        uint32_t primitive_type = e.primitive_type;
                        primitive_type = primitive_type ? : mesh->geometry()->primitive_type();

#ifndef KINSKI_GLES
                        glDrawElementsBaseVertex(primitive_type,
                                                 e.num_indices,
                                                 mesh->geometry()->index_type(),
                                                 BUFFER_OFFSET(e.base_index * mesh->geometry()->index_size()),
                                                 e.base_vertex);
#else
                        glDrawElements(primitive_type,
                                       e.num_indices, mesh->geometry()->index_type(),
                                       BUFFER_OFFSET(e.base_index * mesh->geometry()->index_size()));
#endif
                    }
                }
                
            }
            else
            {
                glDrawElements(mesh->geometry()->primitive_type(),
                               mesh->geometry()->indices().size(), mesh->geometry()->index_type(),
                               BUFFER_OFFSET(0));
            }
        }
        else
        {
            glDrawArrays(mesh->geometry()->primitive_type(), 0,
                         mesh->geometry()->vertices().size());
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
    
    for (const auto &l : light_list)
    {
        std::string light_str = std::string("u_lights") + "[" + to_string(light_count) + "]";
        
        the_mat->uniform(light_str + ".type", (int)l.light->type());
        the_mat->uniform(light_str + ".position", l.transform[3].xyz());
        the_mat->uniform(light_str + ".direction", glm::normalize(-vec3(l.transform[2].xyz())));
        the_mat->uniform(light_str + ".diffuse", l.light->diffuse());
        the_mat->uniform(light_str + ".ambient", l.light->ambient());
        the_mat->uniform(light_str + ".intensity", l.light->intensity());
        
        // point + spot
        if(l.light->type() > 0)
        {
            the_mat->uniform(light_str + ".radius", l.light->radius());
            the_mat->uniform(light_str + ".quadraticAttenuation", l.light->attenuation().quadratic);
            
            if(l.light->type() == Light::SPOT)
            {
                the_mat->uniform(light_str + ".spotCosCutoff", cosf(glm::radians(l.light->spot_cutoff())));
                the_mat->uniform(light_str + ".spotExponent", l.light->spot_exponent());
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
        vec3 direction;
        float intensity;
        float radius;
        float spotCosCutoff;
        float spotExponent;
        float quadraticAttenuation;
//        int pad[1];
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
        buf.direction = glm::normalize(-vec3(l.transform[2].xyz()));
        buf.diffuse = l.light->diffuse();
        buf.ambient = l.light->ambient();
        buf.intensity = l.light->intensity();
        buf.radius = l.light->radius();
        buf.quadraticAttenuation = l.light->attenuation().quadratic;
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
    glBindBufferBase(GL_UNIFORM_BUFFER, gl::Context::LIGHT_BLOCK, m_uniform_buffer[LIGHT_UNIFORM_BUFFER].id());
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

void SceneRenderer::set_shadowmap_size(const glm::ivec2 &the_size)
{
#ifndef KINSKI_GLES
    gl::Fbo::Format fmt;
    fmt.num_color_buffers = 0;
    
    for(size_t i = 0; i < m_shadow_fbos.size(); i++)
    {
        if(!m_shadow_fbos[i] || m_shadow_fbos[i]->size() != the_size)
        {
            m_shadow_fbos[i] = gl::Fbo::create(the_size.x, the_size.y, fmt);
        }
    }
#endif
}

}}
