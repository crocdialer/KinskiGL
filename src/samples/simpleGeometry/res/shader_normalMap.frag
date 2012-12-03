#version 150 core

uniform int u_numTextures;
uniform sampler2D u_textureMap[16];

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;

} u_material;

uniform float u_textureMix;


in vec3 v_normal;
in vec3 v_lightDir;
in vec4 v_texCoord;


out vec4 fragData;

vec4 jet(in float val)
{
    return vec4(min(4.0 * val - 1.5, -4.0 * val + 4.5),
                min(4.0 * val - 0.5, -4.0 * val + 3.5),
                min(4.0 * val + 0.5, -4.0 * val + 2.5),
                1.0);
}

vec3 hsv2rgb(in vec3 hsv) 
{ return mix(vec3(1.),clamp((abs(fract(hsv+vec3(3.,2.,1.)/3.)*6.-3.)-1.),0.,1.),hsv)*hsv; }

vec4 hueRotate(in vec4 color, in float dH)
{
    const mat3 rgb2yiq = mat3(  0.299, 0.587, 0.114,
                                0.595716, -0.274453, -0.321263,
                                0.211456, -0.522591, 0.311135);
                                
    const mat3 yiq2rgb = mat3(  1.0, 0.9563, 0.6210,
                                1.0, -0.2721, -0.6474,
                                1.0, -1.1070, 1.7046);


    //convert to yiq
    vec3 yiq = rgb2yiq * color.rgb;

    // Calculate the hue and chroma
    float   hue     = atan (yiq.z, yiq.y);
    float   chroma  = sqrt (yiq.y * yiq.y + yiq.z * yiq.z);

    // Make the user's adjustments
    hue += dH;

    // Convert back to YIQ
    yiq.z = chroma * sin (hue);
    yiq.y = chroma * cos (hue);

    // Convert back to RGB
    vec3 rgb = yiq2rgb * yiq;

    return vec4(rgb, color.a);
}

float gray(in vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 normalFromHeightMap(sampler2D theMap, vec2 theCoords, float theStrength)
{
    float center = texture(theMap, theCoords).r ;	 //center bump map sample
    float U = texture(theMap, theCoords + vec2( 0.005, 0)).r ;	//U bump map sample
    float V = texture(theMap, theCoords + vec2(0, 0.005)).r ;	 //V bump map sample
    float dHdU = U - center;	 //create bump map U offset
    float dHdV = V - center;	 //create bump map V offset
    vec3 normal = vec3( -dHdU, dHdV, 0.05 / theStrength);	 //create the tangent space normal
    
    return normalize(normal);
}

void main()
{
    vec4 color1 = texture(u_textureMap[0], v_texCoord.xy);
    
    vec3 n;
    // sample normal map
    n = texture(u_textureMap[1], v_texCoord.xy).xyz * 2.0 - 1.0;
    
    // sample bump map
    //n = normalFromHeightMap(u_textureMap[1], v_texCoord.xy, 0.1);

    
    float nDotL = max(0.0, dot(n, normalize(-v_lightDir)));

    fragData = mix(u_material.diffuse * color1 * vec4(vec3(nDotL), 1.0), jet(nDotL), u_textureMix);
}

