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

using namespace std;

namespace kinski{

//////////////////////////////////////// Freenect /////////////////////////////////////////////////

FreenectPtr Freenect::create()
{
    return FreenectPtr(new Freenect());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Freenect::Freenect():
m_stop(false)
{
    if(freenect_init(&m_ctx, NULL) < 0){ throw std::runtime_error("Cannot initialize freenect library"); }

    // We claim both the motor and camera devices, since this class exposes both.
    // It does not support audio, so we do not claim it.
    freenect_select_subdevices(m_ctx,
                               static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR
                                                                  | FREENECT_DEVICE_CAMERA));

    freenect_set_log_level(m_ctx, FREENECT_LOG_ERROR);

    m_thread = std::thread(std::bind(&Freenect::run, this));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Freenect::~Freenect()
{
    m_stop = true;
    try{ m_thread.join(); }
    catch(std::exception &e){ LOG_ERROR<<e.what(); }

    // delete devices
    m_devices.clear();

    if(freenect_shutdown(m_ctx) < 0){ LOG_WARNING << "Freenect did not shutdown in a clean fashion"; }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Freenect::set_log_level(freenect_loglevel lvl)
{
    freenect_set_log_level(m_ctx, lvl);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

KinectDevicePtr Freenect::create_device(int the_index)
{
    auto dev = std::make_shared<KinectDevice>(m_ctx, the_index);
    m_devices[the_index] = dev;
    return dev;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Freenect::remove_device(int the_index)
{
    auto it = m_devices.find(the_index);
    if(it != m_devices.end()){ m_devices.erase(it); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

size_t Freenect::num_devices(){ return freenect_num_devices(m_ctx); }

///////////////////////////////////////////////////////////////////////////////////////////////////

void Freenect::run()
{
    while(!m_stop)
    {
        int res = freenect_process_events(m_ctx);
        if (res < 0)
        {
            // libusb signals an error has occurred
            if (res == LIBUSB_ERROR_INTERRUPTED)
            {
                // This happens sometimes, it means that a system call in libusb was
                // interrupted somehow (perhaps due to a signal)
                // The simple solution seems to be just ignore it.
                continue;
            }
            LOG_ERROR << "Cannot process freenect events -> terminating thread";
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

FreenectDevice::FreenectDevice(freenect_context *_ctx, int the_index)
{
    if(freenect_open_device(_ctx, &m_dev, the_index) < 0){ throw std::runtime_error("Cannot open Kinect"); }
    freenect_set_user(m_dev, this);
    freenect_set_video_mode(m_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
    freenect_set_depth_mode(m_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM));
    freenect_set_depth_callback(m_dev, freenect_depth_callback);
    freenect_set_video_callback(m_dev, freenect_video_callback);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

FreenectDevice::~FreenectDevice()
{
    if (freenect_close_device(m_dev) < 0)
    {
        LOG_WARNING << "Device did not shutdown in a clean fashion";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::start_video()
{
    if(freenect_start_video(m_dev) < 0){ throw std::runtime_error("could not start RGB callback"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::stop_video()
{
    if(freenect_stop_video(m_dev) < 0){ throw std::runtime_error("could not stop RGB callback"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::start_depth()
{
    if(freenect_start_depth(m_dev) < 0){ throw std::runtime_error("could not start depth callback"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::stop_depth()
{
    if(freenect_stop_depth(m_dev) < 0){ throw std::runtime_error("Cannot stop depth callback"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::set_tilt_angle(double the_angle)
{
    if(freenect_set_tilt_degs(m_dev, the_angle) < 0){ throw std::runtime_error("Cannot set angle in degrees"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::set_led(freenect_led_options the_option)
{
    if(freenect_set_led(m_dev, the_option) < 0){ throw std::runtime_error("Cannot set led"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::update_tilt_state()
{
    if(freenect_update_tilt_state(m_dev) < 0){ throw std::runtime_error("Cannot update device state"); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

FreenectTiltState FreenectDevice::tilt_state() const
{
    return FreenectTiltState(freenect_get_tilt_state(m_dev));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::set_video_format(freenect_video_format requested_format)
{
    if(requested_format != m_video_format)
    {
        freenect_stop_video(m_dev);
        freenect_frame_mode mode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM,
                                                            requested_format);
        if(!mode.is_valid){ throw std::runtime_error("Cannot set video format: invalid mode"); }
        if (freenect_set_video_mode(m_dev, mode) < 0){ throw std::runtime_error("Cannot set video format"); }
        m_video_format = requested_format;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

freenect_video_format FreenectDevice::video_format()
{
    return m_video_format;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void FreenectDevice::set_depth_format(freenect_depth_format requested_format)
{
    if(requested_format != m_depth_format)
    {
        freenect_stop_depth(m_dev);
        freenect_frame_mode mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, requested_format);
        if(!mode.is_valid){ throw std::runtime_error("Cannot set depth format: invalid mode"); }
        if(freenect_set_depth_mode(m_dev, mode) < 0){ throw std::runtime_error("Cannot set depth format"); }
        m_depth_format = requested_format;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

freenect_depth_format FreenectDevice::getDepthFormat()
{
    return m_depth_format;
}

///////////////////////////////////// KinectDevice ////////////////////////////////////////////////

const gl::ivec2 KinectDevice::KINECT_RESOLUTION(640,480);

///////////////////////////////////////////////////////////////////////////////////////////////////

KinectDevice::KinectDevice(freenect_context *_ctx, int _index) :
    FreenectDevice(_ctx, _index), m_gamma(2048),
    m_new_rgb_frame(false),
    m_new_depth_frame(false)
{

    const float k1 = 1.1863;
    const float k2 = 2842.5;
    const float k3 = 0.1236;
    
    for (size_t i=0; i < 2048; i++)
    {
        const float depth = k3 * tanf(i/k2 + k1);
        m_gamma[i] = depth;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

KinectDevice::~KinectDevice()
{
    this->stop_video();
	this->stop_depth();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void KinectDevice::video_cb(void *the_data, uint32_t timestamp)
{
    std::unique_lock<std::mutex> lock(m_rgb_mutex);
    m_buffer_rgb = static_cast<uint8_t*>(the_data);
	m_new_rgb_frame = true;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

void KinectDevice::depth_cb(void *the_data, uint32_t timestamp)
{
    std::unique_lock<std::mutex> lock(m_depth_mutex);
    m_buffer_depth = static_cast<uint8_t*>(the_data);
	m_new_depth_frame = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool KinectDevice::copy_frame_rgb(std::vector<uint8_t> &the_buffer)
{
	if(m_new_rgb_frame)
	{
        std::unique_lock<std::mutex> lock(m_rgb_mutex);
        size_t num_bytes = KINECT_RESOLUTION.x * KINECT_RESOLUTION.y * 3;
        the_buffer.resize(num_bytes);
        memcpy(the_buffer.data(), m_buffer_rgb, num_bytes);
		m_new_rgb_frame = false;
		return true;
	}
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool KinectDevice::copy_frame_depth(std::vector<uint8_t> &the_buffer)
{
	if(m_new_depth_frame)
	{
        std::unique_lock<std::mutex> lock(m_depth_mutex);
        size_t num_bytes = KINECT_RESOLUTION.x * KINECT_RESOLUTION.y * sizeof(uint16_t);
        the_buffer.resize(num_bytes);
        memcpy(the_buffer.data(), m_buffer_depth, num_bytes);
		m_new_depth_frame = false;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

}// namespace