//
//  NoiseSample.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "NoiseSample.h"

using namespace std;
using namespace kinski;
using namespace glm;


// Two-channel sawtooth wave generator.
/////////////////////////////////////////////////////////////////
RtAudio::StreamParameters parameters;

unsigned int sampleRate = 44100;
unsigned int bufferFrames = 256; // 256 sample frames
double data[2];

int saw( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
        double streamTime, RtAudioStreamStatus status, void *userData )
{
    unsigned int i, j;
    double *buffer = (double *) outputBuffer;
    double *lastValues = (double *) userData;
    if ( status )
        std::cout << "Stream underflow detected!" << std::endl;
    // Write interleaved audio data.
    for ( i=0; i<nBufferFrames; i++ ) {
        for ( j=0; j<2; j++ ) {
            *buffer++ = lastValues[j];
            lastValues[j] += 0.005 * (j+1+(j*0.1));
            if ( lastValues[j] >= 1.0 ) lastValues[j] -= 2.0;
        }
    }
    return 0;
}

/////////////////////////////////////////////////////////////////

void NoiseSample::setup()
{
    ViewerApp::setup();
    m_font.load("Courier New Bold.ttf", 18);
    outstream_gl().set_color(gl::COLOR_WHITE);
    outstream_gl().set_font(m_font);
    
    int num_audio_devices = m_audio.getDeviceCount();
    
    for(int i = 0; i < num_audio_devices; i++)
    {
        auto dev = m_audio.getDeviceInfo(i);
        LOG_INFO << "found audio device " << dev.name <<": "<< dev.outputChannels
        <<" outputs -- " <<dev.inputChannels <<" inputs";
    }
    
    LOG_INFO << "default device: " << m_audio.getDeviceInfo(m_audio.getDefaultOutputDevice()).name;
    
    // set the stream parameters
    parameters.deviceId = m_audio.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    
}

/////////////////////////////////////////////////////////////////

void NoiseSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::draw()
{
    
}

/////////////////////////////////////////////////////////////////

void NoiseSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
    
    switch (e.getCode())
    {
        case GLFW_KEY_B:
            
            try
            {
                if(m_streaming)
                {
                    // Stop the stream
                    m_audio.stopStream();
                    if ( m_audio.isStreamOpen() ) m_audio.closeStream();
                }
                else
                {
                    m_audio.openStream(&parameters, NULL, RTAUDIO_FLOAT64,
                                       sampleRate, &bufferFrames, &saw, (void *)&data );
                    m_audio.startStream();
                }
            }
            catch ( RtAudioError& e ){ LOG_ERROR << e.what(); }
            
            m_streaming = !m_streaming;
        break;
            
        default:
            break;
    }
}

/////////////////////////////////////////////////////////////////

void NoiseSample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void NoiseSample::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void NoiseSample::tearDown()
{
    if ( m_audio.isStreamOpen() ) m_audio.closeStream();
    
    LOG_PRINT<<"ciao noise sample";
}

/////////////////////////////////////////////////////////////////

void NoiseSample::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
}
