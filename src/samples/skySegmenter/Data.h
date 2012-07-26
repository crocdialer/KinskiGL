#ifndef _DATA_H_IS_INCLUDED
#define _DATA_H_IS_INCLUDED

const char *g_vertShaderSrc = 
"#version 150 core\n\
\n\
uniform mat4 u_modelViewProjectionMatrix;\n\
uniform mat4 u_textureMatrix;\n\
\n\
in vec4 a_position;\n\
in vec4 a_texCoord;\n\
\n\
out vec4 v_texCoord;\n\
\n\
void main()\n\
{\n\
\n\
v_texCoord =  u_textureMatrix * a_texCoord;\n\
\n\
gl_Position = u_modelViewProjectionMatrix * a_position;\n\
}";

const char *g_fragShaderSrc =
"#version 150 core\n\
\n\
uniform sampler2D   u_textureMap[];\n\
uniform vec4   u_color;\n\
in vec4        v_texCoord;\n\
\n\
out vec4 fragData;\n\
\n\
void main()\n\
{\n\
vec4 tex = texture(u_textureMap[0], v_texCoord.xy);\n\
\n\
fragData = tex ;\n\
\
}";

#endif //_DATA_H_IS_INCLUDED