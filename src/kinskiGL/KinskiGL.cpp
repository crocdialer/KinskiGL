//
//  KinskiGL.cpp
//  kinskiGL
//
//  Created by Fabian on 11/6/12.
//
//

#include <stack>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "KinskiGL.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

using namespace glm;
using namespace std;

namespace kinski { namespace gl {
    
    glm::vec2 g_windowDim;
    std::stack<glm::mat4> g_projectionMatrixStack;
    std::stack<glm::mat4> g_modelViewMatrixStack;
    
    void pushMatrix(const Matrixtype type)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.push(g_projectionMatrixStack.top());
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.push(g_modelViewMatrixStack.top());
                break;
                
            default:
                break;
        }
    }
    
    void popMatrix(const Matrixtype type)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                if(g_projectionMatrixStack.size() > 1) g_projectionMatrixStack.pop();
                break;
            case MODEL_VIEW_MATRIX:
                if(g_modelViewMatrixStack.size() > 1) g_modelViewMatrixStack.pop();
                break;
                
            default:
                break;
        }
    }
    
    void multMatrix(const Matrixtype type, const glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.top() *= theMatrix;
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.top() *= theMatrix;
                break;
                
            default:
                break;
        }
    }
    
    void loadMatrix(const Matrixtype type, const glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                g_projectionMatrixStack.top() = theMatrix;
                break;
            case MODEL_VIEW_MATRIX:
                g_modelViewMatrixStack.top() = theMatrix;
                break;
                
            default:
                break;
        }
    }
    
    void getMatrix(const Matrixtype type, glm::mat4 &theMatrix)
    {
        switch (type)
        {
            case PROJECTION_MATRIX:
                theMatrix = g_projectionMatrixStack.top();
                break;
            case MODEL_VIEW_MATRIX:
                theMatrix = g_modelViewMatrixStack.top();
                break;
                
            default:
                break;
        }
    }
    
    void setWindowDimension(const glm::vec2 &theDim)
    {
        g_windowDim = theDim;
        
        if(g_projectionMatrixStack.empty())
            g_projectionMatrixStack.push(mat4());
        
        if(g_modelViewMatrixStack.empty())
            g_modelViewMatrixStack.push(mat4());
    }
    
    void drawLine(const vec2 &a, const vec2 &b, const vec4 &theColor)
    {
        static vector<vec3> thePoints;
        thePoints.clear();
        thePoints.push_back(vec3(a, 0));
        thePoints.push_back(vec3(b, 0));
        
        ScopedMatrixPush pro(gl::PROJECTION_MATRIX), mod(gl::MODEL_VIEW_MATRIX);
        
        loadMatrix(gl::PROJECTION_MATRIX, glm::ortho(0.f, g_windowDim[0],
                                                     0.f, g_windowDim[1],
                                                     0.f, 1000.f));
        
        loadMatrix(gl::MODEL_VIEW_MATRIX, mat4());
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
#ifndef KINSKI_GLES
        glEnable(GL_LINE_SMOOTH);
#endif
        drawLines(thePoints, theColor);
    }
    
    void drawLines(const vector<vec3> &thePoints, const vec4 &theColor)
    {
        if(thePoints.empty()) return;
        
        // no effect in OpenGL 3.2 !?
        // glLineWidth(10.f);
        static gl::Material material;
        
        //create shader
        if(!material.shader())
        {
#ifdef KINSKI_GLES
            const char *vertSrc =
            "uniform mat4 u_modelViewProjectionMatrix;\n"
            "attribute vec4 a_vertex;\n"
            "void main(){gl_Position = u_modelViewProjectionMatrix * a_vertex;}\n";
            
            const char *fragSrc =
            "uniform vec4 u_lineColor;\n"
            "void main(){gl_FragColor = u_lineColor;}\n";
#else
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
#endif
            try
            {
                material.shader().loadFromData(vertSrc, fragSrc);
            } catch (Exception &e)
            {
                LOG_ERROR << e.what();
            }
        }
        
        //lineShader.bind();
        
        material.uniform("u_modelViewProjectionMatrix",
                           g_projectionMatrixStack.top()
                           * g_modelViewMatrixStack.top());
        
        material.uniform("u_lineColor", theColor);
        
        material.apply();
        
        static GLuint lineVBO = 0;
        static GLuint lineVAO = 0;
        
        if(!lineVAO)
        {
#ifndef KINSKI_NO_VAO
            GL_SUFFIX(glGenVertexArrays)(1, &lineVAO);
            GL_SUFFIX(glBindVertexArray)(lineVAO);
#endif            
            if(!lineVBO)
                glGenBuffers(1, &lineVBO);
            
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * thePoints.size(),
                         NULL, GL_STREAM_DRAW);
            
            GLuint vertexAttribLocation = material.shader().getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
            
#ifndef KINSKI_NO_VAO
            GL_SUFFIX(glBindVertexArray)(0);
#endif
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(thePoints[0]) * thePoints.size(),
                     NULL, GL_STREAM_DRAW);
        glBufferData(GL_ARRAY_BUFFER, sizeof(thePoints[0]) * thePoints.size(),
                     &thePoints[0], GL_STREAM_DRAW);
        
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(lineVAO);
#endif
        glDrawArrays(GL_LINES, 0, thePoints.size());
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(0);
#endif
    }
    
    void drawPoints(GLuint thePointVBO, GLsizei theCount, const Material::Ptr &theMaterial,
                    GLsizei stride, GLsizei offset)
    {
        static Material::Ptr staticMat;
        static GLuint pointVAO = 0;
        
        //create shader
        if(!staticMat)
        {
            staticMat = gl::Material::Ptr(new gl::Material);
#ifdef KINSKI_GLES
            const char *vertSrc =
            "uniform mat4 u_modelViewProjectionMatrix;\n"
            "uniform float u_pointSize;\n"
            "attribute vec4 a_vertex;\n"
            "void main(){gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
            "gl_PointSize = u_pointSize;}\n";
            
            const char *fragSrc =
            "uniform int u_numTextures;\n"
            "uniform sampler2D u_textureMap[16];\n"
            "uniform struct{\n"
            "vec4 diffuse;\n"
            "vec4 ambient;\n"
            "vec4 specular;\n"
            "vec4 emission;\n"
            "} u_material;\n"
            "void main(){\n"
            "vec4 texColors = vec4(1);\n"
            //"for(int i = 0; i < u_numTextures; i++)\n"
            //"   {texColors *= texture2D(u_textureMap[i], gl_PointCoord);}\n"
            "gl_FragColor = u_material.diffuse * texColors;\n"
            "}\n";
#else
            const char *vertSrc =
            "#version 150 core\n"
            "uniform mat4 u_modelViewProjectionMatrix;\n"
            "uniform float u_pointSize;\n"
            "in vec4 a_vertex;\n"
            "void main(){gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
            "gl_PointSize = u_pointSize;}\n";
            
            const char *fragSrc =
            "#version 150 core\n"
            "uniform int u_numTextures;\n"
            "uniform sampler2D u_textureMap[16];\n"
            "uniform struct{\n"
            "vec4 diffuse;\n"
            "vec4 ambient;\n"
            "vec4 specular;\n"
            "vec4 emission;\n"
            "} u_material;\n"
            "out vec4 fragData;\n"
            "void main(){\n"
            "vec4 texColors = vec4(1);\n"
            "for(int i = 0; i < u_numTextures; i++)\n"
            "   {texColors *= texture(u_textureMap[i], gl_PointCoord);}\n"
            "fragData = u_material.diffuse * texColors;\n"
            "}\n";
#endif
            try
            {
                staticMat->shader().loadFromData(vertSrc, fragSrc);
                staticMat->setPointSize(2.f);
            } catch (Exception &e)
            {
                LOG_ERROR<<e.what();
            }
        }
        
        Material::Ptr activeMat = theMaterial ? theMaterial : staticMat;
        
        if(!activeMat->shader())
            activeMat->shader() = staticMat->shader();
        
        activeMat->uniform("u_modelViewProjectionMatrix",
                           g_projectionMatrixStack.top()
                           * g_modelViewMatrixStack.top());
        
        activeMat->apply();
        
        if(!pointVAO || (activeMat != staticMat) )
        {   
#ifndef KINSKI_NO_VAO
            if(!pointVAO) GL_SUFFIX(glGenVertexArrays)(1, &pointVAO);
            GL_SUFFIX(glBindVertexArray)(pointVAO);
#endif            
            glBindBuffer(GL_ARRAY_BUFFER, thePointVBO);
            
            GLuint vertexAttribLocation = activeMat->shader().getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(offset));
            
#ifndef KINSKI_NO_VAO
            GL_SUFFIX(glBindVertexArray)(0);
#endif
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, thePointVBO);
        
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(pointVAO);
#endif
        glDrawArrays(GL_POINTS, 0, theCount);
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(0);
#endif
        
        KINSKI_CHECK_GL_ERRORS();
    }
    
    void drawPoints(const std::vector<glm::vec3> &thePoints, const Material::Ptr &theMaterial)
    {
        static GLuint pointVBO = 0;
        
        if(!pointVBO)
            glGenBuffers(1, &pointVBO);
        
        glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        glBufferData(GL_ARRAY_BUFFER, thePoints.size() * sizeof(vec3), NULL,
                     GL_STREAM_DRAW);
        glBufferData(GL_ARRAY_BUFFER, thePoints.size() * sizeof(vec3), &thePoints[0],
                     GL_STREAM_DRAW);

        drawPoints(pointVBO, thePoints.size(), theMaterial);
    }
    
    void drawTexture(gl::Texture &theTexture, const vec2 &theSize, const vec2 &theTopLeft)
    {
        static gl::Material material;
        
        // add the texture to the material
        material.textures().clear();
        material.addTexture(theTexture);
        
        //create shader
        if(!material.shader())
        {
            try
            {
                material.setShader(createShader(SHADER_UNLIT));
            } catch (Exception &e)
            {
                LOG_ERROR<<e.what();
            }
            
            material.setDepthTest(false);
            material.setDepthWrite(false);
        }
        
        vec2 sz = theSize;
        vec2 tl = theTopLeft == vec2(0) ? vec2(0, g_windowDim[1]) : theTopLeft;
        drawQuad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    void drawQuad(gl::Material &theMaterial,
                  const vec2 &theSize,
                  const vec2 &theTl)
    {
        vec2 sz = theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, g_windowDim[1]) : theTl;
        drawQuad(theMaterial, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawQuad(gl::Material &theMaterial,
                  float x0, float y0, float x1, float y1)
    {
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / g_windowDim[0];
        float scaleY = (y0 - y1) / g_windowDim[1];
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / g_windowDim[0],
                                  y1 / g_windowDim[1] , 0, 1);
        
        theMaterial.uniform("u_modelViewProjectionMatrix", projectionMatrix * modelViewMatrix);
        
        theMaterial.apply();
        
        static GLuint canvasVAO = 0, canvasBuffer = 0;
        
        if(!canvasVAO)
        {
            //GL_T2F_V3F
            const GLfloat array[] ={0.0,0.0,0.0,0.0,0.0,
                                    1.0,0.0,1.0,0.0,0.0,
                                    1.0,1.0,1.0,1.0,0.0,
                                    0.0,1.0,0.0,1.0,0.0};
           
#ifndef KINSKI_NO_VAO
            // create VAO to record all VBO calls
            GL_SUFFIX(glGenVertexArrays)(1, &canvasVAO);
            GL_SUFFIX(glBindVertexArray)(canvasVAO);
#endif 
            if(!canvasBuffer) glGenBuffers(1, &canvasBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, canvasBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(array), array, GL_STATIC_DRAW);
            
            GLsizei stride = 5 * sizeof(GLfloat);
            
            GLuint vertexAttribLocation = theMaterial.shader().getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
            
            GLuint texCoordAttribLocation = theMaterial.shader().getAttribLocation("a_texCoord");
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(0));
            
            //glBindBuffer(GL_ARRAY_BUFFER, 0); 
            //GL_SUFFIX(glBindVertexArray)(0);
        }
        
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(canvasVAO);
#endif 
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#ifndef KINSKI_NO_VAO
        GL_SUFFIX(glBindVertexArray)(0);
#endif 
        KINSKI_CHECK_GL_ERRORS();
    }
    
    void drawGrid(float width, float height, int numW, int numH)
    {
        static map<boost::tuple<float,float,int,int>, vector<vec3> > theMap;
        static vec4 colorGrey(.7, .7, .7, 1.0);
        
        // incoming key
        boost::tuple<float,float,int,int> conf (width, height, numW, numH);
        
        map<boost::tuple<float,float,int,int>, vector<vec3> >::iterator it = theMap.find(conf);
        if(it == theMap.end())
        {
            vector<vec3> thePoints;
            
            float stepX = width / numW, stepZ = height / numH;
            
            float w2 = width / 2.f, h2 = height / 2.f;
            
            for (int z = 0; z < numH + 1; z++ )
            {
                for (int x = 0; x < numW + 1; x ++ )
                {
                    // line X
                    thePoints.push_back(vec3(- w2 + x * stepX, 0.f, -h2));
                    thePoints.push_back(vec3(- w2 + x * stepX, 0.f, h2));
                    
                    // line Z
                    thePoints.push_back(vec3(- w2 , 0.f, -h2 + z * stepZ));
                    thePoints.push_back(vec3( w2 , 0.f, -h2 + z * stepZ));
                }
            }
            
            theMap.clear();
            theMap[conf] = thePoints;
        }
        
        drawLines(theMap[conf], colorGrey);
    }
    
    void drawAxes(const std::weak_ptr<const Mesh> &theMesh)
    {
        Mesh::ConstPtr m = theMesh.lock();
        if(!m) return;
        
        BoundingBox bb = m->geometry()->boundingBox();
        vector<vec3> thePoints;
        thePoints.push_back(vec3(0));
        thePoints.push_back(vec3(bb.max.x, 0, 0));
        drawLines(thePoints, vec4(1, 0 ,0, 1));
        
        thePoints[1] = vec3(0, bb.max.x, 0);
        drawLines(thePoints, vec4(0, 1, 0, 1));
        
        thePoints[1] = vec3(0, 0, bb.max.x);
        drawLines(thePoints, vec4(0, 0, 1, 1));
    }
    
    void drawMesh(const std::shared_ptr<const Mesh> &theMesh)
    {
        theMesh->material()->uniform("u_modelViewMatrix", g_modelViewMatrixStack.top());
        
        theMesh->material()->uniform("u_normalMatrix",
                                        glm::inverseTranspose( glm::mat3(g_modelViewMatrixStack.top()) ));
        
        theMesh->material()->uniform("u_modelViewProjectionMatrix",
                                        g_projectionMatrixStack.top()
                                        * g_modelViewMatrixStack.top());
        
        if(theMesh->geometry()->hasBones())
        {
            theMesh->material()->uniform("u_bones", theMesh->geometry()->boneMatrices());
        }
        
        theMesh->material()->apply();
        
#ifndef KINSKI_NO_VAO 
        GL_SUFFIX(glBindVertexArray)(theMesh->vertexArray());
#else
        theMesh->bindVertexPointers();
#endif

        glDrawElements(theMesh->geometry()->primitiveType(), theMesh->geometry()->indices().size(),
                       theMesh->geometry()->indexType(), BUFFER_OFFSET(0));
#ifndef KINSKI_NO_VAO 
        GL_SUFFIX(glBindVertexArray)(0);
#endif
        
        KINSKI_CHECK_GL_ERRORS();
    }
    
    void drawBoundingBox(const std::weak_ptr<const Mesh> &weakMesh)
    {
//#ifndef KINSKI_GLES
        static map<std::weak_ptr<const Mesh>, vector<vec3> > theMap;
        
        
        if(theMap.find(weakMesh) == theMap.end())
        {
            Mesh::ConstPtr theMesh = weakMesh.lock();
            if(!theMesh) return;
            
            BoundingBox bb = theMesh->geometry()->boundingBox();
            
            vector<vec3> thePoints;
            // bottom
            thePoints.push_back(bb.min);
            thePoints.push_back(vec3(bb.min.x, bb.min.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.min.x, bb.min.y, bb.max.z));
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.max.z));
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.min.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.min.z));
            thePoints.push_back(bb.min);
            
            // top
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.min.z));
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.max.z));
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.max.z));
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.min.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.min.z));
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.min.z));
            
            //sides
            thePoints.push_back(vec3(bb.min.x, bb.min.y, bb.min.z));
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.min.z));
            
            thePoints.push_back(vec3(bb.min.x, bb.min.y, bb.max.z));
            thePoints.push_back(vec3(bb.min.x, bb.max.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.max.z));
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.max.z));
            
            thePoints.push_back(vec3(bb.max.x, bb.min.y, bb.min.z));
            thePoints.push_back(vec3(bb.max.x, bb.max.y, bb.min.z));
            
            theMap[weakMesh] = thePoints;
        }
        
        gl::drawLines(theMap[weakMesh], vec4(1));
        
        KINSKI_CHECK_GL_ERRORS();
//#endif
    }

    void drawNormals(const std::weak_ptr<const Mesh> &theMesh)
    {
//#ifndef KINSKI_GLES
        static map<std::weak_ptr<const Mesh>, vector<vec3> > theMap;
        
        if(theMap.find(theMesh) == theMap.end())
        {
            Mesh::ConstPtr m = theMesh.lock();
            
            if(m->geometry()->normals().empty()) return;
            
            vector<vec3> thePoints;
            const vector<vec3> &vertices = m->geometry()->vertices();
            const vector<vec3> &normals = m->geometry()->normals();
            
            float length = (m->geometry()->boundingBox().max -
                            m->geometry()->boundingBox().min).length() * 5;
            
            for (int i = 0; i < vertices.size(); i++)
            {
                thePoints.push_back(vertices[i]);
                thePoints.push_back(vertices[i] + normals[i] * length);
            }
            
            theMap[theMesh] = thePoints;
        }
        
        gl::drawLines(theMap[theMesh], vec4(.7));
        
        KINSKI_CHECK_GL_ERRORS();
//#endif
    }
    
/*********************************** Shader Factory *******************************************/
    
    Shader createShader(ShaderType type)
    {
#ifdef KINSKI_GLES
        const char *unlitVertSrc =
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "attribute vec4 a_vertex;\n"
        "attribute vec4 a_texCoord;\n"
        "varying lowp vec4 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *unlitFragSrc =
        "precision mediump float;\n"
        "precision lowp int;\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[4];\n"
        "uniform struct\n"
        "{\n"
        "    vec4 diffuse;\n"
        "    vec4 ambient;\n"
        "    vec4 specular;\n"
        "    vec4 emission;\n"
        "    float shinyness;\n"
        "} u_material;\n"
        "varying vec4 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 texColors = vec4(1.0);\n"
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
        "varying vec4 v_texCoord;\n"
        "varying vec3 v_normal;\n"
        "varying vec3 v_eyeVec;\n"
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
        "uniform mat4 u_bones[25];\n"
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
        "        newVertex += u_bones[(int)a_boneIds[i]] * a_vertex * a_boneWeights[i];\n"
        "        newNormal += u_bones[(int)a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];\n"
        "    }\n"
        "    v_normal = normalize(u_normalMatrix * newNormal.xyz);\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    v_eyeVec = - (u_modelViewMatrix * newVertex).xyz;\n"
        "    gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);\n"
        "}\n";
        
        const char *phongFragSrc =
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[4];\n"
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
        "    gl_FragColor = u_material.diffuse * texColors * vec4(vec3(nDotL), 1.0) ;\n"
        "}\n";
#else
        const char *unlitVertSrc =
        "#version 150 core\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "in vec4 a_vertex;\n"
        "in vec4 a_texCoord;\n"
        "out vec4 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *unlitFragSrc =
        "#version 150 core\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[16];\n"
        "uniform struct{\n"
        "    vec4 diffuse;\n"
        "    vec4 ambient;\n"
        "    vec4 specular;\n"
        "    vec4 emission;\n"
        "} u_material;\n"
        "in vec4 v_texCoord;\n"
        "out vec4 fragData;\n"
        "void main()\n"
        "{\n"
        "    vec4 texColors = vec4(1);\n"
        "    for(int i = 0; i < u_numTextures; i++)\n"
        "    {\n"
        "        texColors *= texture(u_textureMap[i], v_texCoord.st);\n"
        "    }\n"
        "    fragData = u_material.diffuse * texColors;\n"
        "}\n";
        
        const char *phongVertSrc =
        "#version 150 core\n"
        "uniform mat4 u_modelViewMatrix;\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat3 u_normalMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "in vec4 a_vertex;\n"
        "in vec4 a_texCoord;\n"
        "in vec3 a_normal;\n"
        "out vec4 v_texCoord;\n"
        "out vec3 v_normal;\n"
        "out vec3 v_eyeVec;\n"
        "void main()\n"
        "{\n"
        "    v_normal = normalize(u_normalMatrix * a_normal);\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    v_eyeVec = - (u_modelViewMatrix * a_vertex).xyz;\n"
        "    gl_Position = u_modelViewProjectionMatrix * a_vertex;\n"
        "}\n";
        
        const char *phongVertSrc_skin =
        "#version 150 core\n"
        "uniform mat4 u_modelViewMatrix;\n"
        "uniform mat4 u_modelViewProjectionMatrix;\n"
        "uniform mat3 u_normalMatrix;\n"
        "uniform mat4 u_textureMatrix;\n"
        "uniform mat4 u_bones[25];\n"
        "in vec4 a_vertex;\n"
        "in vec4 a_texCoord;\n"
        "in vec3 a_normal;\n"
        "in ivec4 a_boneIds;\n"
        "in vec4 a_boneWeights;\n"
        "out vec4 v_texCoord;\n"
        "out vec3 v_normal;\n"
        "out vec3 v_eyeVec;\n"
        "void main()\n"
        "{\n"
        "    vec4 newVertex = vec4(0);\n"
        "    vec4 newNormal = vec4(0);\n"
        "    for (int i = 0; i < 4; i++)\n"
        "    {\n"
        "        newVertex += u_bones[a_boneIds[i]] * a_vertex * a_boneWeights[i];\n"
        "        newNormal += u_bones[a_boneIds[i]] * vec4(a_normal, 0.0) * a_boneWeights[i];\n"
        "    }\n"
        "    v_normal = normalize(u_normalMatrix * newNormal.xyz);\n"
        "    v_texCoord =  u_textureMatrix * a_texCoord;\n"
        "    v_eyeVec = - (u_modelViewMatrix * newVertex).xyz;\n"
        "    gl_Position = u_modelViewProjectionMatrix * vec4(newVertex.xyz, 1.0);\n"
        "}\n";
                
        const char *phongFragSrc =
        "#version 150 core\n"
        "uniform int u_numTextures;\n"
        "uniform sampler2D u_textureMap[16];\n"
        "uniform vec3 u_lightDir;\n"
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
        "out vec4 fragData;\n"
        "void main()\n"
        "{\n"
        "    vec4 texColors = vec4(1);\n"
        "    for(int i = 0; i < u_numTextures; i++)\n"
        "    {\n"
        "        texColors *= texture(u_textureMap[i], v_texCoord.st);\n"
        "    }\n"
        "    vec3 N = normalize(v_normal);\n"
        "    vec3 L = normalize(-u_lightDir);\n"
        "    vec3 E = normalize(v_eyeVec);\n"
		"    vec3 R = reflect(-L, N);\n"
        "    float nDotL = max(0.0, dot(N, L));\n"
        "    float specIntesity = pow( max(dot(R, E), 0.0), u_material.shinyness);\n"
        "    vec4 spec = u_material.specular * specIntesity; spec.a = 0.0;\n"
        "    fragData = u_material.diffuse * texColors * vec4(vec3(nDotL), 1.0f) ;\n"
        "}\n";
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
                
            case SHADER_PHONG_SKIN:
                ret.loadFromData(phongVertSrc_skin, phongFragSrc);
                break;
                
            default:
                break;
        }
        KINSKI_CHECK_GL_ERRORS();
        
        return ret;
    }
    
    const std::set<std::string>& getExtensions()
    {
        static std::set<std::string> s_extensions;
        
        if(s_extensions.empty())
        {
#ifndef KINSKI_GLES
            GLint numExtensions = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions) ;
            for (int i = 0; i < numExtensions; ++i) {
                s_extensions.insert((char*)glGetStringi(GL_EXTENSIONS, i)) ;
            }
#endif
        }
        
        return s_extensions;
    }
    
    bool isExtensionSupported(const std::string &theName)
    {
        return getExtensions().find(theName) != getExtensions().end();
    }
    
}}//namespace
