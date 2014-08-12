//
//  Renderer.cpp
//  gl
//
//  Created by Fabian on 4/21/13.
//
//

#include "Renderer.h"
#include "Mesh.h"
#include "Geometry.h"
#include "Material.h"
#include "Camera.h"
#include "Light.h"

namespace kinski{ namespace gl{

    using std::pair;
    using std::map;
    using std::list;
    
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
        
        draw_sorted_by_material(theBin->camera, opaque_items, theBin->lights);
        draw_sorted_by_material(theBin->camera, blended_items, theBin->lights);
    }
    
    void Renderer::draw_sorted_by_material(const CameraPtr &cam, const list<RenderBin::item> &item_list,
                                           const list<RenderBin::light> &light_list)
    {
        KINSKI_CHECK_GL_ERRORS();
        typedef map<pair<Material*, Geometry*>, list<RenderBin::item> > MatMeshMap;
        MatMeshMap mat_mesh_map;
//        for (auto &item : item_list)
//        {
//            mat_mesh_map[std::make_pair(item.mesh->material().get(),
//                                        item.mesh->geometry().get())].push_back(item);
//        }
//        for (auto &pair_item : mat_mesh_map)
        {
//            list<RenderBin::item>& sub_selection = pair_item.second;

            for (const RenderBin::item &item : item_list)
            {
                Mesh *m = item.mesh;
                
                const glm::mat4 &modelView = item.transform;
                
                for(auto &mat : m->materials())
                {
                    mat->uniform("u_modelViewMatrix", modelView);
                    mat->uniform("u_modelViewProjectionMatrix",
                                           cam->getProjectionMatrix() * modelView);
                    
//                    if(m->geometry()->hasNormals())
                    {
                        mat->uniform("u_normalMatrix",
                                     glm::inverseTranspose( glm::mat3(modelView) ));
                    }
                    
                    if(m->geometry()->hasBones())
                    {
                        mat->uniform("u_bones", m->boneMatrices());
                    }
                    
                    // lighting parameters
                    set_light_uniforms(mat, light_list);
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
                            
                            int mat_index = m->entries()[i].material_index;
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
}}