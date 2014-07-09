#version 150

// ------------------ Geometry Shader: Line -> Points --------------------------------
layout(lines) in;
layout (points, max_vertices = 18) out;

uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;

struct PointAttenuation
{
  float constant;
  float linear;
  float quadratic;
};
uniform PointAttenuation u_point_attenuation;

uniform float u_cap_bias = 2;
const int u_num_points = 5;

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
} vertex_out;

// point array
vec3 points[u_num_points];

// points in eye coordinates
vec3 eye_vecs[u_num_points];

// projected points
vec4 pp[u_num_points];

// point attenuations
float attenuations[u_num_points];

void main()
{
    vertex_out.color = vertex_in[0].color;

    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = vertex_in[1].position;	// end of current segment
    
    float point_size = vertex_in[0].pointSize;
    
    // basevectors
    vec3 diff_vec = (p1 - p0); 
    float line_length = length(diff_vec);
    vec3 line_dir = diff_vec / line_length;

    vec3 offset = p0;
    vec3 point_step = diff_vec / u_num_points;
    for(int i = 0; i < u_num_points; i++)
    {
      points[i] = offset;
      offset += point_step;
    }
    
    // calculate projected points
    for(int i = 0; i < u_num_points; i++){pp[i] = u_modelViewProjectionMatrix * vec4(points[i], 1);}
   
    // calculate eye coords and attenuations
    for(int i = 0; i < u_num_points; i++)
    {
      eye_vecs[i] = -(u_modelViewMatrix * vec4(points[i], 1)).xyz; 

      float d = length(eye_vecs[i]);
      attenuations[i] = 1.0 / (1.0 +
                        u_point_attenuation.linear * d +
                        u_point_attenuation.quadratic * (d * d));
    }

    // emit primitives
    for(int i = 0; i < u_num_points; i++)
    {
      gl_PointSize = 10 * point_size * attenuations[i];
      gl_Position = pp[i];
      EmitVertex();
      EndPrimitive();
    }
}
