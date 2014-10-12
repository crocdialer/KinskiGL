#version 150 core

uniform float u_time;
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

uniform struct
{
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
    vec4 emission;
    float shinyness;
} u_material;

uniform float u_textureMix;
in vec3 v_normal;
in vec3 v_eyeVec;
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

vec4 hotIron( in float value )
{
    vec4 color8  = vec4( 255.0 / 255.0, 255.0 / 255.0, 204.0 / 255.0, 1.0 );
    vec4 color7  = vec4( 255.0 / 255.0, 237.0 / 255.0, 160.0 / 255.0, 1.0 );
    vec4 color6  = vec4( 254.0 / 255.0, 217.0 / 255.0, 118.0 / 255.0, 1.0 );
    vec4 color5  = vec4( 254.0 / 255.0, 178.0 / 255.0,  76.0 / 255.0, 1.0 );
    vec4 color4  = vec4( 253.0 / 255.0, 141.0 / 255.0,  60.0 / 255.0, 1.0 );
    vec4 color3  = vec4( 252.0 / 255.0,  78.0 / 255.0,  42.0 / 255.0, 1.0 );
    vec4 color2  = vec4( 227.0 / 255.0,  26.0 / 255.0,  28.0 / 255.0, 1.0 );
    vec4 color1  = vec4( 189.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    vec4 color0  = vec4( 128.0 / 255.0,   0.0 / 255.0,  38.0 / 255.0, 1.0 );
    
    float colorValue = value * 8.0;
    int sel = int( floor( colorValue ) );
    
    if( sel >= 8 )
    {
        return color0;
    }
    else if( sel < 0 )
    {
        return color0;
    }
    else
    {
        colorValue -= float( sel );
        
        if( sel < 1 )
        {
            return ( color1 * colorValue + color0 * ( 1.0 - colorValue ) );
        }
        else if( sel < 2 )
        {
            return ( color2 * colorValue + color1 * ( 1.0 - colorValue ) );
        }
        else if( sel < 3 )
        {
            return ( color3 * colorValue + color2 * ( 1.0 - colorValue ) );
        }
        else if( sel < 4 )
        {
            return ( color4 * colorValue + color3 * ( 1.0 - colorValue ) );
        }
        else if( sel < 5 )
        {
            return ( color5 * colorValue + color4 * ( 1.0 - colorValue ) );
        }
        else if( sel < 6 )
        {
            return ( color6 * colorValue + color5 * ( 1.0 - colorValue ) );
        }
        else if( sel < 7 )
        {
            return ( color7 * colorValue + color6 * ( 1.0 - colorValue ) );
        }
        else if( sel < 8 )
        {
            return ( color8 * colorValue + color7 * ( 1.0 - colorValue ) );
        }
        else
        {
            return color0;
        }
    }
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
    // sample normal map
    //N = texture(u_textureMap[1], v_texCoord.xy).xyz * 2.0 - 1.0;
    
    float height = texture(u_sampler_2D[1], v_texCoord.xy).r;
    // scale and bias
    height = height * 0.08 - 0.0;
    
    vec3 L = normalize(-v_lightDir);
    vec3 E = normalize(v_eyeVec);
    
    // calculate parallax offset
    vec2 newCoords = mod(v_texCoord.xy + (E.xy * height), 1.0);
    vec4 texColors = texture(u_sampler_2D[0], newCoords);
    
    vec3 N = normalFromHeightMap(u_sampler_2D[1], newCoords, 0.8);
    //vec3 N = vec3(0, 0, 1);
    vec3 R = reflect(-L, N);
    
    
    float nDotL = max(0.0, dot(N, L));
    float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
    vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;
    fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0f)) + spec;
}

