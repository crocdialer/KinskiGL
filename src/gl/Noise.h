//
//  Noise.h
//  kinskiGL
//
//  Created by Croc Dialer on 25/08/15.
//
//

#ifndef __kinskiGL__Noise__
#define __kinskiGL__Noise__

#include "KinskiGL.h"

namespace kinski{ namespace gl{

class KINSKI_API Noise
{
 public:
    
    Noise(const vec2 &the_scale = vec2(0.05f), const vec2 &the_tex_size = vec2(128));
    
    gl::Texture simplex(const float the_seed);
    
    const vec2& tex_size() const;
    const vec2& scale() const;
    
    void set_tex_size(const vec2 &the_tex_size);
    void set_scale(const vec2 &the_scale);

 private:
    
    struct Impl;
    typedef std::shared_ptr<Impl> ImplPtr;
    ImplPtr m_impl;
};

}}// namespaces

#endif /* defined(__kinskiGL__Noise__) */
