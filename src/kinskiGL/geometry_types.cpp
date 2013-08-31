// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "geometry_types.h"

namespace kinski { namespace gl {

//ray_intersection intersect_ray_plane(const glm::vec3 &n, const glm::vec3 &p0, const gl::Ray &ray)
//{
//    // assuming vectors are all normalized
//    float denom = glm::dot(n, ray.direction);
//    if (denom > 1e-6)
//    {
//        glm::vec3 p0l0 = p0 - ray.origin;
//        float d = glm::dot(p0l0, n) / denom;
//        if(d >= 0) return ray_intersection(INTERSECT, d);
//    }
//    return REJECT;
//}
    
ray_intersection intersect(const gl::Plane &plane, const gl::Ray &ray)
{
    // assuming vectors are all normalized
    float denom = glm::dot(-plane.normal(), ray.direction);
    if (denom > 1e-6)
    {
        float d = (plane.coefficients.z - glm::dot(ray.origin, -plane.normal())) / denom;
        if(d >= 0) return ray_intersection(INTERSECT, d);
    }
    return REJECT;
}
    
ray_triangle_intersection intersect(const Triangle &theTri, const Ray &theRay)
{
    glm::vec3 e1 = theTri.v1 - theTri.v0, e2 = theTri.v2 - theTri.v0;
    glm::vec3 pvec = glm::cross(theRay.direction, e2);
    float det = glm::dot(e1, pvec);
    static float epsilon = 10.0e-5;
    if(det > -epsilon && det < epsilon) return REJECT;
    float inv_det = 1.0f / det;
    glm::vec3 tvec = theRay.origin - theTri.v0;
    float u = inv_det * glm::dot(tvec, pvec);
    if(u < 0.0f || u > 1.0f) return REJECT;
    glm::vec3 qvec = glm::cross(tvec, e1);
    float v = glm::dot(theRay.direction, qvec) * inv_det;
    if(v < 0.0f || (u + v) > 1.0f) return REJECT;
    return ray_triangle_intersection(INTERSECT, glm::dot(e2, qvec) * inv_det, u, v);
}
    
ray_intersection intersect(const Sphere &theSphere, const Ray &theRay)
{
    glm::vec3 l = theSphere.center - theRay.origin;
    float s = glm::dot(l, theRay.direction);
    float l2 = glm::dot(l, l);
    float r2 = theSphere.radius * theSphere.radius;
    if(s < 0 && l2 > r2) return REJECT;
    float m2 = l2 - s * s;
    if(m2 > r2) return REJECT;
    float q = sqrtf(r2 - m2);
    float t;
    if(l2 > r2) t = s - q;
    else t = s + q;
    return ray_intersection(INTERSECT, t);
}
    
ray_intersection intersect(const OBB &theOBB, const Ray& theRay)
{
    float t_min = std::numeric_limits<float>::min();
    float t_max = std::numeric_limits<float>::max();
    glm::vec3 p = theOBB.center - theRay.origin;
    
    for (int i = 0; i < 3; i++)
    {
        float e = glm::dot(theOBB.axis[i], p);
        float f = glm::dot(theOBB.axis[i], theRay.direction);
        
        // this test avoids overflow from division
        if(std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            float t1 = (e + theOBB.half_lengths[i]) / f;
            float t2 = (e - theOBB.half_lengths[i]) / f;
            
            if(t1 > t2) std::swap(t1, t2);
            if(t1 > t_min) t_min = t1;
            if(t2 < t_max) t_max = t2;
            if(t_min > t_max) return REJECT;
            if(t_max < 0) return REJECT;
        }
        else if( (-e - theOBB.half_lengths[i]) > 0 || (-e + theOBB.half_lengths[i]) < 0 )
        {
            return REJECT;
        }
    }
    
    if(t_min > 0)
        return ray_intersection(INTERSECT, t_min);
    else
        return ray_intersection(INTERSECT, t_max);
}
    
Plane::Plane()
{
    coefficients.w = 0;
    coefficients.xyz() = glm::vec3(0, 1, 0);
}

Plane::Plane(const glm::vec4 &theCoefficients)
{
    float len = glm::length(theCoefficients.xyz());
    coefficients = theCoefficients / len;
}

Plane::Plane(float theA, float theB, float theC, float theD)
{
    const glm::vec4 theCoefficients(theA, theB, theC, theD);
    float len = glm::length(theCoefficients.xyz());
    coefficients = theCoefficients / len;
}

Plane::Plane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    coefficients.xyz() = glm::normalize(glm::cross(v2 - v0, v1 - v0));
    coefficients.w = -glm::dot(v0, glm::vec3(coefficients.xyz()));
}

Plane::Plane(const glm::vec3& theFoot, const glm::vec3& theNormal)
{
    coefficients.xyz() = glm::normalize(theNormal);
    coefficients.w = -glm::dot(theFoot, glm::vec3(coefficients.xyz()));
}
    
OBB::OBB(const AABB &theAABB, const glm::mat4 &t)
{
    center = (t * glm::vec4(theAABB.center(), 1.0f)).xyz();
    glm::vec3 scale (glm::length(t[0]),
                     glm::length(t[1]),
                     glm::length(t[2]));
    axis[0] = t[0].xyz() / scale[0];
    axis[1] = t[1].xyz() / scale[1];
    axis[2] = t[2].xyz() / scale[2];
    half_lengths = theAABB.halfExtents() * scale;
}
    
AABB& AABB::transform(const glm::mat4& t)
{
    glm::vec3 aMin, aMax;
    float     a, b;
    int     i, j;
    
    // Copy box A into min and max array.
    aMin = min;
    aMax = max;
    
    // Begin at T.
    min = max = t[3].xyz();
    
    // Find extreme points by considering product of
    // min and max with each component of t.
    
    for( j=0; j<3; j++ )
    {
        for( i=0; i<3; i++ )
        {
            a = t[i][j] * aMin[i];
            b = t[i][j] * aMax[i];

            if( a < b )
            {
                min[j] += a;
                max[j] += b;
            }
            else
            {
                min[j] += b;
                max[j] += a;
            }
        }
    }
    return *this;
}

ray_intersection AABB::intersect(const Ray& theRay) const
{
    OBB obb(*this, glm::mat4());
    return obb.intersect(theRay);
}
    
uint32_t AABB::intersect(const Triangle& t) const
{
    float triVerts [3][3] =	{	{t.v0[0],t.v0[1],t.v0[2]},
                                {t.v1[0],t.v1[1],t.v1[2]},
                                {t.v2[0],t.v2[1],t.v2[2]}
                            };
    return triBoxOverlap(&center()[0],&halfExtents()[0],triVerts);
}

Frustum::Frustum(const glm::mat4 &the_VP_martix)
{
    planes[0] = Plane(the_VP_martix[2] + the_VP_martix[3]); // near plane
    planes[1] = Plane(the_VP_martix[3] - the_VP_martix[2]); // far plane
    planes[2] = Plane(the_VP_martix[0] + the_VP_martix[3]); // left plane
    planes[3] = Plane(the_VP_martix[3] - the_VP_martix[0]); // right plane
    planes[4] = Plane(the_VP_martix[3] - the_VP_martix[1]); // top plane
    planes[5] = Plane(the_VP_martix[1] + the_VP_martix[3]); // bottom plane
}
    
Frustum::Frustum(float aspect, float fov, float near, float far)
{
    glm::mat4 t;
    static glm::vec3 lookAt = glm::vec3(0, 0, -1), eyePos = glm::vec3(0),
    side = glm::vec3(1, 0, 0), up = glm::vec3(0, 1, 0);
    float angle_y = 90.0f - aspect * fov/2.0f ;
    float angle_x = 90.0f - (fov/2.0f) ;
    
	planes[0] = Plane(eyePos + (near * lookAt), lookAt); // near plane
	planes[1] = Plane(eyePos + (far * lookAt), -lookAt); // far plane

    t = glm::rotate(glm::mat4(), angle_y, up);
	planes[2] = Plane(eyePos, lookAt).transform(t); // left plane

    t = glm::rotate(glm::mat4(), -angle_y, up);
	planes[3] = Plane(eyePos, lookAt).transform(t); // right plane

    t = glm::rotate(glm::mat4(), -angle_x, side);
	planes[4] = Plane(eyePos, lookAt).transform(t); // top plane

    t = glm::rotate(glm::mat4(), angle_x, side);
	planes[5] = Plane(eyePos, lookAt).transform(t); // bottom plane
}

Frustum::Frustum(float left, float right,float bottom, float top,
                 float near, float far)
{
    static glm::vec3 lookAt = glm::vec3(0, 0, -1), eyePos = glm::vec3(0),
    side = glm::vec3(1, 0, 0), up = glm::vec3(0, 1, 0);
	planes[0] = Plane(eyePos + (near * lookAt), lookAt); // near plane
	planes[1] = Plane(eyePos + (far * lookAt), -lookAt); // far plane
	planes[2] = Plane(eyePos + (left * side), side); // left plane
    planes[3] = Plane(eyePos + (right * side), -side); // right plane
	planes[4] = Plane(eyePos + (top * up), -up); // top plane
	planes[5] = Plane(eyePos + (bottom * up), up); // bottom plane
    
}
    
/********************************************************/

/* AABB-triangle overlap test code                      */

/* by Tomas Akenine-Möller                              */

/* Function: int triBoxOverlap(float boxcenter[3],      */

/*          float boxhalfsize[3],float triverts[3][3]); */

/* History:                                             */

/*   2001-03-05: released the code in its first version */

/*   2001-06-18: changed the order of the tests, faster */

/*                                                      */

/* Acknowledgement: Many thanks to Pierre Terdiman for  */

/* suggestions and discussions on how to optimize code. */

/* Thanks to David Hunt for finding a ">="-bug!         */

/********************************************************/

#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
dest[2]=v1[0]*v2[1]-v1[1]*v2[0]; 

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
dest[0]=v1[0]-v2[0]; \
dest[1]=v1[1]-v2[1]; \
dest[2]=v1[2]-v2[2]; 

#define FINDMINMAX(x0,x1,x2,min,max) \
min = max = x0;   \
if(x1<min) min=x1;\
if(x1>max) max=x1;\
if(x2<min) min=x2;\
if(x2>max) max=x2;

int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3])	// -NJMP-
{	
	int q;
	float vmin[3],vmax[3],v;	
	for(q=X;q<=Z;q++)		
	{
		v=vert[q];					// -NJMP-		
		if(normal[q]>0.0f)			
		{			
			vmin[q]=-maxbox[q] - v;	// -NJMP-
			vmax[q]= maxbox[q] - v;	// -NJMP-			
		}		
		else			
		{			
			vmin[q]= maxbox[q] - v;	// -NJMP-
			vmax[q]=-maxbox[q] - v;	// -NJMP-
		}		
	}
	
	if(DOT(normal,vmin)>0.0f) return 0;	// -NJMP-	
	if(DOT(normal,vmax)>=0.0f) return 1;	// -NJMP-

	return 0;	
}

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)			   \
p0 = a*v0[Y] - b*v0[Z];			       	   \
p2 = a*v2[Y] - b*v2[Z];			       	   \
if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
if(min>rad || max<-rad) return 0;


#define AXISTEST_X2(a, b, fa, fb)			   \
p0 = a*v0[Y] - b*v0[Z];			           \
p1 = a*v1[Y] - b*v1[Z];			       	   \
if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
if(min>rad || max<-rad) return 0;

/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)			   \
p0 = -a*v0[X] + b*v0[Z];		      	   \
p2 = -a*v2[X] + b*v2[Z];	       	       	   \
if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
if(min>rad || max<-rad) return 0;


#define AXISTEST_Y1(a, b, fa, fb)			   \
p0 = -a*v0[X] + b*v0[Z];		      	   \
p1 = -a*v1[X] + b*v1[Z];	     	       	   \
if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
if(min>rad || max<-rad) return 0;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)			   \
p1 = a*v1[X] - b*v1[Y];			           \
p2 = a*v2[X] - b*v2[Y];			       	   \
if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
if(min>rad || max<-rad) return 0;


#define AXISTEST_Z0(a, b, fa, fb)			   \
p0 = a*v0[X] - b*v0[Y];				   \
p1 = a*v1[X] - b*v1[Y];			           \
if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
if(min>rad || max<-rad) return 0;

int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3])
{

	/*    use separating axis theorem to test overlap between triangle and box */	
	/*    need to test for overlap in these directions: */	
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */	
	/*       we do not even need to test these) */	
	/*    2) normal of the triangle */	
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */	
	/*       this gives 3x3=9 more tests */
	
	float v0[3],v1[3],v2[3];	
	//   float axis[3];	
	float min,max,p0,p1,p2,rad,fex,fey,fez;		// -NJMP- "d" local variable removed	
	float normal[3],e0[3],e1[3],e2[3];
	
	/* This is the fastest branch on Sun */	
	/* move everything so that the boxcenter is in (0,0,0) */
	
	SUB(v0,triverts[0],boxcenter);	
	SUB(v1,triverts[1],boxcenter);	
	SUB(v2,triverts[2],boxcenter);

	/* compute triangle edges */
	
	SUB(e0,v1,v0);      /* tri edge 0 */	
	SUB(e1,v2,v1);      /* tri edge 1 */
	SUB(e2,v0,v2);      /* tri edge 2 */

	/* Bullet 3:  */
	
	/*  test the 9 tests first (this was faster) */	
	fex = fabsf(e0[X]);	
	fey = fabsf(e0[Y]);	
	fez = fabsf(e0[Z]);
	
	AXISTEST_X01(e0[Z], e0[Y], fez, fey);	
	AXISTEST_Y02(e0[Z], e0[X], fez, fex);	
	AXISTEST_Z12(e0[Y], e0[X], fey, fex);	
	
	fex = fabsf(e1[X]);	
	fey = fabsf(e1[Y]);	
	fez = fabsf(e1[Z]);
	
	AXISTEST_X01(e1[Z], e1[Y], fez, fey);	
	AXISTEST_Y02(e1[Z], e1[X], fez, fex);	
	AXISTEST_Z0(e1[Y], e1[X], fey, fex);
	
	fex = fabsf(e2[X]);	
	fey = fabsf(e2[Y]);	
	fez = fabsf(e2[Z]);
	
	AXISTEST_X2(e2[Z], e2[Y], fez, fey);	
	AXISTEST_Y1(e2[Z], e2[X], fez, fex);	
	AXISTEST_Z12(e2[Y], e2[X], fey, fex);
	
	
	
	/* Bullet 1: */
	
	/*  first test overlap in the {x,y,z}-directions */	
	/*  find min, max of the triangle each direction, and test for overlap in */	
	/*  that direction -- this is equivalent to testing a minimal AABB around */	
	/*  the triangle against the AABB */

	/* test in X-direction */
	
	FINDMINMAX(v0[X],v1[X],v2[X],min,max);
	
	if(min>boxhalfsize[X] || max<-boxhalfsize[X]) return REJECT;

	/* test in Y-direction */	
	FINDMINMAX(v0[Y],v1[Y],v2[Y],min,max);
	
	if(min>boxhalfsize[Y] || max<-boxhalfsize[Y]) return REJECT;
	
	/* test in Z-direction */
	
	FINDMINMAX(v0[Z],v1[Z],v2[Z],min,max);
	
	if(min>boxhalfsize[Z] || max<-boxhalfsize[Z]) return REJECT;
	
	/* Bullet 2: */	
	/*  test if the box intersects the plane of the triangle */	
	/*  compute plane equation of triangle: normal*x+d=0 */	
	CROSS(normal,e0,e1);	
	// -NJMP- (line removed here)	
	if(!planeBoxOverlap(normal,v0,boxhalfsize)) return REJECT;	// -NJMP-
	
	return INTERSECT;   /* box and triangle overlaps */
	
}

}}//namespace