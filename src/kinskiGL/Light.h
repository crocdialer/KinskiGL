//
//  Light.h
//  kinskiGL
//
//  Created by Fabian on 6/18/13.
//
//

#ifndef __kinskiGL__Light__
#define __kinskiGL__Light__

#include "Object3D.h"

namespace kinski { namespace gl {
    
    class KINSKI_API Light : public Object3D
    {
    public:
        
        enum Type {DIRECTIONAL, POINT, SPOT};
        
        Light();
        virtual ~Light();
        
        void accept(Visitor &theVisitor);
        
    private:
        
        Type m_type;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Light__) */
