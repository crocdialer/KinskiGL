#version 150 core

uniform int u_numTextures;
uniform sampler2D u_textureMap[2];
uniform vec2 u_window_size;
uniform float u_focus;
uniform float u_dof_bias;	//aperture - bigger values for shallower depth of field

const float blurclamp = 3.0;  // max blur amount

in VertexData{
    vec4 color;
    vec4 texCoord;
} vertex_in;

out vec4 fragData;
 
void main() 
{

	float aspectratio = u_window_size.x/u_window_size.y;
	vec2 aspectcorrect = vec2(1.0,aspectratio);

	vec4 depth1   = texture(u_textureMap[1],vertex_in.texCoord.xy );
	float factor = ( depth1.x - u_focus );
	vec2 dofblur = vec2 (clamp( factor * u_dof_bias, -blurclamp, blurclamp ));
    
    vec4 center_color = texture(u_textureMap[0], vertex_in.texCoord.xy);
	vec4 col = vec4(0.0);
	
	col += center_color;
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,0.4 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.15,0.37 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,0.29 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.37,0.15 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.4,0.0 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.37,-0.15 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,-0.29 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.15,-0.37 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,-0.4 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.15,0.37 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,0.29 )*aspectcorrect) * dofblur);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.37,0.15 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.4,0.0 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.37,-0.15 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,-0.29 )*aspectcorrect) * dofblur);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.15,-0.37 )*aspectcorrect) * dofblur);
	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.15,0.37 )*aspectcorrect) * dofblur*0.9);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.37,0.15 )*aspectcorrect) * dofblur*0.9);		
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.37,-0.15 )*aspectcorrect) * dofblur*0.9);		
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.15,-0.37 )*aspectcorrect) * dofblur*0.9);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.15,0.37 )*aspectcorrect) * dofblur*0.9);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.37,0.15 )*aspectcorrect) * dofblur*0.9);		
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.37,-0.15 )*aspectcorrect) * dofblur*0.9);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.15,-0.37 )*aspectcorrect) * dofblur*0.9);	
	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,0.29 )*aspectcorrect) * dofblur*0.7);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.4,0.0 )*aspectcorrect) * dofblur*0.7);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,-0.29 )*aspectcorrect) * dofblur*0.7);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,-0.4 )*aspectcorrect) * dofblur*0.7);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,0.29 )*aspectcorrect) * dofblur*0.7);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.4,0.0 )*aspectcorrect) * dofblur*0.7);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,-0.29 )*aspectcorrect) * dofblur*0.7);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,0.4 )*aspectcorrect) * dofblur*0.7);
			 
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,0.29 )*aspectcorrect) * dofblur*0.4);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.4,0.0 )*aspectcorrect) * dofblur*0.4);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.29,-0.29 )*aspectcorrect) * dofblur*0.4);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,-0.4 )*aspectcorrect) * dofblur*0.4);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,0.29 )*aspectcorrect) * dofblur*0.4);
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.4,0.0 )*aspectcorrect) * dofblur*0.4);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( -0.29,-0.29 )*aspectcorrect) * dofblur*0.4);	
	col += texture(u_textureMap[0], vertex_in.texCoord.xy + (vec2( 0.0,0.4 )*aspectcorrect) * dofblur*0.4);	
			
	fragData = col/41.0;
	fragData.a = center_color.a;
}
