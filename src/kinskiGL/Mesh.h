// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef KINSKI_GL_MESH_H_
#define KINSKI_GL_MESH_H_

#include "Object3D.h"
#include "Geometry.h"
#include "Material.h"

namespace kinski { namespace gl {
    
    class KINSKI_API Mesh : public Object3D
    {
    public:
        
        typedef std::shared_ptr<Mesh> Ptr;
        typedef std::shared_ptr<const Mesh> ConstPtr;
        
        static Ptr create(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial)
        {
            return Ptr(new Mesh(theGeom, theMaterial));
        }

        virtual ~Mesh();
        
        const Geometry::Ptr& geometry() const { return m_geometry; };
        Geometry::Ptr& geometry() { return m_geometry; };
        
        const Material::Ptr& material() const { return m_material; };
        Material::Ptr& material() { return m_material; };
        
        void bindVertexPointers() const;
        void createVertexArray();
        GLuint vertexArray() const;
        
        void update(float time_delta);
        AABB boundingBox() const;
        
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
        
        Mesh(const Geometry::Ptr &theGeom, const Material::Ptr &theMaterial);
        
        GeometryPtr m_geometry;
        MaterialPtr m_material;
        
        GLuint m_vertexArray;
        mutable std::pair<MaterialPtr, GLuint> m_material_vertex_array_mapping;
        
        std::string m_vertexLocationName;
        std::string m_normalLocationName;
        std::string m_tangentLocationName;
        std::string m_texCoordLocationName;
        std::string m_colorLocationName;
        std::string m_boneIDsLocationName;
        std::string m_boneWeightsLocationName;
    };
    
    class WrongVertexArrayDefinedException : public kinski::Exception
    {
    public:
        WrongVertexArrayDefinedException(uint32_t theID):
        Exception("wrong vertex array defined for object: " + kinski::as_string(theID)){}
    };
}}

#endif /* defined(__kinskiGL__Mesh__) */
