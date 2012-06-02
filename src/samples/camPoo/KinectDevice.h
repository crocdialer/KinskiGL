// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef helloQtCmake_KincetDevice_h
#define helloQtCmake_KincetDevice_h

#include <stdexcept>
#include <stdio.h>

// Lib-Freenect
#include <libfreenect/libfreenect.h>

// Boost
#include <boost/utility.hpp>
#include <boost/thread/thread.hpp>

// OpenCV
#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

namespace Freenect
{

class FreenectTiltState
{
	friend class FreenectDevice;
	FreenectTiltState(freenect_raw_tilt_state *_state) :
			m_code(_state->tilt_status), m_state(_state)
	{
	}
public:
	void getAccelerometers(double* x, double* y, double* z)
	{
		freenect_get_mks_accel(m_state, x, y, z);
	}
	double getTiltDegs()
	{
		return freenect_get_tilt_degs(m_state);
	}
public:
	freenect_tilt_status_code m_code;
private:
	freenect_raw_tilt_state *m_state;
};

class FreenectDevice: boost::noncopyable
{
public:
	FreenectDevice(freenect_context *_ctx, int _index)
	{
		if (freenect_open_device(_ctx, &m_dev, _index) < 0)
			throw std::runtime_error("Cannot open Kinect");
		freenect_set_user(m_dev, this);
		freenect_set_video_mode(
				m_dev,
				freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM,
						FREENECT_VIDEO_RGB));
		freenect_set_depth_mode(
				m_dev,
				freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM,
						FREENECT_DEPTH_11BIT));
		freenect_set_depth_callback(m_dev, freenect_depth_callback);
		freenect_set_video_callback(m_dev, freenect_video_callback);
	}
	virtual ~FreenectDevice()
	{
		if (freenect_close_device(m_dev) < 0)
		{
            
		} //FN_WARNING("Device did not shutdown in a clean fashion");
	}
	void startVideo()
	{
		if (freenect_start_video(m_dev) < 0)
			throw std::runtime_error("Cannot start RGB callback");
	}
	void stopVideo()
	{
		if (freenect_stop_video(m_dev) < 0)
			throw std::runtime_error("Cannot stop RGB callback");
	}
	void startDepth()
	{
		if (freenect_start_depth(m_dev) < 0)
			throw std::runtime_error("Cannot start depth callback");
	}
	void stopDepth()
	{
		if (freenect_stop_depth(m_dev) < 0)
			throw std::runtime_error("Cannot stop depth callback");
	}
	void setTiltDegrees(double _angle)
	{
		if (freenect_set_tilt_degs(m_dev, _angle) < 0)
			throw std::runtime_error("Cannot set angle in degrees");
	}
	void setLed(freenect_led_options _option)
	{
		if (freenect_set_led(m_dev, _option) < 0)
			throw std::runtime_error("Cannot set led");
	}
	void updateState()
	{
		if (freenect_update_tilt_state(m_dev) < 0)
			throw std::runtime_error("Cannot update device state");
	}
	FreenectTiltState getState() const
	{
		return FreenectTiltState(freenect_get_tilt_state(m_dev));
	}
	void setVideoFormat(freenect_video_format requested_format)
	{
		if (requested_format != m_video_format)
		{
			freenect_stop_video(m_dev);
			freenect_frame_mode mode = freenect_find_video_mode(
					FREENECT_RESOLUTION_MEDIUM, requested_format);
			if (!mode.is_valid)
				throw std::runtime_error(
						"Cannot set video format: invalid mode");
			if (freenect_set_video_mode(m_dev, mode) < 0)
				throw std::runtime_error("Cannot set video format");
			freenect_start_video(m_dev);
			m_video_format = requested_format;
		}
	}
	freenect_video_format getVideoFormat()
	{
		return m_video_format;
	}
	void setDepthFormat(freenect_depth_format requested_format)
	{
		if (requested_format != m_depth_format)
		{
			freenect_stop_depth(m_dev);
			freenect_frame_mode mode = freenect_find_depth_mode(
					FREENECT_RESOLUTION_MEDIUM, requested_format);
			if (!mode.is_valid)
				throw std::runtime_error(
						"Cannot set depth format: invalid mode");
			if (freenect_set_depth_mode(m_dev, mode) < 0)
				throw std::runtime_error("Cannot set depth format");
			freenect_start_depth(m_dev);
			m_depth_format = requested_format;
		}
	}
	freenect_depth_format getDepthFormat()
	{
		return m_depth_format;
	}
	// Do not call directly even in child
	virtual void VideoCallback(void *video, uint32_t timestamp) = 0;

	// Do not call directly even in child
	virtual void DepthCallback(void *depth, uint32_t timestamp) = 0;

protected:
	int getVideoBufferSize()
	{
		switch (m_video_format)
		{
		case FREENECT_VIDEO_RGB:
		case FREENECT_VIDEO_BAYER:
		case FREENECT_VIDEO_IR_8BIT:
		case FREENECT_VIDEO_IR_10BIT:
		case FREENECT_VIDEO_IR_10BIT_PACKED:
		case FREENECT_VIDEO_YUV_RGB:
		case FREENECT_VIDEO_YUV_RAW:
			return freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM,
					m_video_format).bytes;
		default:
			return 0;
		}
	}
	int getDepthBufferSize()
	{
		return freenect_get_current_depth_mode(m_dev).bytes;
	}

private:
	freenect_device *m_dev;
	freenect_video_format m_video_format;
	freenect_depth_format m_depth_format;
	static void freenect_depth_callback(freenect_device *dev, void *depth,
			uint32_t timestamp)
	{
		FreenectDevice* device = static_cast<FreenectDevice*>(freenect_get_user(
				dev));
		device->DepthCallback(depth, timestamp);
	}
	static void freenect_video_callback(freenect_device *dev, void *video,
			uint32_t timestamp)
	{
		FreenectDevice* device = static_cast<FreenectDevice*>(freenect_get_user(
				dev));
		device->VideoCallback(video, timestamp);
	}
};

class Freenect: boost::noncopyable
{
private:
	typedef std::map<int, FreenectDevice*> DeviceMap;
public:
	Freenect() :
			m_stop(false)
	{
		if (freenect_init(&m_ctx, NULL) < 0)
			throw std::runtime_error("Cannot initialize freenect library");

		// We claim both the motor and camera devices, since this class exposes both.
		// It does not support audio, so we do not claim it.
		freenect_select_subdevices(
				m_ctx,
				static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR
						| FREENECT_DEVICE_CAMERA));

		m_thread = boost::thread(boost::ref(*this));
	}
	virtual ~Freenect()
	{
		for (DeviceMap::iterator it = m_devices.begin(); it != m_devices.end();
				++it)
		{
			delete it->second;
		}
		m_stop = true;

		//printf("Freenect is %sjoinable\n",m_thread.joinable()? "":"NOT ");

		if (freenect_shutdown(m_ctx) < 0)
		{
		} //FN_WARNING("Freenect did not shutdown in a clean fashion");
        
        //m_thread.join();

        boost::posix_time::milliseconds t(20);
        m_thread.timed_join(t);
        
		//printf("Freenect thread joined\n");
	}
	template<typename ConcreteDevice>
	ConcreteDevice& createDevice(int _index)
	{
		DeviceMap::iterator it = m_devices.find(_index);
		if (it != m_devices.end())
			delete it->second;
		ConcreteDevice * device = new ConcreteDevice(m_ctx, _index);
		m_devices.insert(std::make_pair<int, FreenectDevice*>(_index, device));
		return *device;
	}
	void deleteDevice(int _index)
	{
		DeviceMap::iterator it = m_devices.find(_index);
		if (it == m_devices.end())
			return;
		delete it->second;
		m_devices.erase(it);
	}
	int deviceCount()
	{
		return freenect_num_devices(m_ctx);
	}
	// Do not call directly, thread runs here
	void operator()()
	{
		while (!m_stop)
		{
			if (freenect_process_events(m_ctx) < 0)
				throw std::runtime_error("Cannot process freenect events");
		}
	}
private:
	freenect_context *m_ctx;
	volatile bool m_stop;

	boost::thread m_thread;

	DeviceMap m_devices;
};

}
;

class KinectDevice: public Freenect::FreenectDevice
{
private:

	static Mat ms_emptyMat ;

	std::vector<uint16_t> m_gamma;
	Mat depthMat;
	Mat rgbMat;
	Mat ownMat;

	boost::mutex m_rgb_mutex;
	boost::mutex m_depth_mutex;

	bool m_new_rgb_frame;
	bool m_new_depth_frame;

public:
    
    static const Size KINECT_RESOLUTION;
    
	KinectDevice(freenect_context *_ctx, int _index);
	virtual ~KinectDevice();

	// Do not call directly even in child
	void VideoCallback(void *video, uint32_t timestamp);
	// Do not call directly even in child
	void DepthCallback(void* _depth, uint32_t timestamp);

	bool getVideo(Mat& output, bool irBool = false);
	bool getDepth(Mat& output,Mat& outputColored = ms_emptyMat);
};

#endif
