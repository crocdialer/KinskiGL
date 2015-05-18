//
//  KeyPointApp.cpp
//  gl
//
//  Created by Fabian on 29/01/14.
//
//

#include "KeyPointApp.hpp"

#include "video/MovieController.h"
#include "video/CameraController.h"

using namespace std;
using namespace kinski;
using namespace glm;

class CameraSource : public CVSourceNode
{
public:
    CameraSource(){ m_camera->start_capture(); };
    
    bool getNextImage(cv::Mat &img) override
    {
        int w, h;
        
        if(m_camera->copy_frame(m_img_buffer, &w, &h))
        {
            img = cv::Mat(h, w, CV_8UC4, &m_img_buffer[0]);
        }
        
        return true;
    }
    
private:
    video::CameraControllerPtr m_camera = video::CameraController::create();
    std::vector<uint8_t> m_img_buffer;
};

/////////////////////////////////////////////////////////////////

void KeyPointApp::setup()
{
    registerProperty(m_activator);
    registerProperty(m_img_path);
    registerProperty(m_imageIndex);
    
    create_tweakbar_from_component(shared_from_this());
    observeProperties();
    
    // CV stuff
    m_cvThread = std::make_shared<CVThread>();
    m_processNode = std::make_shared<KeyPointNode>(cv::imread(search_file("kinder.jpg")));
    
    // trigger observer callbacks
    m_processNode->observeProperties();
    create_tweakbar_from_component(m_processNode);
    
    m_cvThread->setProcessingNode(m_processNode);
    m_cvThread->streamUSBCamera();
    
//    m_cvThread->setSourceNode(std::make_shared<CameraSource>());
//    m_cvThread->start();
    
    if(m_processNode)
    {
        addPropertyListToTweakBar(m_processNode->getPropertyList());
        LOG_INFO<<"CVProcessNode: "<<m_processNode->getDescription();
    }
    
    LOG_INFO<<"CVThread source: "<<m_cvThread->getSourceInfo();
    
    load_settings();
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
    
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

/////////////////////////////////////////////////////////////////

void KeyPointApp::draw()
{
    // draw fullscreen image
    gl::drawTexture(textures()[*m_imageIndex], gl::windowDimension());
    
    if(displayTweakBar()){ draw_textures(textures()); }
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::fileDrop(const MouseEvent &e, const std::vector<std::string> &files)
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

/////////////////////////////////////////////////////////////////

void KeyPointApp::tearDown()
{
    m_cvThread->stop();
    LOG_PRINT<<"ciao keypointer";
}

/////////////////////////////////////////////////////////////////

void KeyPointApp::updateProperty(const Property::ConstPtr &theProperty)
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
