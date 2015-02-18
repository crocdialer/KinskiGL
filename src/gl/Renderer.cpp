//
//  Renderer.cpp
//  gl
//
//  Created by Fabian on 4/21/13.
//
//

#include "Renderer.h"
#include "Mesh.h"
#include "Camera.h"
#include "Light.h"

namespace kinski{ namespace gl{

    using std::pair;
    using std::map;
    using std::list;
    
    using namespace glm;
    
    struct lightstruct_std140
    {
        vec4 position;//pad
        vec4 diffuse;
        vec4 ambient;
        vec4 specular;
        vec4 spotDirection;//pad
        float spotCosCutoff;
        float spotExponent;
        float constantAttenuation;
        float linearAttenuation;
        float quadraticAttenuation;
        int type;
        uint32_t pad[2];//pad
    };
    
    Renderer::Renderer()
    {
        
    }
    
    void Renderer::render(const RenderBinPtr &theBin)
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
        
        // update uniform buffers (only lights at the moment)
        update_uniform_buffers(theBin->lights);
        
        // draw our stuff
        draw_sorted_by_material(theBin->camera, opaque_items, theBin->lights);
        draw_sorted_by_material(theBin->camera, blended_items, theBin->lights);
    }
    
    void Renderer::draw_sorted_by_material(const CameraPtr &cam, const list<RenderBin::item> &item_list,
                                           const list<RenderBin::light> &light_list)
    {
        KINSKI_CHECK_GL_ERRORS();
        typedef map<pair<Material*, Geometry*>, list<RenderBin::item> > MatMeshMap;
        MatMeshMap mat_mesh_map;
        
        for (const RenderBin::item &item : item_list)
        {
            Mesh *m = item.mesh;
            
            const glm::mat4 &modelView = item.transform;
            
            for(auto &mat : m->materials())
            {
                mat->uniform("u_modelViewMatrix", modelView);
                mat->uniform("u_modelViewProjectionMatrix",
                             cam->getProjectionMatrix() * modelView);
                
                //if(m->geometry()->hasNormals())
                {
                    mat->uniform("u_normalMatrix",
                                 glm::inverseTranspose(glm::mat3(modelView)));
                }
                
                if(m->geometry()->hasBones())
                {
                    mat->uniform("u_bones", m->boneMatrices());
                }
                
                // lighting parameters
//                set_light_uniforms(mat, light_list);
                GLuint block_index = mat->shader().getUniformBlockIndex("LightBlock");
                glUniformBlockBinding(mat->shader().getHandle(), block_index, LIGHT_BLOCK);
                
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
                    for (int i = 0; i < m->entries().size(); i++)
                    {
                        // skip disabled entries
                        if(!m->entries()[i].enabled) continue;
                        
                        int mat_index = clamp<int>(m->entries()[i].material_index,
                                                   0,
                                                   m->materials().size() - 1);
                        m->bind_vertex_array(mat_index);
                        apply_material(m->materials()[mat_index]);
                        
//                            GLint current_vao, current_prog;
//                            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
//                            glGetIntegerv(GL_CURRENT_PROGRAM, &current_prog);
//                            std::cout << "vao: " << current_vao << " -- prog: " << current_prog << std::endl;
                        KINSKI_CHECK_GL_ERRORS();
                        
                        glDrawElementsBaseVertex(m->geometry()->primitiveType(),
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
    
    void Renderer::set_light_uniforms(MaterialPtr &the_mat, const list<RenderBin::light> &light_list)
    {
        int light_count = 0;
        
        for (const auto &light : light_list)
        {
            std::string light_str = std::string("u_lights") + "[" + as_string(light_count) + "]";
            
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
    
    void Renderer::update_uniform_buffers(const std::list<RenderBin::light> &light_list)
    {
#ifndef KINSKI_GLES
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
            buf.position = l.transform[3];
            buf.diffuse = l.light->diffuse();
            buf.ambient = l.light->ambient();
            buf.specular = l.light->specular();
            buf.constantAttenuation = l.light->attenuation().constant;
            buf.linearAttenuation = l.light->attenuation().linear;
            buf.quadraticAttenuation = l.light->attenuation().quadratic;
            buf.spotDirection = vec4(glm::normalize(-vec3(l.transform[2].xyz())), 0.f);
            buf.spotCosCutoff = cosf(glm::radians(l.light->spot_cutoff()));
            buf.spotExponent = l.light->spot_exponent();
            light_structs.push_back(buf);
        }
        int num_lights = light_list.size();
        int num_bytes = sizeof(lightstruct_std140) * light_structs.size() + 16;
        m_uniform_buffer[LIGHT_UNIFORM_BUFFER].setData(nullptr, num_bytes);
        uint8_t *ptr = m_uniform_buffer[LIGHT_UNIFORM_BUFFER].map();
        memcpy(ptr, &num_lights, 4);
        memcpy(ptr + 16, &light_structs[0], sizeof(lightstruct_std140) * light_structs.size());
        m_uniform_buffer[LIGHT_UNIFORM_BUFFER].unmap();
        
        glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK, m_uniform_buffer[LIGHT_UNIFORM_BUFFER].id());
#endif
    }
}}