//
//  DopeRecorder.cpp
//  kinskiGL
//
//  Created by Fabian on 4/4/13.
//
//

#include "FaceFilter.h"
#include "kinskiCore/Logger.h"
#include "kinskiCore/file_functions.h"

namespace kinski
{
    using std::vector;
    using namespace cv;
    
    FaceFilter::FaceFilter()
    {
        set_name("FaceFilter");
        try{m_cascade.load(searchFile("haarcascade_frontalface_default.xml"));}
        catch(const std::exception &e){LOG_ERROR<<e.what();}
    }
    
    std::vector<cv::Mat> FaceFilter::doProcessing(const cv::Mat &img)
    {
        vector<Mat> outMats;
        vector<Rect> rects;
        
        int maxWidth = 480;
        float scale = (float)maxWidth / img.cols;

        resize(img, m_small_img, Size(0,0), scale, scale);
        cvtColor(m_small_img, m_small_img, CV_BGR2GRAY);
        cv::Size min_size = cv::Size(maxWidth / 10.f, maxWidth / 10.f);
        m_cascade.detectMultiScale(m_small_img, rects, 1.2, 3, 0, min_size);
        
        vector<Rect>::const_iterator it = rects.begin();
        for (; it != rects.end(); ++it)
        {
            Rect r = *it;
            r.x /= scale;
            r.y /= scale;
            r.width /= scale;
            r.height /= scale;
            //if(it->area() > 5000)
            outMats.push_back(img(r).clone());
        }

        return outMats;
    }
    
    void FaceFilter::updateProperty(const Property::ConstPtr &theProperty)
    {
        
    }
}

