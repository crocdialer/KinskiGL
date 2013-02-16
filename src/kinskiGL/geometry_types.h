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
    
enum intersection_type {REJECT = 0, INTERSECT = 1, INSIDE = 2};

struct Ray
{
    glm::vec3 origin;
	glm::vec3 direction;
    
    Ray(const glm::vec3& theOrigin, const glm::vec3& theDir):
    origin(theOrigin), direction(glm::normalize(theDir)){}
    
    inline Ray& transform(const glm::mat4& t)
	{
		origin += t[3].xyz();
		direction = glm::mat3(t) * direction;
		return *this;
	};
    
    inline Ray transform(const glm::mat4& t) const
	{
        Ray ret = *this;
		return ret.transform(t);
	};
    
    inline friend glm::vec3 operator*(const Ray &theRay, float t)
    {
        return theRay.origin + t * theRay.direction;
    }
    
    inline friend glm::vec3 operator*(float t, const Ray &theRay)
    {
        return theRay.origin + t * theRay.direction;
    }
};
    
struct Plane
{
    glm::vec3 foot;
	glm::vec3 normal;
    
	Plane();
	Plane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
	Plane(const glm::vec3& f, const glm::vec3& n);
	
	inline float distance(const glm::vec3& p) const
	{
		return glm::dot(foot - p, normal);
	};
	
	inline Plane& transform(const glm::mat4& t)
	{
		foot += t[3].xyz();
		normal = glm::mat3(t) * normal;
		return *this;
	};
    
    inline Plane transform(const glm::mat4& t) const
	{
        Plane ret = *this;
		return ret.transform(t);
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
    
    inline Triangle& transform(const glm::mat4& t)
	{
		v1 = (glm::vec4(v1, 1.0f) * t).xyz();
		v1 = (glm::vec4(v2, 1.0f) * t).xyz();
        v1 = (glm::vec4(v3, 1.0f) * t).xyz();
		return *this;
	};
    
    inline Triangle transform(const glm::mat4& t) const
	{
        Triangle ret = *this;
		return ret.transform(t);
	};
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
    
    inline Sphere& transform(const glm::mat4& t)
	{
		center = (glm::vec4(center, 1.0f) * t).xyz();;
		return *this;
	};
    
    inline Sphere transform(const glm::mat4& t) const
	{
        Sphere ret = *this;
		return ret.transform(t);
	};
    
    inline uint32_t intersect(const Ray &theRay)
    {
        glm::vec3 l = center - theRay.origin;
        float s = glm::dot(l, theRay.direction);
        float l2 = glm::dot(l, l);
        float r2 = radius * radius;
        if(s < 0 && l2 > r2) return REJECT;
        float m2 = l2 - s * s;
        if(m2 > r2) return REJECT;
        /*
        float q = sqrtf(r2 - m2);
        float t;
        if(l2 > r2) t = s - q;
        else t = s + q;
        glm::vec3 intersect_point = theRay * t;
        */
        return INTERSECT;
    }
};

/*
 *simple Axis aligned bounding box (AABB) structure
 */
struct AABB
{
	
	glm::vec3 min;
	glm::vec3 max;
	
    AABB():min(glm::vec3(0)), max(glm::vec3(0)){};
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
    
    inline AABB transform(const glm::mat4& t) const
    {
        AABB ret = *this;
        return ret.transform(t);
    }
    
    uint32_t intersect(const Ray& theRay);
    
	uint32_t intersect(const Triangle& t);
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
				return REJECT;
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
				return REJECT ;
			
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