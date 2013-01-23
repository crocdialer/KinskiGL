#include "geometry_types.h"

#define DEGTORAD(degree) ((degree) * 0.017453292522222 ) //(3.141592654f / 180.0f)
#define RADTODEG(radian) ((radian) * 57.295779513082322 )//(180.0f / 3.141592654f)

#define MIN3(a,b,c) ((((a)<(b))&&((a)<(c))) ? (a) : (((b)<(c)) ? (b) : (c)))
#define MAX3(a,b,c) ((((a)>(b))&&((a)>(c))) ? (a) : (((b)>(c)) ? (b) : (c)))

#define PI 3.14159265359f

namespace kinski { namespace gl {
    
Plane::Plane()
{
	foot = glm::vec3(0);
	normal = glm::vec3(0, 1, 0);
}

Plane::Plane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
	foot = v0;
	normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
}

Plane::Plane(const glm::vec3& f, const glm::vec3& n):
foot(f),
normal(n)
{}

Frustum::Frustum(const glm::mat4 &transform,float fov, float near, float far)
{
    glm::mat4 t;
    glm::vec3 lookAt = -transform[2].xyz(), eyePos = transform[3].xyz();
    
    float angle = 90.0f - fov/2.f;

	planes[0] = Plane(eyePos + (near * lookAt), lookAt); // near plane

	planes[1] = Plane(eyePos + (far * lookAt), -lookAt); // far plane

    t = glm::rotate(glm::mat4(), -angle, glm::vec3(1, 0, 0));
	planes[2] = Plane(eyePos, lookAt).transform(t); // left plane

    t = glm::rotate(glm::mat4(), angle, glm::vec3(1, 0, 0));
	planes[3] = Plane(eyePos, lookAt).transform(t); // right plane

    t = glm::rotate(glm::mat4(), -angle, glm::vec3(0, 1, 0));
	planes[4] = Plane(eyePos, lookAt).transform(t); // top plane

    t = glm::rotate(glm::mat4(), -angle, glm::vec3(0, 1, 0));
	planes[5] = Plane(eyePos, lookAt).transform(t); // bottom plane
}

Frustum::Frustum(const glm::mat4 &transform, float left, float right,float bottom, float top,
                 float near, float far)
{
    glm::vec3 lookAt = -transform[2].xyz(), eyePos = transform[3].xyz(),
    side = transform[0].xyz(), up = transform[1].xyz();
    
	planes[0] = Plane(eyePos + (near * lookAt), lookAt); // near plane
    
	planes[1] = Plane(eyePos + (far * lookAt), -lookAt); // far plane
    
	planes[2] = Plane(eyePos + (left * -side), side); // left plane
    
    planes[3] = Plane(eyePos + (right * side), -side); // right plane
    
    
	planes[4] = Plane(eyePos + (top * up), -up); // top plane
    
	planes[5] = Plane(eyePos + (bottom * -up), up); // bottom plane
}
    
unsigned int AABB::intersect(const Triangle& t)
{	
	float triVerts [3][3] =	{	{t.v1[0],t.v1[1],t.v1[2]},
								{t.v2[0],t.v2[1],t.v2[2]},
								{t.v3[0],t.v3[1],t.v3[2]}
							};
	
	return triBoxOverlap(&center()[0],&halfExtents()[0],triVerts);
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
	
	if(min>boxhalfsize[X] || max<-boxhalfsize[X]) return 0;

	/* test in Y-direction */	
	FINDMINMAX(v0[Y],v1[Y],v2[Y],min,max);
	
	if(min>boxhalfsize[Y] || max<-boxhalfsize[Y]) return 0;
	
	/* test in Z-direction */
	
	FINDMINMAX(v0[Z],v1[Z],v2[Z],min,max);
	
	if(min>boxhalfsize[Z] || max<-boxhalfsize[Z]) return 0;
	
	/* Bullet 2: */	
	/*  test if the box intersects the plane of the triangle */	
	/*  compute plane equation of triangle: normal*x+d=0 */	
	CROSS(normal,e0,e1);	
	// -NJMP- (line removed here)	
	if(!planeBoxOverlap(normal,v0,boxhalfsize)) return 0;	// -NJMP-
	
	return 1;   /* box and triangle overlaps */
	
}

void transposeOpenGLMatrix(float src [16],float dst[16])
{	
	//row 1
	dst[0]=src[0];
	dst[4]=src[1];
	dst[8]=src[2];
	dst[12]=src[3];
	
	//row 2
	dst[1]=src[4];
	dst[5]=src[5];
	dst[9]=src[6];
	dst[13]=src[7];
	
	//row 3
	dst[2]=src[8];
	dst[6]=src[9];
	dst[10]=src[10];
	dst[14]=src[11];
	
	//row 4
	dst[3]=src[12];
	dst[7]=src[13];
	dst[11]=src[14];
	dst[15]=src[15];
}

}}//namespace