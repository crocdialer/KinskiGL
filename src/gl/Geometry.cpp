// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "Geometry.hpp"

using namespace std;

namespace kinski{ namespace gl{

namespace
{
    inline uint64_t pack(uint64_t a, uint64_t b){ return (a << 32) | b; }
    inline uint64_t swizzle(uint64_t a){ return ((a & 0xFFFFFFFF) << 32) | (a >> 32); }
}

std::vector<HalfEdge> compute_half_edges(gl::GeometryPtr the_geom)
{
//    Stopwatch timer;
//    timer.start();
    
    std::vector<HalfEdge> ret(3 * the_geom->faces().size());
    std::unordered_map<uint64_t, HalfEdge*> edge_table;
    
    HalfEdge *edge = ret.data();
    
    for(const Face3& face : the_geom->faces())
    {
        // create the half-edge that goes from C to A:
        edge_table[pack(face.c, face.a)] = edge;
        edge->index = face.a;
        edge->next = edge + 1;
        ++edge;
        
        // create the half-edge that goes from A to B:
        edge_table[pack(face.a, face.b)] = edge;
        edge->index = face.b;
        edge->next = edge + 1;
        ++edge;
        
        // create the half-edge that goes from B to C:
        edge_table[pack(face.b, face.c)] = edge;
        edge->index = face.c;
        edge->next = edge - 2;
        ++edge;
    }
    
    // populate the twin pointers by iterating over the edge_table
    int boundaryCount = 0;
    
    for(auto &e : edge_table)
    {
        HalfEdge *current_edge = e.second;
        
        // try to find twin edge in map
        auto it = edge_table.find(swizzle(e.first));
        
        if(it != edge_table.end())
        {
            HalfEdge *twin_edge = it->second;
            twin_edge->twin = current_edge;
            current_edge->twin = twin_edge;
        }
        else{ ++boundaryCount; }
    }
    
    // Now that we have a half-edge structure, it's easy to create adjacency info:
    if(boundaryCount > 0)
    {
        LOG_DEBUG << "mesh is not watertight. contains " << boundaryCount << " boundary edges.";
    }
//    LOG_DEBUG << "half-edge computation took " << timer.time_elapsed() * 1000.0 << " ms";
    return ret;
}
    
Geometry::Geometry():
m_primitive_type(GL_TRIANGLES),
m_dirty_vertex_buffer(true),
m_dirty_normal_buffer(true),
m_dirty_tex_coord_buffer(true),
m_dirty_color_buffer(true),
m_dirty_tangent_buffer(true),
m_dirty_point_size_buffer(true),
m_dirty_index_buffer(true),
m_dirty_bone_buffer(true)
{

}

Geometry::~Geometry()
{

}

void Geometry::compute_bounding_box()
{
    m_bounding_box = gl::calculate_AABB(m_vertices);
}

void Geometry::compute_face_normals()
{
    m_normals.resize(m_vertices.size());
    for(const Face3& face : m_faces)
    {
        const glm::vec3 &vA = m_vertices[face.a];
        const glm::vec3 &vB = m_vertices[face.b];
        const glm::vec3 &vC = m_vertices[face.c];
        glm::vec3 normal = glm::normalize(glm::cross(vB - vA, vC - vA));
        m_normals[face.a] = m_normals[face.b] = m_normals[face.c] = normal;
    }
}

void Geometry::compute_vertex_normals()
{
    //mark gpu buffer as dirty
    m_dirty_normal_buffer = true;

    if(m_faces.empty()) return;

    // create tmp array, if not yet constructed
    if(m_normals.size() != m_vertices.size())
    {
        m_normals.clear();
        m_normals.resize(m_vertices.size(), glm::vec3(0));
    }
    else
    {
        std::fill(m_normals.begin(), m_normals.end(), glm::vec3(0));
    }

    // iterate faces and sum normals for all vertices
    for (const Face3 &face : m_faces)
    {
        const glm::vec3 &vA = m_vertices[face.a];
        const glm::vec3 &vB = m_vertices[face.b];
        const glm::vec3 &vC = m_vertices[face.c];
        glm::vec3 normal = glm::normalize(glm::cross(vB - vA, vC - vA));
        m_normals[face.a] += normal;
        m_normals[face.b] += normal;
        m_normals[face.c] += normal;
    }

    // normalize vertexNormals
    vector<glm::vec3>::iterator normIt = m_normals.begin();
    for (; normIt != m_normals.end(); normIt++)
    {
        glm::vec3 &vertNormal = *normIt;
        vertNormal = glm::normalize(vertNormal);
    }
}

void Geometry::compute_tangents()
{
    if(m_faces.empty()) return;
    if(m_tex_coords.size() != m_vertices.size()) return;

    m_dirty_tangent_buffer = true;
    vector<glm::vec3> tangents;
    if(m_tangents.size() != m_vertices.size())
    {
        m_tangents.clear();
        m_tangents.resize(m_vertices.size(), glm::vec3(0));
        tangents.resize(m_vertices.size(), glm::vec3(0));
    }

    vector<Face3>::iterator faceIt = m_faces.begin();
    for (; faceIt != m_faces.end(); faceIt++)
    {
        Face3 &face = *faceIt;
        const glm::vec3 &v1 = m_vertices[face.a], &v2 = m_vertices[face.b], &v3 = m_vertices[face.c];
        const glm::vec2 &w1 = m_tex_coords[face.a], &w2 = m_tex_coords[face.b], &w3 = m_tex_coords[face.c];

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;
        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float r = 1.0F / (s1 * t2 - s2 * t1);
        glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
                      (t2 * z1 - t1 * z2) * r);
        glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
                      (s1 * z2 - s2 * z1) * r);

        tangents[face.a] += sdir;
        tangents[face.b] += sdir;
        tangents[face.c] += sdir;
    }

    for (uint32_t a = 0; a < m_vertices.size(); a++)
    {
        const glm::vec3& n = m_normals[a];
        const glm::vec3& t = tangents[a];

        // Gram-Schmidt orthogonalize
        m_tangents[a] = glm::normalize(t - n * glm::dot(n, t));

        // Calculate handedness
        //tangent[a].w = (Dot(Cross(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
    }
}

bool Geometry::has_dirty_buffers() const
{
    return m_dirty_vertex_buffer ||
        (has_indices() && m_dirty_index_buffer) ||
        (has_normals() && m_dirty_normal_buffer) ||
        (has_colors() && m_dirty_color_buffer) ||
        (has_tex_coords() && m_dirty_tex_coord_buffer) ||
        (has_tangents() && m_dirty_tangent_buffer) ||
        (has_tex_coords() && m_dirty_tex_coord_buffer) ||
        (has_point_sizes() && m_dirty_point_size_buffer);

}

void Geometry::create_gl_buffers()
{
    if(m_dirty_vertex_buffer && !m_vertices.empty())// pad vec3 -> vec4 (OpenCL compat issue)
    {
        //m_vertex_buffer.set_data(m_vertices);
        m_vertex_buffer.set_data(NULL, m_vertices.size() * sizeof(glm::vec4));
        m_vertex_buffer.set_stride(sizeof(glm::vec4));

        glm::vec4 *buf_ptr = (glm::vec4*) m_vertex_buffer.map();
        vector<glm::vec3>::const_iterator it = m_vertices.begin();
        for (; it != m_vertices.end(); ++buf_ptr, ++it)
        {
            *buf_ptr = glm::vec4(*it, 1.f);
        }
        m_vertex_buffer.unmap();
        KINSKI_CHECK_GL_ERRORS();

        m_dirty_vertex_buffer = false;
    }

    // insert normals
    if(m_dirty_normal_buffer && has_normals())
    {
        m_normal_buffer.set_data(m_normals);
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_normal_buffer = false;
    }

    // insert normals
    if(m_dirty_tex_coord_buffer && has_tex_coords())
    {
        m_tex_coord_buffer.set_data(m_tex_coords);
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_tex_coord_buffer = false;
    }

    // insert tangents
    if(m_dirty_tangent_buffer && has_tangents())
    {
        m_tangent_buffer.set_data(m_tangents);
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_tangent_buffer = false;
    }

    // insert point sizes
    if(m_dirty_point_size_buffer && has_point_sizes())
    {
        m_point_size_buffer.set_data(m_point_sizes);
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_point_size_buffer = false;
    }

    // insert colors
    if(m_dirty_color_buffer && has_colors())
    {
        m_color_buffer.set_data(m_colors);
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_color_buffer = false;
    }

    // insert bone indices and weights
    if(m_dirty_bone_buffer &&has_bones())
    {
#if !defined(KINSKI_GLES)
        m_bone_buffer.set_data(m_bone_vertex_data);
        m_bone_buffer.set_stride(sizeof(gl::BoneVertexData));
#else
        // crunch bone-indices to floats
        size_t bone_stride = 2 * sizeof(glm::vec4);
        m_bone_buffer.set_data(nullptr, m_bone_vertex_data.size() * bone_stride);
        m_bone_buffer.set_stride(bone_stride);
        glm::vec4 *buf_ptr = (glm::vec4*) m_bone_buffer.map();

        for (const auto &b : m_bone_vertex_data)
        {
            *buf_ptr++ = gl::vec4(b.indices);
            *buf_ptr++ = b.weights;
        }
        m_bone_buffer.unmap();
#endif
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_bone_buffer = false;
    }

    if(m_dirty_index_buffer && has_indices())
    {
        // index buffer
        m_index_buffer = gl::Buffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
        m_index_buffer.set_data(NULL, m_indices.size() * sizeof(index_t));
        KINSKI_CHECK_GL_ERRORS();
        index_t *indexPtr = (index_t*) m_index_buffer.map();
        KINSKI_CHECK_GL_ERRORS();

        // insert indices
        for (const auto &index : m_indices){ *indexPtr++ = index; }

        m_index_buffer.unmap();
        KINSKI_CHECK_GL_ERRORS();
        m_dirty_index_buffer = false;
    }
}

GLenum Geometry::index_type()
{
#if defined(KINSKI_GLES)
    GLenum ret = GL_UNSIGNED_SHORT;
#else
    GLenum ret = GL_UNSIGNED_INT;
#endif
    return ret;
}

/********************************* PRIMITIVES ****************************************/

Geometry::Ptr Geometry::create_plane(float width, float height,
                                     uint32_t numSegments_W , uint32_t numSegments_H)
{
    GeometryPtr geom = Geometry::create();

    float width_half = width / 2, height_half = height / 2;
    float segment_width = width / numSegments_W, segment_height = height / numSegments_H;

    uint32_t gridX = numSegments_W, gridZ = numSegments_H, gridX1 = gridX + 1, gridZ1 = gridZ + 1;

    glm::vec3 normal (0, 0, 1);

    // create vertices
    for(uint32_t iz = 0; iz < gridZ1; ++iz)
    {
        for (uint32_t ix = 0; ix < gridX1; ++ix)
        {
            float x = ix * segment_width - width_half;
            float y = iz * segment_height - height_half;
            geom->append_vertex( glm::vec3( x, - y, 0) );
            geom->append_normal(normal);
            geom->append_tex_coord( ix / (float)gridX, (gridZ - iz) / (float)gridZ);
        }
    }

    // fill in colors
    geom->colors().resize(geom->vertices().size(), gl::COLOR_WHITE);

    // create faces and texcoords
    for(uint32_t iz = 0; iz < gridZ; ++iz)
    {
        for( uint32_t ix = 0; ix < gridX; ++ix)
        {
            uint32_t a = ix + gridX1 * iz;
            uint32_t b = ix + gridX1 * ( iz + 1);
            uint32_t c = ( ix + 1 ) + gridX1 * ( iz + 1 );
            uint32_t d = ( ix + 1 ) + gridX1 * iz;

            Face3 f1(a, b, c), f2(c, d, a);
            geom->append_face(f1);
            geom->append_face(f2);
        }
    }
    geom->compute_tangents();
    geom->compute_bounding_box();
    return geom;
}

GeometryPtr Geometry::create_solid_circle(int numSegments, float the_radius)
{
    GeometryPtr ret = Geometry::create();
    ret->set_primitive_type(GL_TRIANGLE_FAN);
    std::vector<glm::vec3> &verts = ret->vertices();
    std::vector<glm::vec2> &texCoords = ret->tex_coords();
    std::vector<glm::vec4> &colors = ret->colors();

    // automatically determine the number of segments from the circumference
//        if( numSegments <= 0 ){ numSegments = (int)floor(radius * M_PI * 2);}
//        numSegments = std::max(numSegments, 2);
    verts.resize((numSegments+2));
    texCoords.resize((numSegments+2));
    colors.resize(verts.size(), gl::COLOR_WHITE);
    verts[0] = glm::vec3(0);
    texCoords[0] = glm::vec2(.5f);

    for( int s = 0; s <= numSegments; s++ )
    {
        float t = s / (float)numSegments * 2.0f * M_PI;
        gl::vec3 unit_val = glm::vec3(glm::vec2(cos(t), sin(t)), 0);
        verts[s + 1] = the_radius * unit_val;
        texCoords[s + 1] = (unit_val.xy() + glm::vec2(1)) / 2.f;
    }
    ret->compute_vertex_normals();
    ret->compute_tangents();
    ret->compute_bounding_box();
    return ret;
}

GeometryPtr Geometry::create_circle(int numSegments, float the_radius)
{
    GeometryPtr ret = Geometry::create();
    ret->set_primitive_type(GL_LINE_STRIP);
    auto &verts = ret->vertices();
    auto &texCoords = ret->tex_coords();

    // automatically determine the number of segments from the circumference
    //        if( numSegments <= 0 ){ numSegments = (int)floor(radius * M_PI * 2);}
    //        numSegments = std::max(numSegments, 2);
    verts.resize(numSegments + 1);
    texCoords.resize(verts.size());
    ret->colors().resize(verts.size(), gl::COLOR_WHITE);

    for(int s = 0; s <= numSegments; s++)
    {
        float t = s / (float)numSegments * 2.0f * M_PI;
        gl::vec3 unit_val = glm::vec3(glm::vec2(cos(t), sin(t)), 0);
        verts[s] = the_radius * unit_val;
        texCoords[s] = (unit_val.xy() + glm::vec2(1)) / 2.f;
    }
    ret->compute_bounding_box();
    return ret;
}

Geometry::Ptr Geometry::create_box(const glm::vec3 &the_half_extents)
{
    GeometryPtr geom = Geometry::create();

    glm::vec3 vertices[8] =
    {
        glm::vec3(-the_half_extents.x, -the_half_extents.y, the_half_extents.z),// bottom left front
        glm::vec3(the_half_extents.x, -the_half_extents.y, the_half_extents.z),// bottom right front
        glm::vec3(the_half_extents.x, -the_half_extents.y, -the_half_extents.z),// bottom right back
        glm::vec3(-the_half_extents.x, -the_half_extents.y, -the_half_extents.z),// bottom left back
        glm::vec3(-the_half_extents.x, the_half_extents.y, the_half_extents.z),// top left front
        glm::vec3(the_half_extents.x, the_half_extents.y, the_half_extents.z),// top right front
        glm::vec3(the_half_extents.x, the_half_extents.y, -the_half_extents.z),// top right back
        glm::vec3(-the_half_extents.x, the_half_extents.y, -the_half_extents.z),// top left back
    };
    glm::vec4 colors[6] = { glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0 , 1, 1),
        glm::vec4(1, 1, 0, 1), glm::vec4(0, 1, 1, 1), glm::vec4(1, 0 , 1, 1)};

    glm::vec2 texCoords[4] = {glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)};

    glm::vec3 normals[6] = {glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1),
        glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 1, 0)};

    std::vector<glm::vec3> vertexVec;
    std::vector<glm::vec4> colorVec;
    std::vector<glm::vec3> normalsVec;
    std::vector<glm::vec2> texCoordVec;

    //front - bottom left - 0
    vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[0]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[0]);
    //front - bottom right - 1
    vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[0]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[0]);
    //front - top right - 2
    vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[0]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[0]);
    //front - top left - 3
    vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[0]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[0]);
    //right - bottom left - 4
    vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[1]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[1]);
    //right - bottom right - 5
    vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[1]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[1]);
    //right - top right - 6
    vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[1]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[1]);
    //right - top left - 7
    vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[1]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[1]);
    //back - bottom left - 8
    vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[2]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[2]);
    //back - bottom right - 9
    vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[2]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[2]);
    //back - top right - 10
    vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[2]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[2]);
    //back - top left - 11
    vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[2]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[2]);
    //left - bottom left - 12
    vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[3]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[3]);
    //left - bottom right - 13
    vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[3]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[3]);
    //left - top right - 14
    vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[3]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[3]);
    //left - top left - 15
    vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[3]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[3]);
    //bottom - bottom left - 16
    vertexVec.push_back(vertices[3]); normalsVec.push_back(normals[4]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[4]);
    //bottom - bottom right - 17
    vertexVec.push_back(vertices[2]); normalsVec.push_back(normals[4]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[4]);
    //bottom - top right - 18
    vertexVec.push_back(vertices[1]); normalsVec.push_back(normals[4]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[4]);
    //bottom - top left - 19
    vertexVec.push_back(vertices[0]); normalsVec.push_back(normals[4]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[4]);
    //top - bottom left - 20
    vertexVec.push_back(vertices[4]); normalsVec.push_back(normals[5]);
    texCoordVec.push_back(texCoords[0]); colorVec.push_back(colors[5]);
    //top - bottom right - 21
    vertexVec.push_back(vertices[5]); normalsVec.push_back(normals[5]);
    texCoordVec.push_back(texCoords[1]); colorVec.push_back(colors[5]);
    //top - top right - 22
    vertexVec.push_back(vertices[6]); normalsVec.push_back(normals[5]);
    texCoordVec.push_back(texCoords[2]); colorVec.push_back(colors[5]);
    //top - top left - 23
    vertexVec.push_back(vertices[7]); normalsVec.push_back(normals[5]);
    texCoordVec.push_back(texCoords[3]); colorVec.push_back(colors[5]);

    geom->append_vertices(vertexVec);
    geom->append_normals(normalsVec);
    geom->append_tex_coords(texCoordVec);
    geom->append_colors(colorVec);

    for (int i = 0; i < 6; i++)
    {
        geom->append_face(i * 4 + 0, i * 4 + 1, i * 4 + 2);
        geom->append_face(i * 4 + 2, i * 4 + 3, i * 4 + 0);
    }
    geom->compute_tangents();
    geom->compute_bounding_box();
    return geom;
}

GeometryPtr Geometry::create_box_lines(const glm::vec3 &the_half_extents)
{
    GeometryPtr geom = Geometry::create();
    geom->set_primitive_type(GL_LINES);
    auto bb = gl::AABB(-the_half_extents, the_half_extents);
    geom->vertices() =
    {
        // botton
        bb.min, vec3(bb.min.x, bb.min.y, bb.max.z),
        vec3(bb.min.x, bb.min.y, bb.max.z), vec3(bb.max.x, bb.min.y, bb.max.z),
        vec3(bb.max.x, bb.min.y, bb.max.z), vec3(bb.max.x, bb.min.y, bb.min.z),
        vec3(bb.max.x, bb.min.y, bb.min.z), bb.min,

        // top
        vec3(bb.min.x, bb.max.y, bb.min.z), vec3(bb.min.x, bb.max.y, bb.max.z),
        vec3(bb.min.x, bb.max.y, bb.max.z), vec3(bb.max.x, bb.max.y, bb.max.z),
        vec3(bb.max.x, bb.max.y, bb.max.z), vec3(bb.max.x, bb.max.y, bb.min.z),
        vec3(bb.max.x, bb.max.y, bb.min.z), vec3(bb.min.x, bb.max.y, bb.min.z),

        //sides
        vec3(bb.min.x, bb.min.y, bb.min.z), vec3(bb.min.x, bb.max.y, bb.min.z),
        vec3(bb.min.x, bb.min.y, bb.max.z), vec3(bb.min.x, bb.max.y, bb.max.z),
        vec3(bb.max.x, bb.min.y, bb.max.z), vec3(bb.max.x, bb.max.y, bb.max.z),
        vec3(bb.max.x, bb.min.y, bb.min.z), vec3(bb.max.x, bb.max.y, bb.min.z)
    };
    geom->colors().resize(geom->vertices().size(), gl::COLOR_WHITE);
    return geom;
}

Geometry::Ptr Geometry::create_sphere(float radius, int numSlices)
{
    uint32_t rings = numSlices, sectors = numSlices;
    GeometryPtr geom = Geometry::create();
    float const R = 1./(float)(rings-1);
    float const S = 1./(float)(sectors-1);
    uint32_t r, s;

    geom->vertices().resize(rings * sectors);
    geom->normals().resize(rings * sectors);
    geom->tex_coords().resize(rings * sectors);
    geom->colors().resize(rings * sectors, gl::COLOR_WHITE);
    std::vector<glm::vec3>::iterator v = geom->vertices().begin();
    std::vector<glm::vec3>::iterator n = geom->normals().begin();
    std::vector<glm::vec2>::iterator t = geom->tex_coords().begin();
    for(r = 0; r < rings; r++)
        for(s = 0; s < sectors; s++, ++v, ++n, ++t)
        {
            float const y = sin( -M_PI_2 + M_PI * r * R );
            float const x = cos(2*M_PI * s * S) * sin( M_PI * r * R );
            float const z = sin(2*M_PI * s * S) * sin( M_PI * r * R );

            *t = glm::clamp(glm::vec2(1 - s * S, r * R), glm::vec2(0), glm::vec2(1));
            *v = glm::vec3(x, y, z) * radius;
            *n = glm::vec3(x, y, z);
        }

    for(r = 0; r < rings-1; r++)
        for(s = 0; s < sectors-1; s++)
        {
            geom->append_face(r * sectors + s, (r+1) * sectors + (s+1), r * sectors + (s+1));
            geom->append_face(r * sectors + s, (r+1) * sectors + s, (r+1) * sectors + (s+1));
        }

    geom->compute_tangents();
    geom->compute_bounding_box();
    return geom;
}

GeometryPtr Geometry::create_cone(float radius, float height, int numSegments)
{
    GeometryPtr ret = Geometry::create();
    ret->set_primitive_type(GL_TRIANGLES);
    std::vector<glm::vec3> &verts = ret->vertices();

    verts.resize(numSegments + 2);
    verts[0] = glm::vec3(0);
    verts[1] = glm::vec3(0, height, 0);
    ret->colors().resize(verts.size(), gl::COLOR_WHITE);
    for(int s = 2; s < numSegments + 2; s++)
    {
        float t = s / (float)numSegments * 2.0f * M_PI;
        verts[s] = radius * glm::vec3(cos(t), 0, sin(t));
        int next_index = (s + 1) > (numSegments + 1) ? (s + 1) % (numSegments + 1) + 1 : s + 1;

        //mantle
        ret->append_face(next_index, s, 1);

        //bottom
        ret->append_face(s, next_index, 0);
    }
    ret->compute_vertex_normals();
    ret->compute_bounding_box();
    return ret;
}

GeometryPtr Geometry::create_grid(float width, float height, uint32_t numSegments_W,
                                  uint32_t numSegments_H)
{
    GeometryPtr ret = Geometry::create();
    ret->set_primitive_type(GL_LINES);

    vector<vec3> &points = ret->vertices();
    vector<vec4> &colors = ret->colors();
    vector<vec2> &tex_coords = ret->tex_coords();

    float stepX = width / numSegments_W, stepZ = height / numSegments_H;
    float w2 = width / 2.f, h2 = height / 2.f;

    glm::vec4 color;
    for (uint32_t x = 0; x <= numSegments_W; ++x)
    {
        if(x == 0){ color = gl::COLOR_BLUE; }
        else{ color = gl::COLOR_GRAY; }

        // line Z
        points.push_back(vec3(- w2 + x * stepX, 0.f, -h2));
        points.push_back(vec3(- w2 + x * stepX, 0.f, h2));
        colors.push_back(color);
        colors.push_back(color);
        tex_coords.push_back(vec2(x / (float)numSegments_W, 0.f));
        tex_coords.push_back(vec2(x / (float)numSegments_W, 1.f));

    }
    for (uint32_t z = 0; z <= numSegments_H; ++z)
    {
        if(z == 0){ color = gl::COLOR_RED; }
        else{ color = gl::COLOR_GRAY; }

        // line X
        points.push_back(vec3(- w2 , 0.f, -h2 + z * stepZ));
        points.push_back(vec3( w2 , 0.f, -h2 + z * stepZ));
        colors.push_back(color);
        colors.push_back(color);
        tex_coords.push_back(vec2(0.f, z / (float)numSegments_H));
        tex_coords.push_back(vec2(1.f, z / (float)numSegments_H));
    }
    return ret;
}
    
}}//namespace
