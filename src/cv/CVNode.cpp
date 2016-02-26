//
//  CVNodes.cpp
//  cv
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVNode.h"
#include "core/file_functions.hpp"
#include "core/Logger.hpp"
#include <boost/format.hpp>

using namespace std;
using namespace cv;

namespace kinski
{
    
    CVCaptureNode::CVCaptureNode(const int camId):
    m_numFrames(-1),
    m_loop(false)
    {
        if(!m_capture.open(camId))
            throw BadInputSourceException("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
    
        boost::format fmt("CVCaptureNode from usb camera(%d)");
        fmt % camId;
        m_description = fmt.str();
    }
    
    CVCaptureNode::CVCaptureNode(const std::string &movieFile):
    m_videoSource(movieFile),
    m_loop(true)
    {
        if(!m_capture.open(kinski::search_file(movieFile)))
            throw BadInputSourceException("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        boost::format fmt("CVCaptureNode from movie file '%s'\n# frames: %d\nfps: %.2f");
        fmt % movieFile % m_numFrames % m_captureFPS;
        m_description = fmt.str();
    }
    
    CVCaptureNode::~CVCaptureNode(){ m_capture.release();};
    
    std::string CVCaptureNode::getDescription(){return m_description;};
    
    bool CVCaptureNode::getNextImage(cv::Mat &img)
    {
        cv::Mat capFrame;
        m_capture >> capFrame;
        
        if(capFrame.empty())
        {
            if(m_loop)
                m_capture.set(CV_CAP_PROP_POS_FRAMES,0);
            else
                return false;
        }
    
        // going safe, have a copy of our own of the data
        capFrame.copyTo(img);
        return true;
    }
    
    int CVCaptureNode::getNumFrames(){return m_numFrames;}
    
    float CVCaptureNode::getFPS(){return m_captureFPS;}
    
    void CVCaptureNode::jumpToFrame(const int newIndex)
    {
        int clampedIndex = std::max(0, newIndex);
        clampedIndex = std::min(clampedIndex ,m_numFrames - 1);
        
        m_capture.set(CV_CAP_PROP_POS_FRAMES,clampedIndex);
    }
    
    void CVCaptureNode::setLoop(bool b){m_loop = b;};
    
/************************* CVBufferedSourceNode *******************************/
    
    CVBufferedSourceNode::CVBufferedSourceNode(const CVSourceNode::Ptr srcNode,
                                               int bufSize):
    m_running(false),
    m_sourceNode(srcNode),
    m_bufferSize(bufSize)
    {
        m_thread = std::thread(std::bind(&CVBufferedSourceNode::run, this));
    }
    
    CVBufferedSourceNode::~CVBufferedSourceNode()
    {
        m_running = false;
        m_conditionVar.notify_all();
        m_thread.join();
    }
    
    std::string CVBufferedSourceNode::getDescription()
    {
        stringstream ss;
        ss << "CVBufferedSourceNode (bufSize: "<<m_bufferSize<<") containing:"<<std::endl;
        ss << m_sourceNode->getDescription();
        return ss.str();
    }
    
    void CVBufferedSourceNode::run()
    {
        m_running = true;
        Mat nextImg;
        
        while (m_running)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
        
            while (m_running &&
                   (!m_sourceNode || m_imgBuffer.size() >= m_bufferSize))
                m_conditionVar.wait(lock);
            
            m_sourceNode->getNextImage(nextImg);
            
//            if(m_imgBuffer.size() >= m_bufferSize)
//                m_imgBuffer.pop_front();
            
            m_imgBuffer.push_back(nextImg);
            m_conditionVar.notify_one();
        }
        m_running = false;
    }
    
    bool CVBufferedSourceNode::getNextImage(cv::Mat &img)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if(!m_running) return false;
        
        while (m_imgBuffer.empty())
            m_conditionVar.wait(lock);
        
        img = m_imgBuffer.front();
        m_imgBuffer.pop_front();
        m_conditionVar.notify_one();
        return true;
    }

/************************* CVCombinedProcessNode ******************************/
    
    std::string CVCombinedProcessNode::getName()
    {
        return "CVCombinedProcessNode (" + as_string(m_processNodes.size())+")";
    }
    
    string CVCombinedProcessNode::getDescription()
    {
        stringstream ss;
        ss<<"CVCombinedProcessNode ("<<m_processNodes.size()<<") containing:"<<std::endl;
        list<CVProcessNode::Ptr>::iterator it = m_processNodes.begin();
        
        for (; it != m_processNodes.end(); it++)
        {
            ss << (*it)->getDescription() << endl;
        }
        return ss.str();
    }
    
    void CVCombinedProcessNode::addNode(const CVProcessNode::Ptr &theNode)
    {
        m_processNodes.push_back(theNode);
        list<Property::Ptr>::const_iterator it = theNode->get_property_list().begin(),
        end = theNode->get_property_list().end();
        
        for (; it != end; ++it)
        {
            register_property(*it);
        }
    }
    
    void CVCombinedProcessNode::update_property(const Property::ConstPtr &theProperty)
    {
        list<CVProcessNode::Ptr>::iterator it = m_processNodes.begin();
        
        for (; it != m_processNodes.end(); it++)
        {
            (*it)->update_property(theProperty);
        }
    }
    
    vector<Mat> CVCombinedProcessNode::doProcessing(const Mat &img)
    {
        vector<Mat> outMats;
        Mat procImg = img;
        list<CVProcessNode::Ptr>::iterator it = m_processNodes.begin();
        
        for (; it != m_processNodes.end(); it++)
        {
            vector<Mat> tmpMats = (*it)->doProcessing(procImg);
            outMats.insert(outMats.end(), tmpMats.begin(), tmpMats.end());
            procImg = tmpMats.empty() ? img : tmpMats.back();
        }
        return outMats;
    }
    
    CVCombinedProcessNode::Ptr link(const CVProcessNode::Ptr &one,
                                    const CVProcessNode::Ptr &other)
    {
        CVCombinedProcessNode::Ptr outPtr(new CVCombinedProcessNode);
        outPtr->addNode(one);
        outPtr->addNode(other);
        return outPtr;
    }
    
    CVCombinedProcessNode::Ptr operator<<(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other)
    {
        return link(other, one);
    }
    
    CVCombinedProcessNode::Ptr operator>>(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other)
    {
        return link(one, other);
    }
    
/****************************** CVWriterNode **********************************/
    
    CVDiskWriterNode::CVDiskWriterNode(const std::string &theFile):
    m_videoSrc(Property_<string>::create("VideoSrc", theFile)),
    m_codec(CV_FOURCC('X','V','I','D'))
    {
        register_property(m_videoSrc);
    }
    
    CVDiskWriterNode::~CVDiskWriterNode()
    {
        m_videoWriter.release();
    }
    
    string CVDiskWriterNode::getDescription()
    {
        stringstream ss;
        ss << "CVDiskWriterNode - encodes incoming frames and writes to file"<<std::endl;
        ss << "file: '"<<*m_videoSrc<<"'"<<std::endl;
        ss << "format: 'xvid'";
        return ss.str();
    }
    
    vector<Mat> CVDiskWriterNode::doProcessing(const Mat &img)
    {
        if(!m_videoWriter.isOpened())
        {
            m_videoWriter.open(kinski::search_file(*m_videoSrc),
                               m_codec,
                               25, img.size());
        }
        m_videoWriter << img;
        return vector<Mat>();
    }
    
    void CVDiskWriterNode::update_property(const Property::ConstPtr &theProperty)
    {
        if(theProperty == m_videoSrc)
        {
            m_videoWriter.release();
        }
    }
}

