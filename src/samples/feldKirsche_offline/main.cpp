#include "Feldkirsche_App.h"

using namespace kinski;

int main(int argc, char *argv[])
{
    App::Ptr theApp(new Feldkirsche_App);
    theApp->set_name("Feldkirsche");
    theApp->setWindowSize(1024, 1024);
    //theApp->setFullSceen();
    return theApp->run();
}
