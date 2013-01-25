/*
 * Geometry.h
 *
 *  Created on: Sep 21, 2008
 *      Author: Fabian
 */

#ifndef KINSKI_GL_GEOMETRY_TYPES_H_
#define KINSKI_GL_GEOMETRY_TYPES_H_

#include "KinskiGL.h"

namespace kinski { namespace gl {
    
enum intersection_type {OUTSIDE = 0, INTERSECT = 1, INSIDE = 2};
    
struct Plane
{
    glm::vec3 foot;
	glm::vec3 normal;
	Plane();
	Plane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	Plane(const glm::vec3& f, const glm::vec3& n);
	
	inline float distance(const glm::vec3& p)
	{
		return glm::dot(foot - p, normal);
	};
	
	inline Plane& transform(const glm::mat4& t)
	{
		foot += t[3].xyz();
		normal = glm::mat3(t) * normal;
		return *this;
	};
};

struct Triangle
{
	glm::vec3 v1 ;
	glm::vec3 v2 ;
	glm::vec3 v3 ;
	
	Triangle(const glm::vec3& _v1, const glm::vec3& _v2, const glm::vec3& _v3)
	:v1(_v1),v2(_v2),v3(_v3)
	{}
};

struct Sphere
{

	glm::vec3 center;
	float radius;

	Sphere(glm::vec3 c,float r)
	{
		center = c;
		radius = r;
	};
};

/*
 *simple Axis aligned bounding box (AABB) structure
 */
struct AABB
{
	
	glm::vec3 min;
	glm::vec3 max;
	
    AABB(){};
	AABB(const glm::vec3& center,
         const glm::vec3& halfExtents)
	{
		min = center - halfExtents;
		max = center + halfExtents;
	}

	inline float width() const
	{
		return max.x - min.x;
	}
	inline float height() const
	{
		return max.y - min.y;
	}
	inline float depth() const
	{
		return max.z - min.z;
	}
	inline glm::vec3 halfExtents() const
	{
		return (max - min) / 2.f ;
	}
	inline glm::vec3 center() const
	{
		return max - halfExtents() ;
	}
	
	/* used for fast AABB <-> Plane intersection test */
	inline glm::vec3 posVertex(const glm::vec3& dir) const
	{
		glm::vec3 ret = min;
		
		if (dir.x >= 0)
			ret.x = max.x;
		if (dir.y >= 0)
			ret.y = max.y;
		if (dir.z >= 0)
			ret.z = max.z;
			
		return ret;
	}
	
	/* used for fast AABB <-> Plane intersection test */
	inline glm::vec3 negVertex(const glm::vec3& dir) const
	{
		glm::vec3 ret = max;
		
		if (dir.x >= 0)
			ret.x = min.x;
		if (dir.y >= 0)
			ret.y = min.y;
		if (dir.z >= 0)
			ret.z = min.z;
		
		return ret;
	}
	
    AABB& transform(const glm::mat4& t);
    
	unsigned int intersect(const Triangle& t);
};

struct Frustum
{

	Plane planes [6];

	Frustum(){};
	Frustum(const glm::mat4 &transform, float fov, float near, float far);
    Frustum(const glm::mat4 &transform, float left, float right,float bottom, float top,
            float near, float far);
	
	inline Frustum& transform(const glm::mat4& t)
	{	
		Plane* end = planes+6 ;
		for (Plane *p = planes; p < end; p++)
			p->transform(t);
		
		return *this;
	}
	
	inline uint32_t intersect(const Sphere& s)
	{	
		Plane* end = planes+6 ;
		for (Plane *p = planes; p < end; p++)
		{
			
			if (p->distance(s.center) > s.radius)
				return OUTSIDE;
		}
		return INSIDE;
	};
	
	inline uint32_t intersect(const AABB& aabb)
	{	
		uint32_t ret = INSIDE ;

		Plane* end = planes+6 ;
		for (Plane *p = planes; p < end; p++)
		{
			//positive vertex outside ?
			if (p->distance(aabb.posVertex(p->normal) ) > 0)
				return OUTSIDE ;
			
			//negative vertex outside ?
			else if(p->distance(aabb.negVertex(p->normal) ) > 0)
				ret = INTERSECT ;
		}
		return ret;
	};
};

/******************** Utility Section *******************/

/* fast AABB <-> Triangle test from Tomas Akenine-Möller */
int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3]);

}}//namespace

#endif /* KINSKI_GL_GEOMETRY_TYPES_H_ */