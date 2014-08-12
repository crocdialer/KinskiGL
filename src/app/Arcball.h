//
//  Arcball.h
//  gl
//
//  Created by Fabian on 6/19/13.
//
//

#ifndef __gl__Arcball__
#define __gl__Arcball__

#include "App.h"

namespace kinski{ namespace gl {
    
    class Arcball {
    public:
        Arcball()
        {
            setNoConstraintAxis();
            resetQuat();
        }
        Arcball( const glm::ivec2 &aScreenSize )
		: m_windowSize( aScreenSize )
        {
            setCenter( glm::vec2( m_windowSize.x / 2.0f, m_windowSize.y / 2.0f ) );
            m_radius = std::min( (float)m_windowSize.x / 2, (float)m_windowSize.y / 2 );
            setNoConstraintAxis();
            m_currentQuat = m_initialQuat = glm::quat();
        }
        
        void mouseDown( const glm::ivec2 &mousePos )
        {
            m_initialMousePos = mousePos;
            m_initialQuat = m_currentQuat;
        }
        
        void mouseDrag( const glm::ivec2 &mousePos )
        {
            glm::vec3 from = mouseOnSphere( m_initialMousePos );
            glm::vec3 to = mouseOnSphere( mousePos );
            if( m_useConstraint )
            {
                from = constrainToAxis( from, m_constraintAxis );
                to = constrainToAxis( to, m_constraintAxis );
            }
            
            glm::vec3 axis = glm::cross(from, to);
            m_currentQuat = m_initialQuat * glm::quat( glm::dot(from, to), axis.x, axis.y, axis.z);
            m_currentQuat = glm::normalize(m_currentQuat);
        }
        
        void	resetQuat() { m_currentQuat = m_initialQuat = glm::quat(); }
        glm::quat	getQuat() { return m_currentQuat; }
        void	setQuat( const glm::quat &quat ) { m_currentQuat = quat; }
        
        void	setWindowSize( const glm::vec2 &aWindowSize ) { m_windowSize = aWindowSize; }
        void	setCenter( const glm::vec2 &aCenter ) { m_center = aCenter; }
        glm::vec2	getCenter() const { return m_center; }
        void	setRadius( float aRadius ) { m_radius = aRadius; }
        float	getRadius() const { return m_radius; }
        void	setConstraintAxis( const glm::vec3 &aConstraintAxis )
        {
            m_constraintAxis = aConstraintAxis;
            m_useConstraint = true;
        }
        void	setNoConstraintAxis() { m_useConstraint = false; }
        bool	isUsingConstraint() const { return m_useConstraint; }
        glm::vec3	getConstraintAxis() const { return m_constraintAxis; }
        
        glm::vec3 mouseOnSphere( const glm::ivec2 &point )
        {
            glm::vec3 result;
            
            result.x = ( point.x - m_center.x ) / ( m_radius * 2 );
            result.y = ( point.y - m_center.y ) / ( m_radius * 2 );
            result.z = 0.0f;
            
            float mag = glm::length2(result);
            if( mag > 1.0f )
            {
                result = glm::normalize(result);
            }
            else
            {
                result.z = sqrtf( 1.0f - mag );
                result = glm::normalize(result);
            }
            return result;
        }
        
    private:
        // Force sphere point to be perpendicular to axis
        glm::vec3 constrainToAxis( const glm::vec3 &loose, const glm::vec3 &axis )
        {
            float norm;
            glm::vec3 onPlane = loose - axis * glm::dot(axis, loose);
            norm = glm::length2(onPlane);
            
            if( norm > 0.0f )
            {
                if( onPlane.z < 0.0f )
                    onPlane = -onPlane;
                return ( onPlane * ( 1.0f / sqrtf( norm ) ) );
            }
            
            if( glm::dot(axis, glm::vec3(0, 0, 1) ) < 0.0001f )
            {
                onPlane = glm::vec3(1, 0, 0);
            }
            else
            {
                onPlane = glm::normalize(glm::vec3(-axis.y, axis.x, 0.0f));
            }
            return onPlane;
        }
        
        glm::vec2		m_windowSize;
        glm::ivec2		m_initialMousePos;
        glm::vec2		m_center;
        glm::quat		m_currentQuat, m_initialQuat;
        float           m_radius;
        glm::vec3		m_constraintAxis;
        bool            m_useConstraint;
    };
    
}}// namespace


#endif /* defined(__gl__Arcball__) */
