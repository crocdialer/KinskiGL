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

namespace kinski{ namespace gl{

    using std::pair;
    using std::map;
    using std::list;
    
    void Renderer::render(const RenderBinPtr &theBin)
    {
        std::list<RenderBin::item> opaque_items, blended_items;
        std::list<RenderBin::item>::iterator it = theBin->items.begin();
        for (; it != theBin->items.end(); ++it)
        {
            if(it->mesh->material()->opaque()){opaque_items.push_back(*it);}
            else{blended_items.push_back(*it);}
        }
        //sort by distance to camera
        opaque_items.sort(RenderBin::sort_items_increasing());
        blended_items.sort(RenderBin::sort_items_decreasing());
        
        draw_sorted_by_material(theBin->camera, opaque_items);
        draw_sorted_by_material(theBin->camera, blended_items);
    }
    
    void Renderer::draw_sorted_by_material(const CameraPtr &cam, const list<RenderBin::item> &item_list)
    {
        KINSKI_CHECK_GL_ERRORS();
        glm::mat4 viewMatrix = cam->getViewMatrix();
        
        map<pair<GeometryPtr, MaterialPtr>, list<MeshPtr> > meshMap;
        list<RenderBin::item>::const_iterator item_it = item_list.begin();
        for (; item_it != item_list.end(); ++item_it)
        {
            const MeshPtr &m = item_it->mesh;
            meshMap[std::make_pair(m->geometry(), m->material())].push_back(m);
        }
        map<pair<GeometryPtr, MaterialPtr>, list<MeshPtr> >::const_iterator it = meshMap.begin();
        for (; it != meshMap.end(); ++it)
        {
            const list<Mesh::Ptr>& meshList = it->second;
            MeshPtr m = meshList.front();
            
            if(m->geometry()->hasBones())
            {
                m->material()->uniform("u_bones", m->boneMatrices());
            }
            gl::apply_material(m->material(), true);
            
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
            list<Mesh::Ptr>::const_iterator transformIt = meshList.begin();
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
                            if(!m->materials()[i]->textures().empty())
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
}}