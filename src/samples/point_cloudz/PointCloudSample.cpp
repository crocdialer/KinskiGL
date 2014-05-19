//
//  PointCloudSample.cpp
//  kinskiGL
//
//  Created by Fabian on 29/01/14.
//
//

#include "PointCloudSample.h"

using namespace std;
using namespace kinski;
using namespace glm;


/////////////////////////////////////////////////////////////////

void PointCloudSample::setup()
{
    ViewerApp::setup();
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::update(float timeDelta)
{
    ViewerApp::update(timeDelta);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::draw()
{
    
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::resize(int w ,int h)
{
    ViewerApp::resize(w, h);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::keyPress(const KeyEvent &e)
{
    ViewerApp::keyPress(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::keyRelease(const KeyEvent &e)
{
    ViewerApp::keyRelease(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mousePress(const MouseEvent &e)
{
    ViewerApp::mousePress(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseRelease(const MouseEvent &e)
{
    ViewerApp::mouseRelease(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseMove(const MouseEvent &e)
{
    ViewerApp::mouseMove(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseDrag(const MouseEvent &e)
{
    ViewerApp::App::mouseDrag(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::mouseWheel(const MouseEvent &e)
{
    ViewerApp::mouseWheel(e);
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::got_message(const std::vector<uint8_t> &the_message)
{
    LOG_INFO<<string(the_message.begin(), the_message.end());
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::tearDown()
{
    LOG_PRINT<<"ciao empty sample";
}

/////////////////////////////////////////////////////////////////

void PointCloudSample::updateProperty(const Property::ConstPtr &theProperty)
{
    ViewerApp::updateProperty(theProperty);
}
