//
//  CVNodes.cpp
//  kinskiCV
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "CVNode.h"
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
            throw std::runtime_error("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
    
        boost::format fmt("usb camera(%d)\n");
        fmt % camId;
        m_description = fmt.str();
    }
    
    CVCaptureNode::CVCaptureNode(const std::string &movieFile):
    m_videoSource(movieFile),
    m_loop(true)
    {
        if(!m_capture.open(movieFile))
            throw std::runtime_error("could not open capture");
        
        m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
        m_numFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
        
        boost::format fmt("movie file '%s'\n# frames: %d\nfps: %.2f\n");
        fmt % movieFile % m_numFrames % m_captureFPS;
        m_description = fmt.str();
    }
    
    CVCaptureNode::~CVCaptureNode(){ m_capture.release();};
    
    std::string CVCaptureNode::getName(){return "CVCaptureNode";};
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
        img = capFrame.clone();
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
        m_thread = boost::thread(boost::ref(*this));
    }
    
    CVBufferedSourceNode::~CVBufferedSourceNode()
    {
        m_running = false;
        m_conditionVar.notify_all();
        m_thread.join();
    }
    
    std::string CVBufferedSourceNode::getName()
    {
        stringstream ss;
        ss << "CVBufferedSourceNode ("<<m_sourceNode->getName()<<")\n";
        return ss.str();
    }
    
    std::string CVBufferedSourceNode::getDescription()
    {
        stringstream ss;
        ss << "CVBufferedSourceNode (bufSize: "<<m_bufferSize<<") containing:\n";
        ss << m_sourceNode->getDescription();
        return ss.str();
    }
    
    void CVBufferedSourceNode::operator()()
    {
        m_running = true;
        Mat nextImg;
        
        while (m_running)
        {
            boost::mutex::scoped_lock lock(m_mutex);
        
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
        boost::mutex::scoped_lock lock(m_mutex);
        
        if(!m_running) return false;
        
        while (m_imgBuffer.empty())
            m_conditionVar.wait(lock);
        
        img = m_imgBuffer.front();
        m_imgBuffer.pop_front();

        m_conditionVar.notify_one();
        
        return true;
    }

/************************* CVCombinedProcessNode ******************************/

    string CVCombinedProcessNode::getName()
    {
        stringstream ss;
        
        ss<<"CVCombinedProcessNode ("<<m_processNodes.size()<<")\n";

        return ss.str();
    }
    
    string CVCombinedProcessNode::getDescription()
    {
        stringstream ss;
        
        ss<<"CVCombinedProcessNode containing:\n";
        
        list<CVProcessNode::Ptr>::iterator it = m_processNodes.begin();
        
        for (; it != m_processNodes.end(); it++)
        {
            ss << (*it)->getDescription() << endl;
        }
        
        return ss.str();
    }
    
    void CVCombinedProcessNode::combineWith(const CVProcessNode::Ptr &one)
    {
        m_processNodes.push_back(one);
        
        list<Property::Ptr>::const_iterator it = one->getPropertyList().begin(),
        end = one->getPropertyList().end();
        
        for (; it != end; it++)
        {
            registerProperty(*it);
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
            
            procImg = tmpMats.back();
        }
        
        return outMats;
    }
    
    CVCombinedProcessNode::Ptr link(const CVProcessNode::Ptr &one,
                                    const CVProcessNode::Ptr &other)
    {
        CVCombinedProcessNode::Ptr outPtr(new CVCombinedProcessNode);

        outPtr->combineWith(one);
        outPtr->combineWith(other);
        
        return outPtr;
    }
    
    const CVCombinedProcessNode::Ptr operator<<(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other)
    {
        return link(one, other);
    };
    
    const CVCombinedProcessNode::Ptr operator>>(const CVProcessNode::Ptr &one,
                                                const CVProcessNode::Ptr &other)
    {
        return link(other, one);
    };
}

