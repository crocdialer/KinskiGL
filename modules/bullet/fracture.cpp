#include "physics_context.h"
#include "LinearMath/btConvexHullComputer.h"

#define CONVEX_MARGIN 0.04
using namespace glm;

void getVerticesInsidePlanes(const btAlignedObjectArray<btVector3>& planes,
                             btAlignedObjectArray<btVector3>& verticesOut,
                             std::set<int>& planeIndicesOut);

namespace kinski{namespace physics{
    
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
    
    ShatterResult voronoi_convex_hull_shatter(const std::vector<glm::vec3>& the_voronoi_points,
                                              const kinski::gl::MeshPtr &the_mesh,
                                              float the_mat_density,
                                              bool use_constraints)
    {
        // points define voronoi cells in world space (avoid duplicates)
        // verts = source (convex hull) mesh vertices in local space
        // bbq & bbt = source (convex hull) mesh quaternion rotation and translation
        // matDensity = Material density for voronoi shard mass calculation
        
        ShatterResult ret;
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
        int cellnum = 0;
        int i, j, k;
        
        // Convert verts to world space and get convexPlanes
        int numverts = mesh_verts.size();
        
        auto mesh_transform = the_mesh->global_transform();
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
            
            std::vector<gl::Face3> my_faces;
            
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
                    my_faces.push_back(gl::Face3(v0, v1, v2));
                    
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

            // create gl::Mesh object for the shard
            auto geom = gl::Geometry::create();
            auto &out_verts = geom->vertices();
            out_verts.resize(numVerts);
            
            // verts
            for(int i = 0; i < out_verts.size(); i++)
            {
                out_verts[i] = type_cast(convexHC->vertices[i]);
            }
            
            // indices
            geom->appendFaces(my_faces);
            geom->computeVertexNormals();
            geom->computeBoundingBox();
            
            auto m = gl::Mesh::create(geom, gl::Material::create());
            m->setPosition(curVoronoiPoint + type_cast(com));
            ret.shard_meshes.push_back(m);
            
            cellnum ++;
            
        }
        LOG_DEBUG << "Generated " << cellnum <<" voronoi shards";
        
        return ret;
    }
    
}}// namespace

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