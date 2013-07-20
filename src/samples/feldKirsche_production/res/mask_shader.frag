#version 150 core

uniform sampler2D u_textureMap[2];

in VertexData{
   vec4 texCoord;
} vertex_in;

out vec4 fragData;

void main()
{
    float mask = texture(u_textureMap[1], vertex_in.texCoord.st).r; 
    if(mask == 0.0) discard;

    fragData = texture(u_textureMap[0], vertex_in.texCoord.st);
}
