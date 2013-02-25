// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2013, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <stack>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "KinskiGL.h"
#include "Material.h"
#include "Camera.h"
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
    
    const glm::vec2& windowDimension(){ return g_windowDim; }

    void setWindowDimension(const glm::vec2 &theDim)
    {
        g_windowDim = theDim;
        
        if(g_projectionMatrixStack.empty())
            g_projectionMatrixStack.push(mat4());
        
        if(g_modelViewMatrixStack.empty())
            g_modelViewMatrixStack.push(mat4());
    }
    
    gl::Ray calculateRay(const CameraPtr &theCamera, uint32_t x, uint32_t y)
    {
        glm::vec3 cam_pos = theCamera->position();
        glm::vec3 lookAt = theCamera->lookAt(),
        side = theCamera->side(), up = theCamera->up();
        float near = theCamera->near();
        // bring click_pos to range -1, 1
        glm::vec2 offset (gl::windowDimension() / 2.0f);
        glm::vec2 click_2D(x, y);
        click_2D -= offset;
        click_2D /= offset;
        click_2D.y = - click_2D.y;
        glm::vec3 click_world_pos;
        
        if(PerspectiveCamera::Ptr cam = dynamic_pointer_cast<PerspectiveCamera>(theCamera) )
        {
            // convert fovy to radians
            float rad = glm::radians(cam->fov());
            float vLength = tan( rad / 2) * near;
            float hLength = vLength * cam->aspectRatio();
            
            click_world_pos = cam_pos + lookAt * near
            + side * hLength * click_2D.x
            + up * vLength * click_2D.y;
            
        }else if (OrthographicCamera::Ptr cam = dynamic_pointer_cast<OrthographicCamera>(theCamera))
        {
            click_world_pos = cam_pos + lookAt * near + side * click_2D.x + up  * click_2D.y;
        }
        LOG_DEBUG<<"clicked_world: ("<<click_world_pos.x<<",  "<<click_world_pos.y<<",  "<<click_world_pos.z<<")";
        return Ray(click_world_pos, click_world_pos - cam_pos);
    }
    
    void clearColor(const glm::vec4 &theColor)
    {
        glClearColor(theColor.r, theColor.g, theColor.b, theColor.a);
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

        static gl::MeshPtr mesh;
        
        //create shader
        if(!mesh)
        {
            gl::Material::Ptr mat(new gl::Material);
            mat->setShader(gl::createShader(gl::SHADER_UNLIT));
            gl::Geometry::Ptr geom(new gl::Geometry);
            mesh = gl::Mesh::Ptr(new gl::Mesh(geom, mat));
            mesh->geometry()->setPrimitiveType(GL_LINES);
        }
        
        vector<vec3>::const_iterator it = thePoints.begin();
        for (; it < thePoints.end(); ++it)
        {
            const glm::vec3 &v = *it;
            mesh->geometry()->appendVertex(v);
            mesh->geometry()->appendColor(theColor);
        }
        mesh->geometry()->createGLBuffers();
        if(!mesh->vertexArray()) mesh->createVertexArray();
        gl::drawMesh(mesh);
        
        mesh->geometry()->vertices().clear();
        mesh->geometry()->colors().clear();
    }
    
    void drawPoints(GLuint thePointVBO, GLsizei theCount, const MaterialPtr &theMaterial,
                    GLsizei stride, GLsizei offset)
    {
        static MaterialPtr staticMat;
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
        
        MaterialPtr activeMat = theMaterial ? theMaterial : staticMat;
        
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
        static gl::Material::Ptr material;
        
        //create shader, if not yet here
        if(!material)
        {
            try
            {
                material = gl::Material::Ptr(new gl::Material);
            } catch (Exception &e)
            {
                LOG_ERROR<<e.what();
            }
            material->setDepthTest(false);
            material->setDepthWrite(false);
        }
        // add the texture to the material
        material->textures().clear();
        material->addTexture(theTexture);
        
        vec2 sz = theSize;
        vec2 tl = theTopLeft == vec2(0) ? vec2(0, g_windowDim[1]) : theTopLeft;
        drawQuad(material, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    void drawQuad(const gl::MaterialPtr &theMaterial,
                  const vec2 &theSize,
                  const vec2 &theTl)
    {
        vec2 tl = theTl == vec2(0) ? vec2(0, g_windowDim[1]) : theTl;
        drawQuad(theMaterial, tl[0], tl[1], (tl + theSize)[0], tl[1] - theSize[1]);
    }
    
    
    void drawQuad(const gl::MaterialPtr &theMaterial,
                  float x0, float y0, float x1, float y1)
    {
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / g_windowDim[0];
        float scaleY = (y0 - y1) / g_windowDim[1];
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / g_windowDim[0],
                                  y1 / g_windowDim[1] , 0, 1);
        
        theMaterial->uniform("u_modelViewProjectionMatrix", projectionMatrix * modelViewMatrix);
        
        theMaterial->apply();
        
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
            
            GLuint vertexAttribLocation = theMaterial->shader().getAttribLocation("a_vertex");
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
            
            GLuint texCoordAttribLocation = theMaterial->shader().getAttribLocation("a_texCoord");
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                  stride, BUFFER_OFFSET(0));
            
            GLuint colorAttribLocation = theMaterial->shader().getAttribLocation("a_color");
            glVertexAttrib4f(colorAttribLocation, 1.0f, 1.0f, 1.0f, 1.0f);
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
        static map<boost::tuple<float,float,int,int>, MeshPtr> theMap;
        static vec4 colorGrey(.7, .7, .7, 1.0), colorRed(1.0, 0, 0 ,1.0), colorBlue(0, 0, 1.0, 1.0);
        
        // search for incoming key
        boost::tuple<float,float,int,int> conf (width, height, numW, numH);
        map<boost::tuple<float,float,int,int>, MeshPtr>::iterator it = theMap.find(conf);
        if(it == theMap.end())
        {
            GeometryPtr geom(new gl::Geometry);
            geom->setPrimitiveType(GL_LINES);
            gl::MaterialPtr mat(new gl::Material);
            MeshPtr mesh (new gl::Mesh(geom, mat));
            
            vector<vec3> &points = geom->vertices();
            vector<vec4> &colors = geom->colors();
            
            float stepX = width / numW, stepZ = height / numH;
            float w2 = width / 2.f, h2 = height / 2.f;
            
            glm::vec4 *color;
            for (int x = 0; x < numW + 1; x ++ )
            {
                if(x == 0) color = &colorBlue;
                else color = &colorGrey;
                
                // line Z
                points.push_back(vec3(- w2 + x * stepX, 0.f, -h2));
                points.push_back(vec3(- w2 + x * stepX, 0.f, h2));
                colors.push_back(*color);
                colors.push_back(*color);
            }
            for (int z = 0; z < numH + 1; z++ )
            {
                if(z == 0) color = &colorRed;
                else color = &colorGrey;
                
                // line X
                points.push_back(vec3(- w2 , 0.f, -h2 + z * stepZ));
                points.push_back(vec3( w2 , 0.f, -h2 + z * stepZ));
                colors.push_back(*color);
                colors.push_back(*color);
            }
            
            theMap.clear();
            
            geom->createGLBuffers();
            mesh->createVertexArray();
            
            theMap[conf] = mesh;
        }
        drawMesh(theMap[conf]);
    }
    
    void drawAxes(const MeshWeakPtr &theMesh)
    {
        Mesh::ConstPtr m = theMesh.lock();
        if(!m) return;
        
        AABB bb = m->geometry()->boundingBox();
        vector<vec3> thePoints;
        thePoints.push_back(vec3(0));
        thePoints.push_back(vec3(bb.max.x, 0, 0));
        drawLines(thePoints, vec4(1, 0 ,0, 1));
        
        thePoints[1] = vec3(0, bb.max.x, 0);
        drawLines(thePoints, vec4(0, 1, 0, 1));
        
        thePoints[1] = vec3(0, 0, bb.max.x);
        drawLines(thePoints, vec4(0, 0, 1, 1));
    }
    
    void drawMesh(const MeshPtr &theMesh)
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
        
        if(theMesh->geometry()->hasIndices())
        {
            glDrawElements(theMesh->geometry()->primitiveType(),
                           theMesh->geometry()->indices().size(), theMesh->geometry()->indexType(),
                           BUFFER_OFFSET(0));
        }
        else
        {
            glDrawArrays(theMesh->geometry()->primitiveType(), 0,
                         theMesh->geometry()->vertices().size());
        }
        
#ifndef KINSKI_NO_VAO 
        GL_SUFFIX(glBindVertexArray)(0);
#endif
        
        KINSKI_CHECK_GL_ERRORS();
    }
    
    void drawBoundingBox(const MeshWeakPtr &weakMesh)
    {
//#ifndef KINSKI_GLES
        static map<std::weak_ptr<const Mesh>, vector<vec3> > theMap;
        
        
        if(theMap.find(weakMesh) == theMap.end())
        {
            Mesh::ConstPtr theMesh = weakMesh.lock();
            if(!theMesh) return;
            
            AABB bb = theMesh->geometry()->boundingBox();
            
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

    void drawNormals(const MeshWeakPtr &theMesh)
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
