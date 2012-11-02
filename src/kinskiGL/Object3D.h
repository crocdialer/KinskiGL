//
//  Object3D.h
//  kinskiGL
//
//  Created by Fabian on 11/2/12.
//
//

#ifndef __kinskiGL__Object3D__
#define __kinskiGL__Object3D__

#include "KinskiGL.h"

namespace kinski { namespace gl {

    class Object3D
    {
    public:
        
        typedef std::shared_ptr<Object3D> Ptr;
        
        Object3D(const glm::mat4 &theTransform = glm::mat4());
        virtual ~Object3D(){};
        
        uint32_t getID() const { return m_id; };
        inline void setPosition(const glm::vec3 &thePos) { m_worldTransform[3].xyz() = thePos; };
        inline glm::vec3 getPosition() const { return m_worldTransform[3].xyz(); }
        
        void setRotation(const glm::quat &theRot);
        void setRotation(const glm::mat3 &theRot);
        glm::quat getRotation() const;
        
        inline void setTransform(const glm::mat4 &theTrans){ m_worldTransform = theTrans; };
        inline glm::mat4& getTransform() { return m_worldTransform; };
        inline const glm::mat4& getTransform() const { return m_worldTransform; };
        
    private:
        
        static uint32_t s_idPool;
        
        uint32_t m_id;
        glm::mat4 m_worldTransform;
    };

}}//namespace

#endif /* defined(__kinskiGL__Object3D__) */
