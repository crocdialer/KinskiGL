//
//  KinskiGL.cpp
//  kinskiGL
//
//  Created by Fabian on 11/6/12.
//
//

#include "KinskiGL.h"
#include "App.h"
#include "Material.h"

using namespace glm;
using namespace std;

namespace kinski { namespace gl {
    
    void drawTexture(gl::Texture &theTexture, const vec2 &theSize, const vec2 &theTopLeft)
    {
        static gl::Material material;
        
        // add the texture to the material
        material.getTextures().clear();
        material.addTexture(theTexture);
        
        //create shader
        if(!material.getShader())
        {
            const char *vertSrc =
            "#version 150 core\n"
            "uniform mat4 u_modelViewProjectionMatrix;\n"
            "uniform mat4 u_textureMatrix;\n"
            "in vec4 a_vertex;\n"
            "in vec4 a_texCoord;\n"
            "out vec4 v_texCoord;\n"
            "void main(){\n"
            "v_texCoord =  u_textureMatrix * a_texCoord;\n"
            "gl_Position = u_modelViewProjectionMatrix * a_vertex;"
            "}\n";
            
            const char *fragSrc =
            "#version 150 core\n"
            "uniform int u_numTextures;\n"
            "uniform sampler2D u_textureMap[];\n"
            "in vec4 v_texCoord;\n"
            "out vec4 fragData;\n"
            "void main(){fragData = texture(u_textureMap[0], v_texCoord.xy);}\n";
            
            try
            {
                material.getShader().loadFromData(vertSrc, fragSrc);
            } catch (Exception &e)
            {
                std::cerr << e.what() << std::endl;
            }
            
            material.setDepthTest(false);
            material.setDepthWrite(false);
        }
        
        vec2 sz = theSize;
        vec2 tl = theTopLeft == vec2(0) ? vec2(0, App::getInstance()->getHeight()) : theTopLeft;
        drawQuad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    void drawQuad(gl::Material &theMaterial,
                  const vec2 &theSize,
                  const vec2 &theTl)
    {
        vec2 sz = theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, App::getInstance()->getHeight()) : theTl;
        drawQuad(theMaterial, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawQuad(gl::Material &theMaterial,
                  float x0, float y0, float x1, float y1)
    {
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        App::Ptr app = App::getInstance();
        
        float scaleX = (x1 - x0) / app->getWidth();
        float scaleY = (y0 - y1) / app->getHeight();
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / app->getWidth(),
                                  y1 / app->getHeight() , 0, 1);
        
        theMaterial.uniform("u_modelViewProjectionMatrix", projectionMatrix * modelViewMatrix);
        
        theMaterial.apply();
        
        static GLuint canvasVAO = 0;
        
        if(!canvasVAO)
        {
            //GL_T2F_V3F
            const GLfloat array[] ={0.0,0.0,0.0,0.0,0.0,
                                    1.0,0.0,1.0,0.0,0.0,
                                    1.0,1.0,1.0,1.0,0.0,
                                    0.0,1.0,0.0,1.0,0.0};
            
            // create VAO to record all VBO calls
            glGenVertexArrays(1, &canvasVAO);
            glBindVertexArray(canvasVAO);
            
            GLuint canvasBuffer;
            glGenBuffers(1, &canvasBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, canvasBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(array), array, GL_STATIC_DRAW);
            
            GLsizei stride = 5 * sizeof(GLfloat);
            
            GLuint vertexAttribLocation = theMaterial.getShader().getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
            
            GLuint texCoordAttribLocation = theMaterial.getShader().getAttribLocation("a_texCoord");
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(0));
            
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            
            glBindVertexArray(0);
        }
        
        glBindVertexArray(canvasVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
    }
    
    void drawLine(const vec2 &a, const vec2 &b, const vec4 &theColor)
    {
        // no effect in OpenGL 3.2 !?
        // glLineWidth(10.f);
        static Shader lineShader;
        
        //create shader
        if(!lineShader)
        {
            const char *vertSrc =
            "#version 150 core\n"
            "uniform mat4 u_modelViewProjectionMatrix;\n"
            "in vec4 a_vertex;\n"
            "void main(){gl_Position = u_modelViewProjectionMatrix * a_vertex;}\n";
            
            const char *fragSrc =
            "#version 150 core\n"
            "uniform vec4 u_lineColor;\n"
            "out vec4 fragData;\n"
            "void main(){fragData = u_lineColor;}\n";
            
            try
            {
                lineShader.loadFromData(vertSrc, fragSrc);
            } catch (Exception &e)
            {
                std::cerr << e.what() << std::endl;
            }
        }
        
        App::Ptr app = App::getInstance();

        lineShader.bind();
        
        lineShader.uniform("u_modelViewProjectionMatrix",
                            glm::ortho(0.f, app->getWidth(),
                                       0.f, app->getHeight(),
                                       0.f, 1000.f));
    
        lineShader.uniform("u_lineColor", theColor);
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);

        static GLuint lineVBO = 0;
        static GLuint lineVAO = 0;
        
        if(!lineVAO)
        {
            glGenVertexArrays(1, &lineVAO);
            glBindVertexArray(lineVAO);
            
            if(!lineVBO)
                glGenBuffers(1, &lineVBO);
            
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
            
            GLuint vertexAttribLocation = lineShader.getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
            
            glBindVertexArray(0);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
        GLfloat *bufferPtr = (GLfloat*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        bufferPtr[0] = a[0]; bufferPtr[1] = a[1];
        bufferPtr[2] = b[0]; bufferPtr[3] = b[1];
        glUnmapBuffer(GL_ARRAY_BUFFER);
        
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2);
        glBindVertexArray(0);
    }
    
}}//namespace