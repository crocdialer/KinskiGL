//
//  CVNodes.h
//  kinskiGL
//
//  Created by Fabian Schmidt on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "opencv2/opencv.hpp"
#include "boost/shared_ptr.hpp"

namespace kinski{
    
class CVNode
{
public:
    typedef boost::shared_ptr<CVNode> Ptr;
    virtual std::string getName() = 0;
    virtual std::string getDescription() = 0;
};
    

class CVSourceNode : public CVNode
{
public:
    typedef boost::shared_ptr<CVSourceNode> Ptr;
    
    // inherited from INode
    virtual std::string getName(){return "Instance of ISourceNode";};
    virtual std::string getDescription(){return "Generic Input-source";};
    
    virtual bool hasImage() = 0;
    virtual cv::Mat getNextImage() = 0;
};

class CVBufferedSourceNode : public CVSourceNode 
{    
public:
    CVBufferedSourceNode(CVSourceNode::Ptr const srcNode):
    m_sourceNode(srcNode)
    {};
    
private:
    CVSourceNode::Ptr m_sourceNode;
};

class CVProcessNode : public CVNode
{
public:
    typedef boost::shared_ptr<CVProcessNode> Ptr;
    
    // inherited from INode
    virtual std::string getName(){return "Instance of IProcessNode";};
    virtual std::string getDescription(){return "Generic processing node";};
    
    virtual cv::Mat doProcessing(const cv::Mat &img) = 0;
};
    
}// namespace kinski