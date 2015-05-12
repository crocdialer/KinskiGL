#include "app/ViewerApp.h"

#include "cv/CVThread.h"
#include "cv/TextureIO.h"
#include "KeyPointNode.h"

using namespace std;
using namespace kinski;
using namespace glm;

class KeypointApp : public ViewerApp
{
private:
    
    Property_<bool>::Ptr
    m_activator = Property_<bool>::create("processing", true);
    
    Property_<std::string>::Ptr
    m_img_path = Property_<std::string>::create("image path", "");
    
    RangedProperty<uint32_t>::Ptr
    m_imageIndex = RangedProperty<uint32_t>::create("Image Index", 2, 0, 2);
    
    CVThread::Ptr m_cvThread;
    CVProcessNode::Ptr m_processNode;
    
public:
    
    void setup()
    {
        registerProperty(m_activator);
        registerProperty(m_img_path);
        registerProperty(m_imageIndex);
        
        create_tweakbar_from_component(shared_from_this());
        observeProperties();
        
        // CV stuff 
        m_cvThread = std::make_shared<CVThread>();
        m_processNode = std::make_shared<KeyPointNode>(cv::imread(search_file("~/Desktop/keypoint_test_01.jpg")));
        
        // trigger observer callbacks
        m_processNode->observeProperties();
        create_tweakbar_from_component(m_processNode);
        
        m_cvThread->setProcessingNode(m_processNode);

        m_cvThread->streamUSBCamera();
        
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
            
            
            for(int i = 0; i < images.size(); i++)
                gl::TextureIO::updateTexture(m_textures[i], images[i]);
            
            m_imageIndex->setRange(0, images.size() - 1);
        }
        
        // trigger processing
        m_cvThread->setProcessing(*m_activator);
    }
    
    void draw()
    {
        // draw fullscreen image
        gl::drawTexture(textures()[*m_imageIndex], gl::windowDimension());
        
        if(displayTweakBar()){ draw_textures(textures()); }
    }
    
    void fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
    {
        ViewerApp::fileDrop(e, files);
        
        for(const string &f : files)
        {
            LOG_DEBUG << f;
            
            switch (get_filetype(f))
            {
                case FileType::FILE_IMAGE:
                    *m_img_path = f;
                    break;
                default:
                    break;
            }
        }
    }
    
    void updateProperty(const Property::ConstPtr &theProperty)
    {
        ViewerApp::updateProperty(theProperty);
        
        if(theProperty == m_img_path)
        {
            if(m_processNode)
            {
                std::dynamic_pointer_cast<KeyPointNode>(m_processNode)->setReferenceImage(cv::imread(search_file(*m_img_path)));
            }
        }
    }
};

int main(int argc, char *argv[])
{
    auto theApp = std::make_shared<KeypointApp>();    
    return theApp->run();
}
