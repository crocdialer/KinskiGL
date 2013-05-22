// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include "KinskiGL.h"
#include "Shader.h"

#define STRINGIFY(A) #A

namespace kinski { namespace gl {
    
    Shader createShaderFromFile(const std::string &vertPath,
                                const std::string &fragPath,
                                const std::string &geomPath)
    {
        Shader ret;
        std::string vertSrc, fragSrc, geomSrc;
        vertSrc = readFile(vertPath);
        fragSrc = readFile(fragPath);
        
        if (!geomPath.empty()) geomSrc = readFile(geomPath);
        
        ret.loadFromData(vertSrc.c_str(), fragSrc.c_str(), geomSrc.empty() ? NULL : geomSrc.c_str());
        return ret;
    }
    
    Shader createShader(ShaderType type)
    {
#ifdef KINSKI_GLES
        const char *unlitVertSrc =
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "attribute vec4 a_vertex;\n"
        "attribute vec4 a_texCoord;\n"
        "attribute vec4 a_color;\n"//
        "varying lowp vec4 v_color;\n"//
        "varying lowp vec4 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    v_color = a_color;\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *unlitFragSrc =
        "precision mediump float;\n"
        "precision lowp int;\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[8];\n"
        "uniform struct\n"
        "{\n"
        "    vec4 diffuse;\n"
        "    vec4 ambient;\n"
        "    vec4 specular;\n"
        "    vec4 emission;\n"
        "    float shinyness;\n"
        "} u_material;\n"
        "varying vec4 v_color;\n"//
        "varying vec4 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 texColors = v_color;\n"//
        "    if(u_numTextures > 0) texColors *= texture2D(u_textureMap[0], v_texCoord.st);\n"
        "    if(u_numTextures > 1) texColors *= texture2D(u_textureMap[1], v_texCoord.st);\n"
        "    if(u_numTextures > 2) texColors *= texture2D(u_textureMap[2], v_texCoord.st);\n"
        "    if(u_numTextures > 3) texColors *= texture2D(u_textureMap[3], v_texCoord.st);\n"
        "    gl_FragColor = u_material.diffuse * texColors;\n"
        "}\n";
        
        const char *phongVertSrc =
        "uniform mat4 u_modelViewMatrix;\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat3 u_normalMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "attribute vec4 a_vertex;\n"
        "attribute vec4 a_texCoord;\n"
        "attribute vec3 a_normal;\n"
        "varying lowp vec4 v_texCoord;\n"
        "varying mediump vec3 v_normal;\n"
        "varying mediump vec3 v_eyeVec;\n"
        "void main()\n"
        "{\n"
        "    v_normal = normalize(u_normalMatrix * a_normal);\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    v_eyeVec = - (u_modelViewMatrix * a_vertex).xyz;\n"
        "    gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *phongVertSrc_skin =
        "uniform mat4 u_modelViewMatrix;\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat3 u_normalMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "uniform mat4 u_bones[18];\n"
        "attribute vec4 a_vertex;\n"
        "attribute vec4 a_texCoord;\n"
        "attribute vec3 a_normal;\n"
        "attribute vec4 a_boneIds;\n"
        "attribute vec4 a_boneWeights;\n"
        "varying vec4 v_texCoord;\n"
        "varying vec3 v_normal;\n"
        "varying vec3 v_eyeVec;\n"
        "void main()\n"
        "{\n"
        "    vec4 newVertex = vec4(0.0);\n"
        "    vec4 newNormal = vec4(0.0);\n"
        "    for (int i = 0; i < 4; i++)\n"
        "    {\n"
        "        newVertex += u_bones[int(floor(a_boneIds[i]))] * a_vertex * a_boneWeights[i];\n"
        "        newNormal += u_bones[int(floor(a_boneIds[i]))] * vec4(a_normal, 0.0) * a_boneWeights[i];\n"
        "    }\n"
        "    v_normal = normalize(u_normalMatrix * newNormal.xyz);\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    v_eyeVec = - (u_modelViewMatrix * newVertex).xyz;\n"
        "    gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);\n"
        "}\n";
        
        const char *phongFragSrc =
        "precision mediump float;\n"
        "precision lowp int;\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[8];\n"
        "uniform vec3 u_lightDir;\n"
        "uniform struct\n"
        "{\n"
        "    vec4 diffuse;\n"
        "    vec4 ambient;\n"
        "    vec4 specular;\n"
        "    vec4 emission;\n"
        "    float shinyness;\n"
        "} u_material;\n"
        "varying vec3 v_normal;\n"
        "varying vec4 v_texCoord;\n"
        "varying vec3 v_eyeVec;\n"
        "void main()\n"
        "{\n"
        "    vec4 texColors = vec4(1);\n"
        "    if(u_numTextures > 0) texColors *= texture2D(u_textureMap[0], v_texCoord.st);\n"
        "    if(u_numTextures > 1) texColors *= texture2D(u_textureMap[1], v_texCoord.st);\n"
        "    if(u_numTextures > 2) texColors *= texture2D(u_textureMap[2], v_texCoord.st);\n"
        "    if(u_numTextures > 3) texColors *= texture2D(u_textureMap[3], v_texCoord.st);\n"
        "    vec3 N = normalize(v_normal);\n"
        "    vec3 L = normalize(-u_lightDir);\n"
        "    vec3 E = normalize(v_eyeVec);\n"
		"    vec3 R = reflect(-L, N);\n"
        "    float nDotL = max(0.0, dot(N, L));\n"
        "    float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);\n"
        "    vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;\n"
        "    gl_FragColor = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;\n"
        "}\n";
        
        const char *phong_normalmap_vertSrc =
        "#version 150 core\n"
        "uniform mat4 u_modelViewMatrix;\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat3 u_normalMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "uniform vec3 u_lightDir;\n"
        "in vec4 a_vertex;\n"
        "in vec4 a_texCoord;\n"
        "in vec3 a_normal;\n"
        "in vec3 a_tangent;\n"
        "out vec4 v_texCoord;\n"
        "out vec3 v_normal;\n"
        "out vec3 v_eyeVec;\n"
        "out vec3 v_lightDir;\n"
        "void main()\n"
        "{\n"
        "   v_normal = normalize(u_normalMatrix * a_normal);\n"
        "   vec3 t = normalize (u_normalMatrix * a_tangent);\n"
        "   vec3 b = cross(v_normal, t);\n"
        "   mat3 tbnMatrix = mat3(t,b, v_normal);\n"
        "   v_eyeVec = tbnMatrix * normalize(- (u_modelViewMatrix * a_vertex).xyz);\n"
        "   v_lightDir = tbnMatrix * u_lightDir;\n"
        "   v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "   gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *phong_normalmap_fragSrc =
        "#version 150 core\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[16];\n"
        "uniform struct\n"
        "{\n"
        "    vec4 diffuse;\n"
        "    vec4 ambient;\n"
        "    vec4 specular;\n"
        "    vec4 emission;\n"
        "    float shinyness;\n"
        "} u_material;\n"
        "in vec3 v_normal;\n"
        "in vec4 v_texCoord;\n"
        "in vec3 v_eyeVec;\n"
        "in vec3 v_lightDir;\n"
        "out vec4 fragData;\n"
        "vec3 normalFromHeightMap(sampler2D theMap, vec2 theCoords, float theStrength)\n"
        "{\n"
        "    float center = texture(theMap, theCoords).r ;	 //center bump map sample\n"
        "    float U = texture(theMap, theCoords + vec2( 0.005, 0)).r ;	//U bump map sample\n"
        "    float V = texture(theMap, theCoords + vec2(0, 0.005)).r ;	 //V bump map sample\n"
        "    float dHdU = U - center;	 //create bump map U offset\n"
        "    float dHdV = V - center;	 //create bump map V offset\n"
        "    vec3 normal = vec3( -dHdU, dHdV, 0.05 / theStrength);	 //create the tangent space normal\n"
        "    return normalize(normal);\n"
        "}\n"
        
        "void main()"
        "{"
        "    //vec2 texCoord = v_texCoord.xy;\n"
        "    vec4 texColors = texture(u_textureMap[0], v_texCoord.xy);\n"
        "    vec3 N;\n"
        "    // sample normal map\n"
        "    //N = texture(u_textureMap[1], v_texCoord.xy).xyz * 2.0 - 1.0;\n"
        "    // sample bump map\n"
        "    N = normalFromHeightMap(u_textureMap[1], v_texCoord.xy, 0.8);\n"
        "    vec3 L = normalize(-v_lightDir);\n"
        "    vec3 E = normalize(v_eyeVec);\n"
		"    vec3 R = reflect(-L, N);\n"
        "    float nDotL = max(0.0, dot(N, L));\n"
        "    float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);\n"
        "    vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;\n"
        "    fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;\n"
        "}"
        ;

#else
        const char *unlitVertSrc =
       "#version 150 core\n"
       STRINGIFY(
       uniform mat4 u_modelViewProjectionMatrix;
       uniform mat4 u_textureMatrix;
       in vec4 a_vertex;
       in vec4 a_texCoord;
       in vec4 a_color;
       out vec4 v_color;
       out vec4 v_texCoord;
       void main()
       {
           v_color = a_color;
           v_texCoord =  u_textureMatrix * a_texCoord;
           gl_Position = u_modelViewProjectionMatrix * a_vertex;
       });
        
        const char *unlitFragSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[16];
        uniform struct{
           vec4 diffuse;
           vec4 ambient;
           vec4 specular;
           vec4 emission;
        } u_material;
        in vec4 v_color;
        in vec4 v_texCoord;
        out vec4 fragData;
        void main()
        {
           vec4 texColors = v_color;
           for(int i = 0; i < u_numTextures; i++)
           {
               texColors *= texture(u_textureMap[i], v_texCoord.st);
           }
           if(texColors.a == 0.0) discard;
           fragData = u_material.diffuse * texColors;
        });
        
        const char *phongVertSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform mat4 u_modelViewMatrix;
        uniform mat4 u_modelViewProjectionMatrix;
        uniform mat3 u_normalMatrix;
        uniform mat4 u_textureMatrix;
        in vec4 a_vertex;
        in vec4 a_texCoord;
        in vec3 a_normal;
        out vec4 v_texCoord;
        out vec3 v_normal;
        out vec3 v_eyeVec;
        void main()
        {
            v_normal = normalize(u_normalMatrix * a_normal);
            v_texCoord = u_textureMatrix * a_texCoord;
            v_eyeVec = - (u_modelViewMatrix * a_vertex).xyz;
            gl_Position = u_modelViewProjectionMatrix * a_vertex;
        });
        
        const char *phongVertSrc_skin =
        "#version 150 core\n"
        STRINGIFY(
        uniform mat4 u_modelViewMatrix;
        uniform mat4 u_modelViewProjectionMatrix;
        uniform mat3 u_normalMatrix;
        uniform mat4 u_textureMatrix;
        uniform mat4 u_bones[110];
        in vec4 a_vertex;
        in vec4 a_texCoord;
        in vec3 a_normal;
        in ivec4 a_boneIds;
        in vec4 a_boneWeights;
        out vec4 v_texCoord;
        out vec3 v_normal;
        out vec3 v_eyeVec;
        void main()
        {
            vec4 newVertex = vec4(0);
            vec4 newNormal = vec4(0);
            for (int i = 0; i < 4; i++)
            {
                newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];
                newNormal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];
            }
            v_normal = normalize(u_normalMatrix * newNormal.xyz);
            v_texCoord =  u_textureMatrix * a_texCoord;
            v_eyeVec = - (u_modelViewMatrix * newVertex).xyz;
            gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);
        });
        
        const char *phongFragSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[16];
        uniform vec3 u_lightDir;
        uniform struct
        {
            vec4 diffuse;
            vec4 ambient;
            vec4 specular;
            vec4 emission;
            float shinyness;
        } u_material;
        in vec3 v_normal;
        in vec4 v_texCoord;
        in vec3 v_eyeVec;
        out vec4 fragData;
        void main()
        {
            vec4 texColors = vec4(1);
            for(int i = 0; i < u_numTextures; i++)
            {
                texColors *= texture(u_textureMap[i], v_texCoord.st);
            }
            vec3 N = normalize(v_normal);
            vec3 L = normalize(-u_lightDir);
            vec3 E = normalize(v_eyeVec);
		    vec3 R = reflect(-L, N);
            float nDotL = max(0.0, dot(N, L));
            float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
            vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;
            fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
        });
        
        const char *phong_normalmap_vertSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform mat4 u_modelViewMatrix;
        uniform mat4 u_modelViewProjectionMatrix;
        uniform mat3 u_normalMatrix;
        uniform mat4 u_textureMatrix;
        uniform vec3 u_lightDir;
        in vec4 a_vertex;
        in vec4 a_texCoord;
        in vec3 a_normal;
        in vec3 a_tangent;
        out vec4 v_texCoord;
        out vec3 v_normal;
        out vec3 v_eyeVec;
        out vec3 v_lightDir;
        void main()
        {
           v_normal = normalize(u_normalMatrix * a_normal);
           vec3 t = normalize (u_normalMatrix * a_tangent);
           vec3 b = cross(v_normal, t);
           mat3 tbnMatrix = mat3(t,b, v_normal);
           v_eyeVec = tbnMatrix * normalize(- (u_modelViewMatrix * a_vertex).xyz);
           v_lightDir = tbnMatrix * u_lightDir;
           v_texCoord =  u_textureMatrix * a_texCoord;
           gl_Position = u_modelViewProjectionMatrix * a_vertex;
        });
        
        const char *phong_normalmap_fragSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[16];
        uniform struct
        {
            vec4 diffuse;
            vec4 ambient;
            vec4 specular;
            vec4 emission;
            float shinyness;
        } u_material;
        in vec3 v_normal;
        in vec4 v_texCoord;
        in vec3 v_eyeVec;
        in vec3 v_lightDir;
        out vec4 fragData;
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
            //vec2 texCoord = v_texCoord.xy;
            vec4 texColors = texture(u_textureMap[0], v_texCoord.xy);
            vec3 N;
            // sample normal map
            //N = texture(u_textureMap[1], v_texCoord.xy).xyz * 2.0 - 1.0;
            // sample bump map
            N = normalFromHeightMap(u_textureMap[1], v_texCoord.xy, 0.8);
            vec3 L = normalize(-v_lightDir);
            vec3 E = normalize(v_eyeVec);
		    vec3 R = reflect(-L, N);
            float nDotL = max(0.0, dot(N, L));
            float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);
            vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;
            fragData = texColors * (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
        });
#endif
        
#ifdef KINSKI_GLES
        const char *point_vertSrc =
        STRINGIFY(
        uniform mat4 u_modelViewProjectionMatrix;
        uniform float u_pointSize;
        attribute vec4 a_vertex;
        attribute vec4 a_color;
        attribute float a_pointSize;
        varying lowp vec4 v_color;
        void main()
        {
           gl_Position = u_modelViewProjectionMatrix * a_vertex;
           gl_PointSize = a_pointSize;
           v_color = a_color;
        });
        
        const char *point_fragSrc =
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[16];
        uniform struct
        {
          vec4 diffuse;
          vec4 ambient;
          vec4 specular;
          vec4 emission;
          float shinyness;
        } u_material;
        varying vec4 v_color;
        void main(){
        vec4 texColors = v_color;
        for(int i = 0; i < u_numTextures; i++)
        {
            texColors *= texture2D(u_textureMap[i], gl_PointCoord);
        }
        gl_FragColor = u_material.diffuse * texColors;
        });
#else
        const char *point_vertSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform mat4 u_modelViewProjectionMatrix;
        in vec4 a_vertex;
        in float a_pointSize;
        in vec4 a_color;
        out vec4 v_color;
        out vec3 v_eyeVec;
        void main()
        {
            v_color = a_color;
            gl_PointSize = a_pointSize;
            gl_Position = u_modelViewProjectionMatrix * a_vertex;
            v_eyeVec = gl_Position.xyz;
        });
        
        const char *point_color_fragSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[16];
        uniform struct
        {
          vec4 diffuse;
          vec4 ambient;
          vec4 specular;
          vec4 emission;
          float shinyness;
        } u_material;
        in vec4 v_color;
        out vec4 fragData;
        void main()
        {
            vec4 texColors = v_color;
            fragData = u_material.diffuse * texColors;
        });
        
        const char *point_texture_fragSrc =
        "#version 150 core\n"
        STRINGIFY(
        uniform int u_numTextures;
        uniform sampler2D u_textureMap[8];
        uniform struct{
        vec4 diffuse;
        vec4 ambient;
        vec4 specular;
        vec4 emission;
        } u_material;
        in vec4 v_color;
        out vec4 fragData;
        void main()
        {
            vec4 texColors = vec4(1);//v_color;
            for(int i = 0; i < u_numTextures; i++)
            {
                texColors *= texture(u_textureMap[i], gl_PointCoord);
            }
            fragData = u_material.diffuse * texColors;
        });
        
        // pixel shader for rendering points as shaded spheres
        const char *point_sphere_fragSrc =
        "#version 150 core\n"
        STRINGIFY(
            uniform float u_pointRadius;  // point size in world space
            uniform vec3 u_lightDir;
            uniform int u_numTextures;
            uniform sampler2D u_textureMap[8];
            uniform struct
            {
              vec4 diffuse;
              vec4 ambient;
              vec4 specular;
              vec4 emission;
              float shinyness;
            } u_material;
            in vec4 v_color;
            in vec3 v_eyeVec;        // position of center in eye space
            out vec4 fragData;
            void main()
            {
                float pointRadius = 10.f;
                
                // calculate normal from texture coordinates
                vec3 N;
                N.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
                float mag = dot(N.xy, N.xy);
                if (mag > 1.0) discard;   // kill pixels outside circle
                N.z = sqrt(1.0-mag);
                
                // point on surface of sphere in eye space
                vec3 spherePosEye = v_eyeVec + N * pointRadius;
                
                vec3 L = normalize(-u_lightDir);
                vec3 E = normalize(v_eyeVec);
                //vec3 R = reflect(-L, N);
                float nDotL = max(0.0, dot(N, L));
                
                vec3 v = normalize(-spherePosEye);
                vec3 h = normalize(u_lightDir + v);
                float specIntesity = pow( max(dot(N, h), 0.0), u_material.shinyness);
                vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;
                fragData = (u_material.ambient + u_material.diffuse * vec4(vec3(nDotL), 1.0)) + spec;
            });
#endif
        
        Shader ret;
        switch (type)
        {
            case SHADER_UNLIT:
                ret.loadFromData(unlitVertSrc, unlitFragSrc);
                break;
                
            case SHADER_PHONG:
                ret.loadFromData(phongVertSrc, phongFragSrc);
                break;
            
            case SHADER_PHONG_NORMALMAP:
                ret.loadFromData(phong_normalmap_vertSrc, phong_normalmap_fragSrc);
                break;
                
            case SHADER_PHONG_SKIN:
                ret.loadFromData(phongVertSrc_skin, phongFragSrc);
                break;
            case SHADER_POINTS_TEXTURE:
                ret.loadFromData(point_vertSrc, point_texture_fragSrc);
                break;
            case SHADER_POINTS_COLOR:
                ret.loadFromData(point_vertSrc, point_color_fragSrc);
                break;
            case SHADER_POINTS_SPHERE:
                ret.loadFromData(point_vertSrc, point_sphere_fragSrc);
                break;
            default:
                break;
        }
        KINSKI_CHECK_GL_ERRORS();
        
        return ret;
    }
    
}} //namespace
