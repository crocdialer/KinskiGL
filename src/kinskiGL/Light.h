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
        
        //! what it is, what it is
        enum Type {DIRECTIONAL = 0, POINT = 1, SPOT = 2};
        
        //! Helper struct to bundle the attenuation params
        struct Attenuation
        {
            float constant, linear, quadratic;
            Attenuation(float c, float l, float q):constant(c), linear(l), quadratic(q){}
        };
        
        Light(Type theType);
        virtual ~Light();
        
        const Color& diffuse() const { return m_diffuse; };
        const Color& ambient() const { return m_ambient; };
        const Color& specular() const { return m_specular; };
        
        void set_diffuse(const Color &theColor);
        void set_ambient(const Color &theColor);
        void set_specular(const Color &theColor);
        
        const Attenuation& attenuation() const;
        void set_attenuation(const Attenuation &theAttenuation);
        void get_attenuation(float &constant, float &linear, float &quadratic) const;
        void set_attenuation(float constant, float linear, float quadratic);
        
        Type type() const {return m_type;}
        void set_type(Type theType){m_type = theType;}
        
        bool enabled() const {return m_enabled;}
        void set_enabled(bool b = true){m_enabled = b;}
        
        void accept(Visitor &theVisitor);
        
    private:
        
        Type m_type;
        Attenuation m_attenuation;
        Color m_ambient, m_diffuse, m_specular;
        bool m_enabled;
    };
    
}}//namespace

#endif /* defined(__kinskiGL__Light__) */
