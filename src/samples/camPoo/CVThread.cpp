/*
 *  CVThread.cpp
 *  TestoRAma
 *
 *  Created by Fabian on 9/13/10.
 *  Copyright 2010. All rights reserved.
 *
 */

#include "CVThread.h"
#include <fstream>

#include "Colormap.h"

using namespace std;
using namespace cv;

using namespace boost::timer;

namespace kinski {
    
    class CvCaptureNode : public ISourceNode
    {
    public:
        CvCaptureNode(): m_camID(0){m_capture.open(m_camID);};
        virtual ~CvCaptureNode(){ m_capture.release();};
        bool hasImage(){ return m_capture.isOpened();};
        
        cv::Mat getNextImage()
        {
            Mat capFrame;
            m_capture.retrieve(capFrame, 0) ;
            
            // going safe, have a copy of our own of the data
            capFrame = capFrame.clone();
            return capFrame;
        };

    private:
        int m_camID;
        cv::VideoCapture m_capture;
    };
    
    class NoobNode : public IProcessNode
    {
        cv::Mat doProcessing(const cv::Mat &img){ return img;};
    };
    
    class BoneNode : public IProcessNode
    {
    public:
        BoneNode():m_colorMap(Colormap::BONE){};
        
        cv::Mat doProcessing(const cv::Mat &img)
        {
            Mat grayImg,out;
            cvtColor(img, grayImg, CV_BGR2GRAY);
            
            out = m_colorMap.apply(grayImg);
            
            return out;
        };
        
    private:
        Colormap m_colorMap;
    };
    
    class ThreshNode : public IProcessNode
    {
    public:
        ThreshNode():m_colorMap(Colormap::BONE){};
        
        cv::Mat doProcessing(const cv::Mat &img)
        {
            Mat grayImg, threshImg, out;
            cvtColor(img, grayImg, CV_BGR2GRAY);
            threshold(grayImg, threshImg, 50, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
            out = threshImg;
            
            return m_colorMap.apply(out);
        };
        
    private:
        Colormap m_colorMap;
    };
    

    
    CVThread::CVThread():m_streamType(NO_STREAM),m_currentFileIndex(0),m_sequenceStep(1),
    m_stopped(true), m_newFrame(false),
    m_processNode(new ThreshNode),
    m_captureFPS(25.f)
    {	
        printf("CVThread -> OpenCV-Version: %s\n\n",CV_VERSION);
        
        //    cout<<cv::getBuildInformation()<<"\n";
        
        m_doProcessing = true;
    }
    
    CVThread::~CVThread()
    {
        stop();
        m_capture.release();
    }
    
    void CVThread::start()
    {
        if(!m_stopped) return;
        
        m_thread = boost::thread(boost::ref(*this));
        m_stopped = false;
        
    }
    
    void CVThread::stop()
    {
        m_stopped=true;
        m_thread.join();
    }
    
    void CVThread::openImage(const std::string& imgPath)
    {	
        setImage(imread(imgPath));
        m_streamType = NO_STREAM ;
        
    }
    
    void CVThread::playPause()
    {
        if(m_stopped)
            start();
        else 
            stop();
    }
    
    void CVThread::openSequence(const std::vector<std::string>& files)
    {	
        streamUSBCamera(false);
        
        m_streamType = STREAM_FILELIST ;
        
        m_currentFileIndex = 0 ;
        m_filesToStream = files;
        m_numVideoFrames = files.size();
        
        //m_bufferThread->setSeq(files);
    }
    
    bool CVThread::saveCurrentFrame(const std::string& savePath)
    {
        return imwrite(savePath,m_doProcessing? m_procImage : m_procImage);
        
    }
    
    void CVThread::skipFrames(int num)
    {	
        jumpToFrame( m_currentFileIndex + num );
        
    }
    
    void CVThread::jumpToFrame(int newIndex)
    {	
        m_currentFileIndex = newIndex < 0 ? 0 : newIndex;
        
        switch (m_streamType) 
        {
            case STREAM_VIDEOFILE:
                
                if(m_currentFileIndex >= m_numVideoFrames)
                    m_currentFileIndex = m_numVideoFrames-1;
                
                m_capture.set(CV_CAP_PROP_POS_FRAMES,m_currentFileIndex);
                
                break;
                
            default:
            case STREAM_FILELIST:
                
                if(m_filesToStream.empty())
                    return;
                
                if(m_currentFileIndex >= (int)m_filesToStream.size())
                    m_currentFileIndex = m_filesToStream.size()-1;
                
                break;
                
        }
        
        if(m_stopped)
        {	
            cpu_timer timer;
            
            if(grabNextFrame() && m_procImage.size() != Size(0,0)) 
            {	
                //m_lastGrabTime = m_timer.elapsed();
                
                timer.start();
                
                if(m_doProcessing && m_processNode)
                {
                    m_procImage = m_processNode->doProcessing(m_procImage);
                    
                    //m_lastProcessTime = timer.getElapsedTime();
                }
                
            }
        }

    }
    
    void CVThread::streamVideo(const std::string& path2Video)
    {
        if(m_capture.isOpened())
            m_capture.release();
        
        if(! m_capture.isOpened())
        {
            if(false)
            {
                //extracts framecount from an avi-file, since opencv fails at this point	
                
                //const char* path2Video = "/Users/Fabian/Desktop/iccv07-data/results/video-iccv07-seq1.avi";
                unsigned char tempSize[4];
                
                // Trying to open the video file
                std::ifstream videoFile( path2Video.c_str() , std::ios::in | std::ios::binary );
                // Checking the availablity of the file
                if ( !videoFile ) 
                {
                    std::string errStr = path2Video+"not found (streamVideo)";
                    
                    //std::exception e;
                    throw std::exception();
                }
                m_videoPath = path2Video ;
                
                // get the number of frames (out of AVI Header)
                videoFile.seekg( 0x30 , std::ios::beg );
                videoFile.read( (char*)tempSize , 4 );
                m_numVideoFrames = tempSize[0] + 0x100 * tempSize[1] + 0x10000 * tempSize[2] + 0x1000000 * tempSize[3];
                m_currentFileIndex = 0;
                
                videoFile.close(  );
            }
            m_capture.open(path2Video);
            
            //Doesn´t work either :( -> read aviheader once more
            m_captureFPS = m_capture.get(CV_CAP_PROP_FPS);
            m_numVideoFrames = m_capture.get(CV_CAP_PROP_FRAME_COUNT);
            printf("%d frames in video - fps: %.2f\n",m_numVideoFrames,m_captureFPS);
            
            m_streamType = STREAM_VIDEOFILE ;
            this->start();
            
        }
        else
        {	
            this->stop();
            m_capture.release();
            
            m_streamType = NO_STREAM ;
        }
        
    }
    
    void CVThread::streamUSBCamera(bool b,int camId)
    {
        
        if(b && ! m_capture.isOpened())
        {
            m_capture.open(camId);
            
            this->start();
            
            m_streamType = STREAM_CAPTURE ;
            
        }
        else
        {	
            this->stop();
            m_capture.release();
            
            m_streamType = NO_STREAM ;
        }
        
    }
    
#ifdef KINSKI_FREENECT
    void CVThread::streamKinect(bool b)
    {
        if(b)
        {
            m_freenect = Freenect::Ptr(new Freenect);
            m_kinectDevice = &(m_freenect->createDevice<KinectDevice>(0));
            
            
            //setKinectUseIR(m_kinectUseIR);
            m_kinectDevice->startVideo();
            m_kinectDevice->startDepth();
            
            m_streamType = STREAM_KINECT;
            this->start();
        }
        else
        {
            
            m_freenect.reset();
            m_kinectDevice=NULL;
            
            this->stop();
            m_streamType = NO_STREAM;
        }
    }
    
    void CVThread::setKinectUseIR(bool b)
    {
        m_kinectUseIR = b;
        
        if(m_kinectDevice)
            m_kinectDevice->setVideoFormat(b ?  FREENECT_VIDEO_IR_8BIT : 
                                           FREENECT_VIDEO_RGB); 
    }
#endif
    
    bool CVThread::grabNextFrame()
    {	
        //auto_cpu_timer t;
        
        bool success = true ;
        
        switch (m_streamType) 
        {
            case STREAM_FILELIST:
                
                if(m_stopped)
                    m_procImage = cv::imread(m_filesToStream[m_currentFileIndex]);
                else
                {
                    //m_frames.m_inFrame = m_bufferThread->grabNextFrame();
                    
                    m_currentFileIndex = m_currentFileIndex + m_sequenceStep;
                }
                
                // last frame reached ?
                if(m_currentFileIndex >= (int)m_filesToStream.size())
                {
                    m_currentFileIndex = m_filesToStream.size()-1;
                    
                    this->stop();
                    success = false ;
                }
                
                break;
                
            case STREAM_CAPTURE:
            case STREAM_VIDEOFILE:
                
                if(m_capture.isOpened() && m_capture.grab())
                {		
                    Mat capFrame;
                    m_capture.retrieve(capFrame, 0) ;
                    
                    // going safe, have a copy of our own of the data
                    m_procImage = capFrame.clone();
                    
                    if(m_streamType==STREAM_VIDEOFILE)
                    {
                        // last frame reached ?
                        if(m_currentFileIndex+1 >= m_numVideoFrames)
                        {
                            m_currentFileIndex = m_numVideoFrames-1;
                            
                            this->stop();
                            success = false ;
                        }
                        else
                            m_currentFileIndex = m_capture.get(CV_CAP_PROP_POS_FRAMES);
                        
                    }
                    
                }
                
                else 
                    success = false ;
                
                break;
                
//            case STREAM_IP_CAM:
//                
//                if(m_stopped)
//                    loadFrameFromIpCamera();
//                else
//                    ;//m_frames.m_inFrame = m_bufferThread->grabNextFrame();
//                break ;
                
#ifdef KINSKI_FREENECT
            case STREAM_KINECT:
                
                m_kinectDevice->getVideo(m_frames.m_inFrame,m_kinectUseIR);
                m_kinectDevice->getDepth(m_frames.m_depthMap,m_frames.m_inFrame);
                
                break ;
#endif
                
            case NO_STREAM:
                success = false ;
                break ;
                
            default:
                break;
        }
        
        return success;
    }
    
    void CVThread::operator()()
    {	
        m_stopped=false;
        
        // measure elapsed time with these
        boost::timer::cpu_timer threadTimer, cpuTimer;
        
        // gets next frame, which will be hold inside m_procImage
        while( !m_stopped )
        {
            //restart timer
            threadTimer.start();        
            cpuTimer.start();
            
            // fetch frame, cancel loop when not possible
            // this call is supposed to be fast and not block the thread too long
            if (!grabNextFrame()) break;
            
            //skip iteration when invalid frame is returned (eg. from camera)
            if(m_procImage.empty()) continue;
            
            cpu_times t = cpuTimer.elapsed();
            m_lastGrabTime = (t.user + t.system) / 1000000000.0;
            
            // image processing
            {   
                boost::mutex::scoped_lock lock(m_mutex);
                cpuTimer.start();
                
                if(m_doProcessing && m_processNode)
                {   
                    Mat procResult = m_processNode->doProcessing(m_procImage);
                    
                    //                Mat grey;
                    //                cvtColor(procResult, grey, CV_BGR2GRAY);
                    //                Canny(procResult, grey, 20, 30);
                    
                    m_procImage = procResult;
                }
                
                m_newFrame = true;
                
                t = cpuTimer.elapsed();
                m_lastProcessTime = (t.user + t.system) / 1000000000.0;
            }
            
            double elapsed_msecs,sleep_msecs;
            
            elapsed_msecs = threadTimer.elapsed().wall / 1000000.0;
            sleep_msecs = max(0.0, (1000.0 / m_captureFPS - elapsed_msecs));
            
            // set thread asleep for a time to achieve desired framerate
            boost::posix_time::milliseconds msecs(sleep_msecs);
            boost::this_thread::sleep(msecs);
        }
        
        m_stopped = true;
    }
    
    string CVThread::getCurrentImgPath()
    {
        string out;
        switch (m_streamType) 
        {
            case STREAM_FILELIST:
                
                out = m_filesToStream[m_currentFileIndex];
                break;
                
            case STREAM_VIDEOFILE:
                
                out = getVideoPath();
                break;
                
            case STREAM_CAPTURE:
            case STREAM_IP_CAM:
                
                out = "camera input";
                break;
                
            default:
                break;
        }
        return out;
    }
    
    // -- Getter / Setter
    bool CVThread::getImage(cv::Mat& img)
    {	
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_newFrame)
        {
            img = m_procImage;
            m_newFrame = false;
            return true;
        }
        
        return false;
    }
    
    void CVThread::setImage(const cv::Mat& img)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_procImage = img.clone();
        m_newFrame = true;
    }
    
    double CVThread::getLastGrabTime()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_lastGrabTime;
    }
    
    double CVThread::getLastProcessTime()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_lastProcessTime;
    }
    
    int CVThread::getCurrentIndex()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_currentFileIndex;
    }
    
}// namespace kinski