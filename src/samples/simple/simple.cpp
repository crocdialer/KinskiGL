#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

#include "TextureIO.h"
#include "opencv2/opencv.hpp"

using namespace std;
using namespace kinski;

class SimpleApp : public App 
{
private:

    gl::Texture m_texture;
    gl::Shader m_shader;
    
    GLuint m_cubeArray;
    GLuint m_vertexBuffer;
    
    GLuint m_otherVertBuf;
    GLuint m_canvasArray;
    
    float m_rotation;
    float m_rotationSpeed;
    
    cv::VideoCapture m_capture;
    
    void activateCamera(bool b = true)
    {
        if(b)
            m_capture.open(0);
        else
            m_capture.release();
    }

    void buildCanvasVBO()
    {
        //GL_T2F_V3F
        const GLfloat array[] ={0.0,0.0,0.0,0.0,0.0,
                                1.0,0.0,1.0,0.0,0.0,
                                1.0,1.0,1.0,1.0,0.0,
                                0.0,1.0,0.0,1.0,0.0};
        
        // create VAO to record all VBO calls
        glGenVertexArrays(1, &m_canvasArray);
        glBindVertexArray(m_canvasArray);
        
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
    
    void buildCubeVBO()
    {
        // create and bind vertex array object
        glGenVertexArrays(1, &m_cubeArray);
        glBindVertexArray(m_cubeArray);
        
        // create vertex buffers and attrib locations
        glGenBuffers(1, &m_vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_CubeVertexData), g_CubeVertexData, GL_STATIC_DRAW);
        
        GLuint positionAttribLocation = m_shader.getAttribLocation("a_position");
        GLuint normalAttribLocation = m_shader.getAttribLocation("a_normal");
        GLuint texCoordAttribLocation = m_shader.getAttribLocation("a_texCoord");
        
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
        
    }
    
    void drawTexture(gl::Texture theTexture)
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(theTexture);
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        
        // orthographic projection with a [0,1] coordinate space
        glm::mat4 projectionMatrix = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        m_shader.uniform("u_textureMap", m_texture.getBoundTextureUnit());
        m_shader.uniform("u_textureMatrix", m_texture.getTextureMatrix());
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix);
        
        glBindVertexArray(m_canvasArray);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
    void drawCube()
    {
        m_shader.uniform("u_textureMap", m_texture.getBoundTextureUnit());
        m_shader.uniform("u_textureMatrix", m_texture.getTextureMatrix());
        
        glBindVertexArray(m_cubeArray);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
public:
    
    void setup()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            //m_shader.loadFromFile("texShader.vsh", "texShader.fsh");
            m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }
        catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_texture = TextureIO::loadTexture("/Volumes/CrocData/Users/Fabian/Pictures/leda.jpg");
        
        buildCubeVBO();
        
        m_rotation = 0.f;
        m_rotationSpeed = 1.f;
    }
    
    void tearDown()
    {
        printf("ciao simple love\n");
    }
    
    void update(const float timeDelta)
    {
        m_rotation += glm::degrees(timeDelta * m_rotationSpeed);
    }
    
    void draw()
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(m_texture);
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        
        glm::mat4 projectionMatrix = glm::perspective(65.0f, getAspectRatio(), 0.1f, 100.0f);
        
        glm::mat4 modelViewMatrix;
        modelViewMatrix = glm::translate(modelViewMatrix, glm::vec3(0, 0, -1.5));
        modelViewMatrix = glm::rotate(modelViewMatrix, m_rotation, glm::vec3(1, 1, 1));
        
        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelViewMatrix));
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix * modelViewMatrix);
        m_shader.uniform("u_normalMatrix", normalMatrix);
        
        drawCube();
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleApp);

    return theApp->run();
}
