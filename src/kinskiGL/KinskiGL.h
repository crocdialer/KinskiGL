#ifndef _KINSKIGL_H
#define _KINSKIGL_H

//#define KINSKI_GLES
#define GLFW_INCLUDE_GL3
#define GLFW_NO_GLU

#include <GL/glfw.h>
#include <AntTweakBar.h>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>

#include "kinskiCore/Definitions.h"
#include "kinskiCore/Exception.h"

#endif //_KINSKIGL_H