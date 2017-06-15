uniform mat4 u_modelViewProjectionMatrix;
uniform mat4 u_textureMatrix;

// TL, TR, BL, BR
uniform vec2 u_control_points[25];
uniform vec2 u_num_subdivisions;

attribute vec4 a_vertex;
attribute vec4 a_texCoord;
attribute vec4 a_color;

varying lowp vec4 v_color;
varying lowp vec4 v_texCoord;

void main()
{
    // find indices of relevant control_points
    vec2 pos = a_vertex.xy * u_num_subdivisions;

    // find TL corner
    int row_step = int(u_num_subdivisions.x + 1.0);
    int index_tl = int(pos.x) + row_step * int(u_num_subdivisions.y - floor(pos.y) - 1.0);
    int index_bl = index_tl + row_step;

    // interpolate top edge x coordinate
    vec2 x2 = mix(u_control_points[index_tl], u_control_points[index_tl + 1],
                  fract(pos.x));

    // interpolate bottom edge x coordinate
    vec2 x1 = mix(u_control_points[index_bl], u_control_points[index_bl + 1],
                  fract(pos.x));

    // interpolate y position
    vec2 p = mix(x1, x2, fract(pos.y));

    v_color = a_color;
    v_texCoord =  u_textureMatrix * a_texCoord;
    gl_Position = u_modelViewProjectionMatrix * vec4(p, 0, 1);
}
