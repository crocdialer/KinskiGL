#include "app/ViewerApp.h"

#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "KeyPointNode.h"

#include "Data.h"

using namespace std;
using namespace kinski;
using namespace glm;

class KeypointApp : public ViewerApp
{
private:
    
    Property_<bool>::Ptr m_activator;
    
    RangedProperty<uint32_t>::Ptr m_imageIndex;
    
    CVThread::Ptr m_cvThread;
    
    CVProcessNode::Ptr m_processNode;
    
public:
    
    void setup()
    {
        m_activator = Property_<bool>::create("processing", true);
        registerProperty(m_activator);
        
        m_imageIndex = RangedProperty<uint32_t>::create("Image Index", 2, 0, 2);
        registerProperty(m_imageIndex);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        // CV stuff 
        m_cvThread = CVThread::Ptr(new CVThread());
        m_processNode = CVProcessNode::Ptr(new KeyPointNode(cv::imread( search_file("~/Desktop/keypoint_test_01.jpg") )));
        
//        m_processNode = m_processNode >> CVProcessNode::Ptr(new CVDiskWriterNode
//                                                           ("/Users/Fabian/Desktop/video.avi"));
        
        // trigger observer callbacks
        m_processNode->observeProperties();
        create_tweakbar_from_component(m_processNode);
        
        m_cvThread->setProcessingNode(m_processNode);

        m_cvThread->streamUSBCamera();
//        m_cvThread->streamVideo("~/Desktop/keypoint_test_01.mov", true);
        
        if(m_processNode)
        {
            addPropertyListToTweakBar(m_processNode->getPropertyList());
            LOG_INFO<<"CVProcessNode: "<<m_processNode->getDescription();
        }
        
        LOG_INFO<<"CVThread source: "<<m_cvThread->getSourceInfo();
        
    }
    
    void tearDown()
    {
        m_cvThread->stop();
        
        LOG_PRINT<<"ciao keypointer";
    }
    
    void update(float timeDelta)
    {
        if(m_cvThread->hasImage())
        {
            vector<cv::Mat> images = m_cvThread->getImages();
            
            float imgAspect = images.front().cols/(float)images.front().rows;
            setWindowSize( vec2(getWidth(), getWidth() / imgAspect) );
            
            
            for(int i=0;i<images.size();i++)
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
            
            m_imageIndex->setRange(0, images.size() - 1);
        }
        
        // trigger processing
        m_cvThread->setProcessing(*m_activator);
    }
    
    void draw()
    {
        // draw fullscreen image
        gl::drawTexture(textures()[*m_imageIndex], windowSize());
        
        // draw process-results map(s)
        glm::vec2 offet(getWidth() - getWidth()/5.f - 10, getHeight() - 10);
        glm::vec2 step(0, - getHeight()/5.f - 10);
        
        if(displayTweakBar())
        {
            draw_textures(textures());
        }
        
    }
};

int main(int argc, char *argv[])
{
    App::Ptr theApp(new KeypointApp);
    
    return theApp->run();
}
