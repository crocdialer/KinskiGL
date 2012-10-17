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
    
    void drawGeometry(const gl::Geometry &theGeom, gl::Material &theMaterial)
    {
        theMaterial.apply();
        
        gl::Shader &shader = theMaterial.getShader();
        
        glm::vec3 position (0,0, -50);
        
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
                
                // texCoords
                const glm::vec2 &texCoord = theGeom.getTexCoords()[idx];
                interleaved[8 * idx ] = texCoord.s;
                interleaved[8 * idx + 1] = texCoord.t;
                
                // normals
                const glm::vec3 &normal = face.vertexNormals[i];
                interleaved[8 * idx + 2] = normal.x;
                interleaved[8 * idx + 3] = normal.y;
                interleaved[8 * idx + 4] = normal.z;
                
                // vertices
                const glm::vec3 &vert = theGeom.getVertices()[idx];
                interleaved[8 * idx + 5] = vert.x;
                interleaved[8 * idx + 6] = vert.y;
                interleaved[8 * idx + 7] = vert.z;
            }
        }
        
        GLuint vertexAttribLocation = shader.getAttribLocation("a_vertex");
        GLuint normalAttribLocation = shader.getAttribLocation("a_normal");
        GLuint texCoordAttribLocation = shader.getAttribLocation("a_texCoord");
        
        // define attrib pointers
        GLsizei stride = 8 * sizeof(GLfloat);
        
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride,
                              interleaved.get());
        
        glEnableVertexAttribArray(normalAttribLocation);
        glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              interleaved.get() + 2);
    
        glEnableVertexAttribArray(vertexAttribLocation);
        glVertexAttribPointer(vertexAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              interleaved.get() + 5);
        
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices.get());
    };
    
    
public:
    
    void setup()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            m_material.getShader().loadFromFile("shader_vert.glsl", "shader_frag.glsl");
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_geometry = gl::Plane(100, 100);
    }
    
    void tearDown()
    {
        printf("ciao simple geometry\n");
    }
    
    void update(const float timeDelta)
    {
        drawGeometry(m_geometry, m_material);
    }
    
    void draw()
    {
        
    }
    
    void resize(int w, int h)
    {
        m_Camera.setAspectRatio(getAspectRatio());
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleGeometryApp);
    
    return theApp->run();
}
