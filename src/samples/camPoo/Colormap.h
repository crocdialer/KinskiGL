#ifndef COLORMAP_H
#define COLORMAP_H

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// This is an opencv implementation of Matlab´s colormap() feature
class Colormap  
{
	
private:
	
	static const double ms_jetValues[192];
	static const double ms_coolValues[192];
	static const double ms_boneValues[192];
	static const double ms_autumnValues[192];
	
	cv::Vec3b m_byteValues[256];
	
	std::vector<cv::Mat> m_lookUpTables;
	std::vector<cv::Mat> m_bgrChannels;
	
public:
	
	enum ColorMapType{JET,COOL,BONE,AUTUMN};
	
	Colormap(const ColorMapType& mt=JET);
	
	~Colormap(){};
	
	cv::Mat apply (const cv::Mat& img1C) const;
	
	inline cv::Scalar valueFor(const unsigned char& v) const
	{
		return cv::Scalar(m_byteValues[v]);
	}
	
	inline cv::Scalar valueFor(float f) const
	{
		if(f < 0) f = 0; 
		else
			if(f > 1) f = 1;
		
		int index = (int) std::floor(f*255);
		
		return cv::Scalar(m_byteValues[index]);
		
	}
	inline cv::Scalar valueFor(const double& f) const {return valueFor((float)f);}
};


#endif//COLORMAP_H