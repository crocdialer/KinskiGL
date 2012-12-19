#include "kinskiApp/App.h"
#include "kinskiApp/TextureIO.h"

#include "kinskiGL/Material.h"

#include "kinskiCV/CVThread.h"
#include "SalienceNode.h"

#include "kinskiGL/SerializerGL.h"

using namespace std;
using namespace kinski;
using namespace glm;

class Saliencer : public App
{
private:
    
    gl::Texture m_textures[4];
    
    gl::Material m_material;
    
    Property_<bool>::Ptr m_activator;
    
    RangedProperty<uint32_t>::Ptr m_imageIndex;
    
    CVThread::Ptr m_cvThread;
    
    CVProcessNode::Ptr m_processNode;
    
public:
    
    void setup()
    {
        setBarColor(vec4(0, 0 ,0 , .5));
        setBarSize(ivec2(250, 500));

        // add 2 empty textures
        m_material.addTexture(m_textures[0]);
        m_material.addTexture(m_textures[1]);
        m_material.setDepthTest(false);
        m_material.setDepthWrite(false);
        
        m_activator = Property_<bool>::create("processing", true);
        m_imageIndex = RangedProperty<uint32_t>::create("Image Index",
                                                         0, 0, 1);
        
        registerProperty(m_activator);
        registerProperty(m_imageIndex);
        
        // add component-props to tweakbar
        addPropertyListToTweakBar(getPropertyList());
        
        // CV stuff
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr(new SalienceNode);
        
        m_cvThread->setProcessingNode(m_processNode);
        
        m_cvThread->streamVideo("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/testMovie_00.MOV", true);
        //m_cvThread->streamUSBCamera();
        
        
        if(m_processNode)
        {
            addPropertyListToTweakBar(m_processNode->getPropertyList());
            cout<<"CVProcessNode: \n"<<m_processNode->getDescription()<<"\n";
        }
        
        cout<<"CVThread source: \n"<<m_cvThread->getSourceInfo()<<"\n";
        
        try
        {
            m_material.shader().loadFromFile("applyMap.vert", "applyMap.frag");
            Serializer::loadComponentState(shared_from_this(), "config.json", PropertyIO_GL());
            
        }catch(Exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
        }catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
            exit(EXIT_FAILURE);
        }
        
    }
    
    void tearDown()
    {
        m_cvThread->stop();
        Serializer::saveComponentState(shared_from_this(), "config.json", PropertyIO_GL());
        
        printf("ciao saliencer\n");
    }
    
    void update(const float timeDelta)
    {
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i=0;i<images.size();i++)
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
            
            m_imageIndex->setRange(0, images.size() - 1);
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
    }
    
    void draw()
    {
        // draw fullscreen image
        if(m_activator->val())
            gl::drawQuad(m_material, getWindowSize());
        else
            gl::drawTexture(m_material.textures()[m_imageIndex->val()], getWindowSize());
        
        // draw process-results map(s)
        glm::vec2 offset(getWidth() - getWidth()/5.f - 10, getHeight() - 10);
        glm::vec2 step(0, - getHeight()/5.f - 10);
        
        for(int i=0;i<m_cvThread->getImages().size();i++)
        {
            gl::drawTexture(m_textures[i],
                            getWindowSize()/5.f,
                            offset);
            
            offset += step;
        }
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new Saliencer);
    
    return theApp->run();
}
