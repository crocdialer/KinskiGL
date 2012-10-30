#include "kinskiGL/App.h"

#include "kinskiGL/Geometry.h"
#include "kinskiGL/Material.h"
#include "kinskiGL/Camera.h"

#include "TextureIO.h"

#include <boost/shared_array.hpp>

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public App
{
private:
    
    gl::Material m_material;
    
    gl::Geometry m_geometry;
    
    gl::PerspectiveCamera m_Camera;
    
    RangedProperty<float>::Ptr m_distance;
    RangedProperty<float>::Ptr m_textureMix;

    
    void drawQuad(gl::Material &theMaterial,
                  const vec2 &theSize,
                  const vec2 &theTl = vec2(0))
    {
        vec2 sz = theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, getHeight()) : theTl;
        drawQuad(theMaterial, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawQuad(gl::Material &theMaterial,
                  float x0, float y0, float x1, float y1)
    {
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / getWidth();
        float scaleY = (y0 - y1) / getHeight();
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / getWidth(), y1 / getHeight() , 0, 1);
        
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
    
    void drawGeometry(const gl::Geometry &theGeom, gl::Material &theMaterial)
    {
        theMaterial.uniform("u_modelViewProjectionMatrix",
                           m_Camera.getProjectionMatrix()
                           * glm::translate( glm::mat4(), vec3(0, 0, -m_distance->val())));
        
        theMaterial.apply();
        
        gl::Shader &shader = theMaterial.getShader();

        // TODO: create interleaved array
        // GL_T2F_N3F_V3F
        uint32_t interleavedCount = 8 * 3 * theGeom.getFaces().size();
        boost::shared_array<GLfloat> interleaved (new GLfloat[interleavedCount]);
        
        uint32_t indexCount = 3 * theGeom.getFaces().size();
        boost::shared_array<GLuint> indices (new GLuint [indexCount]);
        GLuint *indexPtr = indices.get();
        
        vector<gl::Face3>::const_iterator faceIt = theGeom.getFaces().begin();
        
        for (; faceIt != theGeom.getFaces().end(); faceIt++)
        {
            const gl::Face3 &face = *faceIt;
            
            for (int i = 0; i < 3; i++)
            {
                const GLuint &idx = face.indices[i];

                // index
                *(indexPtr++) = idx;
                
//                // texCoords
//                const glm::vec2 &texCoord = theGeom.getTexCoords()[idx];
//                interleaved[8 * idx ] = texCoord.s;
//                interleaved[8 * idx + 1] = texCoord.t;
//                
//                // normals
//                const glm::vec3 &normal = face.vertexNormals[i];
//                interleaved[8 * idx + 2] = normal.x;
//                interleaved[8 * idx + 3] = normal.y;
//                interleaved[8 * idx + 4] = normal.z;
//                
//                // vertices
//                const glm::vec3 &vert = theGeom.getVertices()[idx];
//                interleaved[8 * idx + 5] = vert.x;
//                interleaved[8 * idx + 6] = vert.y;
//                interleaved[8 * idx + 7] = vert.z;
            }
        }
      
        static GLuint vertexArray = 0;
        
        if(!vertexArray)
        {
            glGenVertexArrays(1, &vertexArray);
            glBindVertexArray(vertexArray);
            
            GLuint vertexAttribLocation = shader.getAttribLocation("a_vertex");
            //GLuint normalAttribLocation = shader.getAttribLocation("a_normal");
            GLuint texCoordAttribLocation = shader.getAttribLocation("a_texCoord");
            
            GLsizei stride = 0;
            
            // vertex VBO
            GLuint vertexBuffer;
            glGenBuffers(1, &vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat) * m_geometry.getVertices().size(),
                         &m_geometry.getVertices()[0], GL_DYNAMIC_DRAW);
            
            // define attrib pointer (vertex)
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride,
                                  BUFFER_OFFSET(0));
            
            // texCoord VBO
            GLuint texCoordBuffer;
            glGenBuffers(1, &texCoordBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
            glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(GLfloat) * m_geometry.getTexCoords().size(),
                         &m_geometry.getTexCoords()[0], GL_STATIC_DRAW);
            
            // define attrib pointer (texCoord)
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                  stride,
                                  BUFFER_OFFSET(0));

    //        glEnableVertexAttribArray(normalAttribLocation);
    //        glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
    //                              stride,
    //                              interleaved.get() + 2);
            
            // index buffer
            GLuint indexBuffer;
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), indices.get(),
                         GL_DYNAMIC_DRAW );
            
            glBindVertexArray(0);
            
        }
        
        glBindVertexArray(vertexArray);
        glDrawElements(GL_TRIANGLES, 3 * theGeom.getFaces().size(),
                       GL_UNSIGNED_INT, BUFFER_OFFSET(0));
        glBindVertexArray(0);
    };
    
    
public:
    
    void setup()
    {
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            m_material.getShader().loadFromFile("shader_vert.glsl", "shader_frag.glsl");
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_geometry = gl::Plane(20, 20, 50, 50);
        
        m_material.addTexture(TextureIO::loadTexture("/Users/Fabian/Pictures/artOfNoise.png"));
        m_material.addTexture(TextureIO::loadTexture("/Users/Fabian/Pictures/David_Jien_02.png"));
        m_material.setWireframe(true);
        
        m_distance = RangedProperty<float>::create("view distance", 25, -50, 50);
        registerProperty(m_distance);
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());

        // enable observer mechanism
        observeProperties();
    }
    
    void update(const float timeDelta)
    {
        m_material.uniform("u_textureMix", m_textureMix->val());
    }
    
    void draw()
    {
        gl::Material cloneMat = m_material;
        
        cloneMat.setDepthWrite(false);
        cloneMat.setWireframe(false);
        drawQuad(cloneMat, getWindowSize());
        
        drawGeometry(m_geometry, m_material);
    }
    
    void resize(int w, int h)
    {
        m_Camera.setAspectRatio(getAspectRatio());
    }
    
    void tearDown()
    {
        printf("ciao simple geometry\n");
    }
    
    void update(const Property::Ptr &theProperty)
    {
        // one of our porperties was changed
        
        
    
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    
    return theApp->run();
}
