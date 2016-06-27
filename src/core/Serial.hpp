// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include "UART.hpp"

#if defined(KINSKI_MSW)
#include <winbase.h>
#include <tchar.h>
#include <iostream>
#include <string.h>
#include <setupapi.h>
#include <regstr.h>
#define MAX_SERIAL_PORTS 256
#include <winioctl.h>
    #ifdef __MINGW32__
    #define INITGUID
    #include <initguid.h> // needed for dev-c++ & DEFINE_GUID
    #endif
#else
#include <termios.h>
#endif

// serial error codes
#define KINSKI_SERIAL_NO_DATA 	-2
#define KINSKI_SERIAL_ERROR		-1

namespace kinski
{

typedef std::shared_ptr<class Serial> SerialPtr;
    
class Serial : public UART
{
    
public:
    
    static SerialPtr create();
    
    struct DeviceInfo
    {
        DeviceInfo(std::string devicePathIn, std::string deviceNameIn, int deviceIDIn)
        {
            devicePath			= devicePathIn;
            deviceName			= deviceNameIn;
            deviceID			= deviceIDIn;
        }
        
        DeviceInfo()
        {
            deviceName = "device undefined";
            deviceID   = -1;
        }
        std::string devicePath;			//eg: /dev/tty.cu/usbdevice-a440
        std::string deviceName;			//eg: usbdevice-a440 / COM4
        int deviceID;				//eg: 0,1,2,3 etc
    };
    
    virtual ~Serial();
    
    void list_devices();
    std::vector<Serial::DeviceInfo> device_list();
    
    bool setup() override;	// use default port, baud (0,9600)
    bool setup(string portName, int baudrate);
    bool setup(int deviceNumber, int baudrate);
    void close() override;
    bool is_initialized() const override;
    size_t read_bytes(void *buffer, size_t sz) override;
    size_t write_bytes(const void *buffer, size_t sz) override;
    size_t available() const override;
    std::string description() const override { return "serial"; }
    void drain() override;
    void flush(bool flushIn = true, bool flushOut = true) override;
    
    std::vector<std::string> read_lines(const char delim = '\n');
    
private:
    Serial();
    void buildDeviceList();
    
    std::string				deviceType;
    std::vector <DeviceInfo> devices;
    
    bool bHaveEnumeratedDevices;
    
    bool bInited;
    
    // needed for buffered get_line member
    string m_accum_str;
    std::vector<char> m_read_buffer;
    
#ifdef KINSKI_MSW
    
    char** portNamesShort;//[MAX_SERIAL_PORTS];
    char** portNamesFriendly; ///[MAX_SERIAL_PORTS];
    HANDLE hComm;		// the handle to the serial port pc
    int nPorts;
    bool bPortsEnumerated;
    void enumerateWin32Ports();
    COMMTIMEOUTS oldTimeout;	// we alter this, so keep a record
    
#else
    int m_handle;			// the handle to the serial port mac
    struct termios m_old_options;
#endif
    
};
    
    //----------------------------------------------------------------------
}