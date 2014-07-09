#version 150

// ------------------ Geometry Shader: Line -> Points --------------------------------
layout(lines) in;
layout (points, max_vertices = 72) out;

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

const int num_circle_points = 24;
const int num_turns = 3;
const int u_num_points = num_turns * num_circle_points;

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

vec3 unit_circle[num_circle_points] = vec3[](
vec3(0.0, 0.0, -1.0),
vec3(0.25881904510252074, 0.0, -0.9659258262890683),
vec3(0.49999999999999994, 0.0, -0.8660254037844387),
vec3(0.7071067811865475, 0.0, -0.7071067811865476),
vec3(0.8660254037844386, 0.0, -0.5000000000000001),
vec3(0.9659258262890683, 0.0, -0.25881904510252074),
vec3(1.0, 0.0, -6.123233995736766e-17),
vec3(0.9659258262890683, 0.0, 0.25881904510252085),
vec3(0.8660254037844388, 0.0, 0.4999999999999998),
vec3(0.7071067811865476, 0.0, 0.7071067811865475),
vec3(0.49999999999999994, 0.0, 0.8660254037844387),
vec3(0.258819045102521, 0.0, 0.9659258262890682),
vec3(1.2246467991473532e-16, 0.0, 1.0),
vec3(-0.2588190451025208, 0.0, 0.9659258262890683),
vec3(-0.5000000000000001, 0.0, 0.8660254037844386),
vec3(-0.7071067811865475, 0.0, 0.7071067811865477),
vec3(-0.8660254037844384, 0.0, 0.5000000000000004),
vec3(-0.9659258262890683, 0.0, 0.25881904510252063),
vec3(-1.0, 0.0, 1.8369701987210297e-16),
vec3(-0.9659258262890684, 0.0, -0.2588190451025203),
vec3(-0.8660254037844386, 0.0, -0.5),
vec3(-0.7071067811865477, 0.0, -0.7071067811865475),
vec3(-0.5000000000000004, 0.0, -0.8660254037844384),
vec3(-0.2588190451025207, 0.0, -0.9659258262890683));

void main()
{
    vertex_out.color = vertex_in[0].color;
    vertex_out.color.a = .7;

    vec3 p0 = vertex_in[0].position;	// start of current segment
    vec3 p1 = vertex_in[1].position;	// end of current segment
    
    float point_size = vertex_in[0].pointSize;
    
    // basevectors
    vec3 diff_vec = (p1 - p0); 
    float line_length = length(diff_vec);
    vec3 line_dir = diff_vec / line_length;
    
    // midpoint on line
    vec3 mid_point = p0 + 0.5 * diff_vec;
    vec3 up_vec = vertex_in[0].normal;

    vec3 offset = p0;
    vec3 point_step = diff_vec / u_num_points;

    // tmp vals for spiral creation
    float current_radius = 3.0, current_height = 0.0;

    // generate points
    for(int i = 0; i < u_num_points; i++)
    {
      points[i] = mid_point + unit_circle[i % num_circle_points] * current_radius 
        + up_vec * current_height;
      current_radius += 0.08;
      current_height += 0.3;
      //up_vec = mix(up_vec, vec3(0, 1, 0), 0.001);
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
      //vertex_out.color = mix(vertex_in[0].color, vec4(1), float(i) / float(u_num_points));
      gl_PointSize = 5 * point_size * attenuations[i];
      gl_Position = pp[i];
      EmitVertex();
      EndPrimitive();
    }
}
