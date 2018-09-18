precision mediump float;
precision lowp int;
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[2];

#define COLORMAP 0
#define MASK 1

struct Material
{
    vec4 diffuse;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    float occlusion;
    int shadow_properties;
    int texture_properties;
};
uniform Material u_material;

varying vec4 v_color;
varying vec4 v_texCoord;

void main(void)
{
    vec4 texColors = v_color;
    if(u_numTextures > 0)
        texColors *= texture2D(u_sampler_2D[COLORMAP], v_texCoord.st);

    float mask = texture2D(u_sampler_2D[MASK], v_texCoord.st).x;
    texColors.a *= mask;
    gl_FragColor = u_material.diffuse * texColors;
}
