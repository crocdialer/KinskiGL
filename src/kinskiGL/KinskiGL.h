#ifndef _KINSKIGL_H
#define _KINSKIGL_H

//#define KINSKI_GLES
#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU

#include <GL/glfw.h>
#include <AntTweakBar.h>

#include <vector>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/static_assert.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

namespace kinski {

    using boost::int8_t;
    using boost::uint8_t;
    using boost::int16_t;
    using boost::uint16_t;
    using boost::int32_t;
    using boost::uint32_t;
    using boost::int64_t;
    using boost::uint64_t;
    
namespace gl{
 
 // implement cinder like draw routines here !?   
}}

#endif //_KINSKIGL_H