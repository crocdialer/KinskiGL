#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

#include "TextureIO.h"

using namespace std;
using namespace kinski;

class PoopApp : public App 
{
private:
    
    glm::mat4 m_projectionMatrix;
    gl::Texture m_texture;
    gl::Shader m_shader;
    
    GLuint m_vertexArrayObject;
    GLuint m_vertexBuffer;
    
    GLuint m_otherVertBuf;
    GLuint m_otherVertArray;
    
    float m_rotation;

    void buildCanvasVBO()
    {
        //GL_T2F_V3F
        const GLfloat array[] ={0.0,0.0,0.0,0.0,0.0,
                                1.0,0.0,1.0,0.0,0.0,
                                1.0,1.0,1.0,1.0,0.0,
                                0.0,1.0,0.0,1.0,0.0};
        
        // create VAO to record all VBO calls
        glGenVertexArrays(1, &m_otherVertArray);
        glBindVertexArray(m_otherVertArray);
        
        glGenBuffers(1, &m_otherVertBuf);
        glBindBuffer(GL_ARRAY_BUFFER, m_otherVertBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(array), array, GL_STATIC_DRAW);
        
        GLsizei stride = 5 * sizeof(GLfloat);
        
        GLuint positionAttribLocation = m_shader.getAttribLocation("a_position");
        glEnableVertexAttribArray(positionAttribLocation);
        glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
        
        GLuint texCoordAttribLocation = m_shader.getAttribLocation("a_texCoord");
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(0));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glBindVertexArray(0);
        
    }
    
public:
    
    void setup()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        m_texture = TextureIO::loadTexture("/Volumes/CrocData/Users/Fabian/Pictures/leda.jpg");
        
        try 
        {
            //m_shader.loadFromFile("texShader.vsh", "texShader.fsh");
            m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }
        catch (std::exception &e) 
        {
            fprintf(stdout, "%s\n",e.what());
        }
        
        //m_shader.bindFragDataLocation("fragData");
        
        // orthographic projection with a [0,1] coordinate space
        m_projectionMatrix = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        // create and bind vertex array object
        glGenVertexArrays(1, &m_vertexArrayObject);
        glBindVertexArray(m_vertexArrayObject);
        
        // create vertex buffers and attrib locations
        glGenBuffers(1, &m_vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_CubeVertexData), g_CubeVertexData, GL_STATIC_DRAW);
        
        GLuint positionAttribLocation = m_shader.getAttribLocation("a_position");
        GLuint normalAttribLocation = m_shader.getAttribLocation("a_normal");
        GLuint texCoordAttribLocation = m_shader.getAttribLocation("a_texCoord");
        
//        printf("pos: %d -- normal: %d -- tex: %d\n",    positionAttribLocation,
//                                                        0,
//                                                        texCoordAttribLocation);
        // define attrib pointers
        GLsizei stride = 8 * sizeof(GLfloat);
        
        glEnableVertexAttribArray(positionAttribLocation);
        glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(0));
        glEnableVertexAttribArray(normalAttribLocation);
        glVertexAttribPointer(normalAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride,
                              BUFFER_OFFSET(6 * sizeof(GLfloat)));
        // unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        m_rotation = 0.f;
        
        buildCanvasVBO();
    }
    
    void update(const float timeDelta)
    {
//        float aspect = fabsf(getWidth() / getHeight());
//        m_projectionMatrix = glm::perspective(65.0f, aspect, 0.1f, 100.0f);
//        
//        glm::mat4 modelViewMatrix;
//        modelViewMatrix = glm::translate(modelViewMatrix, glm::vec3(0, 0, -4.0));
//        //modelViewMatrix = glm::rotate(modelViewMatrix, m_rotation, glm::vec3(1, 1, 1));
//        
//        m_shader.uniform("u_modelViewProjectionMatrix", 
//                         m_projectionMatrix * modelViewMatrix);
//        m_rotation += timeDelta * 0.5;
    }
    
    void draw()
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(m_texture);
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        

        m_shader.uniform("u_textureMap", m_texture.getBoundTextureUnit());
        m_shader.uniform("u_textureMatrix", m_texture.getTextureMatrix());
        
//        glBindVertexArray(m_vertexArrayObject);
//        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         m_projectionMatrix);
        
        //fragData
        //m_shader.bindFragDataLocation("fragData");
        
        glBindVertexArray(m_otherVertArray);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
};

int main(int argc, char *argv[])
{
    char b[512];
    getcwd(b, sizeof(b));
    printf("working dir: '%s'\n", b);
    
    PoopApp::Ptr theApp(new PoopApp);
    
    return theApp->run();
}
