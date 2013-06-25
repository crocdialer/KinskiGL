//
//  Renderer.cpp
//  kinskiGL
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
        glm::mat4 viewMatrix = cam->getViewMatrix();
        
        typedef map<pair<GeometryPtr, MaterialPtr>, list<Mesh*> > MeshMap;
        MeshMap meshMap;
        for (auto &item : item_list)
        {
            Mesh *m = item.mesh;
            meshMap[std::make_pair(m->geometry(), m->material())].push_back(m);
        }
        MeshMap::iterator it = meshMap.begin();
        for (; it != meshMap.end(); ++it)
        {
            const list<Mesh*>& meshList = it->second;
            Mesh *m = meshList.front();
            
            if(m->geometry()->hasBones())
            {
                m->material()->uniform("u_bones", m->boneMatrices());
            }
            
            for (auto &material : m->materials())
            {
                set_light_uniforms(material, light_list);
            }
            gl::apply_material(m->material(), false);
            
#ifndef KINSKI_NO_VAO
            try{GL_SUFFIX(glBindVertexArray)(m->vertexArray());}
            catch(const WrongVertexArrayDefinedException &e)
            {
                m->createVertexArray();
                try{GL_SUFFIX(glBindVertexArray)(m->vertexArray());}
                catch(std::exception &e)
                {
                    // should not arrive here
                    LOG_ERROR<<e.what();
                    return;
                }
            }
#else
            m->bindVertexPointers();
#endif
            KINSKI_CHECK_GL_ERRORS();
            list<Mesh*>::const_iterator transformIt = meshList.begin();
            for (; transformIt != meshList.end(); ++transformIt)
            {
                m = *transformIt;
                
                glm::mat4 modelView = viewMatrix * m->transform();
                m->material()->shader().uniform("u_modelViewMatrix", modelView);
                m->material()->shader().uniform("u_normalMatrix",
                                                glm::inverseTranspose( glm::mat3(modelView) ));
                m->material()->shader().uniform("u_modelViewProjectionMatrix",
                                                cam->getProjectionMatrix() * modelView);
                
                if(m->geometry()->hasIndices())
                {
#ifndef KINSKI_GLES
                    if(!m->entries().empty())
                    {
                        for (int i = 0; i < m->entries().size(); i++)
                        {
                            apply_material(m->materials()[i]);
                            
                            glDrawElementsBaseVertex(m->geometry()->primitiveType(),
                                                     m->entries()[i].numdices,
                                                     m->geometry()->indexType(),
                                                     BUFFER_OFFSET(m->entries()[i].base_index * sizeof(m->geometry()->indexType())),
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
        char buf[256];
        for (const auto &light : light_list)
        {
            sprintf(buf, "u_lights[%d].type", light_count);
            the_mat->uniform(buf, (int)light.light->type());
            
            sprintf(buf, "u_lights[%d].position", light_count);
            the_mat->uniform(buf, light.transform[3].xyz());
            
            sprintf(buf, "u_lights[%d].diffuse", light_count);
            the_mat->uniform(buf, light.light->diffuse());
            
            sprintf(buf, "u_lights[%d].ambient", light_count);
            the_mat->uniform(buf, light.light->ambient());
            
            sprintf(buf, "u_lights[%d].specular", light_count);
            the_mat->uniform(buf, light.light->specular());
            
            sprintf(buf, "u_lights[%d].constantAttenuation", light_count);
            the_mat->uniform(buf, light.light->attenuation().constant);
            
            sprintf(buf, "u_lights[%d].linearAttenuation", light_count);
            the_mat->uniform(buf, light.light->attenuation().linear);
            
            sprintf(buf, "u_lights[%d].quadraticAttenuation", light_count);
            the_mat->uniform(buf, light.light->attenuation().quadratic);
            
            light_count++;
        }
        the_mat->uniform("u_numLights", light_count);
    }
}}