#include "kinskiGL/App.h"
#include "kinskiGL/Texture.h"
#include "kinskiGL/Shader.h"
#include "kinskiGL/TextureIO.h"

#include "kinskiCV/CVThread.h"
#include "KeyPointNode.h"

#include "Data.h"

using namespace std;
using namespace kinski;
using namespace glm;

class KeypointApp : public App 
{
private:
    
    gl::Texture m_textures[4];
    
    gl::Shader m_texShader;
    gl::Shader m_applyMapShader;
    
    GLuint m_canvasBuffer;
    GLuint m_canvasArray;
    
    _Property<bool>::Ptr m_activator;
    
    _RangedProperty<uint32_t>::Ptr m_imageIndex;
    
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
        
        GLuint positionAttribLocation = m_texShader.getAttribLocation("a_position");
        glEnableVertexAttribArray(positionAttribLocation);
        glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(2 * sizeof(GLfloat)));
        
        GLuint texCoordAttribLocation = m_texShader.getAttribLocation("a_texCoord");
        glEnableVertexAttribArray(texCoordAttribLocation);
        glVertexAttribPointer(texCoordAttribLocation, 2, GL_FLOAT, GL_FALSE,
                              stride, BUFFER_OFFSET(0));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glBindVertexArray(0);
        
    }
    
    void drawTexture(gl::Texture &theTexture,
                     gl::Shader &theShader,
                     const vec2 &theTl = vec2(0),
                     const vec2 &theSize = vec2(0))
    {
        vec2 sz = theSize == vec2(0) ? theTexture.getSize() : theSize;
        vec2 tl = theTl == vec2(0) ? vec2(0, getHeight()) : theTl;
        drawTexture(theTexture, theShader, tl[0], tl[1], (tl+sz)[0], tl[1]-sz[1]);
    }
    
    
    void drawTexture(gl::Texture &theTexture,
                     gl::Shader &theShader,
                     float x0, float y0, float x1, float y1)
    {
        // Texture and Shader bound for this scope
        gl::scoped_bind<gl::Texture> texBind(theTexture);
        gl::scoped_bind<gl::Shader> shaderBind(theShader);
        
        // orthographic projection with a [0,1] coordinate space
        static mat4 projectionMatrix = ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        
        float scaleX = (x1 - x0) / getWidth();
        float scaleY = (y0 - y1) / getHeight();
        
        mat4 modelViewMatrix = glm::scale(mat4(), vec3(scaleX, scaleY, 1));
        modelViewMatrix[3] = vec4(x0 / getWidth(), y1 / getHeight() , 0, 1);
        
        theShader.uniform("u_textureMap[0]", theTexture.getBoundTextureUnit());
        theShader.uniform("u_textureMatrix", theTexture.getTextureMatrix());
        
        theShader.uniform("u_modelViewProjectionMatrix", 
                         projectionMatrix * modelViewMatrix);
        
        glBindVertexArray(m_canvasArray);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
public:
    
    void setup()
    {
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glClearColor(0, 0, 0, 1);
        
        try 
        {
            m_applyMapShader.loadFromFile("applyMap.vert", "applyMap.frag");
            m_texShader.loadFromData(g_vertShaderSrc, g_fragShaderSrc);
            
        }catch (std::exception &e) 
        {
            fprintf(stderr, "%s\n",e.what());
            exit(EXIT_FAILURE);
        }
        
        buildCanvasVBO();
        
        m_activator = _Property<bool>::create("processing", true);
        m_imageIndex = _RangedProperty<uint32_t>::create("Image Index",
                                                         2, 0, 2);
        
        Property::Ptr bla = static_pointer_cast<Property>(m_imageIndex);
        
        // add component-props to tweakbar
        addPropertyToTweakBar(m_activator);
        addPropertyToTweakBar(m_imageIndex);
        
        // CV stuff 
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr(new KeyPointNode(cv::imread("kinder.jpg")));
        
//        m_processNode = m_processNode >> CVProcessNode::Ptr(new CVDiskWriterNode
//                                                           ("/Users/Fabian/Desktop/video.avi"));
        
        // trigger observer callbacks
        m_processNode->observeProperties();
        
        m_cvThread->setProcessingNode(m_processNode);

        //m_cvThread->streamUSBCamera();
        m_cvThread->streamVideo("kinderFeat.mov", true);
        
        if(m_processNode)
        {
            addPropertyListToTweakBar(m_processNode->getPropertyList());
            cout<<"CVProcessNode: \n"<<m_processNode->getDescription()<<"\n";
        }
        
        cout<<"CVThread source: \n"<<m_cvThread->getSourceInfo()<<"\n";
        
    }
    
    void tearDown()
    {
        m_cvThread->stop();
        
        printf("ciao keypointer\n");
    }
    
    void update(const float timeDelta)
    {
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            float imgAspect = images.front().cols/(float)images.front().rows;
            setWindowSize(getWidth(), getWidth() / imgAspect);
            
            
            for(int i=0;i<images.size();i++)
                TextureIO::updateTexture(m_textures[i], images[i]);
            
            m_imageIndex->setRange(0, images.size() - 1);
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
    }
    
    void draw()
    {
        char buf[128];
     
        m_applyMapShader.bind();
        
        for(int i=0;i<m_cvThread->getImages().size();i++)
        {
            m_textures[i].bind(i);
            sprintf(buf, "u_textureMap[%d]", i);
            m_applyMapShader.uniform(buf, m_textures[i].getBoundTextureUnit());
        }

        // draw fullscreen image
        drawTexture(m_textures[m_imageIndex->val()],
                    m_activator->val() ? m_applyMapShader : m_texShader,
                    vec2(0, getHeight()),
                    getWindowSize());
        
        // draw process-results map(s)
        glm::vec2 offet(getWidth() - getWidth()/5.f - 10, getHeight() - 10);
        glm::vec2 step(0, - getHeight()/5.f - 10);
        
        for(int i=0;i<m_cvThread->getImages().size();i++)
        {
            drawTexture(m_textures[i],
                        m_texShader,
                        offet,
                        getWindowSize()/5.f);
            
            offet += step;
        }
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new KeypointApp);
    
    return theApp->run();
}
