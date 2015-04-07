#version 410

struct Material
{
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec4 emission; 
  vec4 point_vals;// (size, constant_att, linear_att, quad_att) 
  float shinyness;
};

struct Lightsource
{
  vec3 position; 
  int type; 
  vec4 diffuse; 
  vec4 ambient; 
  vec4 specular; 
  vec3 spotDirection; 
  float spotCosCutoff; 
  float spotExponent; 
  float constantAttenuation; 
  float linearAttenuation; 
  float quadraticAttenuation; 
};

vec3 projected_coords(in vec4 the_lightspace_pos)
{
    vec3 proj_coords = the_lightspace_pos.xyz / the_lightspace_pos.w;
    proj_coords = (vec3(1) + proj_coords) * 0.5;
    return proj_coords;
}

vec4 shade(in Lightsource light, in Material mat, in vec3 normal, in vec3 eyeVec, in vec4 base_color,
           float shade_factor)
{
  vec3 lightDir = light.type > 0 ? (light.position - eyeVec) : -light.position; 
  vec3 L = normalize(lightDir); 
  vec3 E = normalize(-eyeVec); 
  vec3 R = reflect(-L, normal); 
  vec4 ambient = mat.ambient * light.ambient; 
  float att = 1.0; 
  float nDotL = dot(normal, L); 
  
  if (light.type > 0)
  {
    float dist = length(lightDir); 
    att = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist); 
    
    if(light.type > 1)
    {
      float spotEffect = dot(normalize(light.spotDirection), -L); 
      
      if (spotEffect < light.spotCosCutoff)
      {
        att = 0.0;
        base_color * ambient; 
      }
      spotEffect = pow(spotEffect, light.spotExponent); 
      att *= spotEffect; 
    }
  }
  nDotL = max(0.0, nDotL); 
  float specIntesity = clamp(pow( max(dot(R, E), 0.0), mat.shinyness), 0.0, 1.0); 
  vec4 diffuse = mat.diffuse * light.diffuse * vec4(att * shade_factor * vec3(nDotL), 1.0); 
  vec4 spec = shade_factor * att * mat.specular * light.specular * specIntesity; 
  spec.a = 0.0; 
  return base_color * (ambient + diffuse) + spec; 
}


//uniform Material u_material;
layout(std140) uniform MaterialBlock
{
  Material u_material;
};

layout(std140) uniform LightBlock
{
  int u_numLights;
  Lightsource u_lights[16];
};

// regular textures
uniform int u_numTextures;
uniform sampler2D u_sampler_2D[4];

#define NUM_SHADOW_LIGHTS 2
#define EPSILON 0.00001

//struct Shadow
//{
//    mat4 matrix;
//    int shadow_map;
//    vec2 map_size;
//    float poisson_radius;
//};
//layout(std140) uniform ShadowBlock
//{
//  Shadow u_shadow[NUM_SHADOW_LIGHTS];
//};

uniform sampler2D u_shadow_map[NUM_SHADOW_LIGHTS];
uniform vec2 u_shadow_map_size = vec2(1024);
uniform float u_poisson_radius = 5.0;

in VertexData
{
  vec4 color;
  vec4 texCoord;
  vec3 normal; 
  vec3 eyeVec; 
  vec4 lightspace_pos[NUM_SHADOW_LIGHTS];
} vertex_in;

out vec4 fragData; 

float nrand( vec2 n ) 
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

vec2 rot2d( vec2 p, float a ) 
{
	vec2 sc = vec2(sin(a),cos(a));
	return vec2( dot( p, vec2(sc.y, -sc.x) ), dot( p, sc.xy ) );
}

const int NUM_TAPS = 12;
vec2 fTaps_Poisson[NUM_TAPS];
	
float shadow_factor(in sampler2D shadow_map, in vec3 light_space_pos)
{
  float rnd = 6.28 * nrand(light_space_pos.xy);
  float factor = 0.0;

	vec4 basis = vec4( rot2d(vec2(1,0),rnd), rot2d(vec2(0,1),rnd) );
	for(int i = 0; i < NUM_TAPS; i++)
	{
		vec2 ofs = fTaps_Poisson[i]; ofs = vec2(dot(ofs,basis.xz),dot(ofs,basis.yw) );
		//vec2 ofs = rot2d( fTaps_Poisson[i], rnd );
		vec2 texcoord = light_space_pos.xy + u_poisson_radius * ofs / u_shadow_map_size;
    float depth = texture(shadow_map, texcoord).x;
    bool is_in_shadow = depth < (light_space_pos.z - EPSILON);
    factor += is_in_shadow ? 0 : 1;
  }
  return factor / NUM_TAPS;
  //float xOffset = 1.0/u_shadow_map_size.x;
  //float yOffset = 1.0/u_shadow_map_size.y;

  //for(int y = -2 ; y <= 2 ; y++) 
  //{
  //  for (int x = -2 ; x <= 2 ; x++) 
  //  {
  //    vec2 offset = vec2(x * xOffset, y * yOffset);
  //    float depth = texture(u_shadow_map[shadow_index], proj_coords.xy + offset).x;
  //    bool is_in_shadow = depth < (proj_coords.z - EPSILON);
  //    factor += is_in_shadow ? 0 : 1;
  //  }
  //}
  //return (0.5 + (factor / 50.0));
}

void main() 
{
	fTaps_Poisson[0]  = vec2(-.326,-.406);
	fTaps_Poisson[1]  = vec2(-.840,-.074);
	fTaps_Poisson[2]  = vec2(-.696, .457);
	fTaps_Poisson[3]  = vec2(-.203, .621);
	fTaps_Poisson[4]  = vec2( .962,-.195);
	fTaps_Poisson[5]  = vec2( .473,-.480);
	fTaps_Poisson[6]  = vec2( .519, .767);
	fTaps_Poisson[7]  = vec2( .185,-.893);
	fTaps_Poisson[8]  = vec2( .507, .064);
	fTaps_Poisson[9]  = vec2( .896, .412);
	fTaps_Poisson[10] = vec2(-.322,-.933);
	fTaps_Poisson[11] = vec2(-.792,-.598);

  vec4 texColors = vec4(1); 
  
  for(int i = 0; i < u_numTextures; i++) 
    texColors *= texture(u_sampler_2D[i], vertex_in.texCoord.st); 
  
  vec3 normal = normalize(vertex_in.normal); 
  vec4 shade_color = vec4(0); 
  
  float factor[2];
  factor[0] = shadow_factor(u_shadow_map[0], projected_coords(vertex_in.lightspace_pos[0]));
  factor[1] = shadow_factor(u_shadow_map[1], projected_coords(vertex_in.lightspace_pos[1]));
  
  float min_shade = 0.1, max_shade = 1.0;
  factor[0] = mix(min_shade, max_shade, factor[0]);
  factor[1] = mix(min_shade, max_shade, factor[1]);

  for(int i = 0; i < NUM_SHADOW_LIGHTS; i++)
  {
    shade_color += shade(u_lights[i], u_material, normal, vertex_in.eyeVec, texColors, factor[i]);
  }

  fragData = shade_color; 
}
