#include "kinskiGL/App.h"

#include "kinskiGL/Mesh.h"
#include "kinskiGL/Camera.h"

#include "TextureIO.h"

#include "kinskiCV/CVThread.h"

#include <boost/shared_array.hpp>


using namespace std;
using namespace kinski;
using namespace glm;

class SimpleGeometryApp : public App
{
private:
    
    gl::Material::Ptr m_material;
    gl::Geometry::Ptr m_geometry;
    
    gl::Mesh::Ptr m_mesh;
    
    gl::PerspectiveCamera m_Camera;
    
    RangedProperty<float>::Ptr m_distance;
    RangedProperty<float>::Ptr m_textureMix;
    Property_<bool>::Ptr m_wireFrame;
    
    Property_<glm::vec4>::Ptr m_color;
    
    Property_<glm::mat3>::Ptr m_rotation;
    
    CVThread::Ptr m_cvThread;

    void drawLine(const vec2 &a, const vec2 &b)
    {
        
    }
    
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
        glm::mat4 transform = m_Camera.getViewMatrix();//glm::translate( glm::mat4(), vec3(0, 0, -m_distance->val()))
                              //  * glm::mat4(m_rotation->val());
        
        theMaterial.uniform("u_modelViewProjectionMatrix",
                           m_Camera.getProjectionMatrix()
                           * transform);
        
        theMaterial.apply();
        
        gl::Shader &shader = theMaterial.getShader();

        static GLuint vertexArray = 0;
        
        if(!vertexArray)
        {
            uint32_t indexCount = 3 * theGeom.getFaces().size();
            boost::shared_array<GLuint> indices (new GLuint [indexCount]);
            GLuint *indexPtr = indices.get();
            
            // insert indices
            vector<gl::Face3>::const_iterator faceIt = theGeom.getFaces().begin();
            for (; faceIt != theGeom.getFaces().end(); faceIt++)
            {
                const gl::Face3 &face = *faceIt;
                
                for (int i = 0; i < 3; i++)
                {
                    // index
                    *(indexPtr++) = face.indices[i];
                }
            }
            
            // create interleaved array
            // GL_T2F_N3F_V3F
            uint32_t numFloats = 8;
            uint32_t interleavedCount = numFloats * theGeom.getVertices().size();
            boost::shared_array<GLfloat> interleaved (new GLfloat[interleavedCount]);
            
            for (int i = 0; i < theGeom.getVertices().size(); i++)
            {
                // texCoords
                const glm::vec2 &texCoord = theGeom.getTexCoords()[i];
                interleaved[numFloats * i ] = texCoord.s;
                interleaved[numFloats * i + 1] = texCoord.t;
                
                // normals
                const glm::vec3 &normal = theGeom.getNormals()[i];
                interleaved[numFloats * i + 2] = normal.x;
                interleaved[numFloats * i + 3] = normal.y;
                interleaved[numFloats * i + 4] = normal.z;
                
                // vertices
                const glm::vec3 &vert = theGeom.getVertices()[i];
                interleaved[numFloats * i + 5] = vert.x;
                interleaved[numFloats * i + 6] = vert.y;
                interleaved[numFloats * i + 7] = vert.z;
            }
            
            GLfloat *ptr = interleaved.get();
            
//            for (int i = 0; i < interleavedCount; i+=8)
//            {
//                printf("texCoord: (%.2f, %.2f) -- normals: (%.2f, %.2f, %.2f) -- position: (%.2f, %.2f, %.2f)\n",
//                       ptr[i],ptr[i+1],ptr[i+2],ptr[i+3],ptr[i+4],ptr[i+5],ptr[i+6],ptr[i+7]);
//            }
            
            glGenVertexArrays(1, &vertexArray);
            glBindVertexArray(vertexArray);
            
            GLuint vertexAttribLocation = shader.getAttribLocation("a_vertex");
            GLuint normalAttribLocation = shader.getAttribLocation("a_normal");
            GLuint texCoordAttribLocation = shader.getAttribLocation("a_texCoord");
            
            GLsizei stride = numFloats * sizeof(GLfloat);
            
            // interleaved VBO
            GLuint interleavedBuffer;
            glGenBuffers(1, &interleavedBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, interleavedBuffer);
            glBufferData(GL_ARRAY_BUFFER, numFloats * sizeof(GLfloat) * theGeom.getVertices().size(),
                         interleaved.get(), GL_STREAM_DRAW);//STREAM
            
            // define attrib pointer (texCoord)
            glEnableVertexAttribArray(texCoordAttribLocation);
            glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                                  stride,
                                  BUFFER_OFFSET(0));
            
            // define attrib pointer (normal)
            glEnableVertexAttribArray(normalAttribLocation);
            glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride,
                                  BUFFER_OFFSET(2 * sizeof(GLfloat)));
   
            // define attrib pointer (vertex)
            glEnableVertexAttribArray(vertexAttribLocation);
            glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                                  stride,
                                  BUFFER_OFFSET(5 * sizeof(GLfloat)));
            
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
        
        m_material = gl::Material::Ptr(new gl::Material);
        try
        {
            m_material->getShader().loadFromFile("shader_vert.glsl", "shader_frag.glsl");
        }catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_geometry =  gl::Geometry::Ptr( new gl::Plane(20, 20, 100, 100) );
        m_geometry->createGLBuffers();
        
        m_mesh = gl::Mesh::Ptr(new gl::Mesh(m_geometry, m_material));

        m_Camera.setPosition(glm::vec3(0, 0, 60));
        m_Camera.setLookAt(glm::vec3(0));
        
        m_material->addTexture(TextureIO::loadTexture("/Users/Fabian/Pictures/artOfNoise.png"));
        m_material->addTexture(TextureIO::loadTexture("/Users/Fabian/Pictures/David_Jien_02.png"));
        m_material->setTwoSided();
        
        m_distance = RangedProperty<float>::create("view distance", 25, -50, 50);
        registerProperty(m_distance);
        
        m_textureMix = RangedProperty<float>::create("texture mix ratio", 0.2, 0, 1);
        registerProperty(m_textureMix);
        m_material->uniform("u_textureMix", m_textureMix->val());
        
        m_wireFrame = Property_<bool>::create("Wireframe", false);
        registerProperty(m_wireFrame);
        
        m_color = Property_<glm::vec4>::create("Material color", glm::vec4(1 ,1 ,0, 0.6));
        registerProperty(m_color);
        m_material->setDiffuse(m_color->val());
        
        m_rotation = Property_<glm::mat3>::create("Geometry Rotation", glm::mat3());
        registerProperty(m_rotation);
        
        // add properties
        addPropertyListToTweakBar(getPropertyList());

        // enable observer mechanism
        observeProperties();
        
//        m_cvThread = CVThread::Ptr(new CVThread());
//        m_cvThread->streamUSBCamera();
    }
    
    void update(const float timeDelta)
    {

//        if(m_cvThread->hasImage())
//        {
//            vector<cv::Mat> images = m_cvThread->getImages();
//            TextureIO::updateTexture(m_material.getTextures()[0], images[0]);
//        }
        
    }
    
    void draw()
    {
        gl::Material cloneMat1 = *m_material, cloneMat2 = *m_material;
        
        cloneMat1.setDepthWrite(false);
        
        drawQuad(cloneMat1, getWindowSize());
        
        drawGeometry(*m_geometry, cloneMat2);
    }
    
    void resize(int w, int h)
    {
        m_Camera.setAspectRatio(getAspectRatio());
    }
    
    void tearDown()
    {
        printf("ciao simple geometry\n");
    }
    
    // Property observer callback
    void updateProperty(const Property::Ptr &theProperty)
    {
        // one of our porperties was changed
        if(theProperty == m_wireFrame)
            m_material->setWireframe(m_wireFrame->val());
        
        else if(theProperty == m_color)
            m_material->setDiffuse(m_color->val());
        
        else if(theProperty == m_textureMix)
            m_material->uniform("u_textureMix", m_textureMix->val());
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    
    return theApp->run();
}
