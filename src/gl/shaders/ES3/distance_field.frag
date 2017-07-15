precision mediump float;
precision lowp int;

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[1];
uniform float u_buffer;
uniform float u_gamma;

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float shinyness;
};
uniform Material u_material;

varying vec4 v_color;
varying vec4 v_texCoord;

void main(void)
{
    vec4 color = v_color * u_material.diffuse;
    float dist = texture2D(u_sampler_2D[0], v_texCoord.st).r;
    float alpha = smoothstep(u_buffer - u_gamma, u_gamma, dist);
    gl_FragColor = vec4(color.rgb, color.a * alpha);
}
