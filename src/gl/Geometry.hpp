 // __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "gl/gl.hpp"
#include "geometry_types.hpp"
#include "Buffer.hpp"

namespace kinski{ namespace gl{

struct Face3
{
    Face3():a(0), b(0), c(0){};
    Face3(index_t theA, index_t theB, index_t theC):
    a(theA), b(theB), c(theC){}
    
    // vertex indices
    union
    {
        struct{index_t a, b, c;};
        index_t indices[3];
    };
};

// each vertex can reference up to 4 bones
struct BoneVertexData
{
    ivec4 indices;
    vec4 weights;
    
    BoneVertexData():
    indices(ivec4(0)),
    weights(vec4(0)){};
};

struct HalfEdge
{
    //! Vertex index at the end of this half-edge
    index_t index;
    
    //! Oppositely oriented adjacent half-edge
    HalfEdge* twin = nullptr;
    
    //! Next half-edge around the face
    HalfEdge* next = nullptr;
};

std::vector<HalfEdge> compute_half_edges(gl::GeometryPtr the_geom);

class Geometry
{
public:
    
    enum BufferBit
    {
        VERTEX_BIT = 1UL << 0, NORMAL_BIT = 1UL << 1, TANGENT_BIT = 1UL << 2,
        POINTSIZE_BIT = 1UL << 3, TEXCOORD_BIT = 1UL << 4, COLOR_BIT = 1UL << 5,
        BONE_INDEX_BIT = 1UL << 6, BONE_WEIGHT_BIT = 1UL << 7, INDEX_BIT = 1UL << 8,
        CUSTOM_BIT = 1UL << 9
    };
    
    ~Geometry();
    
    inline void append_vertex(const vec3 &theVert)
    { vertices().push_back(theVert); };
    
    inline void append_vertices(const std::vector<vec3> &theVerts)
    {
        vertices().insert(m_vertices.end(), theVerts.begin(), theVerts.end());
    }
    inline void append_vertices(const vec3 *theVerts, size_t numVerts)
    {
        vertices().insert(m_vertices.end(), theVerts, theVerts + numVerts);
    }
    
    inline void append_normal(const vec3 &theNormal)
    { normals().push_back(theNormal); };
    
    inline void append_normals(const std::vector<vec3> &theNormals)
    {
        normals().insert(m_normals.end(), theNormals.begin(), theNormals.end());
    }
    inline void append_normals(const vec3 *theNormals, size_t numNormals)
    {
        normals().insert(m_normals.end(), theNormals, theNormals + numNormals);
    }
    
    inline void append_tex_coord(float theU, float theV)
    { tex_coords().push_back(vec2(theU, theV)); };
    
    inline void append_tex_coord(const vec2 &theUV)
    { tex_coords().push_back(theUV); };
    
    inline void append_tex_coords(const std::vector<vec2> &theVerts)
    {
        tex_coords().insert(m_tex_coords.end(), theVerts.begin(), theVerts.end());
    }
    inline void append_texcoords(const vec2 *theVerts, size_t numVerts)
    {
        tex_coords().insert(m_tex_coords.end(), theVerts, theVerts + numVerts);
    }
    
    inline void append_color(const vec4 &theColor)
    { colors().push_back(theColor); };
    
    inline void append_colors(const std::vector<vec4> &theColors)
    {
        colors().insert(m_colors.end(), theColors.begin(), theColors.end());
    }
    
    inline void append_colors(const vec4 *theColors, size_t numColors)
    {
        colors().insert(m_colors.end(), theColors, theColors + numColors);
    }
    
    inline void append_index(index_t theIndex)
    { indices().push_back(theIndex); };
    
    inline void append_indices(const std::vector<index_t> &theIndices)
    {
        indices().insert(m_indices.end(), theIndices.begin(), theIndices.end());
    }
    inline void append_indices(const index_t *theIndices, size_t numIndices)
    {
        indices().insert(m_indices.end(), theIndices, theIndices + numIndices);
    }
    
    inline void append_face(index_t a, index_t b, index_t c){ append_face(Face3(a, b, c)); }
    inline void append_face(const Face3 &theFace)
    {
        m_faces.push_back(theFace);
        append_indices(theFace.indices, 3);
    }
    inline void append_faces(const std::vector<gl::Face3> &the_faces)
    {
        m_faces.insert(m_faces.end(), the_faces.begin(), the_faces.end());
        index_t *start = (index_t*) the_faces.data();
        m_indices.insert(m_indices.end(), start, start + 3 * the_faces.size());
    }
    
    void compute_aabb();
    void compute_face_normals();
    void compute_vertex_normals();
    void compute_tangents();
    
    GLenum index_type();
    inline size_t index_size() const { return sizeof(index_t); };
    inline GLenum primitive_type() const {return m_primitive_type;};
    void set_primitive_type(GLenum type){ m_primitive_type = type; };
    
    bool has_vertices() const { return !m_vertices.empty(); };
    inline std::vector<vec3>& vertices(){ set_flag(VERTEX_BIT); return m_vertices;};
    inline const std::vector<vec3>& vertices() const { return m_vertices; };
    
    bool has_normals() const { return !m_vertices.empty() && m_vertices.size() == m_normals.size(); };
    inline std::vector<vec3>& normals(){ set_flag(NORMAL_BIT); return m_normals; };
    inline const std::vector<vec3>& normals() const { return m_normals; };
    
    bool has_tangents() const { return !m_vertices.empty() && m_vertices.size() == m_tangents.size(); };
    inline std::vector<vec3>& tangents(){ set_flag(TANGENT_BIT); return m_tangents; };
    inline const std::vector<vec3>& tangents() const { return m_tangents; };
    
    bool has_point_sizes() const { return !m_vertices.empty() && m_vertices.size() == m_point_sizes.size(); };
    inline std::vector<float>& point_sizes(){ set_flag(POINTSIZE_BIT); return m_point_sizes; };
    inline const std::vector<float>& point_sizes() const { return m_point_sizes; };
    
    bool has_tex_coords() const { return !m_vertices.empty() && m_vertices.size() == m_tex_coords.size(); };
    inline std::vector<vec2>& tex_coords(){ set_flag(TEXCOORD_BIT); return m_tex_coords; };
    inline const std::vector<vec2>& tex_coords() const { return m_tex_coords; };
    
    bool has_colors() const { return !m_vertices.empty() && m_vertices.size() == m_colors.size(); };
    std::vector<vec4>& colors(){ set_flag(COLOR_BIT); return m_colors; };
    const std::vector<vec4>& colors() const { return m_colors; };
    
    bool has_indices() const { return !m_indices.empty(); };
    std::vector<index_t>& indices(){ set_flag(INDEX_BIT); return m_indices; };
    const std::vector<index_t>& indices() const { return m_indices; };
    
    inline std::vector<Face3>& faces(){ return m_faces; };
    inline const std::vector<Face3>& faces() const { return m_faces; };
    
    bool has_bones() const { return !m_vertices.empty() && m_vertices.size() == m_bone_vertex_data.size(); };
    
    std::vector<BoneVertexData>& bone_vertex_data()
    { set_flag(BONE_INDEX_BIT | BONE_WEIGHT_BIT); return m_bone_vertex_data; };
    
    const std::vector<BoneVertexData>& bone_vertex_data() const { return m_bone_vertex_data; };
    
    inline const AABB& aabb() const { return m_bounding_box; };
    
    inline void set_flag(uint32_t b){ m_dirty_bits |= b; }
    inline void remove_flag(uint32_t b){ m_dirty_bits &= ~b; }
    inline bool has_flag(uint32_t b){ return m_dirty_bits & b; }
    
    // GL buffers
    const gl::Buffer& vertex_buffer() const { return m_vertex_buffer; };
    const gl::Buffer& normal_buffer() const { return m_normal_buffer; };
    const gl::Buffer& tex_coord_buffer() const { return m_tex_coord_buffer; };
    const gl::Buffer& tangent_buffer() const { return m_tangent_buffer; };
    const gl::Buffer& point_size_buffer() const { return m_point_size_buffer; };
    const gl::Buffer& color_buffer() const { return m_color_buffer; };
    const gl::Buffer& bone_buffer() const { return m_bone_buffer; };
    const gl::Buffer& index_buffer() const { return m_index_buffer; };
    
    bool has_dirty_buffers() const;
    void create_gl_buffers(GLenum usage = GL_DONT_CARE);
    
    /********************************* Factory methods ****************************************/
    
    static GeometryPtr create(){return GeometryPtr(new Geometry());};
    static GeometryPtr create_grid(float width, float height,
                                   uint32_t numSegments_W = 20,
                                   uint32_t numSegments_H = 20);
    static GeometryPtr create_plane(float width, float height,
                                   uint32_t numSegments_W = 1,
                                   uint32_t numSegments_H = 1);
    static GeometryPtr create_solid_circle(int numSegments, float the_radius = 1.f);
    static GeometryPtr create_circle(int numSegments, float the_radius = 1.f);
    static GeometryPtr create_box(const vec3 &the_half_extents);
    static GeometryPtr create_box_lines(const glm::vec3 &the_half_extents = glm::vec3(.5f));
    static GeometryPtr create_sphere(float radius, int numSlices);
    static GeometryPtr create_cone(float radius, float height, int numSegments);
    
private:
    
    Geometry();
    
    // defaults to GL_TRIANGLES
    GLenum m_primitive_type;
    
    std::vector<vec3> m_vertices;
    std::vector<vec3> m_normals;
    std::vector<vec2> m_tex_coords;
    std::vector<vec4> m_colors;
    std::vector<vec3> m_tangents;
    std::vector<float> m_point_sizes;
    std::vector<index_t> m_indices;
    std::vector<Face3> m_faces;
    std::vector<BoneVertexData> m_bone_vertex_data;
    
    AABB m_bounding_box;
    gl::Buffer m_vertex_buffer;
    gl::Buffer m_normal_buffer;
    gl::Buffer m_tex_coord_buffer;
    gl::Buffer m_color_buffer;
    gl::Buffer m_tangent_buffer;
    gl::Buffer m_point_size_buffer;
    gl::Buffer m_index_buffer;
    gl::Buffer m_bone_buffer;
    
    // bitmask for dirty buffers
    uint32_t m_dirty_bits;
};

}//gl
}//kinski
