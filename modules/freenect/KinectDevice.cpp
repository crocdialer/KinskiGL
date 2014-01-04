// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//
#include "KinectDevice.h"

using namespace cv;
using namespace std;

namespace kinski
{

const Size KinectDevice::KINECT_RESOLUTION = Size(640,480);

KinectDevice::KinectDevice(freenect_context *_ctx, int _index) :
    FreenectDevice(_ctx, _index), m_gamma(2048),
    m_depth(Size(640, 480), CV_16UC1),
    m_rgb(Size(640, 480), CV_8UC3, Scalar(0)),
    m_new_rgb_frame(false),
    m_new_depth_frame(false)
{

	for (unsigned int i = 0; i < 2048; i++)
	{
		float v = i / 2048.0;
		v = std::pow(v, 3) * 6;
		m_gamma[i] = v * 6 * 256;
	}
}

KinectDevice::~KinectDevice()
{
	this->stopVideo();
	this->stopDepth();
}

// Do not call directly even in child
void KinectDevice::VideoCallback(void* _rgb, uint32_t timestamp)
{
    boost::mutex::scoped_lock lock(m_rgb_mutex);

	uint8_t* rgb = static_cast<uint8_t*>(_rgb);
	m_rgb.data = rgb;
	m_new_rgb_frame = true;

}

// Do not call directly even in child
void KinectDevice::DepthCallback(void* _depth, uint32_t timestamp)
{
    boost::mutex::scoped_lock lock(m_depth_mutex);

	uint16_t* depth = static_cast<uint16_t*>(_depth);
	m_depth.data = (uchar*) depth;
	m_new_depth_frame = true;
}

bool KinectDevice::getVideo(Mat& output, bool irBool)
{
	if (m_new_rgb_frame)
	{
        boost::mutex::scoped_lock lock(m_rgb_mutex);
        
		if (irBool)
		{
			//IR 8BIT
			output = Mat(m_rgb.size(),
                         CV_8UC1,
                         m_rgb.data + m_rgb.cols * 4).clone();
		}
		else
			//RGB
			cv::cvtColor(m_rgb, output, CV_RGB2BGR);

		m_new_rgb_frame = false;
		return true;
	}
    return false;
}

bool KinectDevice::getDepth(Mat& output, Mat outputColored)
{
	if (m_new_depth_frame)
	{
        boost::mutex::scoped_lock lock(m_depth_mutex);
        
//        output.create(m_depth.size(), CV_32FC1);
        m_depth.copyTo(output);
        
//        float *depth_mapped = (float*)output.data;
        

        uchar *depth_color = NULL;
        
        if (!outputColored.empty())
        {
            outputColored.create(m_depth.size(),CV_8UC3);
            depth_color = outputColored.data;
        }
        
        if (depth_color)
        {
            uint16_t *depth_raw = (uint16_t*) m_depth.data;
            uint16_t *depth_raw_end = depth_raw+m_depth.size().area();
            
            for (;depth_raw < depth_raw_end;depth_raw++)
            {
                int pval = m_gamma[*depth_raw];

    //			float finalVal = pval / (float) (2047 * .7);
    //            
    //			if (finalVal > 1.0)
    //				finalVal = 1.0;
    //			*depth_mapped++ = finalVal;

                int lb = pval & 0xff;

                switch (pval >> 8)
                {
                case 0:
                    *depth_color++ = 255 - lb;
                    *depth_color++ = 255 - lb;
                    *depth_color++ = 255;
                    break;
                case 1:
                    *depth_color++ = 0;
                    *depth_color++ = lb;
                    *depth_color++ = 255;
                    break;
                case 2:
                    *depth_color++ = 0;
                    *depth_color++ = 255;
                    *depth_color++ = 255 - lb;
                    break;
                case 3:
                    *depth_color++ = lb;
                    *depth_color++ = 255;
                    *depth_color++ = 0;
                    break;
                case 4:
                    *depth_color++ = 255;
                    *depth_color++ = 255 - lb;
                    *depth_color++ = 0;
                    break;
                case 5:
                    *depth_color++ = 255 - lb;
                    *depth_color++ = 0;
                    *depth_color++ = 0;
                    break;
                default:
                    *depth_color++ = 0;
                    *depth_color++ = 0;
                    *depth_color++ = 0;
                    break;
                }
            }
		}
        
		m_new_depth_frame = false;
		return true;
	}
	else
	{
		return false;
	}
}
}