#version 410 core
#extension GL_ARB_separate_shader_objects : enable

uniform int u_numTextures;
uniform sampler2D u_sampler_2D[3];

#define COLOR 0
#define EMISSION 1
#define DEPTH 2

struct Material
{
    vec4 diffuse;
    vec4 ambient;
    vec4 emission;
    vec4 point_vals;// (size, constant_att, linear_att, quad_att)
    float metalness;
    float roughness;
    float occlusion;
    int shadow_properties;
    int texture_properties;
};

layout(std140) uniform MaterialBlock
{
    Material u_material;
};

in VertexData
{
    vec4 color;
    vec2 texCoord;
} vertex_in;

out vec4 fragData;

uniform vec2 u_window_dimension;
uniform int u_show_edges = 0;
uniform int u_use_fxaa = 1;

uniform float u_luma_thresh = 0.5;
uniform float u_mulReduce = 1.0 / 256.0;
uniform float u_minReduce = 1.0 / 512.0;
uniform float u_maxSpan = 16.0;

//float linear_depth(float val)
//{
//    float zNear = 0.5;    // TODO: Replace by the zNear of your perspective projection
//    float zFar  = 2000.0; // TODO: Replace by the zFar  of your perspective projection
//    return (2.0 * zNear) / (zFar + zNear - val * (zFar - zNear));
//}

// fast approximate anti-aliasing, by the book
// @see http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
vec4 fxaa(sampler2D the_sampler, vec2 the_tex_coord)
{
    vec4 color = texture(the_sampler, the_tex_coord);
    vec3 rgbM = color.rgb;

    // sampling neighbour texels using offsets
    vec3 rgbNW = textureOffset(the_sampler, the_tex_coord, ivec2(-1, 1)).rgb;
    vec3 rgbNE = textureOffset(the_sampler, the_tex_coord, ivec2(1, 1)).rgb;
    vec3 rgbSW = textureOffset(the_sampler, the_tex_coord, ivec2(-1, -1)).rgb;
    vec3 rgbSE = textureOffset(the_sampler, the_tex_coord, ivec2(1, -1)).rgb;

    // determine texel-step size
    vec2 texel_step = 1.0 / textureSize(the_sampler, 0);

    // NTSC luma formula
    const vec3 toLuma = vec3(0.299, 0.587, 0.114);

    // Convert from RGB to luma.
    float lumaNW = dot(rgbNW, toLuma);
    float lumaNE = dot(rgbNE, toLuma);
    float lumaSW = dot(rgbSW, toLuma);
    float lumaSE = dot(rgbSE, toLuma);
    float lumaM = dot(rgbM, toLuma);

    // Gather minimum and maximum luma.
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // If contrast is lower than a maximum threshold ...
    if(lumaMax - lumaMin < lumaMax * u_luma_thresh)
    {
        // ... do no AA and return.
        return color;
    }

    // Sampling is done along the gradient.
    vec2 samplingDirection;
    samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    samplingDirection.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    // Sampling step distance depends on the luma: The brighter the sampled texels, the smaller the final sampling step direction.
    // This results, that brighter areas are less blurred/more sharper than dark areas.
    float samplingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * u_mulReduce, u_minReduce);

    // Factor for norming the sampling direction plus adding the brightness influence.
    float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + samplingDirectionReduce);

    // Calculate final sampling direction vector by reducing, clamping to a range and finally adapting to the texture size.
    samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-u_maxSpan, -u_maxSpan), vec2(u_maxSpan, u_maxSpan)) * texel_step;

    // Inner samples on the tab.
    vec3 rgbSampleNeg = texture(the_sampler, the_tex_coord + samplingDirection * (1.0/3.0 - 0.5)).rgb;
    vec3 rgbSamplePos = texture(the_sampler, the_tex_coord + samplingDirection * (2.0/3.0 - 0.5)).rgb;

    vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;

    // Outer samples on the tab.
    vec3 rgbSampleNegOuter = texture(the_sampler, the_tex_coord + samplingDirection * (0.0/3.0 - 0.5)).rgb;
    vec3 rgbSamplePosOuter = texture(the_sampler, the_tex_coord + samplingDirection * (3.0/3.0 - 0.5)).rgb;

    vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;

    // Calculate luma for checking against the minimum and maximum value.
    float lumaFourTab = dot(rgbFourTab, toLuma);

    // outer samples of the tab beyond the edge?
    vec4 ret = color;

    // use only two samples.
    if(lumaFourTab < lumaMin || lumaFourTab > lumaMax){ ret.rgb = rgbTwoTab; }
    // use four samples
    else{ ret.rgb = rgbFourTab; }

    // Show edges for debug purposes.
    if(u_show_edges != 0){ ret.r = 1.0; }
    return ret;
}

vec3 filmicTonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x*(A*x+C*B)+D*E) / (x*(A*x+B)+D*F))- E / F;
}

vec3 applyFilmicToneMap(vec3 color)
{
    color = 2.0 * filmicTonemap(color);
    vec3 whiteScale = 1.0 / filmicTonemap(vec3(11.2));
    color *= whiteScale;
    return color;
}

void main()
{
    vec4 texColors;
    if(u_use_fxaa != 0){ texColors = fxaa(u_sampler_2D[COLOR], vertex_in.texCoord.st); }
    else{ texColors = texture(u_sampler_2D[COLOR], vertex_in.texCoord.st); }
    // texColors.rgb = applyFilmicToneMap(texColors.rgb);

    float depth = texture(u_sampler_2D[DEPTH], vertex_in.texCoord.st).x;
    gl_FragDepth = depth;
    fragData = u_material.diffuse * texColors + vec4(texture(u_sampler_2D[EMISSION], vertex_in.texCoord.st).rgb, 1.0);
}
