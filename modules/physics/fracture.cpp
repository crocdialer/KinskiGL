#include "physics_context.h"
#include "LinearMath/btConvexHullComputer.h"

#define CONVEX_MARGIN 0.04
using namespace glm;

void getVerticesInsidePlanes(const btAlignedObjectArray<btVector3>& planes,
                             btAlignedObjectArray<btVector3>& verticesOut,
                             std::set<int>& planeIndicesOut);

namespace kinski{namespace physics{
    
    inline bool is_equal(const btVector3 &v0, const btVector3 &v1, const float eps = 0.0001f)
    {
        if(fabs(v0[0] - v1[0]) > eps) return false;
        if(fabs(v0[1] - v1[1]) > eps) return false;
        if(fabs(v0[2] - v1[2]) > eps) return false;
//        if(fabs(v0[3] - v1[3]) > eps) return false;
        return true;
    }
    
    struct pointCmp
    {
        glm::vec3 current_point;
        
        pointCmp(const glm::vec3& p): current_point(p){}
        
        bool operator()(const glm::vec3& p1, const glm::vec3& p2) const
        {
            float v1 = glm::length2(p1 - current_point);
            float v2 = glm::length2(p2 - current_point);
            return v1 < v2;
        }
    };
    
    std::list<VoronoiShard>
    voronoi_convex_hull_shatter(const gl::MeshPtr &the_mesh,
                                const std::vector<glm::vec3>& the_voronoi_points)
    {
        // points define voronoi cells in world space (avoid duplicates)
        // verts = source (convex hull) mesh vertices in local space
        
        std::list<VoronoiShard> ret;
        std::vector<glm::vec3> mesh_verts = the_mesh->geometry()->vertices();
        
        auto convexHC = std::make_shared<btConvexHullComputer>();
        btAlignedObjectArray<btVector3> vertices;
        
        btVector3 rbb, nrbb;
        btScalar nlength, maxDistance, distance;
        std::vector<glm::vec3> sortedVoronoiPoints = the_voronoi_points;
        
        btVector3 normal, plane;
        btAlignedObjectArray<btVector3> planes, convexPlanes;
        std::set<int> planeIndices;
        std::set<int>::iterator planeIndicesIter;
        int numplaneIndices;
        int i, j, k;
        
        // Normalize verts (numerical stability), convert to world space and get convexPlanes
        int numverts = mesh_verts.size();
//        auto aabb = the_mesh->boundingBox();
//        float scale_val = 1.f;//std::max(std::max(aabb.width(), aabb.height()), aabb.depth());
        
        auto mesh_transform = the_mesh->global_transform() ;//* scale(glm::mat4(), vec3(1.f / scale_val));
        std::vector<glm::vec3> world_space_verts;
        
        world_space_verts.resize(mesh_verts.size());
        for (i = 0; i < numverts ;i++)
        {
            world_space_verts[i] = (mesh_transform * vec4(mesh_verts[i], 1.f)).xyz();
        }
        
        //btGeometryUtil::getPlaneEquationsFromVertices(chverts, convexPlanes);
        // Using convexHullComputer faster than getPlaneEquationsFromVertices for large meshes...
        convexHC->compute(&world_space_verts[0].x, sizeof(world_space_verts[0]), numverts, 0.0, 0.0);
        
        int numFaces = convexHC->faces.size();
        int v0, v1, v2; // vertices
        
        // get plane equations for the convex-hull n-gons
        for (i = 0; i < numFaces; i++)
        {
            const btConvexHullComputer::Edge* edge = &convexHC->edges[convexHC->faces[i]];
            v0 = edge->getSourceVertex();
            v1 = edge->getTargetVertex();
            edge = edge->getNextEdgeOfFace();
            v2 = edge->getTargetVertex();
            plane = (convexHC->vertices[v1]-convexHC->vertices[v0]).cross(convexHC->vertices[v2]-convexHC->vertices[v0]).normalize();
            plane[3] = -plane.dot(convexHC->vertices[v0]);
            convexPlanes.push_back(plane);
        }
        const int numconvexPlanes = convexPlanes.size();
        
        int numpoints = the_voronoi_points.size();
        
        for (i = 0; i < numpoints ; i++)
        {
            auto curVoronoiPoint = the_voronoi_points[i];
            planes.copyFromArray(convexPlanes);
            
            for (j = 0; j < numconvexPlanes; j++)
            {
                planes[j][3] += planes[j].dot(type_cast(the_voronoi_points[i]));
            }
            maxDistance = SIMD_INFINITY;
            
            // sort voronoi points
            std::sort(sortedVoronoiPoints.begin(), sortedVoronoiPoints.end(), pointCmp(curVoronoiPoint));
            
            for (j=1; j < numpoints; j++)
            {
                normal = type_cast(sortedVoronoiPoints[j] - curVoronoiPoint);
                nlength = normal.length();
                if (nlength > maxDistance)
                    break;
                plane = normal.normalized();
                plane[3] = -nlength / btScalar(2.);
                planes.push_back(plane);
                getVerticesInsidePlanes(planes, vertices, planeIndices);
                
                if (vertices.size() == 0) break;
                
                numplaneIndices = planeIndices.size();
                if (numplaneIndices != planes.size())
                {
                    planeIndicesIter = planeIndices.begin();
                    for (k=0; k < numplaneIndices; k++)
                    {
                        if (k != *planeIndicesIter)
                            planes[k] = planes[*planeIndicesIter];
                        planeIndicesIter++;
                    }
                    planes.resize(numplaneIndices);
                }
                maxDistance = vertices[0].length();
                for (k=1; k < vertices.size(); k++)
                {
                    distance = vertices[k].length();
                    if (maxDistance < distance)
                        maxDistance = distance;
                }
                maxDistance *= btScalar(2.);
            }
            if (vertices.size() == 0)
                continue;
            
            // Clean-up voronoi convex shard vertices and generate edges & faces
            convexHC->compute(&vertices[0].getX(), sizeof(btVector3), vertices.size(),0.0,0.0);
            
            // At this point we have a complete 3D voronoi shard mesh contained in convexHC
            
            // Calculate volume and center of mass (Stan Melax volume integration)
            numFaces = convexHC->faces.size();
            btScalar volume = btScalar(0.);
            btVector3 com(0., 0., 0.);
            
            for (j = 0; j < numFaces; j++)
            {
                const btConvexHullComputer::Edge* edge = &convexHC->edges[convexHC->faces[j]];
                v0 = edge->getSourceVertex();
                v1 = edge->getTargetVertex();
                edge = edge->getNextEdgeOfFace();
                v2 = edge->getTargetVertex();
                
                while (v2 != v0)
                {
                    // Counter-clockwise triangulated voronoi shard mesh faces (v0-v1-v2) and edges here...
                    btScalar vol = convexHC->vertices[v0].triple(convexHC->vertices[v1], convexHC->vertices[v2]);
                    volume += vol;
                    com += vol * (convexHC->vertices[v0] + convexHC->vertices[v1] + convexHC->vertices[v2]);
                    edge = edge->getNextEdgeOfFace();
                    
                    v1 = v2;
                    v2 = edge->getTargetVertex();
                }
            }
            com /= volume * btScalar(4.);
            volume /= btScalar(6.);
            
            // Shift all vertices relative to center of mass
            int numVerts = convexHC->vertices.size();
            for (j = 0; j < numVerts; j++)
            {
                convexHC->vertices[j] -= com;
            }
            
            // now create our output geometry with indices
            std::vector<gl::Face3> outer_faces, inner_faces;
            std::vector<glm::vec3> outer_vertices, inner_vertices;
            int cur_outer_index = 0, cur_inner_index = 0;
            
            for (j = 0; j < numFaces; j++)
            {
                const btConvexHullComputer::Edge* edge = &convexHC->edges[convexHC->faces[j]];
                v0 = edge->getSourceVertex();
                v1 = edge->getTargetVertex();
                edge = edge->getNextEdgeOfFace();
                v2 = edge->getTargetVertex();
                
                // determine if it is an inner or outer face
                btVector3 cur_plane = (convexHC->vertices[v1] - convexHC->vertices[v0]).cross(convexHC->vertices[v2]-convexHC->vertices[v0]).normalize();
                cur_plane[3] = -cur_plane.dot(convexHC->vertices[v0]);
                bool is_outside = false;
                
                for(uint32_t q = 0; q < convexPlanes.size(); q++)
                {
                    if(is_equal(convexPlanes[q], cur_plane, 0.01f)){ is_outside = true; break;}
                }
                std::vector<gl::Face3> *shard_faces = &outer_faces;
                std::vector<glm::vec3> *shard_vertices = &outer_vertices;
                int *shard_index = &cur_outer_index;
                
                if(!is_outside)
                {
                    shard_faces = &inner_faces;
                    shard_vertices = &inner_vertices;
                    shard_index = &cur_inner_index;
                }
                
                int face_start_index = *shard_index;
                
                // advance index
                *shard_index += 3;
                
                // first 3 verts of n-gon
                glm::vec3 tmp[] = { type_cast(convexHC->vertices[v0]),
                                    type_cast(convexHC->vertices[v1]),
                                    type_cast(convexHC->vertices[v2])};
                
                shard_vertices->insert(shard_vertices->end(), tmp, tmp + 3);
                shard_faces->push_back(gl::Face3(face_start_index,
                                                 face_start_index + 1,
                                                 face_start_index + 2));
                
                // add remaining triangles of face (if any)
                while (true)
                {
                    edge = edge->getNextEdgeOfFace();
                    v1 = v2;
                    v2 = edge->getTargetVertex();
                    
                    // end of n-gon
                    if(v2 == v0) break;
                    
                    shard_vertices->push_back(type_cast(convexHC->vertices[v2]));
                    shard_faces->push_back(gl::Face3(face_start_index,
                                                     *shard_index - 1,
                                                     *shard_index));
                    (*shard_index)++;
                }
            }
            
            // entry construction
            gl::Mesh::Entry e0, e1;
            
            // outer entry
            e0.num_vertices = outer_vertices.size();
            e0.num_indices = outer_faces.size() * 3;
            e0.material_index = 0;
            
            // inner entry
            e1.base_index = e0.num_indices;
            e1.base_vertex = e0.num_vertices;
            e1.num_vertices = inner_vertices.size();
            e1.num_indices = inner_faces.size() * 3;
            e1.material_index = 1;
            
            // create gl::Mesh object for the shard
            auto inner_geom = gl::Geometry::create(), outer_geom = gl::Geometry::create();
            
            // append verts and indices
            outer_geom->appendFaces(outer_faces);
            outer_geom->vertices() = outer_vertices;
            outer_geom->computeFaceNormals();
            
            inner_geom->appendFaces(inner_faces);
            inner_geom->appendVertices(inner_vertices);
            inner_geom->computeFaceNormals();
            
            // merge geometries
            outer_geom->appendVertices(inner_geom->vertices());
            outer_geom->appendNormals(inner_geom->normals());
            outer_geom->appendIndices(inner_geom->indices());
            outer_geom->faces().insert(outer_geom->faces().end(),
                                       inner_geom->faces().begin(), inner_geom->faces().end());
            outer_geom->computeBoundingBox();
            
            auto inner_mat = gl::Material::create();
            
            auto m = gl::Mesh::create(outer_geom, gl::Material::create());
            m->entries() = {e0, e1};
            m->materials().push_back(inner_mat);
            m->setPosition(curVoronoiPoint + type_cast(com));
//            m->transform() *= glm::scale(mat4(), vec3(scale_val));
            
            // compute projected texcoords (outside)
            gl::project_texcoords(the_mesh, m);
            
            // compute box mapped texcoords for inside vertices
            auto &indices = m->geometry()->indices();
            auto &vertices = m->geometry()->vertices();
            
            // aabb
            auto out_aabb = the_mesh->boundingBox();
            vec3 aabb_extents = out_aabb.halfExtents() * 2.f;
            
            uint32_t base_vertex = m->entries()[1].base_vertex;
            uint32_t k = m->entries()[1].base_index, kl = k + m->entries()[1].num_indices;
            
            for(;k < kl; k += 3)
            {
                gl::Face3 f(indices[k] + base_vertex,
                            indices[k] + base_vertex + 1,
                            indices[k] + base_vertex + 2);
                
                // normal
                const vec3 &v0 = vertices[f.a];
                const vec3 &v1 = vertices[f.b];
                const vec3 &v2 = vertices[f.c];
                
                vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                
                float abs_vals[3] = {fabsf(n[0]), fabsf(n[1]), fabsf(n[2])};
                
                // get principal direction
                int principle_axis = std::distance(abs_vals, std::max_element(abs_vals, abs_vals + 3));
                
//                switch (principle_axis)
//                {
//                    // X-axis
//                    case 0:
//                        //ZY plane
//                        m->geometry()->texCoords()[f.a] = vec2(v0.z - out_aabb.min.z / aabb_extents.z,
//                                                               v0.y - out_aabb.min.y / aabb_extents.y);
//                        m->geometry()->texCoords()[f.b] = vec2(v1.z - out_aabb.min.z / aabb_extents.z,
//                                                               v1.y - out_aabb.min.y / aabb_extents.y);
//                        m->geometry()->texCoords()[f.c] = vec2(v2.z - out_aabb.min.z / aabb_extents.z,
//                                                               v2.y - out_aabb.min.y / aabb_extents.y);
//                        break;
//                        
//                    // Y-axis
//                    case 1:
//                    // XZ plane
//                        m->geometry()->texCoords()[f.a] = vec2(v0.x - out_aabb.min.x / aabb_extents.x,
//                                                               v0.z - out_aabb.min.z / aabb_extents.z);
//                        m->geometry()->texCoords()[f.b] = vec2(v1.x - out_aabb.min.x / aabb_extents.x,
//                                                               v1.z - out_aabb.min.z / aabb_extents.z);
//                        m->geometry()->texCoords()[f.c] = vec2(v2.x - out_aabb.min.x / aabb_extents.x,
//                                                               v2.z - out_aabb.min.z / aabb_extents.z);
//                        break;
//                        
//                    // Z-axis
//                    case 2:
//                        //XY plane
//                        m->geometry()->texCoords()[f.a] = vec2(v0.x - out_aabb.min.x / aabb_extents.x,
//                                                               v0.y - out_aabb.min.y / aabb_extents.y);
//                        m->geometry()->texCoords()[f.b] = vec2(v1.x - out_aabb.min.x / aabb_extents.x,
//                                                               v1.y - out_aabb.min.y / aabb_extents.y);
//                        m->geometry()->texCoords()[f.c] = vec2(v2.x - out_aabb.min.x / aabb_extents.x,
//                                                               v2.y - out_aabb.min.y / aabb_extents.y);
//                        break;
//                        
//                    default:
//                        break;
//                }
            }
            // push to return structure
            ret.push_back({m, volume});
        }
        LOG_DEBUG << "Generated " << ret.size() <<" voronoi shards";
        return ret;
    }
    
}}// namespace

// TODO: this routine appears to be numerically instable ...
void getVerticesInsidePlanes(const btAlignedObjectArray<btVector3>& planes,
                             btAlignedObjectArray<btVector3>& verticesOut,
                             std::set<int>& planeIndicesOut)
{
    // Based on btGeometryUtil.cpp (Gino van den Bergen / Erwin Coumans)
    verticesOut.resize(0);
    planeIndicesOut.clear();
    const int numPlanes = planes.size();
    int i, j, k, l;
    for (i = 0; i < numPlanes; i++)
    {
        const btVector3& N1 = planes[i];
        for (j = i + 1; j < numPlanes; j++)
        {
            const btVector3& N2 = planes[j];
            btVector3 n1n2 = N1.cross(N2);
            if (n1n2.length2() > btScalar(0.0001))
            {
                for (k = j + 1; k < numPlanes; k++)
                {
                    const btVector3& N3 = planes[k];
                    btVector3 n2n3 = N2.cross(N3);
                    btVector3 n3n1 = N3.cross(N1);
                    if ((n2n3.length2() > btScalar(0.0001)) && (n3n1.length2() > btScalar(0.0001) ))
                    {
                        btScalar quotient = (N1.dot(n2n3));
                        if (btFabs(quotient) > btScalar(0.0001))
                        {
                            btVector3 potentialVertex = (n2n3 * N1[3] + n3n1 * N2[3] + n1n2 * N3[3]) * (btScalar(-1.) / quotient);
                            for (l = 0; l < numPlanes; l++)
                            {
                                const btVector3& NP = planes[l];
                                if (btScalar(NP.dot(potentialVertex))+btScalar(NP[3]) > btScalar(0.000001))
                                    break;
                            }
                            if (l == numPlanes)
                            {
                                // vertex (three plane intersection) inside all planes
                                verticesOut.push_back(potentialVertex);
                                planeIndicesOut.insert(i);
                                planeIndicesOut.insert(j);
                                planeIndicesOut.insert(k);
                            }
                        }
                    }
                }
            }
        }
    }
}