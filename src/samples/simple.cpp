#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

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
    
public:
    
    void setup()
    {
//        printf("%s\n", g_vertShaderSrc);
//        printf("%s\n", g_fragShaderSrc);
        
        glEnable(GL_TEXTURE_2D);
        
        try 
        {
            //m_shader.loadFromFile("texShader.vsh", "texShader.fsh");
            m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }
        catch (std::exception &e) 
        {
            fprintf(stdout, "%s\n",e.what());
        }
        
        m_shader.bindFragDataLocation("fragData");
        
        // orthographic projection with a [0,1] coordinate space
        //m_projectionMatrix = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
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
    
    void update(const float timeDelta)
    {
        float aspect = fabsf(getWidth() / getHeight());
        m_projectionMatrix = glm::perspective(65.0f, aspect, 0.1f, 100.0f);

        glm::mat4 modelViewMatrix;
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         m_projectionMatrix * modelViewMatrix);
    }
    
    void draw()
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(m_texture);
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        
        if(m_texture)
        {
            m_shader.uniform("u_textureMap", m_texture.getBoundTextureUnit());
            m_shader.uniform("u_textureMatrix", m_texture.getTextureMatrix());
        }
        
        glBindVertexArray(m_vertexArrayObject);
        glDrawArrays(GL_TRIANGLES, 0, 36);
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
