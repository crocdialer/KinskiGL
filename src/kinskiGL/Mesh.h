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
        typedef std::shared_ptr<const Mesh> ConstPtr;
        
        Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial);
        
        virtual ~Mesh();
        
        const Geometry::Ptr& geometry() const { return m_geometry; };
        Geometry::Ptr& geometry() { return m_geometry; };
        
        const Material::Ptr& material() const { return m_material; };
        Material::Ptr& material() { return m_material; };
        
        void bindVertexPointers() const;
        
#ifndef KINSKI_NO_VAO
        void createVertexArray();
#endif
        
        GLuint vertexArray() const { return m_vertexArray; };
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_vertex"
         */
        void setVertexLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_normal"
         */
        void setNormalLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_tangent"
         */
        void setTangentLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_texCoord"
         */
        void setTexCoordLocationName(const std::string &theName);
        
        /*!
         * Set the name under which the attribute will be accessible in the shader.
         * Defaults to "a_texCoord"
         */
        void setColorLocationName(const std::string &theName);
        
    private:
        
        Geometry::Ptr m_geometry;
        Material::Ptr m_material;
        
        GLuint m_vertexArray;
        
        /*!
         * choose one of GL_TRIANGLES, GL_POINTS here
         */
        GLuint m_drawMode;
        
        std::string m_vertexLocationName;
        std::string m_normalLocationName;
        std::string m_tangentLocationName;
        std::string m_texCoordLocationName;
        std::string m_colorLocationName;
        std::string m_boneIDsLocationName;
        std::string m_boneWeightsLocationName;
    };
}}

#endif /* defined(__kinskiGL__Mesh__) */
