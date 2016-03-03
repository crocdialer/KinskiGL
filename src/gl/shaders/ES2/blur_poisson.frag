precision mediump float;
precision lowp int;
uniform int u_numTextures;
uniform sampler2D u_textureMap[1];

uniform float u_poisson_radius = 3.0;
uniform vec2 u_window_dimension = vec2(1280, 720);

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

///////////////////////////// POISSON STUFF ///////////////////////////////////////////////////////
const int NUM_TAPS = 12;
vec2 fTaps_Poisson[NUM_TAPS];

float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

vec2 rot2d( vec2 p, float a )
{
	vec2 sc = vec2(sin(a),cos(a));
	return vec2( dot( p, vec2(sc.y, -sc.x) ), dot( p, sc.xy ) );
}

vec4 poisson_blur(in sampler2D the_texture, in vec2 tex_coord)
{
  float rnd = 6.28 * nrand(tex_coord);
  vec4 color_sum = vec4(0);

	vec4 basis = vec4( rot2d(vec2(1,0),rnd), rot2d(vec2(0,1),rnd) );
	for(int i = 0; i < NUM_TAPS; i++)
	{
		vec2 ofs = fTaps_Poisson[i]; ofs = vec2(dot(ofs,basis.xz),dot(ofs,basis.yw) );
		vec2 poisson_coord = tex_coord + u_poisson_radius * ofs / u_window_dimension;
    color_sum += texture2D(the_texture, poisson_coord);
  }
  return color_sum / NUM_TAPS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

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

  vec4 texColors = vec4(0, 0, 0, 1);//vertex_in.color;
  texColors = poisson_blur(u_sampler_2D[0], v_texCoord.st);

  gl_FragColor = vec4(mix(texColors.rgb, u_material.diffuse.rgb, u_material.diffuse.a), texColors.a);
}
