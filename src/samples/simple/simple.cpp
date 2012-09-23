#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

#include "TextureIO.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SimpleApp : public App 
{
private:

    gl::Texture m_texture;
    gl::Shader m_shader;
    
    GLuint m_cubeArray;
    GLuint m_cubeBuffer;
    
    GLuint m_canvasBuffer;
    GLuint m_canvasArray;
    
    float m_rotation;
    
    Property_<float>::Ptr m_distance;
    Property_<float>::Ptr m_rotationSpeed;
    Property_<vec3>::Ptr m_lightDir;
    
    Property_<mat4>::Ptr m_viewMatrix;
    
    Property_<string>::Ptr m_infoString;
    
    Property_<vec4>::Ptr m_lightColor;

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
        
        glGenBuffers(1, &m_canvasBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_canvasBuffer);
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
        glGenBuffers(1, &m_cubeBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeBuffer);
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
        mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
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
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
        }
        
        m_texture = TextureIO::loadTexture("/Volumes/CrocData/Users/Fabian/Pictures/leda.jpg");
        
        buildCubeVBO();
        
        m_rotation = 0.f;
        
        m_distance = Property_<float>::create("Cam distance", 1);
        m_rotationSpeed = Property_<float>::create("RotationSpeed", 1.f);
        m_lightDir = Property_<vec3>::create("LightDir", vec3(0.0, 1.0, 1.0));
        m_infoString = Property_<string>::create("Info Bla",
                                                 "This is some infoo bla for uu");
        
        m_lightColor = Property_<vec4>::create("lightColor", vec4(1));
        
        m_viewMatrix = Property_<mat4>::create("viewMatrix", mat4());
        
        // add props to tweakbar
        addPropertyToTweakBar(m_distance, "Floats");
        addPropertyToTweakBar(m_rotationSpeed, "Floats");
        addPropertyToTweakBar(m_lightDir, "Vecs");
        addPropertyToTweakBar(m_viewMatrix);
        addPropertyToTweakBar(m_infoString);
        addPropertyToTweakBar(m_lightColor);
        
        // properties can be tweaked at any time
        m_distance->val(2);
        *m_distance = 3;
        m_lightDir->val(vec3(0.0f, 3.0f, 1.0f));
    }
    
    void tearDown()
    {
        printf("ciao simple love\n");
    }
    
    void update(const float timeDelta)
    {
        m_rotation += degrees(timeDelta * (*m_rotationSpeed) );
        
        //*m_distance += 0.01;
    }
    
    void draw()
    {
        //drawTexture(m_texture);
        
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(m_texture);
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        
        mat4 projectionMatrix = perspective(65.0f, getAspectRatio(), 0.1f, 100.0f);
        
        m_viewMatrix->val(lookAt(m_distance->val() * vec3(0, 1, 1), // eye
                                 vec3(0),                           // lookat
                                 vec3(0, 1, 0)));                    // up
        
        mat4 modelViewTmp = m_viewMatrix->val();
        //modelViewMatrix = translate(modelViewMatrix, vec3(0, 0, -1.5));
        modelViewTmp = rotate(modelViewTmp, m_rotation, vec3(1, 1, 1));
        
        mat3 normalMatrix = inverseTranspose(mat3(modelViewTmp));
        
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix * modelViewTmp);
        m_shader.uniform("u_normalMatrix", normalMatrix);
        m_shader.uniform("u_lightDir", m_lightDir->val());
        m_shader.uniform("u_lightColor", m_lightColor->val());
        
        drawCube();
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SimpleApp);
    
    return theApp->run();
}
