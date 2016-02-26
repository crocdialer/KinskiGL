#include "Serial.hpp"
#include "Logger.hpp"

#if !defined(KINSKI_MSW)
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <dirent.h>
#include <cstring>
#endif

#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <algorithm>

namespace kinski
{

//---------------------------------------------
#ifdef KINSKI_MSW
//---------------------------------------------

//------------------------------------
   // needed for serial bus enumeration:
   //4d36e978-e325-11ce-bfc1-08002be10318}
   DEFINE_GUID (GUID_SERENUM_BUS_ENUMERATOR, 0x4D36E978, 0xE325,
   0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
//------------------------------------

void Serial::enumerateWin32Ports()
{
	if (bPortsEnumerated == true) return;

	HDEVINFO hDevInfo = NULL;
	SP_DEVINFO_DATA DeviceInterfaceData;
	int i = 0;
	DWORD dataType, actualSize = 0;
	unsigned char dataBuf[MAX_PATH + 1];

	// Reset Port List
	nPorts = 0;
	// Search device set
	hDevInfo = SetupDiGetClassDevs((struct _GUID *)&GUID_SERENUM_BUS_ENUMERATOR,0,0,DIGCF_PRESENT);
	if ( hDevInfo ){
      while (TRUE){
         ZeroMemory(&DeviceInterfaceData, sizeof(DeviceInterfaceData));
         DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
         if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInterfaceData)){
             // SetupDiEnumDeviceInfo failed
             break;
         }

         if (SetupDiGetDeviceRegistryPropertyA(hDevInfo,
             &DeviceInterfaceData,
             SPDRP_FRIENDLYNAME,
             &dataType,
             dataBuf,
             sizeof(dataBuf),
             &actualSize)){

			sprintf(portNamesFriendly[nPorts], "%s", dataBuf);
			portNamesShort[nPorts][0] = 0;

			// turn blahblahblah(COM4) into COM4

            char *   begin    = NULL;
            char *   end    = NULL;
            begin          = strstr((char *)dataBuf, "COM");


            if (begin)
                {
                end          = strstr(begin, ")");
                if (end)
                    {
                      *end = 0;   // get rid of the )...
                      strcpy(portNamesShort[nPorts], begin);
                }
                if (nPorts++ > MAX_SERIAL_PORTS)
                        break;
            }
         }
            i++;
      }
   }
   SetupDiDestroyDeviceInfoList(hDevInfo);

   bPortsEnumerated = false;
}


//---------------------------------------------
#endif
//---------------------------------------------



//----------------------------------------------------------------
Serial::Serial():
m_handle(0)
{
	//---------------------------------------------
	#ifdef KINSKI_MSW
	//---------------------------------------------
		nPorts 				= 0;
		bPortsEnumerated 	= false;

		portNamesShort = new char * [MAX_SERIAL_PORTS];
		portNamesFriendly = new char * [MAX_SERIAL_PORTS];
		for (int i = 0; i < MAX_SERIAL_PORTS; i++){
			portNamesShort[i] = new char[10];
			portNamesFriendly[i] = new char[MAX_PATH];
		}
	//---------------------------------------------
	#endif
	//---------------------------------------------
	bInited = false;
}

//----------------------------------------------------------------
Serial::~Serial()
{

	close();

	//---------------------------------------------
	#ifdef KINSKI_MSW
	//---------------------------------------------
		nPorts 				= 0;
		bPortsEnumerated 	= false;

		for (int i = 0; i < MAX_SERIAL_PORTS; i++) {
			delete [] portNamesShort[i];
			delete [] portNamesFriendly[i];
		}
		delete [] portNamesShort;
		delete [] portNamesFriendly;

	//---------------------------------------------
	#endif
	//---------------------------------------------

	bInited = false;
}

//----------------------------------------------------------------
static bool isDeviceArduino( SerialDeviceInfo & A )
{
	return ( strstr(A.getDeviceName().c_str(), "usbserial") ||
		 strstr(A.getDeviceName().c_str(), "usbmodem") );
}

//----------------------------------------------------------------
void Serial::buildDeviceList()
{

	deviceType = "serial";
	devices.clear();

	vector <string> prefixMatch;

	#ifdef KINSKI_MAC
		prefixMatch.push_back("cu.");
		prefixMatch.push_back("tty.");
	#endif
	#ifdef KINSKI_LINUX
		prefixMatch.push_back("ttyACM");
		prefixMatch.push_back("ttyS");
		prefixMatch.push_back("ttyUSB");
		prefixMatch.push_back("rfc");
	#endif


	#if defined( KINSKI_MAC ) || defined( KINSKI_LINUX )

	DIR *dir;
	struct dirent *entry;
	dir = opendir("/dev");

	string deviceName	= "";
	int deviceCount		= 0;

	if (dir == NULL){LOG_ERROR << "buildDeviceList(): error listing devices in /dev";}
    else
    {
		//for each device
		while( (entry = readdir(dir)) )
        {
			deviceName = (char *)entry->d_name;

			//we go through the prefixes
			for(int k = 0; k < (int)prefixMatch.size(); k++){
				//if the device name is longer than the prefix
				if( deviceName.size() > prefixMatch[k].size() ){
					//do they match ?
					if( deviceName.substr(0, prefixMatch[k].size()) == prefixMatch[k].c_str() ){
						devices.push_back(SerialDeviceInfo("/dev/"+deviceName, deviceName, deviceCount));
						deviceCount++;
						break;
					}
				}
			}
		}
		closedir(dir);
	}

	#endif

	//---------------------------------------------
	#ifdef KINSKI_MSW
	//---------------------------------------------
	enumerateWin32Ports();
	ofLogNotice("Serial") << "found " << nPorts << " devices";
	for (int i = 0; i < nPorts; i++){
		//NOTE: we give the short port name for both as that is what the user should pass and the short name is more friendly
		devices.push_back(SerialDeviceInfo(string(portNamesShort[i]), string(portNamesShort[i]), i));
	}
	//---------------------------------------------
	#endif
    //---------------------------------------------

	//here we sort the device to have the aruino ones first.
	partition(devices.begin(), devices.end(), isDeviceArduino);
	//we are reordering the device ids. too!
	for(int k = 0; k < (int)devices.size(); k++){
		devices[k].deviceID = k;
	}

	bHaveEnumeratedDevices = true;
}


//----------------------------------------------------------------
void Serial::listDevices()
{
	buildDeviceList();
	for(int k = 0; k < (int)devices.size(); k++)
    {
		LOG_INFO << "[" << devices[k].getDeviceID() << "] = "<< devices[k].getDeviceName().c_str();
	}
}

//----------------------------------------------------------------
vector <SerialDeviceInfo> Serial::getDeviceList()
{
	buildDeviceList();
	return devices;
}

//----------------------------------------------------------------
void Serial::enumerateDevices()
{
	listDevices();
}

//----------------------------------------------------------------
void Serial::close()
{
    if(bInited){LOG_DEBUG<<"closing serial port";}
    
	//---------------------------------------------
	#ifdef KINSKI_MSW
	//---------------------------------------------
		if (bInited)
        {
			SetCommTimeouts(hComm,&oldTimeout);
			CloseHandle(hComm);
			hComm 		= INVALID_HANDLE_VALUE;
			bInited 	= false;
		}
	//---------------------------------------------
    #else
    //---------------------------------------------
    	if (bInited && m_handle)
        {
    		tcsetattr(m_handle,TCSANOW,&m_old_options);
    		::close(m_handle);
    		bInited = false;
            m_handle = 0;
            
            m_read_buffer.clear();
            m_accum_str.clear();
    	}
    	// [CHECK] -- anything else need to be reset?
    //---------------------------------------------
    #endif
    //---------------------------------------------

}

//----------------------------------------------------------------
bool Serial::setup()
{
	return setup(0,9600);		// the first one, at 9600 is a good choice...
}

//----------------------------------------------------------------
bool Serial::setup(int deviceNumber, int baud)
{
	buildDeviceList();
	if( deviceNumber < (int)devices.size() )
    {
		return setup(devices[deviceNumber].devicePath, baud);
	}
    else
    {
		LOG_ERROR << "couldn't find device " << deviceNumber << ", only " << devices.size() << " devices found";
		return false;
	}
}

//----------------------------------------------------------------
bool Serial::setup(string portName, int baud)
{

	close();

    //---------------------------------------------
	#ifdef KINSKI_MSW
	//---------------------------------------------

	char pn[sizeof(portName)];
	int num;
	if (sscanf(portName.c_str(), "COM%d", &num) == 1)
    {
		// Microsoft KB115831 a.k.a if COM > COM9 you have to use a different
		// syntax
		sprintf(pn, "\\\\.\\COM%d", num);
	}
    else
    {
		strncpy(pn, (const char *)portName.c_str(), sizeof(portName)-1);
	}

	// open the serial port:
	// "COM4", etc...

	hComm=CreateFileA(pn,GENERIC_READ|GENERIC_WRITE,0,0,
					OPEN_EXISTING,0,0);

	if(hComm==INVALID_HANDLE_VALUE)
    {
		LOG_ERROR << "setup(): unable to open " << portName;
		return false;
	}


	// now try the settings:
	COMMCONFIG cfg;
	DWORD cfgSize;
	char  buf[80];

	cfgSize=sizeof(cfg);
	GetCommConfig(hComm,&cfg,&cfgSize);
	int bps = baud;
	sprintf(buf,"baud=%d parity=N data=8 stop=1",bps);

	#if (_MSC_VER)       // microsoft visual studio
		// msvc doesn't like BuildCommDCB,
		//so we need to use this version: BuildCommDCBA
		if(!BuildCommDCBA(buf,&cfg.dcb))
        {
			LOG_ERROR << "setup(): unable to build comm dcb, (" << buf << ")";
		}
	#else
		if(!BuildCommDCB(buf,&cfg.dcb))
        {
			LOG_ERROR << "setup(): unable to build comm dcb, (" << buf << ")";
		}
	#endif


	// Set baudrate and bits etc.
	// Note that BuildCommDCB() clears XON/XOFF and hardware control by default

	if(!SetCommState(hComm,&cfg.dcb))
    {
		LOG_ERROR << "setup(): couldn't set comm state: " << cfg.dcb.BaudRate << " bps, xio " << cfg.dcb.fInX << "/" << cfg.dcb.fOutX;;
	}

	// Set communication timeouts (NT)
	COMMTIMEOUTS tOut;
	GetCommTimeouts(hComm,&oldTimeout);
	tOut = oldTimeout;
	// Make timeout so that:
	// - return immediately with buffered characters
	tOut.ReadIntervalTimeout=MAXDWORD;
	tOut.ReadTotalTimeoutMultiplier=0;
	tOut.ReadTotalTimeoutConstant=0;
	SetCommTimeouts(hComm,&tOut);

	bInited = true;
	return true;
	//---------------------------------------------
    #else
    //---------------------------------------------
    
    //lets account for the name being passed in instead of the device path
    if( portName.size() > 5 && portName.substr(0, 5) != "/dev/" )
    {
        portName = "/dev/" + portName;
    }
    
    LOG_DEBUG << "opening " << portName << " @ " << baud << " bps";
    m_handle = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(m_handle == -1)
    {
        LOG_ERROR << "unable to open " << portName;
        return false;
    }
    
    struct termios options;
    tcgetattr(m_handle, &m_old_options);
    options = m_old_options;
    switch(baud)
    {
        case 300:
            cfsetispeed(&options, B300);
            cfsetospeed(&options, B300);
            break;
        case 1200:
            cfsetispeed(&options, B1200);
            cfsetospeed(&options, B1200);
            break;
        case 2400:
            cfsetispeed(&options, B2400);
            cfsetospeed(&options, B2400);
            break;
        case 4800:
            cfsetispeed(&options, B4800);
            cfsetospeed(&options, B4800);
            break;
        case 9600:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
#ifndef KINSKI_LINUX
        case 14400:
            cfsetispeed(&options, B14400);
            cfsetospeed(&options, B14400);
            break;
        case 28800:
            cfsetispeed(&options, B28800);
            cfsetospeed(&options, B28800);
            break;
#endif
        case 19200:
            cfsetispeed(&options, B19200);
            cfsetospeed(&options, B19200);
            break;
        case 38400:
            cfsetispeed(&options, B38400);
            cfsetospeed(&options, B38400);
            break;
        case 57600:
            cfsetispeed(&options, B57600);
            cfsetospeed(&options, B57600);
            break;
        case 115200:
            cfsetispeed(&options, B115200);
            cfsetospeed(&options, B115200);
            break;
        case 230400:
            cfsetispeed(&options, B230400);
            cfsetospeed(&options, B230400);
            break;
        default:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            LOG_ERROR << "setup(): cannot set " << baud << " bps, setting to 9600";
            break;
    }
    
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(m_handle,TCSANOW,&options);
    
    bInited = true;
    LOG_DEBUG << "opened " << portName << "sucessfully @ " << baud << " bps";
    
    return true;
	#endif
	//---------------------------------------------
}

//----------------------------------------------------------------
    
int Serial::write_string(const std::string &str)
{
    return writeBytes(str.c_str(), str.length() + 1);
}
    
//----------------------------------------------------------------
int Serial::writeBytes(const void *buffer, int length)
{

	if (!bInited)
    {
		LOG_ERROR << "writeBytes(): serial not inited";
		return KINSKI_SERIAL_ERROR;
	}

    //---------------------------------------------
	#ifdef KINSKI_MSW
		DWORD written;
		if(!WriteFile(hComm, buffer, length, &written,0)){
			 LOG_ERROR << "writeBytes(): couldn't write to port";
			 return KINSKI_SERIAL_ERROR;
		}
		ofLogVerbose("Serial") <<  "wrote " << (int) written << " bytes";
		return (int)written;
	#else
    int numWritten = (int)write(m_handle, buffer, length);
    if(numWritten <= 0)
    {
        if ( errno == EAGAIN )
            return 0;
        LOG_ERROR << "writeBytes(): couldn't write to port: " << errno << " " << strerror(errno);
        return KINSKI_SERIAL_ERROR;
    }
    
    LOG_TRACE << "wrote " << (int) numWritten << " bytes";
    
    return numWritten;
	#endif
	//---------------------------------------------

}

//----------------------------------------------------------------
int Serial::readBytes(void *buffer, int length)
{

	if (!bInited)
    {
		LOG_ERROR << "readBytes(): serial not inited";
		return KINSKI_SERIAL_ERROR;
	}

    //---------------------------------------------
#ifdef KINSKI_MSW
    DWORD nRead = 0;
    if (!ReadFile(hComm,buffer,length,&nRead,0)){
        LOG_ERROR << "readBytes(): couldn't read from port";
        return KINSKI_SERIAL_ERROR;
    }
    return (int)nRead;
    //---------------------------------------------
#else
    //---------------------------------------------
    int nRead = (int)read(m_handle, buffer, length);
    if(nRead < 0)
    {
        if ( errno == EAGAIN )
            return KINSKI_SERIAL_NO_DATA;
        LOG_ERROR << "readBytes(): couldn't read from port: " << errno << " " << strerror(errno);
        return KINSKI_SERIAL_ERROR;
    }
    return nRead;
#endif
    //---------------------------------------------

}

//----------------------------------------------------------------
bool Serial::writeByte(unsigned char singleByte){

	if (!bInited)
    {
		LOG_ERROR << "writeByte(): serial not inited";
		//return KINSKI_SERIAL_ERROR; // this looks wrong.
		return false;
	}

	unsigned char tmpByte[1];
	tmpByte[0] = singleByte;


    //---------------------------------------------
	#ifdef KINSKI_MSW
		DWORD written = 0;
		if(!WriteFile(hComm, tmpByte, 1, &written,0)){
			 LOG_ERROR << "writeByte(): couldn't write to port";
			 //return KINSKI_SERIAL_ERROR; // this looks wrong.
			 return false;
		}

		ofLogVerbose("Serial") << "wrote byte";

		return ((int)written > 0 ? true : false);
    //---------------------------------------------
    #else
    //---------------------------------------------
    size_t numWritten = 0;
    numWritten = write(m_handle, tmpByte, 1);
    if(numWritten <= 0 )
    {
        if ( errno == EAGAIN )
            return 0;
        LOG_ERROR << "writeByte(): couldn't write to port: " << errno << " " << strerror(errno);
        //return KINSKI_SERIAL_ERROR; // this looks wrong.
        return false;
    }
    LOG_TRACE << "wrote byte";
    
    return (numWritten > 0 ? true : false);

    //---------------------------------------------
	#endif

}

//----------------------------------------------------------------
int Serial::readByte(){

	if (!bInited){
		LOG_ERROR << "readByte(): serial not inited";
		return KINSKI_SERIAL_ERROR;
	}

	unsigned char tmpByte[1];
	memset(tmpByte, 0, 1);

	//---------------------------------------------
	#if defined( KINSKI_MAC ) || defined( KINSKI_LINUX )
		int nRead = read(m_handle, tmpByte, 1);
		if(nRead < 0){
			if ( errno == EAGAIN )
				return KINSKI_SERIAL_NO_DATA;
			LOG_ERROR << "readByte(): couldn't read from port: " << errno << " " << strerror(errno);
            return KINSKI_SERIAL_ERROR;
		}
		if(nRead == 0)
			return KINSKI_SERIAL_NO_DATA;
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef KINSKI_MSW
		DWORD nRead;
		if (!ReadFile(hComm, tmpByte, 1, &nRead, 0)){
			LOG_ERROR << "readByte(): couldn't read from port";
			return KINSKI_SERIAL_ERROR;
		}
	#endif
	//---------------------------------------------

	return (int)(tmpByte[0]);
}


//----------------------------------------------------------------
void Serial::flush(bool flushIn, bool flushOut){

	if (!bInited){
		LOG_ERROR << "flush(): serial not inited";
		return;
	}

	int flushType = 0;

	//---------------------------------------------
	#if defined( KINSKI_MAC ) || defined( KINSKI_LINUX )
		if( flushIn && flushOut) flushType = TCIOFLUSH;
		else if(flushIn) flushType = TCIFLUSH;
		else if(flushOut) flushType = TCOFLUSH;
		else return;

		tcflush(m_handle, flushType);
    #endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef KINSKI_MSW
		if( flushIn && flushOut) flushType = PURGE_TXCLEAR | PURGE_RXCLEAR;
		else if(flushIn) flushType = PURGE_RXCLEAR;
		else if(flushOut) flushType = PURGE_TXCLEAR;
		else return;

		PurgeComm(hComm, flushType);
	#endif
	//---------------------------------------------
}

void Serial::drain(){
    if (!bInited){
		LOG_ERROR << "drain(): serial not inited";
		return;
    }

    #if defined( KINSKI_MAC ) || defined( KINSKI_LINUX )
        tcdrain( m_handle );
    #endif
}

//-------------------------------------------------------------
size_t Serial::available()
{

	if (!bInited)
    {
		LOG_ERROR << "available(): serial not inited";
		return KINSKI_SERIAL_ERROR;
	}

	size_t numBytes = 0;

	//---------------------------------------------
	#if defined( KINSKI_MAC ) || defined( KINSKI_LINUX )
		ioctl(m_handle, FIONREAD, &numBytes);
	#endif
    //---------------------------------------------

    //---------------------------------------------
	#ifdef KINSKI_MSW
	COMSTAT stat;
       	DWORD err;
       	if(hComm!=INVALID_HANDLE_VALUE){
           if(!ClearCommError(hComm, &err, &stat)){
               numBytes = 0;
           } else {
               numBytes = stat.cbInQue;
           }
       	} else {
           numBytes = 0;
       	}
	#endif
    //---------------------------------------------

	return numBytes;
}

bool Serial::isInitialized() const
{
    return bInited;
}

vector<string> Serial::read_lines(const char delim)
{
    vector<string> ret;
    
    m_read_buffer.resize(2048);
    std::fill(m_read_buffer.begin(), m_read_buffer.end(), 0);
    
    // try reading some bytes
    int num_read = readBytes(&m_read_buffer[0], std::min(available(), m_read_buffer.size()));
    
    if(num_read > 0)
    {
        m_accum_str.append(&m_read_buffer[0], num_read);
        std::stringstream input(m_accum_str);
        m_accum_str.clear();
        
        for (string line; std::getline(input, line, delim); )
        {
            if(input.eof())
            {
                m_accum_str = line;
                break;
            }
            // remove all carriage-return chars
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            
            if(!line.empty())
                ret.push_back(line);
        }
    }
    return ret;
}
    
}
