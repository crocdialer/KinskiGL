#version 330

// ------------------ Geometry Shader: Line -> Cuboid --------------------------------
layout(lines) in;
layout (line_strip, max_vertices = 4) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;

uniform float u_split_limit = 60.0;
uniform float u_cap_bias = 5.0;

in VertexData 
{
    vec3 position;
    vec3 normal;
    vec4 color;
    vec2 texCoord;
    float pointSize;
} vertex_in[2];

out VertexData 
{
  vec4 color;
  vec4 texCoord;
  vec3 normal;
  vec3 eyeVec;
} vertex_out;

void main()
{
    vertex_out.color = vertex_in[0].color;
    vertex_out.texCoord = vec4(0, 0, 0, 0);

    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = vertex_in[1].position;	// end of current segment
    
    // basevectors for the cuboid
    vec3 line_seq = p1 - p0;
    float line_length = length(line_seq);
    vec3 line_dir = line_seq / line_length;

    // one large cuboid without caps
    if(line_length < u_split_limit)
    {
      // pass thru
      vec3 v[2] = vec3[](p0, p1);
      vec4 vp[2];

      // projected coords
      for(int i = 0; i < 2 ; i++)
      {
        vp[i] = u_modelViewProjectionMatrix * vec4(v[i], 1.0);
      }
      
      gl_Position = vp[0];
      EmitVertex();
      gl_Position = vp[1];      
      EmitVertex();
      EndPrimitive();
    }
    else
    {
      // 2 smaller lines
      // generate 4 vertes
      vec3 v[4] = vec3[]
      ( 
        p0 - line_dir * u_cap_bias, p0 + line_dir * u_cap_bias,
        p1 - line_dir * u_cap_bias, p1 + line_dir * u_cap_bias
      );
      vec4 vp[4];

      // projected coords
      for(int i = 0; i < 4 ; i++)
      {
        vp[i] = u_modelViewProjectionMatrix * vec4(v[i], 1.0);
      }

      vertex_out.texCoord = vec4(0, 0.5, 0, 0);
      gl_Position = vp[0];      
      EmitVertex();
      vertex_out.texCoord = vec4(1, 0.5, 0, 0);
      gl_Position = vp[1];      
      EmitVertex();
      EndPrimitive();

      vertex_out.texCoord = vec4(0, 0.5, 0, 0);
      gl_Position = vp[2];
      EmitVertex();
      vertex_out.texCoord = vec4(1, 0.5, 0, 0);
      gl_Position = vp[3];
      EmitVertex();
      EndPrimitive();
      
    }
}
