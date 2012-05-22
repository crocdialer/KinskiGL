#pragma once

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <gl/gl.h>
#endif

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
