#include "kinskiApp/App.h"
#include "kinskiApp/TextureIO.h"
#include "kinskiGL/Material.h"

#include "kinskiCV/CVThread.h"
#include "ColorHistNode.h"

using namespace std;
using namespace kinski;
using namespace glm;

class ColorHistApp : public App
{
private:
    
    gl::Texture m_textures[4];
    
    gl::Material::Ptr m_material;
    
    Property_<bool>::Ptr m_activator;
    
    CVThread::Ptr m_cvThread;
    
    CVProcessNode::Ptr m_processNode;
    
public:
    
    void setup()
    {
        m_material = gl::Material::Ptr(new gl::Material);
        
        try
        {
            m_material->getShader().loadFromFile("applyMap.vert", "applyMap.frag");
            m_material->addTexture(m_textures[0]);
            m_material->addTexture(m_textures[1]);
            m_material->addTexture(m_textures[2]);
            
        }catch (std::exception &e)
        {
            fprintf(stderr, "%s\n",e.what());
            exit(EXIT_FAILURE);
        }
        
        m_activator = Property_<bool>::create("processing", true);
        
        // add component-props to tweakbar
        addPropertyToTweakBar(m_activator);
        
        // CV stuff
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr(new ColorHistNode);
        
        m_cvThread->setProcessingNode(m_processNode);
        
        m_cvThread->streamVideo("/Users/Fabian/dev/testGround/python/cvScope/scopeFootage/testMovie_00.MOV", true);
        
        
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
        
        printf("ciao colorHisto\n");
    }
    
    void update(const float timeDelta)
    {
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            for(int i=0;i<images.size();i++)
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
        }
        
        // trigger processing
        m_cvThread->setProcessing(m_activator->val());
    }
    
    void draw()
    {
        // draw fullscreen image
        if(m_activator->val())
        {
            gl::drawQuad(*m_material, getWindowSize());
        }
        else
        {
            gl::drawTexture(m_textures[0], getWindowSize());
        }
        
        // draw process-results map(s)
        glm::vec2 offset(getWidth() - getWidth()/5.f - 10, getHeight() - 10);
        glm::vec2 step(0, - getHeight()/5.f - 10);
        
        for(int i=0;i<m_cvThread->getImages().size();i++)
        {
            drawTexture(m_textures[i], getWindowSize()/5.f, offset);
            
            offset += step;
        }
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new ColorHistApp);
    
    return theApp->run();
}
