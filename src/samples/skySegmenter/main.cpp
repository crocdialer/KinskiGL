#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "Data.h"

#include "kinskiGL/TextureIO.h"
#include "kinskiCV/CVThread.h"

#include "SkySegmentNode.h"
#include "ColorHistNode.h"

using namespace std;
using namespace kinski;
using namespace glm;

class SkySegmenter : public App 
{
private:
    
    gl::Texture m_texture;
    gl::Shader m_shader;
    
    GLuint m_canvasBuffer;
    GLuint m_canvasArray;
    
    _Property<bool>::Ptr m_activator;
    
    CVThread::Ptr m_cvThread;
    
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
    
    void drawTexture(gl::Texture theTexture, gl::Shader theShader)
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
    
public:
    
    void setup()
    {
        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
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
        
        m_cvThread = CVThread::Ptr(new CVThread());
        //CVProcessNode::Ptr procNode(new SkySegmentNode);
        CVProcessNode::Ptr procNode(new ColorHistNode);
        
        m_cvThread->setProcessingNode(procNode);
        
        
        addPropertyListToTweakBar(procNode->getPropertyList());
        
        buildCanvasVBO();
        
        m_activator = _Property<bool>::create("processing", true);
        
        // add props to tweakbar
        addPropertyToTweakBar(m_activator);
        
        m_cvThread->streamVideo("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/testMovie_00.mov",
                                true);
        
        cout<<"CVThread source: \n"<<m_cvThread->getSourceInfo()<<"\n";
    }
    
    void tearDown()
    {
        m_cvThread->stop();
        
        printf("ciao skySegmenter\n");
    }
    
    void update(const float timeDelta)
    {
        cv::Mat camFrame;
        if(m_cvThread->getImage(camFrame))
        {	
            TextureIO::updateTexture(m_texture, camFrame);
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
        
        //printf("processing: %.2f ms\n", m_cvThread->getLastProcessTime() * 1000.);
    }
    
    void draw()
    {
        glDisable(GL_DEPTH_TEST);
        drawTexture(m_texture, m_shader);
        glEnable(GL_DEPTH_TEST);
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SkySegmenter);
    
    return theApp->run();
}
