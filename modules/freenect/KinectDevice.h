// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 1993-2011, ART+COM AG Berlin, Germany <www.artcom.de>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#ifndef __KINECT_DEVICE_INCLUDED_
#define __KINECT_DEVICE_INCLUDED_

#include <map>

#include "gl/gl.hpp"

// Lib-Freenect
#include <libfreenect.h>
#include <libusb-1.0/libusb.h>

// threading
#include <thread>
#include <mutex>

namespace kinski
{

DEFINE_CLASS_PTR(Freenect);
DEFINE_CLASS_PTR(KinectDevice);

class FreenectTiltState
{
public:

    FreenectTiltState(freenect_raw_tilt_state *the_state): m_state(the_state)
    {}

    void get_accelerometers(double* x, double* y, double* z)
    {
        freenect_get_mks_accel(m_state, x, y, z);
    }
    double get_tilt_angle()
    {
        return freenect_get_tilt_degs(m_state);
    }

    freenect_tilt_status_code code() const {return m_state->tilt_status; }

private:
    freenect_raw_tilt_state *m_state;
};

class FreenectDevice
{
public:
    FreenectDevice(freenect_context *_ctx, int the_index);

    virtual ~FreenectDevice();

    void start_video();

    void stop_video();

    void start_depth();

    void stop_depth();

    void set_tilt_angle(double the_angle);

    void set_led(freenect_led_options the_option);

    void update_tilt_state();

    FreenectTiltState tilt_state() const;

    void set_video_format(freenect_video_format requested_format);

    freenect_video_format video_format();

    void set_depth_format(freenect_depth_format requested_format);

    freenect_depth_format getDepthFormat();

    size_t num_bytes_depth();

    size_t num_bytes_video();

    // internal video callback
    virtual void video_cb(void *video, uint32_t timestamp) = 0;

    // internal
    virtual void depth_cb(void *depth, uint32_t timestamp) = 0;

private:
    freenect_device *m_dev;
    freenect_video_format m_video_format;
    freenect_depth_format m_depth_format;

    static void freenect_depth_callback(freenect_device *dev, void *depth,
                                        uint32_t timestamp)
    {
        FreenectDevice* device = static_cast<FreenectDevice*>(freenect_get_user(dev));
        device->depth_cb(depth, timestamp);
    }

    static void freenect_video_callback(freenect_device *dev, void *video,
                                        uint32_t timestamp)
    {
        FreenectDevice* device = static_cast<FreenectDevice*>(freenect_get_user(dev));
        device->video_cb(video, timestamp);
    }
};

class Freenect
{
public:
    using device_map_t = std::map<int, std::shared_ptr<FreenectDevice>>;

    static FreenectPtr create();

    virtual ~Freenect();

    void set_log_level(freenect_loglevel lvl);

    KinectDevicePtr create_device(int the_index);

    void remove_device(int the_index);

    size_t num_devices();

    // Do not call directly, thread runs here
    void run();

private:

    Freenect();
    freenect_context *m_ctx = nullptr;
    volatile bool m_stop = true;
    std::thread m_thread;
    device_map_t m_devices;
};

class KinectDevice : public FreenectDevice
{
private:

    std::vector<uint16_t> m_gamma;

    uint8_t *m_buffer_rgb = nullptr, *m_buffer_depth = nullptr;

    std::mutex m_rgb_mutex;
    std::mutex m_depth_mutex;

    bool m_new_rgb_frame;
    bool m_new_depth_frame;

public:

    static const gl::ivec2 KINECT_RESOLUTION;

    KinectDevice(freenect_context *_ctx, int _index);
    virtual ~KinectDevice();

    // do not call directly
    void video_cb(void *the_data, uint32_t timestamp);

    // do not call directly
    void depth_cb(void *the_data, uint32_t timestamp);

    //! copy rgb bytes into buffer, allocate space if necessary
    bool copy_frame_rgb(std::vector<uint8_t> &the_buffer);

    //! copy depth bytes into buffer, allocate space if necessary
    bool copy_frame_depth(std::vector<uint8_t> &the_buffer);
};
    
}//namespace
#endif // __KINECT_DEVICE_INCLUDED_
