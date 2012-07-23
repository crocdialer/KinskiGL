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
    gl::Texture m_texture_thumb;
    
    gl::Shader m_shader;
    
    GLuint m_canvasBuffer;
    GLuint m_canvasArray;
    
    _Property<bool>::Ptr m_activator;
    
    CVThread::Ptr m_cvThread;
    
    CVProcessNode::Ptr m_processNode;
    
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
    
    void drawTexture(gl::Texture theTexture,
                     const vec2 &theTl = vec2(0),
                     const vec2 &theSize = vec2(0))
    {
        vec2 sz = theSize == vec2(0) ? theTexture.getSize() : theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, getHeight()) : theTl;
        drawTexture(theTexture, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawTexture(const gl::Texture &theTexture, float x0, float y0, float x1, float y1)
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(theTexture);
        
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / getWidth();
        float scaleY = (y0 - y1) / getHeight();
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / getWidth(), y1 / getHeight() , 0, 1);
        
        m_shader.uniform("u_textureMap[0]", theTexture.getBoundTextureUnit());
        m_shader.uniform("u_textureMatrix", theTexture.getTextureMatrix());
        
        m_shader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix * modelViewMatrix);
        
        glBindVertexArray(m_canvasArray);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
public:
    
    void setup()
    {
        glEnable(GL_TEXTURE_2D);
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            m_shader.loadFromFile("applyMap.vert", "applyMap.frag");
            //m_shader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
            exit(EXIT_FAILURE);
        }
        
        buildCanvasVBO();
        
        m_activator = _Property<bool>::create("processing", true);
        
        // add component-props to tweakbar
        addPropertyToTweakBar(m_activator);
        
        // CV stuff 
        
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr (new ColorHistNode);
        
        m_cvThread->setProcessingNode(m_processNode);
        
        addPropertyListToTweakBar(m_processNode->getPropertyList());
        
        CVSourceNode::Ptr sourceNode(new CVCaptureNode);
        
        //sourceNode = CVSourceNode::Ptr(new CVBufferedSourceNode(sourceNode));
        
        m_cvThread->streamVideo("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/testMovie_00.mov",
                                true);
        
        //m_cvThread->setSourceNode(sourceNode);
        m_cvThread->start();
        
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
            vector<cv::Mat> images = m_cvThread->getImages();
            
            TextureIO::updateTexture(m_texture, images[0]);
            TextureIO::updateTexture(m_texture_thumb,
                                     images[1]);
            
            
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
    }
    
    void draw()
    {
        gl::scoped_bind<gl::Shader> shaderBind(m_shader);
        
        m_texture_thumb.bind(1);
        m_shader.uniform("u_textureMap[1]", m_texture_thumb.getBoundTextureUnit());
        
        drawTexture(m_texture, vec2(0, getHeight()), getWindowSize());
        
        drawTexture(m_texture_thumb,
                    vec2(getWidth() - getWidth()/5.f - 20, getHeight() - 20),
                    getWindowSize()/5.f);
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new SkySegmenter);
    
    return theApp->run();
}
