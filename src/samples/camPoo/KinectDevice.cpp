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

const Size KinectDevice::KINECT_RESOLUTION = Size(640,480);

Mat KinectDevice::ms_emptyMat = Mat();

KinectDevice::KinectDevice(freenect_context *_ctx, int _index) :
		Freenect::FreenectDevice(_ctx, _index), m_gamma(2048), depthMat(
				Size(640, 480), CV_16UC1), rgbMat(Size(640, 480), CV_8UC3,
				Scalar(0)), ownMat(Size(640, 480), CV_8UC3, Scalar(0)), m_new_rgb_frame(
				false), m_new_depth_frame(false)
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
	rgbMat.data = rgb;
	m_new_rgb_frame = true;

}

// Do not call directly even in child
void KinectDevice::DepthCallback(void* _depth, uint32_t timestamp)
{
    boost::mutex::scoped_lock lock(m_depth_mutex);

	uint16_t* depth = static_cast<uint16_t*>(_depth);
	depthMat.data = (uchar*) depth;
	m_new_depth_frame = true;
}

bool KinectDevice::getVideo(Mat& output, bool irBool)
{
    boost::mutex::scoped_lock lock(m_rgb_mutex);
    
	if (m_new_rgb_frame)
	{
		if (irBool)
		{
			//IR 8BIT
			output = Mat(rgbMat.size(),
                         CV_8UC1,
                         rgbMat.data + rgbMat.cols * 4).clone();
		}
		else
			//RGB
			cv::cvtColor(rgbMat, output, CV_RGB2BGR);

		m_new_rgb_frame = false;
		return true;
	}
	else
	{
		return false;
	}
}

bool KinectDevice::getDepth(Mat& output,Mat& outputColored)
{
	boost::mutex::scoped_lock lock(m_depth_mutex);
    
	if (m_new_depth_frame)
	{
		//depthMat.convertTo(output, CV_32F, 1.0 / 2047.0);
        //double min,max;
        //minMaxLoc(depthMat,&min,&max);
        //printf("%.2lf -- %.2lf\n",min,max);
        
        output.create(depthMat.size(), CV_32FC1);
        
        float *depth_mapped = (float*)output.data;
        uint16_t *depth_raw = (uint16_t*) depthMat.data;
        uint16_t *depth_raw_end = depth_raw+depthMat.size().area();
        
        uchar *depth_color = NULL;
        
        if (!outputColored.empty())
        {
            outputColored.create(depthMat.size(),CV_8UC3);
            depth_color = outputColored.data;
        }
        
        for (;depth_raw<depth_raw_end;depth_raw++)
		{
			int pval = m_gamma[*depth_raw];

			float finalVal = pval / (float) (2047 * .7);
			if (finalVal > 1.0)
				finalVal = 1.0;
			*depth_mapped++ = finalVal;

			if (depth_color)
			{
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
