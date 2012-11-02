//
//  Mesh.h
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#ifndef __kinskiGL__Mesh__
#define __kinskiGL__Mesh__

#include "Object3D.h"
#include "Geometry.h"
#include "Material.h"

namespace kinski { namespace gl {
    
    class Mesh : public Object3D
    {
    public:
        
        typedef std::shared_ptr<Mesh> Ptr;
        
        Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial);
        
        const Geometry::Ptr& getGeometry() const { return m_geometry; };
        
        const Material::Ptr& getMaterial() const { return m_material; };
        
        GLuint getVertexArray(){ return m_vertexArray; };
        
    private:
        
        void createVertexArray();
        
        Geometry::Ptr m_geometry;
        Material::Ptr m_material;
        
        GLuint m_vertexArray;
    };
}}

#endif /* defined(__kinskiGL__Mesh__) */
